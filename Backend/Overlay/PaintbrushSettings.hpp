#pragma once
#include "Backend/GraphicPrimitives/Vector3D.hpp"

#include <vtkSmartPointer.h>
#include <vtkPoints2D.h>
#include <itkImage.h>

namespace Backend::Overlay
{
    class PaintbrushSettings
    {
    public:
        enum class BrushMode : uint8_t
        {
            Circle,
            Square,
            Adaptive2D,
            Sphere,
            Cube,
            Adaptive3D,
            COUNT
        };

        enum class CursorShape
        {
            Round,
            Square
        };

        void SetRadius(double radius);

        void IncreaseRadius();

        void DecreaseRadius();

        void SetBrushMode(BrushMode mode);

        CursorShape GetCursorShape() const;

        BrushMode GetBrushMode() const;

        double GetRadius() const;

    private:
        CursorShape cursor_ = CursorShape::Square;

        BrushMode brush_ = BrushMode::Adaptive3D;

        double radius_ = 16.0;
    };
}