#pragma once

#include <vtkInteractorStyleImage.h>
#include "PaintbrushSettings.hpp"
#include "BrushPaintContext.hpp"
#include "BrushCursorController.hpp"
#include "BrushWatershedPipeline.h"
#include "Backend/GraphicPrimitives/Vector3D.hpp"
#include "Backend/Segmentation/ManualSegmentationManager.hpp"
#include "Backend/RLEImage/RLEImage.h"

#include <vtkPolyData.h>
#include <vtkImageSlice.h>
#include <vtkImageData.h>
#include <vtkCellPicker.h>
#include <vtkPoints.h>

#include <itkWatershedImageFilter.h>
#include <itkVTKImageToImageFilter.h>

#include <optional>

namespace Backend::Overlay
{
    class PaintbrushInteractorStyle : public vtkInteractorStyleImage
    {
    public:
        using PixelType = float;
        using ImageType2D = itk::Image<PixelType, 2>;
        using ImageType3D = itk::Image<PixelType, 3>;
        using WatershedFilterType = itk::WatershedImageFilter<ImageType2D>;
        using ExtractFilterType = itk::ExtractImageFilter<ImageType3D, ImageType2D>;
        using VTK2ITKFilterType = itk::VTKImageToImageFilter<ImageType3D>;

        enum class PaintMode
        {
            None,
            Erase,
            Draw
        };

        enum class CheckedPixel
        {
            IsAnotherLayer,
            IsOnLayer,
            IsFound
        };

        static PaintbrushInteractorStyle* New();
        vtkTypeMacro(PaintbrushInteractorStyle, vtkInteractorStyleImage);

        PaintbrushInteractorStyle();

        void BuildHoverOutlineActor();

        void ChangeBrushShape();
        void Configure(int shape, double radius, bool erase);
        void RefreshLabels();
        boost::signals2::signal<void(int)> radiusStepRequested;

        void OnMouseMove() override;
        void OnMouseWheelForward() override;
        void OnMouseWheelBackward() override;

        void OnRightButtonDoubleClick() override;
        void OnLeftButtonDown() override;
        void OnLeftButtonUp() override;

        void SetData(vtkSmartPointer<vtkImageData> data);
        void SetSlice(int number);
        void SetSliceActor(vtkSmartPointer<vtkImageSlice> sliceActor);
        void SetReader(VTK2ITKFilterType::Pointer reader);
        void SetProjectionType(ManualSegmentationManager::ProjectionType type);
        void SetSegmentationManage(ManualSegmentationManager* manager);
        void SetManager(std::shared_ptr<ManualSegmentationManager> manager)
        {
            managerOwner_ = std::move(manager);
            context_.segmentationManager_ = managerOwner_.get();
        }

        void CreatMask(VTK2ITKFilterType::Pointer reader);

    private:
        bool WorldToIJK(const double w[3], int ijk[3]);

        void Paint();

        void ReloadPaintedPixels();

        vtkSmartPointer<vtkImageSlice> imageSliceActor_ = nullptr;

        PaintMode mode = PaintMode::None;
        BrushPaintContext context_;
        BrushCursorController cursor_;

        std::shared_ptr<ManualSegmentationManager> managerOwner_;
    };

}
