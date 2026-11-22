#include "Volume.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Dental
{
    std::size_t Count(const Shape& shape)
    {
        std::size_t result = 1;
        for (int n : shape)
        {
            if (n <= 0 ||
                result > std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(n))
            {
                throw std::runtime_error("Invalid volume dimensions");
            }
            result *= n;
        }
        return result;
    }
    namespace
    {
        // Cubic B-spline prefilter with mirror endpoints. Twelve replicated samples
        // implement scipy.ndimage's nearest-mode prepadding before spline_filter.
        std::vector<double> Coefficients(const std::vector<double>& line)
        {
            const int n = static_cast<int>(line.size()) + 24;
            std::vector<double> coefficients(n), upper(n);
            for (int i = 0; i < n; ++i)
            {
                const double sample =
                    line[std::clamp(i - 12, 0, static_cast<int>(line.size()) - 1)];
                const double lower = i == n - 1 ? 2.0 : 1.0;
                const double diagonal = i == 0 ? 4.0 : 4.0 - lower * upper[i - 1];
                upper[i] = (i == 0 ? 2.0 : 1.0) / diagonal;
                coefficients[i] =
                    (6.0 * sample - (i ? lower * coefficients[i - 1] : 0.0)) / diagonal;
            }
            for (int i = n - 2; i >= 0; --i)
            {
                coefficients[i] -= upper[i] * coefficients[i + 1];
            }
            return coefficients;
        }
        double Sample(
            const std::vector<double>& line,
            const std::vector<double>& coefficients,
            double x,
            int order
        )
        {
            if (order == 0)
            {
                return line[std::clamp(
                    static_cast<int>(std::floor(x + .5)), 0, static_cast<int>(line.size()) - 1
                )];
            }
            if (order == 1)
            {
                x = std::clamp(x, 0.0, static_cast<double>(line.size() - 1));
                const int i = static_cast<int>(std::floor(x));
                const double t = x - i;
                return line[i] * (1 - t) +
                       line[std::min(i + 1, static_cast<int>(line.size()) - 1)] * t;
            }
            x += 12;
            const int i = static_cast<int>(std::floor(x));
            const double t = x - i, u = 1 - t;
            return (coefficients[i - 1] * u * u * u +
                    coefficients[i] * (3 * t * t * t - 6 * t * t + 4) +
                    coefficients[i + 1] * (-3 * t * t * t + 3 * t * t + 3 * t + 1) +
                    coefficients[i + 2] * t * t * t) /
                   6;
        }
    } // namespace
    Volume Resize(const Volume& input, const Shape& shape, int order, int nearestAxis)
    {
        if (input.data.size() != Count(input.shape) || (order != 1 && order != 3) ||
            nearestAxis < -1 || nearestAxis > 2)
        {
            throw std::runtime_error("Invalid resampling input");
        }
        Count(shape);
        auto current = input;
        const auto limits = std::minmax_element(input.data.begin(), input.data.end());
        const float low = *limits.first, high = *limits.second;
        // In anisotropic cases interpolate in-plane before the nearest slice gather.
        std::array<int, 3> axes{ 0, 1, 2 };
        if (nearestAxis >= 0)
        {
            std::swap(axes[nearestAxis], axes[2]);
        }
        for (int axis : axes)
        {
            if (axis == nearestAxis)
            {
                const int slices = input.shape[axis];
                std::vector<float> sliceLow(slices, std::numeric_limits<float>::infinity());
                std::vector<float> sliceHigh(slices, -std::numeric_limits<float>::infinity());
                std::size_t originalStride = 1, currentStride = 1;
                for (int a = axis + 1; a < 3; ++a)
                {
                    originalStride *= input.shape[a];
                    currentStride *= current.shape[a];
                }
                for (std::size_t i = 0; i < input.data.size(); ++i)
                {
                    const auto slice = (i / originalStride) % slices;
                    sliceLow[slice] = std::min(sliceLow[slice], input.data[i]);
                    sliceHigh[slice] = std::max(sliceHigh[slice], input.data[i]);
                }
                for (std::size_t i = 0; i < current.data.size(); ++i)
                {
                    const auto slice = (i / currentStride) % slices;
                    current.data[i] =
                        std::clamp(current.data[i], sliceLow[slice], sliceHigh[slice]);
                }
            }
            if (current.shape[axis] == shape[axis])
            {
                continue;
            }
            auto nextShape = current.shape;
            nextShape[axis] = shape[axis];
            Volume next{ nextShape, std::vector<float>(Count(nextShape)) };
            std::size_t stride = 1;
            for (int a = axis + 1; a < 3; ++a)
            {
                stride *= current.shape[a];
            }
            const int n = current.shape[axis], m = nextShape[axis];
            const auto outer = current.data.size() / (stride * n);
            const int axisOrder = axis == nearestAxis ? 0 : order;
            std::vector<double> line(n);
            for (std::size_t block = 0; block < outer; ++block)
            {
                for (std::size_t inner = 0; inner < stride; ++inner)
                {
                    for (int i = 0; i < n; ++i)
                    {
                        line[i] = current.data[(block * n + i) * stride + inner];
                    }
                    const auto coefficients =
                        axisOrder == 3 ? Coefficients(line) : std::vector<double>{};
                    for (int i = 0; i < m; ++i)
                    {
                        const double x = (i + .5) * n / m - .5;
                        next.data[(block * m + i) * stride + inner] =
                            static_cast<float>(Sample(line, coefficients, x, axisOrder));
                    }
                }
            }
            current = std::move(next);
        }
        for (auto& value : current.data)
        {
            value = std::clamp(value, low, high);
        }
        return current;
    }

    std::vector<int> Steps(int size, int patch)
    {
        if (patch <= 0 || size < patch)
        {
            throw std::runtime_error("Invalid sliding window dimensions");
        }
        const int count = static_cast<int>(std::ceil((size - patch) / (patch * .5))) + 1;
        std::vector<int> result;
        for (int i = 0; i < count; ++i)
        {
            result.push_back(
                count == 1 ? 0
                           : static_cast<int>(
                                 std::nearbyint(i * static_cast<double>(size - patch) / (count - 1))
                             )
            );
        }
        return result;
    }
} // namespace Dental
