#include "LabelRegistry.hpp"
#include "LabelMeshBuilder.hpp"
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

void LabelRegistry::RenderMask(SegmentationMask& storage, const LabelMetadataStore& metadata)
{
    const auto& mask = storage.mask;
    auto& dirtyLabels = storage.dirtyLabels;
    const auto& labels = metadata.labels;
    if (mask == nullptr || sceneRenderer == nullptr)
    {
        return;
    }
    const bool firstSurface = labelActors.empty();
    std::set<unsigned short> requested = dirtyLabels;
    for (const auto& label : labels)
    {
        if (renderedLabels.count(label.label) == 0)
        {
            requested.insert(label.label);
        }
    }
    if (requested.empty())
    {
        return;
    }

    std::set<unsigned short> buildLabels;
    for (const auto& label : labels)
    {
        if (requested.count(label.label))
        {
            buildLabels.insert(label.label);
        }
    }
    const auto meshes = LabelMeshBuilder::Build(storage, buildLabels);
    std::map<unsigned short, vtkSmartPointer<vtkActor>> rebuilt;
    for (const auto& entry : meshes)
    {
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(entry.second);
        mapper->ScalarVisibilityOff();
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetInterpolationToPhong();
        rebuilt.emplace(entry.first, actor);
    }
    for (const auto label : requested)
    {
        const auto old = labelActors.find(label);
        if (old != labelActors.end())
        {
            sceneRenderer->RemoveActor(old->second);
            labelActors.erase(old);
        }
        const auto next = rebuilt.find(label);
        if (next != rebuilt.end())
        {
            sceneRenderer->AddActor(next->second);
            labelActors.emplace(label, next->second);
        }
    }
    for (const auto& label : labels)
    {
        if (requested.count(label.label))
        {
            renderedLabels.insert(label.label);
        }
    }
    dirtyLabels.clear();
    UpdateLabelAppearance(metadata);
    if (firstSurface && labelActors.empty() == false)
    {
        sceneRenderer->ResetCamera();
    }
}

void LabelRegistry::ClearSurfaces()
{
    if (sceneRenderer)
    {
        for (const auto& entry : labelActors)
        {
            sceneRenderer->RemoveActor(entry.second);
        }
    }
    labelActors.clear();
    renderedLabels.clear();
}

void LabelRegistry::UpdateLabelAppearance(const LabelMetadataStore& metadata)
{
    for (const auto& entry : labelActors)
    {
        const auto color = metadata.ColorFor(entry.first);
        entry.second->SetVisibility(color.a != 0);
        entry.second->GetProperty()->SetColor(color.r / 255.0, color.g / 255.0, color.b / 255.0);
    }
}

void LabelRegistry::RemoveLabel(unsigned short label)
{
    const auto it = labelActors.find(label);
    if (it != labelActors.end())
    {
        if (sceneRenderer)
        {
            sceneRenderer->RemoveActor(it->second);
        }
        labelActors.erase(it);
    }
    renderedLabels.erase(label);
}
