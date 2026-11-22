#include "Backend/Layering/CoronalSliceData.hpp"

#include <vtkCamera.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCallbackCommand.h>
#include <vtkImageProperty.h>
#include <vtkImageData.h>
#include <vtkPlane.h>

#include <vtkImageReslice.h>
#include <vtkPNGWriter.h>
#include <vtkResliceImageViewer.h>
#include <vtkImageMapToWindowLevelColors.h>

using namespace Dicom::Viewer::Slicer;
using namespace Dicom::Math;

CoronalSliceData::CoronalSliceData() :
    AbstractSliceData(ProjectionType::Coronal)
{
}

void CoronalSliceData::SetupCamera() noexcept
{
}

void CoronalSliceData::SliceFromDicom()
{
    ImageType3D::RegionType inputRegion = vtk2itk->GetOutput()->GetLargestPossibleRegion();
    ImageType3D::SizeType size = inputRegion.GetSize();

    ImageType3D::IndexType start = inputRegion.GetIndex();
    start[1] = size[1] / 2;
    size[1] = 0;

    ImageType3D::RegionType wholeCoronalImageRegion;
    wholeCoronalImageRegion.SetSize(size);
    wholeCoronalImageRegion.SetIndex(start);

    extractFilter->SetDirectionCollapseToSubmatrix();
    extractFilter->SetInput(vtk2itk->GetOutput());
    extractFilter->SetExtractionRegion(wholeCoronalImageRegion);
    extractFilter->Modified();
    extractFilter->Update();
}
