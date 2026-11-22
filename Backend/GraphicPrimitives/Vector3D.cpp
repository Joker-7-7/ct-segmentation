#include "Backend/GraphicPrimitives/Vector3D.hpp"

#include <vtkMath.h>

#include <cmath>

using namespace Dicom::Math;

Vector3D::Vector3D(double x, double y, double z) noexcept :
    x(x),
    y(y),
    z(z)
{
}

Vector3D::Vector3D(const double* value) noexcept :
    x(value[0]),
    y(value[1]),
    z(value[2])
{
}

double Vector3D::angleBetween(const Vector3D& lhs, const Vector3D& rhs) noexcept
{
    return vtkMath::DegreesFromRadians(vtkMath::AngleBetweenVectors(lhs.ptr(), rhs.ptr()));
}

Vector3D& Vector3D::operator=(const double* value) noexcept
{
    x = value[0];
    y = value[1];
    z = value[2];
    return *this;
}

double* Vector3D::ptr() noexcept
{
    return &x;
}

const double* Vector3D::ptr() const noexcept
{
    return &x;
}

void Vector3D::round() noexcept
{
    x = std::round(x);
    y = std::round(y);
    z = std::round(z);
}

Vector3D Vector3D::normalized() const noexcept
{
    const double norm = length();
    return (norm == 0.0 || norm == 1.0) ? *this : *this / norm;
}

void Vector3D::normalize() noexcept
{
    *this = normalized();
}

double Vector3D::length() const noexcept
{
    return std::sqrt(x * x + y * y + z * z);
}

namespace Dicom::Math
{
    bool operator==(const Vector3D& lhs, const Vector3D& rhs) noexcept
    {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    Vector3D operator+(const Vector3D& lhs, const Vector3D& rhs) noexcept
    {
        return Vector3D{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
    }

    Vector3D operator-(const Vector3D& lhs, const Vector3D& rhs) noexcept
    {
        return Vector3D{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
    }

    Vector3D operator*(const Vector3D& vec, double factor) noexcept
    {
        return { vec.x * factor, vec.y * factor, vec.z * factor };
    }

    Vector3D operator*(double factor, const Vector3D& vec) noexcept
    {
        return vec * factor;
    }

    Vector3D operator/(const Vector3D& vec, double factor) noexcept
    {
        return vec * (1.0 / factor);
    }

    Vector3D operator-(const Vector3D& vec) noexcept
    {
        return -1.0 * vec;
    }
}
