#include "Backend/Layering/SagittalSliceItem.hpp"
#include "Backend/Layering/SagittalSliceData.hpp"
#include "Backend/SceneVtkData.hpp"

using namespace Dicom::Viewer::Slicer;

vtkStandardNewMacro(Dicom::Viewer::Slicer::SagittalSliceData);

SagittalSliceItem::SagittalSliceItem(QQuickVtkItem* parent) :
    AbstractSliceItem(parent)
{
}

QQuickVtkItem::vtkUserData SagittalSliceItem::initializeVTK(vtkRenderWindow* renderWindow)
{
    auto vtkSlice = vtkNew<SagittalSliceData>();
    sliceData_ = vtkSlice;
    sliceData_->Init(renderWindow);
    return vtkSlice;
}
