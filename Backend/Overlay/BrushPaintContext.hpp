#pragma once
#include "PaintbrushSettings.hpp"
#include "BrushWatershedPipeline.h"
#include "Backend/Segmentation/ManualSegmentationManager.hpp"
#include <vtkImageData.h>
#include <cmath>

namespace Backend::Overlay
{
    // Painting state and algorithms, independent of mouse events and cursor actors.
    class BrushPaintContext
    {
    public:
        void PaintSquare(int x, int y);
        void PaintCircle(int x, int y);
        void PaintSpan2D(int j_proj, int x1_proj, int x2_proj, int xClicked, int yClicked);
        void PaintSpan3D(int jy, int kz, int x1_proj, int x2_proj, int xClicked, int yClicked);
        void PaintCube(int x, int y, int z);
        void PaintSphere(int x, int y, int z);
        bool DrawPixel(int x, int y, int z);
        bool ErasePixel(int x, int y, int z);
        void ReloadPaintedPixels();

    private:
        friend class PaintbrushInteractorStyle;
        vtkSmartPointer<vtkImageData> imageData_;
        vtkSmartPointer<vtkImageData> copyImageData_ = vtkSmartPointer<vtkImageData>::New();
        int slice_ = 0;
        bool eraseMode = false;
        bool strokeChanged_ = false;
        BrushWatershedPipeline filterPipeline_;
        PaintbrushSettings settings_;
        ManualSegmentationManager::ProjectionType projectionType_ =
            ManualSegmentationManager::ProjectionType::Axial;
        ManualSegmentationManager* segmentationManager_ =
            nullptr; // Owned by the interactor's shared manager.

    private:
        template <typename SpanFunc> void PaintBrush2D(int x, int y, SpanFunc spanFunc);
        template <typename SpanFunc> void PaintBrush3D(int x, int y, int z, SpanFunc spanFunc);
    };
    template <typename SpanFunc>
    void BrushPaintContext::PaintBrush2D(int x, int y, SpanFunc spanFunc)
    {
        int R = static_cast<int>(std::lround(settings_.GetRadius()));

        auto brush = settings_.GetBrushMode();
        if (eraseMode == false && brush == PaintbrushSettings::BrushMode::Adaptive2D)
        {
            filterPipeline_.PrecomputeWatersheds(x, y, slice_, R + 5);
        }

        for (int j = y - R; j <= y + R; ++j)
        {
            auto [x1, x2] = spanFunc(j, x, y, R);
            if (x1 > x2)
            {
                continue;
            }

            PaintSpan2D(j, x1, x2, x, y);
        }

        imageData_->Modified();
    }

    template <typename SpanFunc>
    void BrushPaintContext::PaintBrush3D(int x, int y, int z, SpanFunc spanFunc)
    {
        int R = static_cast<int>(std::lround(settings_.GetRadius()));

        auto brush = settings_.GetBrushMode();
        if (eraseMode == false && brush == PaintbrushSettings::BrushMode::Adaptive3D)
        {
            filterPipeline_.PrecomputeWatersheds(x, y, z, R + 5);
        }

        for (int z_proj = z - R; z_proj <= z + R; ++z_proj)
        {
            for (int j_proj = y - R; j_proj <= y + R; ++j_proj)
            {
                auto [x1, x2] = spanFunc(j_proj, z_proj, x, y, z, R);
                if (x1 > x2)
                {
                    continue;
                }

                PaintSpan3D(j_proj, z_proj, x1, x2, x, y);
            }
        }

        imageData_->Modified();
    }

}
