#pragma once
#include <array>
#include <cstddef>
#include <vector>

namespace Dental
{
    using Shape = std::array<int, 3>;
    struct Volume
    {
        Shape shape{};
        std::vector<float> data;
    };
    std::size_t Count(const Shape& shape);
    Volume Resize(const Volume& input, const Shape& shape, int order, int nearestAxis = -1);
    std::vector<int> Steps(int size, int patch);
} 
