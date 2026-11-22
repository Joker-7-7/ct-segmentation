#pragma once
#include "LabelMetaData.hpp"
#include <vector>
#include <cstdint>

// Render-thread snapshot. UI publishes value copies through the adapter queue.
class LabelMetadataStore
{
public:
    struct PixelData
    {
        uint8_t r, g, b, a;
    };
    PixelData ColorFor(unsigned short label) const;
    std::vector<LabelMetaData> labels;
    unsigned short activeLabel = 0;
};
