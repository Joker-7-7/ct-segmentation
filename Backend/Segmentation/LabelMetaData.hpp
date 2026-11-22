#pragma once
#include <QColor>
#include <QString>
#include <vector>
#include <cstdint>

// Palette and color identities ported from dentomatic-desktop LabelMetaData.
// Zero is background; duplicate palette colors share the first color identity.
struct LabelMetaData
{
    using LabelType = std::uint16_t;
    LabelType label = 0;
    QString name;
    QColor color;
    bool isVisible = true;
    static const std::vector<QColor>& Palette();
    static LabelType EncodeColor(const QColor& color);
};
