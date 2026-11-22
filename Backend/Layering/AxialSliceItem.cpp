#include "Backend/Layering/AxialSliceItem.hpp"
#include "Backend/Layering/AxialSliceData.hpp"
#include "Backend/SceneVtkItem.hpp"
#include "Backend/SceneVtkData.hpp"

using namespace Dicom::Viewer::Slicer;

vtkStandardNewMacro(Dicom::Viewer::Slicer::AxialSliceData);

AxialSliceItem::AxialSliceItem(QQuickVtkItem* parent) :
    AbstractSliceItem(parent)
{
}

QQuickVtkItem::vtkUserData AxialSliceItem::initializeVTK(vtkRenderWindow* renderWindow)
{
    auto vtkSlice = vtkNew<AxialSliceData>();
    sliceData_ = vtkSlice;
    sliceData_->Init(renderWindow);
    return vtkSlice;
}

void AxialSliceItem::AutoSpline()
{
    std::function<void(vtkRenderWindow*, vtkUserData)> convertToPNG(
        [this](vtkRenderWindow*, const vtkUserData&) {
        }
    );
    QQuickVtkItem::dispatch_async(convertToPNG);
}

void AxialSliceItem::ChangeInteractorStyle()
{
    Callback changeStyle([this](vtkRenderWindow*, const vtkUserData&) {
    });
    QQuickVtkItem::dispatch_async(changeStyle);
}

void AxialSliceItem::OnResetSplineClicked()
{
    Callback resetSpline([this](vtkRenderWindow*, const vtkUserData&) {
        qInfo() << "Reset spline";
    });
    QQuickVtkItem::dispatch_async(resetSpline);
}

void AxialSliceItem::OnPanoramicViewUpdateClicked()
{
}
