#include "LabelMeshBuilder.hpp"

#include <vtkImageData.h>
#include <vtkDiscreteFlyingEdges3D.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <itkImageRegionConstIterator.h>
#include <algorithm>

LabelMeshBuilder::Meshes LabelMeshBuilder::Build(
    const SegmentationMask& storage, const std::set<unsigned short>& requested
)
{
    const auto& mask = storage.mask;
    if (mask == nullptr || requested.empty())
    {
        return {};
    }
    using Buffer2D = SegmentationMask::Buffer2D;
    const auto region = mask->GetLargestPossibleRegion();
    const auto start = region.GetIndex();
    const auto size = region.GetSize();
    struct Bounds
    {
        int low[3];
        int high[3];
    };
    std::map<unsigned short, Bounds> bounds;
    auto buffer = mask->GetBuffer();
    itk::ImageRegionConstIterator<Buffer2D> it(buffer, buffer->GetBufferedRegion());
    for (it.GoToBegin(); it.IsAtEnd() == false; ++it)
    {
        const auto index = it.GetIndex();
        int x = static_cast<int>(start[0]);
        for (const auto& run : it.Get())
        {
            if (run.second != 0 && requested.count(run.second))
            {
                auto [pos, inserted] = bounds.try_emplace(run.second);
                if (inserted)
                {
                    pos->second = { { x, static_cast<int>(index[0]), static_cast<int>(index[1]) },
                                    { x + run.first - 1,
                                      static_cast<int>(index[0]),
                                      static_cast<int>(index[1]) } };
                }
                else
                {
                    pos->second.low[0] = std::min(pos->second.low[0], x);
                    pos->second.high[0] = std::max(pos->second.high[0], x + run.first - 1);
                    pos->second.low[1] = std::min(pos->second.low[1], static_cast<int>(index[0]));
                    pos->second.high[1] = std::max(pos->second.high[1], static_cast<int>(index[0]));
                    pos->second.low[2] = std::min(pos->second.low[2], static_cast<int>(index[1]));
                    pos->second.high[2] = std::max(pos->second.high[2], static_cast<int>(index[1]));
                }
            }
            x += run.first;
        }
    }

    Meshes rebuilt;
    const auto direction = mask->GetDirection();
    double matrix[9];
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            matrix[r * 3 + c] = direction[r][c];
        }
    }
    for (const auto label : requested)
    {
        const auto found = bounds.find(label);
        if (found == bounds.end())
        {
            continue;
        }
        const auto& box = found->second;
        int low[3], high[3];
        for (int axis = 0; axis < 3; ++axis)
        {
            // A zero border also closes surfaces touching the volume boundary.
            low[axis] = box.low[axis] - 1;
            high[axis] = box.high[axis] + 1;
        }
        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetExtent(low[0], high[0], low[1], high[1], low[2], high[2]);
        image->SetSpacing(mask->GetSpacing().GetDataPointer());
        image->SetOrigin(mask->GetOrigin().GetDataPointer());
        image->SetDirectionMatrix(matrix);
        image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
        for (int z = low[2]; z <= high[2]; ++z)
        {
            for (int y = low[1]; y <= high[1]; ++y)
            {
                auto* pixels = static_cast<unsigned char*>(image->GetScalarPointer(low[0], y, z));
                std::fill_n(pixels, high[0] - low[0] + 1, 0);
                if (y < start[1] || y >= start[1] + static_cast<int>(size[1]) || z < start[2] ||
                    z >= start[2] + static_cast<int>(size[2]))
                {
                    continue;
                }
                Buffer2D::IndexType index = { { y, z } };
                int x = static_cast<int>(start[0]);
                for (const auto& run : buffer->GetPixel(index))
                {
                    if (run.second == label)
                    {
                        const int from = std::max(x, low[0]);
                        const int to = std::min(x + static_cast<int>(run.first), high[0] + 1);
                        if (from < to)
                        {
                            std::fill_n(pixels + from - low[0], to - from, 1);
                        }
                    }
                    x += run.first;
                }
            }
        }
        auto surface = vtkSmartPointer<vtkDiscreteFlyingEdges3D>::New();
        surface->SetInputData(image);
        surface->SetValue(0, 1);
        surface->ComputeScalarsOff();
        surface->ComputeGradientsOff();
        surface->ComputeNormalsOff();
        surface->Update();
        if (surface->GetOutput()->GetNumberOfPoints() == 0)
        {
            continue;
        }
        auto normals = vtkSmartPointer<vtkPolyDataNormals>::New();
        normals->SetInputConnection(surface->GetOutputPort());
        normals->ConsistencyOn();
        normals->SplittingOff();
        normals->AutoOrientNormalsOn();
        normals->Update();
        auto mesh = vtkSmartPointer<vtkPolyData>::New();
        mesh->DeepCopy(normals->GetOutput());
        rebuilt.emplace(label, mesh);
    }
    return rebuilt;
}
