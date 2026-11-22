#pragma once
#include <vtkSmartPointer.h>
#include "SegmentationMask.hpp"
#include <vtkPolyData.h>
#include <map>

// Stateless geometry construction; never attaches actors to a renderer.
class LabelMeshBuilder
{
public:
    using Meshes = std::map<unsigned short, vtkSmartPointer<vtkPolyData>>;
    static Meshes Build(const SegmentationMask& storage, const std::set<unsigned short>& requested);
};
