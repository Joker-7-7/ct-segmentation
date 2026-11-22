#include "Backend/Layering/AxialSliceData.hpp"
#include "Backend/GraphicPrimitives/Vector3D.hpp"

#include <vtkSplineRepresentation.h>
#include <vtkParametricSpline.h>
#include <vtkCamera.h>

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

using namespace Dicom::Viewer::Slicer;
using namespace Dicom::Math;

AxialSliceData::AxialSliceData() :
    AbstractSliceData(ProjectionType::Axial)
{
}

void AxialSliceData::SetupCamera() noexcept
{
}

void AxialSliceData::SliceFromDicom()
{
    ImageType3D::RegionType inputRegion = vtk2itk->GetOutput()->GetLargestPossibleRegion();
    ImageType3D::SizeType size = inputRegion.GetSize();

    ImageType3D::IndexType start = inputRegion.GetIndex();
    start[2] = size[2] / 2;
    size[2] = 0;

    ImageType3D::RegionType wholeCoronalImageRegion;
    wholeCoronalImageRegion.SetSize(size);
    wholeCoronalImageRegion.SetIndex(start);

    extractFilter->SetDirectionCollapseToSubmatrix();
    extractFilter->SetInput(vtk2itk->GetOutput());
    extractFilter->SetExtractionRegion(wholeCoronalImageRegion);
    extractFilter->Modified();
    extractFilter->Update();
}
