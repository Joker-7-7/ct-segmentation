#include "AbstractSliceData.hpp"

#include <vtkCallbackCommand.h>
#include <vtkImageProperty.h>
#include <vtkImageData.h>
#include <vtkPlane.h>
#include <vtkImageReslice.h>
#include <vtkPNGWriter.h>
#include <vtkResliceImageViewer.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformation.h>
#include <vtkCamera.h>

using namespace Dicom::Viewer::Slicer;
using namespace Backend::Overlay;
namespace
{
    void DisableEvent(vtkObject*, long unsigned int, void*, void*)
    {
    }
}

AbstractSliceData::AbstractSliceData(ProjectionType projectionType) :
    extractFilter{ ExtractFilterTypeTo2D::New() },
    actor{ vtkSmartPointer<vtkImageActor>::New() },
    filterActor{ vtkSmartPointer<vtkImageActor>::New() },
    itkToVtkFilter{ ITKToVTKFilterType::New() },
    itkToVtkOriginal{ ITKToVTKFilterType::New() },
    imgToColors_{ vtkSmartPointer<vtkImageMapToWindowLevelColors>::New() },
    threshold{ ThresholdFilterType::New() },
    filterMode{ false },
    projectionType(projectionType),
    renderer_{ vtkSmartPointer<vtkRenderer>::New() }
{
}

void AbstractSliceData::Init(vtkRenderWindow* renderWindow)
{
    renderWindow_ = renderWindow;
    SetupRender();
}

void AbstractSliceData::AddDataSet(VTK2ITKFilterType::Pointer reader)
{
    RemoveDataSet();
    vtk2itk = reader;
    SliceFromDicom();
    sliceNumber = static_cast<int>(extractFilter->GetExtractionRegion().GetIndex(
    )[static_cast<int>(projectionType)]);

    double ColorLevel = 2500;
    double ColorWindow = 7000;

    threshold->SetLowerThreshold(1000);
    threshold->SetUpperThreshold(3500);
    threshold->SetOutsideValue(0);
    threshold->SetInsideValue(255);
    threshold->SetInput(extractFilter->GetOutput());

    itkToVtkOriginal->SetInput(extractFilter->GetOutput());
    itkToVtkOriginal->Update();

    itkToVtkFilter->SetInput(threshold->GetOutput());
    itkToVtkFilter->Update();

    imgToColors_->SetInputData(itkToVtkOriginal->GetOutput());
    imgToColors_->SetLevel(ColorLevel);
    imgToColors_->SetWindow(ColorWindow);

    imgToColors_->Update();

    actor->SetInputData(imgToColors_->GetOutput());
    actor->InterpolateOff();

    filterActor->GetMapper()->SetInputData(itkToVtkFilter->GetOutput());

    renderer_->AddActor(actor);

    paintStyle_->SetSliceActor(actor);
    paintStyle_->SetData(imgToColors_->GetOutput());
    paintStyle_->CreatMask(reader);
    paintStyle_->SetReader(vtk2itk);
    paintStyle_->SetProjectionType(
        static_cast<ManualSegmentationManager::ProjectionType>(projectionType)
    );
    paintStyle_->SetSlice(sliceNumber);

    renderer_->ResetCamera();
    renderer_->GetActiveCamera()->ParallelProjectionOn();
    renderer_->ResetCameraScreenSpace();
    renderWindow_->Render();

    DataSetAdded();
}

void AbstractSliceData::RemoveDataSet() const noexcept
{
    renderer_->RemoveActor(actor);
    renderWindow_->Render();
}

void AbstractSliceData::SetUpperThreshold(int value)
{
    if (value < threshold->GetLowerThreshold())
    {
        return;
    }
    threshold->SetUpperThreshold(value);
    threshold->Update();
    itkToVtkFilter->Update();
    renderWindow_->Render();
}

void AbstractSliceData::SetLowerThreshold(int value)
{
    if (value > threshold->GetUpperThreshold())
    {
        return;
    }
    threshold->SetLowerThreshold(value);
    threshold->Update();
    itkToVtkFilter->Update();
    renderWindow_->Render();
}

void AbstractSliceData::SetupRender() noexcept
{
    constexpr double BackgroundColor[] = { 0, 0, 0 };

    renderer_->SetBackground(BackgroundColor);
    renderer_->ResetCamera();
    renderer_->ResetCameraScreenSpace();
    SetupCamera();

    renderWindow_->SetNumberOfLayers(2);
    renderWindow_->SetSize(renderWindow_->GetScreenSize());
    renderer_->SetLayer(0);
    renderWindow_->AddRenderer(renderer_);

    vtkNew<vtkCallbackCommand> disableEventCallback;
    disableEventCallback->SetCallback(DisableEvent);

    imageInteractionStyle->AddObserver(vtkCommand::WindowLevelEvent, disableEventCallback);
    imageInteractionStyle->KeyPressActivationOff();
    imageInteractionStyle->PickingManagedOff();

    paintStyle_->SetCurrentRenderer(renderer_);

    EnableImageInteractionStyle();
}

void AbstractSliceData::EnablePaintBrushInteractorStyle()
{
    renderWindow_->GetInteractor()->SetInteractorStyle(paintStyle_);
    paintStyle_->BuildHoverOutlineActor();
}

void AbstractSliceData::EnableImageInteractionStyle()
{
    renderWindow_->GetInteractor()->SetInteractorStyle(imageInteractionStyle);
}

void AbstractSliceData::ChangeFilterImage()
{
    RemoveDataSet();
    if (filterMode == false)
    {
        renderer_->AddActor(filterActor);
    }
    else
    {
        renderer_->AddActor(actor);
    }
    filterMode = !filterMode;
    renderWindow_->Render();
}

void AbstractSliceData::SetSliceNumber(int slice)
{
    if (vtk2itk == nullptr)
    {
        return;
    }

    ImageType3D::RegionType inputRegion = vtk2itk->GetOutput()->GetLargestPossibleRegion();
    ImageType3D::SizeType size = inputRegion.GetSize();
    ImageType3D::IndexType start = inputRegion.GetIndex();

    int projectionIndex = static_cast<int>(projectionType);

    if (slice < start[projectionIndex] ||
        slice >= start[projectionIndex] + static_cast<int>(size[projectionIndex]))
    {
        return;
    }

    sliceNumber = slice;

    start[projectionIndex] = slice;
    size[projectionIndex] = 0;

    ImageType3D::RegionType wholeImageRegion;
    wholeImageRegion.SetSize(size);
    wholeImageRegion.SetIndex(start);

    extractFilter->SetExtractionRegion(wholeImageRegion);
    itkToVtkFilter->Update();
    itkToVtkOriginal->Update();
    imgToColors_->Update();

    imgToColors_->Modified();
    imgToColors_->Update();
    paintStyle_->SetData(imgToColors_->GetOutput());
    paintStyle_->SetSlice(sliceNumber);

    renderWindow_->Render();
}

vtkSmartPointer<PaintbrushInteractorStyle> AbstractSliceData::GetPaintStyle() const
{
    return paintStyle_;
}

void AbstractSliceData::InitSegmentationManager(ManualSegmentationManager& manager)
{
    paintStyle_->SetSegmentationManage(&manager);
}
