#include "BrushPaintContext.hpp"
#include <algorithm>

using namespace Backend::Overlay;
namespace
{
    constexpr double DELTA = 1e-9;

    bool CheckPixelBorders(int i, int j, vtkImageData* image)
    {
        int* extents = image->GetExtent();

        int xCheck = std::clamp(i, extents[0], extents[1]);
        int yCheck = std::clamp(j, extents[2], extents[3]);

        return xCheck != i || yCheck != j;
    }
}

void BrushPaintContext::PaintSquare(int x, int y)
{
    PaintBrush2D(x, y, [](int /*j*/, int x, int /*y*/, int R) {
        return std::make_pair(x - R, x + R);
    });
}

void BrushPaintContext::PaintCircle(int x, int y)
{
    x += 1;
    y += 1;
    PaintBrush2D(x, y, [](int j, int x, int y, int R) {
        const double R2 = double(R) * R + DELTA;
        const double dy = (j + 0.5) - double(y);
        const double rhs = R2 - dy * dy;

        if (rhs < 0.0)
        {
            return std::make_pair(1, 0);
        }

        const double dx = std::sqrt(rhs);

        const int x1 = static_cast<int>(std::ceil(x - dx - 0.5));
        const int x2 = static_cast<int>(std::floor(x + dx - 0.5));

        if (x1 > x2)
        {
            return std::make_pair(1, 0);
        }

        return std::make_pair(x1, x2);
    });
}

void BrushPaintContext::PaintSpan2D(
    int j_proj, int x1_proj, int x2_proj, int xClicked, int yClicked
)
{
    const unsigned short DRAW_VALUE = segmentationManager_->metadata.activeLabel;
    const unsigned short ERASE_VALUE = 0;

    auto rle = segmentationManager_->storage.mask;
    if (rle == nullptr)
    {
        return;
    }
    const auto region = rle->GetLargestPossibleRegion();
    const int axis = projectionType_ == ManualSegmentationManager::ProjectionType::Sagittal ? 1 : 0;
    x1_proj = std::max(x1_proj, static_cast<int>(region.GetIndex()[axis]));
    x2_proj =
        std::min(x2_proj, static_cast<int>(region.GetIndex()[axis] + region.GetSize()[axis] - 1));
    if (x1_proj > x2_proj)
    {
        return;
    }
    if (projectionType_ == ManualSegmentationManager::ProjectionType::Axial ||
        projectionType_ == ManualSegmentationManager::ProjectionType::Coronal)
    {
        int realY = j_proj;
        int realZ = slice_;
        if (projectionType_ == ManualSegmentationManager::ProjectionType::Coronal)
        {
            realY = slice_;
            realZ = j_proj;
        }

        if (eraseMode)
        {
            auto intervals = segmentationManager_->FindIntervalsWithValue(
                x1_proj, x2_proj, realY, realZ, DRAW_VALUE
            );

            for (auto [a, b] : intervals)
            {
                strokeChanged_ |=
                    segmentationManager_->PaintSegment(realY, realZ, a, b, ERASE_VALUE);
                for (int i = a; i <= b; ++i)
                {
                    ErasePixel(i, j_proj, slice_);
                }
            }
        }
        else
        {
            if (segmentationManager_->IsSegmentSet(x1_proj, x2_proj, realY, realZ, DRAW_VALUE))
            {
                return;
            }

            auto brush = settings_.GetBrushMode();

            auto intervals = segmentationManager_->FindUncoveredIntervalsInLine(
                x1_proj, x2_proj, realY, realZ, DRAW_VALUE
            );

            if (brush == PaintbrushSettings::BrushMode::Adaptive2D)
            {
                filterPipeline_.FindWatershedIntervalsInLine(
                    x1_proj, j_proj, slice_, xClicked, yClicked, slice_, intervals
                );
            }

            for (auto [a, b] : intervals)
            {
                strokeChanged_ |=
                    segmentationManager_->PaintSegment(realY, realZ, a, b, DRAW_VALUE);
                for (int i = a; i <= b; ++i)
                {
                    DrawPixel(i, j_proj, slice_);
                }
            }
        }
    }
    else if (projectionType_ == ManualSegmentationManager::ProjectionType::Sagittal)
    {
        int realX = slice_;

        auto region = rle->GetLargestPossibleRegion();
        auto size = region.GetSize();
        int sizeY = static_cast<int>(size[1]);
        int sizeZ = static_cast<int>(size[2]);

        for (int i = x1_proj; i <= x2_proj; ++i)
        {
            int realY = i;
            int realZ = j_proj;

            if (realY < 0 || realY >= sizeY || realZ < 0 || realZ >= sizeZ)
            {
                continue;
            }

            bool isSet = segmentationManager_->IsPixelSet(realX, realY, realZ, DRAW_VALUE);

            if (eraseMode == false && isSet)
            {
                continue;
            }
            if (eraseMode && isSet == false)
            {
                continue;
            }

            auto brush = settings_.GetBrushMode();
            if (brush == PaintbrushSettings::BrushMode::Adaptive2D && eraseMode == false &&
                filterPipeline_.CheckFilterPixel(xClicked, yClicked, slice_, i, j_proj, slice_) ==
                    false)
            {
                continue;
            }

            strokeChanged_ |= segmentationManager_->PaintSegment(
                realY, realZ, realX, realX, eraseMode ? ERASE_VALUE : DRAW_VALUE
            );

            if (eraseMode)
            {
                ErasePixel(i, j_proj, slice_);
            }
            else
            {
                DrawPixel(i, j_proj, slice_);
            }
        }
    }
}

void BrushPaintContext::PaintSpan3D(
    int jy, int kz, int x1_proj, int x2_proj, int xClicked, int yClicked
)
{
    const unsigned short DRAW_VALUE = segmentationManager_->metadata.activeLabel;
    const unsigned short ERASE_VALUE = 0;

    auto rle = segmentationManager_->storage.mask;
    if (rle == nullptr)
    {
        return;
    }

    auto region = rle->GetLargestPossibleRegion();
    auto size = region.GetSize();
    int sizeX = static_cast<int>(size[0]);
    int sizeY = static_cast<int>(size[1]);
    int sizeZ = static_cast<int>(size[2]);

    int x1 = std::max(0, x1_proj);
    int x2 = std::min(
        (projectionType_ == ManualSegmentationManager::ProjectionType::Sagittal ? sizeY : sizeX) -
            1,
        x2_proj
    );
    if (x2 < x1)
    {
        return;
    }

    if (projectionType_ == ManualSegmentationManager::ProjectionType::Axial ||
        projectionType_ == ManualSegmentationManager::ProjectionType::Coronal)
    {
        int realY = jy;
        int realZ = kz;

        if (projectionType_ == ManualSegmentationManager::ProjectionType::Coronal)
        {
            realY = kz;
            realZ = jy;
        }

        if (realY < 0 || realY >= sizeY || realZ < 0 || realZ >= sizeZ)
        {
            return;
        }

        if (eraseMode)
        {
            auto intervals =
                segmentationManager_->FindIntervalsWithValue(x1, x2, realY, realZ, DRAW_VALUE);

            for (auto [a, b] : intervals)
            {
                strokeChanged_ |=
                    segmentationManager_->PaintSegment(realY, realZ, a, b, ERASE_VALUE);

                if (projectionType_ == ManualSegmentationManager::ProjectionType::Axial)
                {
                    if (realZ == slice_)
                    {
                        for (int i = a; i <= b; ++i)
                        {
                            ErasePixel(i, jy, kz);
                        }
                    }
                }
                else
                {
                    if (realY == slice_)
                    {
                        for (int i = a; i <= b; ++i)
                        {
                            ErasePixel(i, jy, kz);
                        }
                    }
                }
            }
        }
        else
        {
            if (segmentationManager_->IsSegmentSet(x1, x2, realY, realZ, DRAW_VALUE))
            {
                return;
            }

            auto intervals = segmentationManager_->FindUncoveredIntervalsInLine(
                x1, x2, realY, realZ, DRAW_VALUE
            );

            auto brush = settings_.GetBrushMode();
            if (brush == PaintbrushSettings::BrushMode::Adaptive3D)
            {
                filterPipeline_.FindWatershedIntervalsInLine(
                    x1_proj, jy, kz, xClicked, yClicked, slice_, intervals
                );
            }

            for (auto [a, b] : intervals)
            {
                strokeChanged_ |=
                    segmentationManager_->PaintSegment(realY, realZ, a, b, DRAW_VALUE);

                if (projectionType_ == ManualSegmentationManager::ProjectionType::Axial)
                {
                    if (realZ == slice_)
                    {
                        for (int i = a; i <= b; ++i)
                        {
                            DrawPixel(i, jy, kz);
                        }
                    }
                }
                else
                {
                    if (realY == slice_)
                    {
                        for (int i = a; i <= b; ++i)
                        {
                            DrawPixel(i, jy, kz);
                        }
                    }
                }
            }
        }
    }
    else if (projectionType_ == ManualSegmentationManager::ProjectionType::Sagittal)
    {
        int realX = kz;
        int realZ = jy;

        if (realZ < 0 || realZ >= sizeZ || realX < 0 || realX >= sizeX)
        {
            return;
        }

        for (int realY = x1; realY <= x2; ++realY)
        {
            if (realY < 0 || realY >= sizeY)
            {
                continue;
            }

            bool isSet = segmentationManager_->IsPixelSet(realX, realY, realZ, DRAW_VALUE);

            if (eraseMode == false && isSet)
            {
                continue;
            }

            if (eraseMode && isSet == false)
            {
                continue;
            }

            auto brush = settings_.GetBrushMode();
            if (brush == PaintbrushSettings::BrushMode::Adaptive3D && eraseMode == false &&
                filterPipeline_.CheckFilterPixel(xClicked, yClicked, slice_, realY, jy, kz) ==
                    false)
            {
                continue;
            }

            strokeChanged_ |= segmentationManager_->PaintSegment(
                realY, realZ, realX, realX, eraseMode ? ERASE_VALUE : DRAW_VALUE
            );

            if (realX == slice_)
            {
                if (eraseMode)
                {
                    ErasePixel(realY, jy, kz);
                }
                else
                {
                    DrawPixel(realY, jy, kz);
                }
            }
        }
    }
}

void BrushPaintContext::PaintCube(int x, int y, int z)
{
    PaintBrush3D(x, y, z, [](int /*jy*/, int /*kz*/, int xCenter, int /*y*/, int /*z*/, int R) {
        return std::make_pair(xCenter - R, xCenter + R);
    });
}

void BrushPaintContext::PaintSphere(int x, int y, int z)
{
    x += 1;
    y += 1;
    z += 1;
    PaintBrush3D(x, y, z, [](int y_proj, int z_proj, int x, int y, int z, int R) {
        const double R2 = double(R) * R + DELTA;
        const double dy = (y_proj + 0.5) - double(y);
        const double dz = (z_proj + 0.5) - double(z);

        const double rhs = R2 - dy * dy - dz * dz;
        if (rhs < 0.0)
        {
            return std::make_pair(1, 0);
        }

        const double dx = std::sqrt(rhs);

        const int x1 = static_cast<int>(std::ceil(x - dx - 0.5));
        const int x2 = static_cast<int>(std::floor(x + dx - 0.5));

        if (x1 > x2)
        {
            return std::make_pair(1, 0);
        }

        return std::make_pair(x1, x2);
    });
}

bool BrushPaintContext::DrawPixel(int x, int y, int z)
{
    if (CheckPixelBorders(x, y, imageData_))
    {
        return false;
    }

    const auto* unitScalar =
        static_cast<const unsigned char*>(copyImageData_->GetScalarPointer(x, y, 0));
    auto* dst = static_cast<unsigned char*>(imageData_->GetScalarPointer(x, y, 0));
    if (unitScalar == nullptr || dst == nullptr)
    {
        return false;
    }
    ManualSegmentationManager::PixelData originalColor = { static_cast<uint8_t>(unitScalar[0]),
                                                           static_cast<uint8_t>(unitScalar[1]),
                                                           static_cast<uint8_t>(unitScalar[2]),
                                                           static_cast<uint8_t>(unitScalar[3]) };

    ManualSegmentationManager::PixelData canvasColor =
        segmentationManager_->ColorFor(segmentationManager_->metadata.activeLabel);

    ManualSegmentationManager::PixelData newColor = {
        static_cast<uint8_t>(
            (canvasColor.r * canvasColor.a + originalColor.r * (255 - canvasColor.a)) / 255.0
        ),
        static_cast<uint8_t>(
            (canvasColor.g * canvasColor.a + originalColor.g * (255 - canvasColor.a)) / 255.0
        ),
        static_cast<uint8_t>(
            (canvasColor.b * canvasColor.a + originalColor.b * (255 - canvasColor.a)) / 255.0
        ),
        255
    };

    dst[0] = newColor.r;
    dst[1] = newColor.g;
    dst[2] = newColor.b;
    dst[3] = newColor.a;
    return true;
}

bool BrushPaintContext::ErasePixel(int x, int y, int z)
{
    if (CheckPixelBorders(x, y, imageData_))
    {
        return false;
    }

    unsigned char* src = static_cast<unsigned char*>(copyImageData_->GetScalarPointer(x, y, 0));
    unsigned char* dst = static_cast<unsigned char*>(imageData_->GetScalarPointer(x, y, 0));
    if (src && dst)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
    }

    return true;
}

void BrushPaintContext::ReloadPaintedPixels()
{
    if (segmentationManager_ == nullptr || segmentationManager_->storage.mask == nullptr ||
        imageData_ == nullptr || copyImageData_ == nullptr ||
        copyImageData_->GetScalarPointer() == nullptr)
    {
        return;
    }
    imageData_->DeepCopy(copyImageData_);
    auto mask = segmentationManager_->storage.mask;
    const auto region = mask->GetLargestPossibleRegion();
    const auto* extent = imageData_->GetExtent();
    const auto start = region.GetIndex();
    const auto size = region.GetSize();
    auto buffer = mask->GetBuffer();
    auto blend = [&](int x, int y, int count, unsigned short label) {
        const auto color = segmentationManager_->ColorFor(label);
        if (color.a == 0 || y < extent[2] || y > extent[3])
        {
            return;
        }
        const int end = std::min(x + count - 1, extent[1]);
        x = std::max(x, extent[0]);
        if (x > end)
        {
            return;
        }
        auto* pixel = static_cast<unsigned char*>(imageData_->GetScalarPointer(x, y, 0));
        for (; x <= end; ++x, pixel += 4)
        {
            pixel[0] = (color.r * color.a + pixel[0] * (255 - color.a)) / 255;
            pixel[1] = (color.g * color.a + pixel[1] * (255 - color.a)) / 255;
            pixel[2] = (color.b * color.a + pixel[2] * (255 - color.a)) / 255;
        }
    };
    const int normal = projectionType_ == ManualSegmentationManager::ProjectionType::Axial     ? 2
                       : projectionType_ == ManualSegmentationManager::ProjectionType::Coronal ? 1
                                                                                               : 0;
    if (slice_ < start[normal] || slice_ >= start[normal] + static_cast<int>(size[normal]))
    {
        return;
    }
    if (normal == 0)
    {
        for (int z = start[2]; z < start[2] + static_cast<int>(size[2]); ++z)
        {
            for (int y = start[1]; y < start[1] + static_cast<int>(size[1]); ++y)
            {
                SegmentationMask::Buffer2D::IndexType index = { { y, z } };
                int x = static_cast<int>(start[0]);
                for (const auto& run : buffer->GetPixel(index))
                {
                    if (slice_ < x + run.first)
                    {
                        blend(y, z, 1, run.second);
                        break;
                    }
                    x += run.first;
                }
            }
        }
    }
    else
    {
        const int vertical = normal == 2 ? 1 : 2;
        for (int row = start[vertical]; row < start[vertical] + static_cast<int>(size[vertical]);
             ++row)
        {
            SegmentationMask::Buffer2D::IndexType index = {
                { normal == 2 ? row : slice_, normal == 2 ? slice_ : row }
            };
            int x = static_cast<int>(start[0]);
            for (const auto& run : buffer->GetPixel(index))
            {
                blend(x, row, run.first, run.second);
                x += run.first;
            }
        }
    }
    imageData_->Modified();
}
