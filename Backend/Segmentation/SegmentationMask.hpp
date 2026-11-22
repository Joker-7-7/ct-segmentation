#pragma once
#include "Backend/RLEImage/RLEImage.h"
#include <itkVTKImageToImageFilter.h>
#include <set>
#include <vector>

// Render-thread voxel storage. No actors or UI state.
class SegmentationMask
{
public:
    using RLE3Type = RLEImage<unsigned short, 3>;
    using Buffer2D = typename RLE3Type::BufferType;
    using RLLine = typename RLE3Type::RLLine;
    using RLSegment = typename RLE3Type::RLSegment;
    using ImageType3D = itk::Image<float, 3>;
    using FloatImageType3D = itk::Image<float, 3>;
    using VTK2ITKFilterType = itk::VTKImageToImageFilter<ImageType3D>;

    bool IsPixelSet(int x, int y, int z, unsigned short expectedValue);
    bool IsSegmentSet(int x1, int x2, int y, int z, unsigned short expectedValue);
    bool PaintSegment(int y, int z, int x1, int x2, unsigned short value);
    std::vector<std::pair<int, int>> FindUncoveredIntervalsInLine(
        int x1, int x2, int y, int z, unsigned short expectedValue
    );
    std::vector<std::pair<int, int>> FindIntervalsWithValue(
        int x1, int x2, int y, int z, unsigned short targetValue
    );
    void CreateEmptyMask(VTK2ITKFilterType::Pointer reader);
    void RemoveLabel(unsigned short label);
    RLE3Type::Pointer mask;
    VTK2ITKFilterType::Pointer maskReader;
    std::set<unsigned short> dirtyLabels;
};
