#pragma once

#include "Backend/Segmentation/ManualSegmentationManager.hpp"

#include <itkImage.h>
#include <itkExtractImageFilter.h>
#include <itkImageSeriesReader.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkWatershedImageFilter.h>
#include <itkVTKImageToImageFilter.h>

class BrushWatershedPipeline
{
public:
    using PixelType = float;
    using ImageType2D = itk::Image<PixelType, 2>;
    using ImageType3D = itk::Image<PixelType, 3>;
    using ADFType = itk::GradientAnisotropicDiffusionImageFilter<ImageType3D, ImageType3D>;
    using GMFType = itk::GradientMagnitudeImageFilter<ImageType3D, ImageType3D>;
    using WatershedFilterType = itk::WatershedImageFilter<ImageType3D>;
    using ExtractFilterType = itk::ExtractImageFilter<ImageType3D, ImageType3D>;
    using VTK2ITKFilterType = itk::VTKImageToImageFilter<ImageType3D>;
    using RescaleType = itk::RescaleIntensityImageFilter<ImageType3D, ImageType3D>;

    BrushWatershedPipeline();
    void SetReader(VTK2ITKFilterType::Pointer reader);
    void PrecomputeWatersheds(int x, int y, int z, int regionSize);
    bool CheckFilterPixel(int clickX, int clickY, int clickZ, int x, int y, int z);
    WatershedFilterType::Pointer GetFilter();

    ManualSegmentationManager::ProjectionType projectionType;

    void FindWatershedIntervalsInLine(
        int xCur,
        int yCur,
        int zCur,
        int xClicked,
        int yClicked,
        int zClicked,
        std::vector<std::pair<int, int>>& intervals
    );

private:
    ADFType::Pointer adf;
    GMFType::Pointer gmf;
    WatershedFilterType::Pointer watershedFilter;
    ExtractFilterType::Pointer extractFilter;
    VTK2ITKFilterType::Pointer vtk2itk;
    RescaleType::Pointer rescale;
};
