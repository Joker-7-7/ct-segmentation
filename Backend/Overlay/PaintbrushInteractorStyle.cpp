#include "PaintbrushInteractorStyle.hpp"
#include "Backend/Layering/AbstractSliceData.hpp"
#include "Backend/LogDuration.h"

#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkCellPicker.h>
#include <vtkRendererCollection.h>
#include <vtkImageActor.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkMath.h>

#include <cmath>
#include <cassert>
#include <type_traits>

using namespace Backend::Overlay;

vtkStandardNewMacro(PaintbrushInteractorStyle);

PaintbrushInteractorStyle::PaintbrushInteractorStyle() = default;

void PaintbrushInteractorStyle::OnMouseWheelForward()
{
    radiusStepRequested(1);
}

void PaintbrushInteractorStyle::OnMouseWheelBackward()
{
    radiusStepRequested(-1);
}

void PaintbrushInteractorStyle::OnRightButtonDoubleClick()
{
    // Eraser state is controlled explicitly by the shared toolbar.
}

void PaintbrushInteractorStyle::ChangeBrushShape()
{
    auto brushMode = context_.settings_.GetBrushMode();
    using UnderType = std::underlying_type_t<PaintbrushSettings::BrushMode>;
    auto newBrushMode = static_cast<PaintbrushSettings::BrushMode>(
        (static_cast<UnderType>(brushMode) + 1) %
        static_cast<UnderType>(PaintbrushSettings::BrushMode::COUNT)
    );
    context_.settings_.SetBrushMode(newBrushMode);
}

void PaintbrushInteractorStyle::BuildHoverOutlineActor()
{
    assert(imageSliceActor_ != nullptr);
    cursor_.hoverPicker_->AddPickList(imageSliceActor_);

    assert(CurrentRenderer != nullptr);
    cursor_.overlayRen_->SetActiveCamera(CurrentRenderer->GetActiveCamera());

    assert(Interactor != nullptr);
    Interactor->GetRenderWindow()->AddRenderer(cursor_.overlayRen_);
    cursor_.overlayRen_->AddActor(cursor_.hoverOutlineActor_);
}

bool PaintbrushInteractorStyle::WorldToIJK(const double w[3], int ijk[3])
{
    if (context_.imageData_ == nullptr)
    {
        return false;
    }
    double pcoords[3];
    return context_.imageData_->ComputeStructuredCoordinates(w, ijk, pcoords) == 1;
}

void PaintbrushInteractorStyle::OnMouseMove()
{
    vtkInteractorStyleImage::OnMouseMove();

    if (cursor_.hoverPicker_ == nullptr)
    {
        return;
    }

    if (CurrentRenderer == nullptr)
    {
        cursor_.HideHoverOutline();
        return;
    }

    const int* position = GetInteractor()->GetEventPosition();

    if (cursor_.hoverPicker_->Pick(position[0], position[1], 0, CurrentRenderer) == 0 ||
        cursor_.hoverPicker_->GetCellId() == -1)
    {
        cursor_.HideHoverOutline();
        return;
    }

    if (context_.settings_.GetCursorShape() == PaintbrushSettings::CursorShape::Round)
    {
        cursor_.UpdateOutlineCircleForCellById(
            cursor_.hoverPicker_->GetCellId(), context_.imageData_, context_.settings_
        );
    }
    else if (context_.settings_.GetCursorShape() == PaintbrushSettings::CursorShape::Square)
    {
        cursor_.UpdateOutlineSquareForCellById(
            cursor_.hoverPicker_->GetCellId(), context_.imageData_, context_.settings_
        );
    }

    if (mode != PaintMode::None)
    {
        Paint();
    }

    CurrentRenderer->GetRenderWindow()->Render();
}

void PaintbrushInteractorStyle::OnLeftButtonDown()
{
    context_.strokeChanged_ = false;
    mode = PaintMode::Draw;
    Paint();
    if (CurrentRenderer)
    {
        CurrentRenderer->GetRenderWindow()->Render();
    }
}

void PaintbrushInteractorStyle::OnLeftButtonUp()
{
    mode = PaintMode::None;
    if (context_.strokeChanged_ && context_.segmentationManager_)
    {
        context_.segmentationManager_->storage.mask->Modified();
        context_.segmentationManager_->maskChanged();
        context_.strokeChanged_ = false;
    }
}

void PaintbrushInteractorStyle::Paint()
{
    if (context_.segmentationManager_ == nullptr ||
        context_.segmentationManager_->CanPaint() == false || context_.imageData_ == nullptr)
    {
        return;
    }
    auto* interactor = this->GetInteractor();
    auto* renderer = interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    if (renderer == nullptr)
    {
        cursor_.HideHoverOutline();
        return;
    }

    const int* position = interactor->GetEventPosition();

    if (cursor_.hoverPicker_ == nullptr ||
        cursor_.hoverPicker_->Pick(position[0], position[1], 0, renderer) == 0 ||
        cursor_.hoverPicker_->GetCellId() == -1)
    {
        return;
    }

    vtkIdType cellId = cursor_.hoverPicker_->GetCellId();

    double b[6];
    context_.imageData_->GetCellBounds(cellId, b);
    const double xCellCenter = 0.5 * (b[0] + b[1]);
    const double yCellCenter = 0.5 * (b[2] + b[3]);

    double cellCenter[3] = { xCellCenter, yCellCenter, 0.0 };
    int ijk[3];

    if (WorldToIJK(cellCenter, ijk) == false)
    {
        return;
    }

    auto brush = context_.settings_.GetBrushMode();
    if (brush == PaintbrushSettings::BrushMode::Circle)
    {
        context_.PaintCircle(ijk[0], ijk[1]);
    }
    else if (brush == PaintbrushSettings::BrushMode::Square ||
             brush == PaintbrushSettings::BrushMode::Adaptive2D)
    {
        context_.PaintSquare(ijk[0], ijk[1]);
    }
    else if (brush == PaintbrushSettings::BrushMode::Cube ||
             brush == PaintbrushSettings::BrushMode::Adaptive3D)
    {
        context_.PaintCube(ijk[0], ijk[1], context_.slice_);
    }
    else if (brush == PaintbrushSettings::BrushMode::Sphere)
    {
        context_.PaintSphere(ijk[0], ijk[1], context_.slice_);
    }
}

void PaintbrushInteractorStyle::SetSliceActor(vtkSmartPointer<vtkImageSlice> sliceActor)
{
    imageSliceActor_ = sliceActor;
}

void PaintbrushInteractorStyle::SetSlice(int number)
{
    context_.slice_ = number;
    ReloadPaintedPixels();
}

void PaintbrushInteractorStyle::SetReader(VTK2ITKFilterType::Pointer reader)
{
    context_.filterPipeline_.SetReader(reader);
}

void PaintbrushInteractorStyle::SetProjectionType(ManualSegmentationManager::ProjectionType type)
{
    context_.projectionType_ = type;
    context_.filterPipeline_.projectionType = type;
}

void PaintbrushInteractorStyle::SetSegmentationManage(ManualSegmentationManager* manager)
{
    context_.segmentationManager_ = manager;
}

void PaintbrushInteractorStyle::Configure(int shape, double radius, bool erase)
{
    if (context_.strokeChanged_ && context_.segmentationManager_)
    {
        context_.segmentationManager_->storage.mask->Modified();
        context_.segmentationManager_->maskChanged();
    }
    context_.strokeChanged_ = false;
    if (shape >= 0 && shape < static_cast<int>(PaintbrushSettings::BrushMode::COUNT))
    {
        context_.settings_.SetBrushMode(static_cast<PaintbrushSettings::BrushMode>(shape));
    }
    context_.settings_.SetRadius(radius);
    context_.eraseMode = erase;
    mode = PaintMode::None;
    cursor_.HideHoverOutline();
}

void PaintbrushInteractorStyle::RefreshLabels()
{
    ReloadPaintedPixels();
}

void PaintbrushInteractorStyle::SetData(vtkSmartPointer<vtkImageData> data)
{
    context_.imageData_ = data;
    context_.copyImageData_->DeepCopy(data);
}

void PaintbrushInteractorStyle::CreatMask(VTK2ITKFilterType::Pointer reader)
{
    if (context_.segmentationManager_ &&
        context_.segmentationManager_->storage.maskReader != reader)
    {
        context_.segmentationManager_->CreateEmptyMask(reader);
    }
}

void PaintbrushInteractorStyle::ReloadPaintedPixels()
{
    if (context_.segmentationManager_)
    {
        const auto color = context_.segmentationManager_->ColorFor(
            context_.segmentationManager_->metadata.activeLabel
        );
        cursor_.hoverOutlineActor_->GetProperty()->SetColor(
            color.r / 255.0, color.g / 255.0, color.b / 255.0
        );
    }
    context_.ReloadPaintedPixels();
}
