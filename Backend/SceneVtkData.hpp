#ifndef SCENE_VTK_DATA_HPP
#define SCENE_VTK_DATA_HPP

#include "Backend//DicomRenderModule/SceneInteractionParameters/Representation.hpp"

#include <QString.h>

#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkVolumeProperty.h>
#include <vtkVolume.h>
#include <memory>
#include <vtkImageReader2.h>
#include <vtkDICOMReader.h>
#include <vtkrenderWindowInteractor.h>
#include <vtkRenderWindow.h>

#include <itkGDCMImageIO.h>
#include <itkImage.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageSeriesReader.h>
#include <itkExtractImageFilter.h>
#include <itkVTKImageToImageFilter.h>

class SceneVtkData final : public vtkObject
{
public:
    using PixelType = float;
    using ImageType3D = itk::Image<PixelType, 3>;
    using ImageType2D = itk::Image<PixelType, 2>;
    using ImageIoType = itk::GDCMImageIO;
    using NamesGeneratorType = itk::GDCMSeriesFileNames;
    using MetaDataStringType = itk::MetaDataObject<std::string>;

    using VTK2ITKFilterType = itk::VTKImageToImageFilter<ImageType3D>;
    struct ViewSettings final
    {
        int lLevel = 444;
        int wLevel = 2449;
    };

    vtkTypeMacro(SceneVtkData, vtkObject);

    static SceneVtkData* New();

    SceneVtkData();

    void InitSceneVTKData(vtkRenderWindow* renderWindow);

    void AddDataSet(vtkSmartPointer<vtkImageReader2> dataSet);

    void SetupRender();

    void SetupGPU() const noexcept;

    void RemoveDataSet() const;

    void SetupReader(vtkSmartPointer<vtkImageReader2> reader);

    void CreateRepresentations() noexcept;

    [[nodiscard]] bool OpenSingleFile(QString singleFile);

    [[nodiscard]] bool OpenDirectory(QString directory);

    /**
     * Сheck the validity of the reader and add data to the scene if successful
     * @param reader Reader with selected file
     * @param dataSet Dataset for render
     * @return Error code: true - success, false - error
     */
    [[nodiscard]] bool CheckReader(
        vtkSmartPointer<vtkDICOMReader> reader, vtkSmartPointer<vtkImageReader2> dataSet
    );

    // Reader for second buffer
    // vtkSmartPointer<vtkImageReader2> _preReader;
    // Reader for first buffer
    //vtkSmartPointer<vtkImageReader2> _reader;
    // Is an object that controls the rendering process for objects
    vtkSmartPointer<vtkRenderer> _renderer;

    // A window in a graphical user interface where renderers draw their images
    vtkSmartPointer<vtkRenderWindow> _renderWindow;

    // Sliders value
    ViewSettings viewSettings;
    // Represents the common properties for rendering a volume
    vtkNew<vtkVolumeProperty> volumeProperty;

    vtkSmartPointer<vtkImageReader2> vtkReader;
    VTK2ITKFilterType::Pointer vtk2itk;

private:
    // Window background color
    const double _backgroundColor[3];
    // Represents a volume (data & properties) in a rendered scene
    vtkNew<vtkVolume> _volume;

    // Scene view elements
    std::shared_ptr<Representation> _representation;
};

#endif
