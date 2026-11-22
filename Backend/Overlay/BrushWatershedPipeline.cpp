#include "BrushWatershedPipeline.h"

#include <itkStatisticsImageFilter.h>
#include <QDebug>

BrushWatershedPipeline::BrushWatershedPipeline() :
    adf{ ADFType::New() },
    gmf{ GMFType::New() },
    rescale{ RescaleType::New() },
    watershedFilter{ WatershedFilterType::New() },
    extractFilter{ ExtractFilterType::New() }
{

    adf->SetConductanceParameter(0.5);
    adf->SetTimeStep(0.025);

    gmf->SetInput(adf->GetOutput());

    float threshold = 0.015;
    float level = 0.4;

    rescale->SetInput(gmf->GetOutput());
    rescale->SetOutputMinimum(0.0);
    rescale->SetOutputMaximum(1.0);

    watershedFilter->SetThreshold(threshold);
    watershedFilter->SetLevel(level);
    watershedFilter->SetInput(rescale->GetOutput());
}

void BrushWatershedPipeline::SetReader(VTK2ITKFilterType::Pointer reader)
{
    vtk2itk = reader;
}

void BrushWatershedPipeline::FindWatershedIntervalsInLine(
    int xCur,
    int yCur,
    int zCur,
    int xClicked,
    int yClicked,
    int zClicked,
    std::vector<std::pair<int, int>>& intervals
)
{
    if (intervals.empty())
    {
        return;
    }

    std::vector<std::pair<int, int>> filtered;
    filtered.reserve(intervals.size());

    for (const auto& [a, b] : intervals)
    {
        int runStart = -1;

        for (int x = a; x <= b; ++x)
        {
            if (CheckFilterPixel(xClicked, yClicked, zClicked, x, yCur, zCur))
            {
                if (runStart < 0)
                {
                    runStart = x;
                }
            }
            else
            {
                if (runStart >= 0)
                {
                    filtered.emplace_back(runStart, x - 1);
                    runStart = -1;
                }
            }
        }

        if (runStart >= 0)
        {
            filtered.emplace_back(runStart, b);
        }
    }

    intervals.swap(filtered);
}

void BrushWatershedPipeline::PrecomputeWatersheds(int x, int y, int z, int regionSize)
{
    auto* img3D = vtk2itk->GetOutput();
    const auto inputRegion = img3D->GetLargestPossibleRegion();
    const auto fullSize = inputRegion.GetSize();
    const auto fullIndex = inputRegion.GetIndex();

    int ijk3D[3];
    int ijkProjection[3] = { x, y, z };

    ManualSegmentationManager::TransformFromProjectionTo3D(ijkProjection, ijk3D, projectionType);

    const int r = regionSize / 2;

    // left border
    int sx = std::max<int>(fullIndex[0], ijk3D[0] - r);
    int sy = std::max<int>(fullIndex[1], ijk3D[1] - r);
    int sz = std::max<int>(fullIndex[2], ijk3D[2] - r);

    // right border
    int ex = std::min<int>(fullIndex[0] + static_cast<int>(fullSize[0]) - 1, ijk3D[0] + r);
    int ey = std::min<int>(fullIndex[1] + static_cast<int>(fullSize[1]) - 1, ijk3D[1] + r);
    int ez = std::min<int>(fullIndex[2] + static_cast<int>(fullSize[2]) - 1, ijk3D[2] + r);

    ImageType3D::IndexType start;
    start[0] = sx;
    start[1] = sy;
    start[2] = sz;

    ImageType3D::SizeType size;
    size[0] = ex - sx + 1;
    size[1] = ey - sy + 1;
    size[2] = ez - sz + 1;

    ImageType3D::RegionType roi;
    roi.SetIndex(start);
    roi.SetSize(size);

    extractFilter->SetDirectionCollapseToSubmatrix();
    extractFilter->SetInput(img3D);
    extractFilter->SetExtractionRegion(roi);
    extractFilter->Modified();

    adf->SetInput(extractFilter->GetOutput());


    watershedFilter->Modified();
    watershedFilter->Update();
}

bool BrushWatershedPipeline::CheckFilterPixel(
    int clickX, int clickY, int clickZ, int x, int y, int z
)
{
    using OutType = WatershedFilterType::OutputImageType;
    auto* out = watershedFilter->GetOutput();
    const auto region = out->GetLargestPossibleRegion();

    int ijk3DPixel[3];
    int ijkProjectionPixel[3] = { x, y, z };

    ManualSegmentationManager::TransformFromProjectionTo3D(
        ijkProjectionPixel, ijk3DPixel, projectionType
    );

    int ijk3DClick[3];
    int ijkProjectionClick[3] = { clickX, clickY, clickZ };

    ManualSegmentationManager::TransformFromProjectionTo3D(
        ijkProjectionClick, ijk3DClick, projectionType
    );

    ImageType3D::IndexType idxClick{ ijk3DClick[0], ijk3DClick[1], ijk3DClick[2] };
    ImageType3D::IndexType idxPixel{ ijk3DPixel[0], ijk3DPixel[1], ijk3DPixel[2] };

    if (region.IsInside(idxClick) == false || region.IsInside(idxPixel) == false)
    {
        return false;
    }

    const auto wctr = out->GetPixel(idxClick);
    const auto widx = out->GetPixel(idxPixel);
    return wctr == widx;
}

BrushWatershedPipeline::WatershedFilterType::Pointer BrushWatershedPipeline::GetFilter()
{
    return watershedFilter;
}
