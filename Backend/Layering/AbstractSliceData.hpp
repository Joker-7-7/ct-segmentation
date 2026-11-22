#pragma once
#include "Backend/Overlay/PaintbrushInteractorStyle.hpp"
#include "Backend/Segmentation/ManualSegmentationManager.hpp"

#include <vtkObject.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkrenderWindowInteractor.h>
#include <vtkImageResliceMapper.h>
#include <vtkInteractorStyleImage.h>
#include <vtkImageActor.h>
#include <vtkRenderWindow.h>
#include <vtkImageMapToWindowLevelColors.h>

#include <itkImage.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageToVTKImageFilter.h>
#include <itkImageSeriesReader.h>
#include <itkExtractImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkVTKImageToImageFilter.h>

#include "boost/signals2/signal.hpp"

namespace Dicom::Viewer::Slicer
{
    class AbstractSliceData : public vtkObject
    {
    public:
        enum class ProjectionType
        {
            Sagittal,
            Coronal,
            Axial,
        };

        ProjectionType projectionType;

        using FloatPixelType = float;
        using ImageType3D = itk::Image<FloatPixelType, 3>;
        using FloatImageType2D = itk::Image<FloatPixelType, 2>;

        using ImageIoType = itk::GDCMImageIO;
        using NamesGeneratorType = itk::GDCMSeriesFileNames;
        using MetaDataStringType = itk::MetaDataObject<std::string>;
        //using ExtractFilterType = itk::ExtractImageFilter<ImageType3D, FloatImageType2D>;
        using ITKToVTKFilterType = itk::ImageToVTKImageFilter<FloatImageType2D>;
        using ThresholdFilterType =
            itk::BinaryThresholdImageFilter<FloatImageType2D, FloatImageType2D>;
        using ExtractFilterTypeTo2D = itk::ExtractImageFilter<ImageType3D, FloatImageType2D>;

        using VTK2ITKFilterType = itk::VTKImageToImageFilter<ImageType3D>;

        AbstractSliceData(ProjectionType projectionType);

        /**
         * GPU setup, data and scene initialization
         *
         * @param renderWindow graphical user interface where renderers draw their images
         */
        void Init(vtkRenderWindow* renderWindow);

        /**
         * Add a data set to the scene
         *
         * @param dataSet the data set to add
         */
        void AddDataSet(VTK2ITKFilterType::Pointer reader);

        /**
         * Remove the data set from the scene
         */
        void RemoveDataSet() const noexcept;

        /**
         * Setup renderer, renderWindow, interactor
         */
        void SetupRender() noexcept;

        void ChangeFilterImage();

        void SetLowerThreshold(int value);

        void SetUpperThreshold(int value);

        void EnableImageInteractionStyle();
        void EnablePaintBrushInteractorStyle();
        /**
         * Set DICOM slice number
         *
         * @param slice slice to set
         */
        void SetSliceNumber(int slice);

        virtual void SetupCamera() noexcept = 0;

        virtual void SliceFromDicom() = 0;

        vtkSmartPointer<Backend::Overlay::PaintbrushInteractorStyle> GetPaintStyle() const;

        boost::signals2::signal<void()> DataSetAdded;

        void InitSegmentationManager(ManualSegmentationManager& manager);

        VTK2ITKFilterType::Pointer vtk2itk;

    protected:
        bool filterMode;

        int sliceNumber = 0;

        ThresholdFilterType::Pointer threshold;

        vtkNew<vtkInteractorStyleImage> imageInteractionStyle;
        vtkNew<Backend::Overlay::PaintbrushInteractorStyle> paintStyle_;
        ExtractFilterTypeTo2D::Pointer extractFilter;

        ITKToVTKFilterType::Pointer itkToVtkOriginal;

        ITKToVTKFilterType::Pointer itkToVtkFilter;

        vtkSmartPointer<vtkImageMapToWindowLevelColors> imgToColors_;

        vtkSmartPointer<vtkImageActor> actor;

        vtkSmartPointer<vtkImageActor> filterActor;

        vtkSmartPointer<vtkRenderer> renderer_;

        vtkSmartPointer<vtkRenderWindow> renderWindow_;
    };
}
