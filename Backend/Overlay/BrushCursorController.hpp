#pragma once
#include <vtkSmartPointer.h>
#include "PaintbrushSettings.hpp"
#include <vtkImageData.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkCellPicker.h>
#include <vtkPolyData.h>

namespace Backend::Overlay
{
    class BrushCursorController
    {
    public:
        BrushCursorController();
        void HideHoverOutline();
        void UpdateOutlineCircleForCellById(
            vtkIdType cellId, vtkImageData* image, const PaintbrushSettings& settings
        );
        void UpdateOutlineSquareForCellById(
            vtkIdType cellId, vtkImageData* image, const PaintbrushSettings& settings
        );

    private:
        friend class PaintbrushInteractorStyle;
        vtkSmartPointer<vtkRenderer> overlayRen_;
        vtkSmartPointer<vtkCellPicker> hoverPicker_;
        vtkSmartPointer<vtkPolyData> hoverPoly_;
        vtkSmartPointer<vtkActor> hoverOutlineActor_;
    };
}
