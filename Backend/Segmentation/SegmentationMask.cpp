#include "SegmentationMask.hpp"
#include <itkImageRegionIterator.h>
#include <algorithm>

void SegmentationMask::CreateEmptyMask(VTK2ITKFilterType::Pointer reader)
{
    dirtyLabels.clear();
    maskReader = reader;
    ImageType3D::RegionType region = reader->GetOutput()->GetLargestPossibleRegion();

    mask = SegmentationMask::RLE3Type::New();
    mask->SetLargestPossibleRegion(region);
    mask->SetBufferedRegion(region); // buffered region must cover full X-lines
    mask->Allocate(true);            // fill with default PixelType() == 0
    mask->SetOnTheFlyCleanup(true);  // merge adjacent runs with the same value
    mask->SetSpacing(reader->GetOutput()->GetSpacing());
    mask->SetOrigin(reader->GetOutput()->GetOrigin());
    mask->SetDirection(reader->GetOutput()->GetDirection());
}

void SegmentationMask::RemoveLabel(unsigned short label)
{
    if (mask)
    {
        auto buffer = mask->GetBuffer();
        itk::ImageRegionIterator<Buffer2D> it(buffer, buffer->GetBufferedRegion());
        for (it.GoToBegin(); it.IsAtEnd() == false; ++it)
        {
            auto& line = buffer->GetPixel(it.GetIndex());
            RLLine compact;
            compact.reserve(line.size());
            for (auto run : line)
            {
                if (run.second == label)
                {
                    run.second = 0;
                }
                if (compact.empty() == false && compact.back().second == run.second)
                {
                    compact.back().first += run.first;
                }
                else
                {
                    compact.push_back(run);
                }
            }
            line.swap(compact);
        }
        mask->Modified();
    }
    dirtyLabels.erase(label);
}

bool SegmentationMask::IsPixelSet(int x, int y, int z, unsigned short expectedValue)
{
    using BufferType = typename RLE3Type::BufferType;
    using BufferIndexType = typename BufferType::IndexType;
    using RLLine = typename RLE3Type::InternalPixelType;
    using IndexValueType = typename RLE3Type::IndexValueType;

    // Check bounds against the largest region.
    typename SegmentationMask::RLE3Type::RegionType region = mask->GetLargestPossibleRegion();
    typename SegmentationMask::RLE3Type::IndexType idx3;
    idx3[0] = x;
    idx3[1] = y;
    idx3[2] = z;
    if (region.IsInside(idx3) == false)
    {
        return false;
    }
    // Form the buffer index for (y, z).
    BufferIndexType bidx;
    bidx[0] = y;
    bidx[1] = z;

    auto buffer = mask->GetBuffer();
    if (buffer == nullptr)
    {
        return false;
    }
    RLLine& line = buffer->GetPixel(bidx); // RLE line along X

    // Accumulate run lengths until reaching x.
    IndexValueType X = region.GetIndex()[0];
    for (const auto& seg : line)
    {
        IndexValueType count = static_cast<IndexValueType>(seg.first);
        if (x >= static_cast<int>(X) && x < static_cast<int>(X + count))
        {
            // x falls inside this run.
            return seg.second == expectedValue;
        }
        X += count;
    }

    return false;
}

bool SegmentationMask::IsSegmentSet(int x1, int x2, int y, int z, unsigned short expectedValue)
{
    using BufferType = typename SegmentationMask::RLE3Type::BufferType;
    using BufferIndexType = typename BufferType::IndexType;
    using RLLine = typename SegmentationMask::RLE3Type::InternalPixelType;
    using IndexValueType = typename SegmentationMask::RLE3Type::IndexValueType;

    // Check 3D bounds.
    typename SegmentationMask::RLE3Type::RegionType region = mask->GetLargestPossibleRegion();
    typename SegmentationMask::RLE3Type::IndexType idx3;
    idx3[0] = x1;
    idx3[1] = y;
    idx3[2] = z;
    if (region.IsInside(idx3) == false)
    {
        return false;
    }
    idx3[0] = x2;
    if (region.IsInside(idx3) == false)
    {
        return false;
    }
    BufferIndexType bidx;
    bidx[0] = y;
    bidx[1] = z;

    auto buffer = mask->GetBuffer();
    if (buffer == nullptr)
    {
        return false;
    }
    RLLine& line = buffer->GetPixel(bidx);

    IndexValueType X = region.GetIndex()[0];
    for (const auto& seg : line)
    {
        IndexValueType count = static_cast<IndexValueType>(seg.first);
        IndexValueType segStart = X;
        IndexValueType segEnd = X + count - 1;

        if (segEnd < x1)
        {
            X += count;
            continue;
        }
        if (segStart > x2)
        {
            break;
        }
        // Run intersection with [x1..x2].
        IndexValueType interL = std::max<IndexValueType>(segStart, x1);
        IndexValueType interR = std::min<IndexValueType>(segEnd, x2);

        // A different value in the intersection means the interval is not fully set.
        if (seg.second != expectedValue)
        {
            return false;
        }
        X += count;
    }

    // All intersecting runs match expectedValue (or there were none).
    return true;
}

bool SegmentationMask::PaintSegment(int y, int z, int x1, int x2, unsigned short value)
{
    using RLEType = SegmentationMask::RLE3Type;
    using BufferType = typename RLEType::BufferType;

    if (mask == nullptr)
    {
        return false;
    }
    const auto region = mask->GetLargestPossibleRegion();
    const auto start = region.GetIndex();
    const auto size = region.GetSize();
    if (y < start[1] || y >= start[1] + static_cast<int>(size[1]) || z < start[2] ||
        z >= start[2] + static_cast<int>(size[2]))
    {
        return false;
    }
    x1 = std::max(x1, static_cast<int>(start[0]));
    x2 = std::min(x2, static_cast<int>(start[0] + size[0] - 1));
    if (x2 < x1)
    {
        return false;
    }
    auto buffer = mask->GetBuffer();
    typename BufferType::IndexType bidx;
    bidx[0] = y;
    bidx[1] = z;
    auto& line = buffer->GetPixel(bidx);

    if (x2 < x1)
    {
        return false;
    }

    const int newLen = x2 - x1 + 1;

    // 1) Find the first run intersecting x1.
    int idx = 0;
    int64_t X = start[0]; // start position of run idx
    const int n = static_cast<int>(line.size());
    while (idx < n && (X + static_cast<int64_t>(line[idx].first) - 1) < x1)
    {
        X += line[idx].first;
        ++idx;
    }

    // If idx == n, x1 is beyond the runs despite the region check above.
    if (idx == n)
    {
        // A valid RLE line covers the full width; a missing tail is an error.
        return false;
    }

    int firstIdx = idx;
    int64_t firstSegStart = X;
    unsigned short firstVal = line[firstIdx].second;
    int firstSegCount = static_cast<int>(line[firstIdx].first);
    int leftCount = static_cast<int>(std::max<int64_t>(0, (int64_t)x1 - firstSegStart)
    ); // part of the first run left of x1

    // 2) Find the last run intersecting x2, continuing from idx.
    while (idx < n && X <= x2)
    {
        X += line[idx].first;
        ++idx;
    }
    int lastIdx = idx - 1;
    bool changed = false;
    for (int i = firstIdx; i <= lastIdx; ++i)
    {
        if (line[i].second == value)
        {
            continue;
        }
        changed = true;
        if (line[i].second != 0)
        {
            dirtyLabels.insert(line[i].second);
        }
    }
    if (changed == false)
    {
        return false;
    }
    if (value != 0)
    {
        dirtyLabels.insert(value);
    }
    int64_t afterLastPos = X; // position after lastIdx
    unsigned short lastVal = line[lastIdx].second;
    int lastSegCount = static_cast<int>(line[lastIdx].first);
    int rightCount = static_cast<int>(std::max<int64_t>(0, afterLastPos - (int64_t)(x2 + 1))
    ); // part of the last run right of x2

    // 3) Prepare up to three runs: left remainder, new value, right remainder.
    std::vector<std::pair<int, unsigned short>> inserts;
    inserts.reserve(3);

    if (leftCount > 0)
    {
        inserts.emplace_back(leftCount, firstVal);
    }

    inserts.emplace_back(newLen, value);

    if (rightCount > 0)
    {
        inserts.emplace_back(rightCount, lastVal);
    }

    // 4) Replace [firstIdx..lastIdx] with the new runs.
    auto itBeg = line.begin() + firstIdx;
    auto itEnd = line.begin() + (lastIdx + 1);
    itBeg = line.erase(itBeg, itEnd); // points to the insertion position

    // Insert the prepared runs.
    line.insert(itBeg, inserts.begin(), inserts.end());

    // 5) Merge adjacent equal-valued runs around the changed range.
    int mergeFrom = std::max(0, firstIdx - 1);
    int mergeTo = std::min(
        static_cast<int>(line.size()) - 1, firstIdx + static_cast<int>(inserts.size()) + 1
    );

    // Merge runs linearly within [mergeFrom..mergeTo].
    std::vector<std::pair<int, unsigned short>> compact;
    compact.reserve(mergeTo - mergeFrom + 3);
    for (int i = mergeFrom; i <= mergeTo; ++i)
    {
        auto cur = line[i];
        if (compact.empty() == false && compact.back().second == cur.second)
        {
            compact.back().first += cur.first;
        }
        else
        {
            compact.push_back(cur);
        }
    }

    // Replace [mergeFrom..mergeTo] with the compacted runs.
    auto itA = line.begin() + mergeFrom;
    auto itB = line.begin() + (mergeTo + 1);
    itA = line.erase(itA, itB);
    line.insert(itA, compact.begin(), compact.end());
    return true;
}

std::vector<std::pair<int, int>> SegmentationMask::FindUncoveredIntervalsInLine(
    int x1, int x2, int y, int z, unsigned short expectedValue
)
{
    using BufferType = typename SegmentationMask::RLE3Type::BufferType;
    using BufferIndexType = typename BufferType::IndexType;
    using RLLine = typename SegmentationMask::RLE3Type::InternalPixelType;
    using IndexValueType = typename SegmentationMask::RLE3Type::IndexValueType;

    std::vector<std::pair<int, int>> intervals;
    if (x2 < x1)
    {
        return {};
    }

    typename SegmentationMask::RLE3Type::RegionType region = mask->GetLargestPossibleRegion();
    typename SegmentationMask::RLE3Type::IndexType idx3;
    idx3[0] = x1;
    idx3[1] = y;
    idx3[2] = z;
    if (region.IsInside(idx3) == false)
    {
        return {};
    }
    idx3[0] = x2;
    if (region.IsInside(idx3) == false)
    {
        return {};
    }

    BufferIndexType bidx;
    bidx[0] = y;
    bidx[1] = z;

    auto buffer = mask->GetBuffer();
    if (buffer == nullptr)
    {
        return {};
    }

    RLLine& line = buffer->GetPixel(bidx);

    int cur = x1;
    IndexValueType X = region.GetIndex()[0];
    for (const auto& seg : line)
    {
        IndexValueType count = static_cast<IndexValueType>(seg.first);
        int segStart = static_cast<int>(X);
        int segEnd = static_cast<int>(X + count - 1);

        if (segEnd < x1)
        {
            X += count;
            continue;
        }

        // Stop when the run starts beyond the requested range.
        if (segStart > x2)
        {
            break;
        }

        // Run intersection with [x1..x2].
        int interL = std::max(segStart, x1);
        int interR = std::min(segEnd, x2);

        // A gap before the intersection is not covered by earlier runs.
        if (cur < interL)
        {
            // A valid RLE line has no gaps; report one as uncovered if present.
            intervals.emplace_back(cur, interL - 1);
        }

        // An intersection with a different value is uncovered.
        if (seg.second != expectedValue)
        {
            intervals.emplace_back(interL, interR);
        }

        // Advance cur past the intersection.
        cur = interR + 1;

        X += count;
        if (cur > x2)
        {
            break;
        }
    }

    // Handle the tail after the last run within [x1..x2].
    if (cur <= x2)
    {
        // Treat a tail not covered by RLE as uncovered.
        intervals.emplace_back(cur, x2);
    }
    return intervals;
}

std::vector<std::pair<int, int>> SegmentationMask::FindIntervalsWithValue(
    int x1, int x2, int y, int z, unsigned short targetValue
)
{
    std::vector<std::pair<int, int>> intervals;

    if (x2 < x1)
    {
        return {};
    }

    typename SegmentationMask::RLE3Type::RegionType region = mask->GetLargestPossibleRegion();
    typename SegmentationMask::RLE3Type::IndexType idx3;
    idx3[0] = x1;
    idx3[1] = y;
    idx3[2] = z;
    if (region.IsInside(idx3) == false)
    {
        return {};
    }
    idx3[0] = x2;
    if (region.IsInside(idx3) == false)
    {
        return {};
    }

    using BufferType = SegmentationMask::RLE3Type::BufferType;
    BufferType::IndexType bidx;
    bidx[0] = y;
    bidx[1] = z;

    auto buffer = mask->GetBuffer();
    if (buffer == nullptr)
    {
        return {};
    }

    auto& line = buffer->GetPixel(bidx);

    int X = static_cast<int>(region.GetIndex()[0]);
    for (auto& seg : line)
    {
        int segStart = X;
        int segEnd = X + seg.first - 1;

        if (segEnd < x1)
        {
            X += seg.first;
            continue;
        }
        if (segStart > x2)
        {
            break;
        }

        if (seg.second == targetValue)
        {
            int a = std::max(segStart, x1);
            int b = std::min(segEnd, x2);
            intervals.emplace_back(a, b);
        }

        X += seg.first;
    }
    return intervals;
}
