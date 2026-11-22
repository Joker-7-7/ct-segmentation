#include "Backend/Layering/AbstractSliceItem.hpp"

using namespace Dicom;
using namespace Dicom::Viewer::Slicer;

AbstractSliceItem::AbstractSliceItem(QQuickVtkItem* parent) :
    QQuickVtkItem(parent)
{
}

void AbstractSliceItem::SetLowerThreshold(int value)
{
    Callback sliderChanged([this, value](vtkRenderWindow*, const vtkUserData&) {
        sliceData_->SetLowerThreshold(value);
    });
    QQuickVtkItem::dispatch_async(sliderChanged);
}

void AbstractSliceItem::SetUpperThreshold(int value)
{
    Callback sliderChanged([this, value](vtkRenderWindow*, const vtkUserData&) {
        sliceData_->SetUpperThreshold(value);
    });
    QQuickVtkItem::dispatch_async(sliderChanged);
}

void AbstractSliceItem::AddData(SceneVtkData::VTK2ITKFilterType::Pointer data) noexcept
{
    sliceData_->AddDataSet(data);
}

void AbstractSliceItem::EnableImageInteraction()
{
    sliceData_->EnableImageInteractionStyle();
}

void AbstractSliceItem::EnablePaintBrushInteraction()
{
    sliceData_->EnablePaintBrushInteractorStyle();
}

void AbstractSliceItem::OnOpenClicked()
{
    Callback openDirectory([this](vtkRenderWindow*, const vtkUserData&) {
        if (sceneItem_->sceneData->vtk2itk)
        {
            AddData(sceneItem_->sceneData->vtk2itk);
            qInfo() << "Dicom opened";
            emit dicomOpened();
        }
    });
    QQuickVtkItem::dispatch_async(openDirectory);
}

void AbstractSliceItem::OnSliderChanged(int value)
{
    Callback sliderChanged([this, value](vtkRenderWindow*, const vtkUserData&) {
        sliceData_->SetSliceNumber(value);
    });
    QQuickVtkItem::dispatch_async(sliderChanged);
}

void AbstractSliceItem::ChangeFilterView()
{
    Callback changeView([this](vtkRenderWindow*, const vtkUserData&) {
        sliceData_->ChangeFilterImage();
    });
    QQuickVtkItem::dispatch_async(changeView);
}

void AbstractSliceItem::ChangeBrushShape()
{
    Callback sliderChanged([this](vtkRenderWindow*, const vtkUserData&) {
        sliceData_->GetPaintStyle()->ChangeBrushShape();
    });
    QQuickVtkItem::dispatch_async(sliderChanged);
}

int AbstractSliceItem::GetMaxSliceNumber() const noexcept
{
    return 500;
}

void AbstractSliceItem::SetSceneView(SceneVtkItem* item) noexcept
{
    if (item == nullptr || sceneItem_ == item)
    {
        return;
    }
    sceneItem_ = item;
}

AbstractSliceData* AbstractSliceItem::GetData() const noexcept
{
    return sliceData_;
}
