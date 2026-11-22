#ifndef REPRESENTATION_HPP
#define REPRESENTATION_HPP

#include <vtkBoxWidget2.h>
#include <vtkNew.h>
#include <vtkCameraOrientationWidget.h>
#include <vtkPlanes.h>
#include <vtkImageReader2.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkBoxRepresentation.h>
#include <vtkAxesActor.h>
#include <vtkImageData.h>

/// <summary>
/// A class that includes all application representation objects
/// </summary>
class Representation final
{
public:
    // clipping box
    vtkNew<vtkBoxWidget2> boxWidget;
    // clipping planes
    vtkNew<vtkPlanes> planes;
    // dynamic orientation axis
    vtkNew<vtkCameraOrientationWidget> cameraAxisOrientManipulator;

    Representation(vtkRenderWindowInteractor* interactor, vtkRenderer* renderer);
    ~Representation() = default;

    void SetReader(vtkImageReader2* ptrReader);
};

#endif
