#pragma once

#include "LabelMetadataStore.hpp"
#include "SegmentationMask.hpp"
#include "LabelRegistry.hpp"
#include <boost/signals2/signal.hpp>

#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkNew.h>

#include <vector>
#include <map>
#include <set>

// Render-thread coordinator: owns mask storage, metadata and the scene registry.
// BrushPaintContext edits storage; LabelRegistry uses LabelMeshBuilder to publish geometry.
// UI state crosses threads as value snapshots through SegmentationAdapter.
class ManualSegmentationManager
{
public:
    using PixelData = LabelMetadataStore::PixelData;

    enum class ProjectionType
    {
        Sagittal,
        Coronal,
        Axial,
    };

    ManualSegmentationManager();

    void CreateEmptyMask(SegmentationMask::VTK2ITKFilterType::Pointer reader);

    static void TransformFromProjectionTo3D(
        const int originalIJK[3], int* transformedIJK, ProjectionType projectionType
    );

    bool IsPixelSet(int x, int y, int z, unsigned short expectedValue);

    bool IsSegmentSet(int x1, int x2, int y, int z, unsigned short expectedValue);

    bool PaintSegment(int y, int z, int x1, int x2, unsigned short value);

    void RenderMask();
    void RemoveLabel(unsigned short label);
    void ClearSurfaces();
    void UpdateLabelAppearance();
    PixelData ColorFor(unsigned short label) const;
    bool CanPaint() const;
    boost::signals2::signal<void()> maskChanged;

    std::vector<std::pair<int, int>> FindUncoveredIntervalsInLine(
        int x1, int x2, int y, int z, unsigned short expectedValue
    );

    std::vector<std::pair<int, int>> FindIntervalsWithValue(
        int x1, int x2, int y, int z, unsigned short targetValue
    );

    SegmentationMask storage;
    LabelMetadataStore metadata;
    LabelRegistry scene;
};
