#include "UI/Adapters/LabelListModel.hpp"
#include "Backend/Segmentation/ManualSegmentationManager.hpp"
#include "Backend/Overlay/PaintbrushInteractorStyle.hpp"
#include <QCoreApplication>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <iostream>
#include <stdexcept>

void check(bool value, const char* message)
{
    if (value == false)
    {
        throw std::runtime_error(message);
    }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try
    {
        LabelListModel model;
        check(model.add(QColor("#123456")) == 0, "non-palette color accepted");
        check(model.add(QColor()) == 0, "invalid color accepted");
        const auto red = model.add(QColor(0xFD0007));
        const auto green = model.add(QColor(0x22FE07));
        check(red == 12 && green == 15, "source palette identities changed");
        model.add(QColor(0xFD0007));
        check(model.rowCount() == 2 && model.activeLabel() == red, "duplicate color identity");
        model.rename(green, "  Green  ");
        check(
            model.data(model.index(1), LabelListModel::CaptionRole).toString() == "Green", "rename"
        );
        model.rename(green, "   ");
        check(
            model.data(model.index(1), LabelListModel::CaptionRole).toString() == "Green",
            "empty rename"
        );
        model.select(65000);
        check(model.activeLabel() == red, "invalid selection");

        ManualSegmentationManager manager;
        auto volume = vtkSmartPointer<vtkImageData>::New();
        volume->SetDimensions(8, 6, 4);
        volume->AllocateScalars(VTK_FLOAT, 1);
        std::fill_n(static_cast<float*>(volume->GetScalarPointer()), 8 * 6 * 4, 0.0f);
        auto reader = SegmentationMask::VTK2ITKFilterType::New();
        reader->SetInput(volume);
        reader->Update();
        manager.CreateEmptyMask(reader);
        manager.metadata.labels = model.items();
        manager.PaintSegment(-1, 0, 0, 2, red);
        manager.PaintSegment(0, 0, -5, 20, red);
        check(manager.IsSegmentSet(0, 7, 0, 0, red), "edge span clipping");
        manager.RemoveLabel(red);
        manager.metadata.activeLabel = red;
        check(manager.CanPaint(), "visible label not paintable");
        // All projections must rebuild from the original grayscale, without accumulating tint.
        manager.PaintSegment(3, 1, 2, 2, red);
        for (int normal = 0; normal < 3; ++normal)
        {
            const int dimensions[3] = { 8, 6, 4 };
            const int ijk[3] = { 2, 3, 1 };
            const int horizontal = normal == 0 ? 1 : 0;
            const int vertical = normal == 2 ? 1 : 2;
            auto rgba = vtkSmartPointer<vtkImageData>::New();
            rgba->SetDimensions(dimensions[horizontal], dimensions[vertical], 1);
            rgba->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
            auto* begin = static_cast<unsigned char*>(rgba->GetScalarPointer());
            std::fill_n(begin, dimensions[horizontal] * dimensions[vertical] * 4, 100);
            auto style = vtkSmartPointer<Backend::Overlay::PaintbrushInteractorStyle>::New();
            style->SetManager(std::shared_ptr<ManualSegmentationManager>(&manager, [](auto*) {
            }));
            style->SetData(rgba);
            style->SetProjectionType(static_cast<ManualSegmentationManager::ProjectionType>(normal)
            );
            style->SetSlice(ijk[normal]);
            auto* pixel = static_cast<unsigned char*>(
                rgba->GetScalarPointer(ijk[horizontal], ijk[vertical], 0)
            );
            const auto expected = (253 * 80 + 100 * 175) / 255;
            check(pixel[0] == expected, "wrong projection overlay");
            style->RefreshLabels();
            pixel = static_cast<unsigned char*>(
                rgba->GetScalarPointer(ijk[horizontal], ijk[vertical], 0)
            );
            check(pixel[0] == expected, "overlay tint accumulated");
            manager.metadata.labels[0].isVisible = false;
            style->RefreshLabels();
            pixel = static_cast<unsigned char*>(
                rgba->GetScalarPointer(ijk[horizontal], ijk[vertical], 0)
            );
            check(pixel[0] == 100, "hidden overlay was not cleared");
            manager.metadata.labels[0].isVisible = true;
            style->SetSlice(dimensions[normal]); // One past end must not read an RLE line.
        }
        manager.RemoveLabel(red);
        manager.PaintSegment(2, 1, 1, 5, red);
        check(manager.PaintSegment(2, 1, 3, 4, green), "new label span was not written");
        check(
            manager.PaintSegment(2, 1, 3, 4, green) == false,
            "unchanged span was reported as modified"
        );
        check(
            manager.storage.dirtyLabels.count(red) && manager.storage.dirtyLabels.count(green),
            "overwritten label was not marked dirty"
        );
        check(
            manager.IsPixelSet(1, 2, 1, red) && manager.IsPixelSet(3, 2, 1, green),
            "overlapping label ownership"
        );
        auto intervals = manager.FindIntervalsWithValue(0, 7, 2, 1, red);
        for (auto span : intervals)
        {
            manager.PaintSegment(2, 1, span.first, span.second, 0);
        }
        check(
            manager.IsPixelSet(3, 2, 1, green) && manager.IsPixelSet(1, 2, 1, 0),
            "erase touched another label"
        );
        manager.PaintSegment(2, 1, 0, 1, red);
        manager.RemoveLabel(red);
        check(
            manager.IsPixelSet(0, 2, 1, 0) && manager.IsPixelSet(3, 2, 1, green),
            "delete touched another label"
        );
        const auto& line = manager.storage.mask->GetBuffer()->GetPixel({ { 2, 1 } });
        int count = 0;
        unsigned short previous = 65535;
        for (auto run : line)
        {
            check(run.first && run.second != previous, "invalid RLE after delete");
            count += run.first;
            previous = run.second;
        }
        check(count == 8, "RLE line length changed");
        model.setVisible(red, false);
        manager.metadata.labels = model.items();
        check(
            manager.CanPaint() == false && manager.ColorFor(red).a == 0, "hidden label paintable"
        );
        check(manager.ColorFor(green).g == 254, "palette color mismatch");
        manager.scene.sceneRenderer = vtkSmartPointer<vtkRenderer>::New();
        manager.RenderMask();
        check(
            manager.scene.labelActors.count(green) == 1 &&
                manager.scene.labelActors.count(red) == 0,
            "per-label surfaces"
        );
        const auto actor = manager.scene.labelActors.at(green);
        const auto mesh = vtkPolyDataMapper::SafeDownCast(actor->GetMapper())->GetInput();
        check(mesh && mesh->GetPointData()->GetNormals(), "label mesh has no smooth normals");
        check(
            actor->GetProperty()->GetInterpolation() == VTK_PHONG, "label actor is not Phong shaded"
        );
        check(manager.storage.dirtyLabels.empty(), "rendered labels remained dirty");
        manager.PaintSegment(4, 2, 5, 5, red);
        manager.RenderMask();
        check(
            manager.scene.labelActors.at(green) == actor && manager.scene.labelActors.count(red),
            "unchanged label surface was rebuilt"
        );
        manager.PaintSegment(4, 2, 5, 5, 0);
        manager.RenderMask();
        check(
            manager.scene.labelActors.at(green) == actor &&
                manager.scene.labelActors.count(red) == 0,
            "erased label surface was retained"
        );
        model.setVisible(green, false);
        manager.metadata.labels = model.items();
        manager.UpdateLabelAppearance();
        check(manager.scene.labelActors.at(green)->GetVisibility() == 0, "3D visibility");
        manager.CreateEmptyMask(reader);
        check(
            manager.IsPixelSet(3, 2, 1, 0) && manager.scene.labelActors.empty(),
            "reload retained mask/surfaces"
        );
        model.remove(red);
        check(model.activeLabel() == green && model.rowCount() == 1, "selection after delete");
        model.remove(green);
        check(model.activeLabel() == 0 && model.rowCount() == 0, "last label removal");
        std::cout
            << "Label model, palette, mask, erase, deletion, visibility and reload tests passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
