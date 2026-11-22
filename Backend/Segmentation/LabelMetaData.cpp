#include "LabelMetaData.hpp"
#include <algorithm>
const std::vector<QColor>& LabelMetaData::Palette()
{
    static const std::vector<QColor> colors = {
        QColor(),         QColor(0x323233), QColor(0x535252), QColor(0x878787), QColor(0xA9A9A8),
        QColor(0xC0C1C1), QColor(0xD1D1D0), QColor(0xEAEAEB), QColor(0xF0F0F0), QColor(0xFFFFFF),
        QColor(0x850000), QColor(0xFD0007), QColor(0xFD8707), QColor(0xFFFE08), QColor(0x22FE07),
        QColor(0x20FEFF), QColor(0x3B6FE3), QColor(0x0101FF), QColor(0x8501FF), QColor(0xFB01FE),
        QColor(0xDEA99E), QColor(0xF1BFC1), QColor(0xFBDFC2), QColor(0xFEF0C0), QColor(0xD0E4C9),
        QColor(0xC6D9DD), QColor(0xBFD0F7), QColor(0xC4DAF1), QColor(0xD0C7E3), QColor(0xE5C4D3),
        QColor(0xD26958), QColor(0xE28486), QColor(0xF6C08B), QColor(0xFEE188), QColor(0xA8D199),
        QColor(0x93B8BD), QColor(0x95B2F2), QColor(0x8FB8E2), QColor(0xA595CD), QColor(0xCA93B0),
        QColor(0xBE2C1D), QColor(0xD74F54), QColor(0xF3A258), QColor(0xFFD254), QColor(0x5B9B3E),
        QColor(0x64959F), QColor(0x5D8AE4), QColor(0x5F96D2), QColor(0x7B65B6), QColor(0xB5658E),
        QColor(0x930F02), QColor(0xBE0105), QColor(0xDF7E2B), QColor(0xECB627), QColor(0x5B9B3E),
        QColor(0x396F7A), QColor(0x2E60CE), QColor(0x3170BA), QColor(0x533996), QColor(0x943865),
        QColor(0x70140C), QColor(0x840002), QColor(0xA24A0A), QColor(0xB07E07), QColor(0x2C6417),
        QColor(0x133F48), QColor(0x103DC0), QColor(0x0E4180), QColor(0x290F61), QColor(0x610F37),
        QColor(0x460A00), QColor(0x520101), QColor(0x633106), QColor(0x6B4F02), QColor(0x1E3E10),
        QColor(0x0C272F), QColor(0x173273), QColor(0x0B2950), QColor(0x180C3C), QColor(0x3B0C24)
    };
    return colors;
}

LabelMetaData::LabelType LabelMetaData::EncodeColor(const QColor& color)
{
    if (color.isValid() == false)
    {
        return 0;
    }
    const auto& colors = Palette();
    auto it = std::find_if(colors.begin(), colors.end(), [&](const QColor& c) {
        return c.isValid() && c.rgb() == color.rgb();
    });
    return it == colors.end() ? 0 : static_cast<LabelType>(it - colors.begin() + 1);
}
