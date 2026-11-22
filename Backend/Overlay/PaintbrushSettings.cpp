// SimplePaintbrush.cpp
#include "PaintbrushSettings.hpp"

#include <vtkPoints.h>
#include <vtkPolygon.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkImageData.h>
#include <itkImageRegionIterator.h>
#include <math.h>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Backend::Overlay;

namespace
{
    constexpr double Step = 1.0;
}

void PaintbrushSettings::IncreaseRadius()
{
    radius_ = std::min(32.0, radius_ + Step);
}

void PaintbrushSettings::DecreaseRadius()
{
    radius_ = std::max(1.0, radius_ - Step);
}

void PaintbrushSettings::SetBrushMode(BrushMode mode)
{
    if (mode == BrushMode::Circle || mode == BrushMode::Sphere)
    {
        cursor_ = CursorShape::Round;
    }
    else
    {
        cursor_ = CursorShape::Square;
    }
    brush_ = mode;
}

double PaintbrushSettings::GetRadius() const
{
    return radius_;
}

PaintbrushSettings::CursorShape PaintbrushSettings::GetCursorShape() const
{
    return cursor_;
}

PaintbrushSettings::BrushMode PaintbrushSettings::GetBrushMode() const
{
    return brush_;
}

void PaintbrushSettings::SetRadius(double radius)
{
    if (std::isfinite(radius))
    {
        radius_ = std::clamp(radius, 1.0, 32.0);
    }
}
