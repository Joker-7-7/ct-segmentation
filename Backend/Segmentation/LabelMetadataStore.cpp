#include "LabelMetadataStore.hpp"

LabelMetadataStore::PixelData LabelMetadataStore::ColorFor(unsigned short label) const
{
    for (const auto& item : labels)
    {
        if (item.label == label && item.isVisible)
        {
            return { static_cast<uint8_t>(item.color.red()),
                     static_cast<uint8_t>(item.color.green()),
                     static_cast<uint8_t>(item.color.blue()),
                     80 };
        }
    }
    return { 0, 0, 0, 0 };
}
