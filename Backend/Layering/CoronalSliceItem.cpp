#include "Backend/Layering/CoronalSliceItem.hpp"
#include "Backend/Layering/CoronalSliceData.hpp"

using namespace Dicom;
using namespace Dicom::Viewer::Slicer;

vtkStandardNewMacro(Dicom::Viewer::Slicer::CoronalSliceData);

CoronalSliceItem::CoronalSliceItem(QQuickVtkItem* parent) :
    AbstractSliceItem(parent)
{
}

QQuickVtkItem::vtkUserData CoronalSliceItem::initializeVTK(vtkRenderWindow* renderWindow)
{
    auto vtkSlice = vtkNew<CoronalSliceData>();
    sliceData_ = vtkSlice;
    sliceData_->Init(renderWindow);
    return vtkSlice;
}
