#include "ManualSegmentationManager.hpp"

ManualSegmentationManager::ManualSegmentationManager() = default;

void ManualSegmentationManager::TransformFromProjectionTo3D(
    const int projectionIJK[3], int* ijk3D, ManualSegmentationManager::ProjectionType projectionType
)
{
    if (projectionType == ManualSegmentationManager::ProjectionType::Sagittal)
    {
        ijk3D[0] = projectionIJK[2];
        ijk3D[1] = projectionIJK[0];
        ijk3D[2] = projectionIJK[1];
    }
    else if (projectionType == ManualSegmentationManager::ProjectionType::Coronal)
    {
        ijk3D[0] = projectionIJK[0];
        ijk3D[1] = projectionIJK[2];
        ijk3D[2] = projectionIJK[1];
    }
    else
    {
        ijk3D[0] = projectionIJK[0];
        ijk3D[1] = projectionIJK[1];
        ijk3D[2] = projectionIJK[2];
    }
}

void ManualSegmentationManager::CreateEmptyMask(SegmentationMask::VTK2ITKFilterType::Pointer reader)
{
    ClearSurfaces();
    storage.CreateEmptyMask(reader);
}

void ManualSegmentationManager::ClearSurfaces()
{
    scene.ClearSurfaces();
    storage.dirtyLabels.clear();
}

void ManualSegmentationManager::RenderMask()
{
    scene.RenderMask(storage, metadata);
}

void ManualSegmentationManager::UpdateLabelAppearance()
{
    scene.UpdateLabelAppearance(metadata);
}

void ManualSegmentationManager::RemoveLabel(unsigned short label)
{
    storage.RemoveLabel(label);
    scene.RemoveLabel(label);
}

ManualSegmentationManager::PixelData ManualSegmentationManager::ColorFor(unsigned short label) const
{
    return metadata.ColorFor(label);
}

bool ManualSegmentationManager::CanPaint() const
{
    return storage.mask && metadata.activeLabel != 0 && ColorFor(metadata.activeLabel).a != 0;
}

bool ManualSegmentationManager::IsPixelSet(int x, int y, int z, unsigned short expectedValue)
{
    return storage.IsPixelSet(x, y, z, expectedValue);
}

bool ManualSegmentationManager::IsSegmentSet(
    int x1, int x2, int y, int z, unsigned short expectedValue
)
{
    return storage.IsSegmentSet(x1, x2, y, z, expectedValue);
}

bool ManualSegmentationManager::PaintSegment(int y, int z, int x1, int x2, unsigned short value)
{
    return storage.PaintSegment(y, z, x1, x2, value);
}

std::vector<std::pair<int, int>> ManualSegmentationManager::FindUncoveredIntervalsInLine(
    int x1, int x2, int y, int z, unsigned short expectedValue
)
{
    return storage.FindUncoveredIntervalsInLine(x1, x2, y, z, expectedValue);
}

std::vector<std::pair<int, int>> ManualSegmentationManager::FindIntervalsWithValue(
    int x1, int x2, int y, int z, unsigned short targetValue
)
{
    return storage.FindIntervalsWithValue(x1, x2, y, z, targetValue);
}
