#include "BrushCursorController.hpp"
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPoints.h>
#include <vtkNew.h>
#include <vtkCellArray.h>
#include <algorithm>
#include <cmath>

using namespace Backend::Overlay;

BrushCursorController::BrushCursorController() :
    overlayRen_{ vtkSmartPointer<vtkRenderer>::New() },
    hoverPicker_{ vtkSmartPointer<vtkCellPicker>::New() },
    hoverPoly_{ vtkSmartPointer<vtkPolyData>::New() },
    hoverOutlineActor_{ vtkSmartPointer<vtkActor>::New() }
{
    hoverPicker_->SetTolerance(0.001);
    hoverPicker_->InitializePickList();
    hoverPicker_->PickFromListOn();

    overlayRen_->SetLayer(1);
    overlayRen_->SetInteractive(0);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(hoverPoly_);

    hoverOutlineActor_->SetMapper(mapper);
    hoverOutlineActor_->GetProperty()->SetRepresentationToWireframe();
    hoverOutlineActor_->GetProperty()->SetLineWidth(2.0);
    hoverOutlineActor_->GetProperty()->SetOpacity(1.0);
    hoverOutlineActor_->GetProperty()->SetColor(1.0, 0.4, 0.1);
    hoverOutlineActor_->GetProperty()->SetLighting(false);
    hoverOutlineActor_->SetVisibility(0);
}

inline static void AddLineSegment(
    vtkPoints* pts,
    vtkCellArray* lines,
    double x0,
    double y0,
    double z,
    double x1,
    double y1,
    double spacing
)
{
    x0 += spacing / 2;
    y0 += spacing / 2;

    x1 += spacing / 2;
    y1 += spacing / 2;

    const vtkIdType a = pts->InsertNextPoint(x0, y0, z);
    const vtkIdType b = pts->InsertNextPoint(x1, y1, z);
    vtkIdType ids[2] = { a, b };
    lines->InsertNextCell(2, ids);
}

void BrushCursorController::HideHoverOutline()
{
    if (hoverOutlineActor_)
    {
        hoverOutlineActor_->SetVisibility(0);
    }
}

void BrushCursorController::UpdateOutlineCircleForCellById(
    vtkIdType cellId, vtkImageData* image, const PaintbrushSettings& settings
)
{
    if (image == nullptr || cellId < 0)
    {
        return;
    }

    // Circle center: indices of the cell under the cursor.
    double b[6];
    image->GetCellBounds(cellId, b);
    const double cxw = 0.5 * (b[0] + b[1]);
    const double cyw = 0.5 * (b[2] + b[3]);
    double arr[3] = { cxw, cyw, 0.0 };

    int ijk[3];
    double pcoords[3];
    if (image->ComputeStructuredCoordinates(arr, ijk, pcoords) == 0)
    {
        return;
    }
    const int ci = ijk[0], cj = ijk[1];

    int ext[6];
    image->GetExtent(ext);
    const double* sp = image->GetSpacing();
    const double* og = image->GetOrigin();

    double brushRadius = settings.GetRadius();
    const int r = std::max(1, (int)std::lround(brushRadius));
    const int i0 = std::max(ext[0], ci - r - 1);
    const int i1 = std::min(ext[1], ci + r + 1);
    const int j0 = std::max(ext[2], cj - r - 1);
    const int j1 = std::min(ext[3], cj + r + 1);

    auto inside = [&](int i, int j) -> bool {
        const double dx = (i + 0.5) - (double)ci;
        const double dy = (j + 0.5) - (double)cj;
        return dx * dx + dy * dy <= (double)r * (double)r + 1e-12;
    };

    vtkNew<vtkPoints> pts;
    vtkNew<vtkCellArray> lines;

    // Vertical edges separate (i-1,j) and (i,j); the outline lies on x = i.
    for (int i = i0; i <= i1; ++i)
    {
        const double X = og[0] + i * sp[0]; // X boundary.
        for (int j = j0; j <= j1; ++j)
        {
            const bool L = (i - 1 >= ext[0]) ? inside(i - 1, j) : false;
            const bool R = (i <= ext[1]) ? inside(i, j) : false;
            if (L ^ R)
            {
                const double Y0 = og[1] + j * sp[1];
                const double Y1 = og[1] + (j + 1) * sp[1];
                AddLineSegment(pts, lines, X, Y0, 0.0, X, Y1, sp[1]);
            }
        }
    }

    // Horizontal edges separate (i,j-1) and (i,j); the outline lies on y = j.
    for (int j = j0; j <= j1; ++j)
    {
        const double Y = og[1] + j * sp[1]; // Y boundary.
        for (int i = i0; i <= i1; ++i)
        {
            const bool B = (j - 1 >= ext[2]) ? inside(i, j - 1) : false;
            const bool T = (j <= ext[3]) ? inside(i, j) : false;
            if (B ^ T)
            {
                const double X0 = og[0] + i * sp[0];
                const double X1 = og[0] + (i + 1) * sp[0];
                AddLineSegment(pts, lines, X0, Y, 0.0, X1, Y, sp[0]);
            }
        }
    }

    hoverPoly_->SetPoints(pts);
    hoverPoly_->SetLines(lines);
    hoverPoly_->Modified();
    hoverOutlineActor_->SetVisibility(1);
}

void BrushCursorController::UpdateOutlineSquareForCellById(
    vtkIdType cellId, vtkImageData* image, const PaintbrushSettings& settings
)
{
    if (image == nullptr || cellId < 0)
    {
        return;
    }
    // Find the pixel index under the cursor.
    double b[6];
    image->GetCellBounds(cellId, b);
    const double cxw = 0.5 * (b[0] + b[1]);
    const double cyw = 0.5 * (b[2] + b[3]);
    double arr[3] = { cxw, cyw, 0.0 };
    int ijk[3];
    double pcoords[3];
    if (image->ComputeStructuredCoordinates(arr, ijk, pcoords) == 0)
    {
        return;
    }
    const int ci = ijk[0], cj = ijk[1];

    const double* sp = image->GetSpacing();
    const double* og = image->GetOrigin();
    int r = static_cast<int>(std::lround(settings.GetRadius()));

    // Set bounds from indices, accounting for the pixel center at i + 0.5.
    double x0 = og[0] + (ci - r - 0.5) * sp[0];
    double x1 = og[0] + (ci + r + 0.5) * sp[0];
    double y0 = og[1] + (cj - r - 0.5) * sp[1];
    double y1 = og[1] + (cj + r + 0.5) * sp[1];
    double z = 0.0;

    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(x0, y0, z);
    pts->InsertNextPoint(x1, y0, z);
    pts->InsertNextPoint(x1, y1, z);
    pts->InsertNextPoint(x0, y1, z);
    pts->InsertNextPoint(x0, y0, z);

    auto lines = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType ids[5] = { 0, 1, 2, 3, 0 };
    lines->InsertNextCell(5, ids);

    hoverPoly_->SetPoints(pts);
    hoverPoly_->SetLines(lines);
    hoverPoly_->Modified();
    hoverOutlineActor_->SetVisibility(1);
}
