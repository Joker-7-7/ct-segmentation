#pragma once
#include "SegmentationMask.hpp"
#include "LabelMetadataStore.hpp"
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <map>

// Owns scene actors. All methods run on the render thread.
class LabelRegistry
{
public:
    void RenderMask(SegmentationMask& storage, const LabelMetadataStore& metadata);
    void ClearSurfaces();
    void RemoveLabel(unsigned short label);
    void UpdateLabelAppearance(const LabelMetadataStore& metadata);
    vtkSmartPointer<vtkRenderer> sceneRenderer;
    std::map<unsigned short, vtkSmartPointer<vtkActor>> labelActors;
    std::set<unsigned short> renderedLabels;
};
