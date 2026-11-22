#include "SceneVtkData.hpp"
#include "Backend/DicomRenderModule/SceneInteractionParameters/LayersConfiguration.hpp"
#include "Backend/ThirdParty/String.hpp"
#include "LogDuration.h"

#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkGlobFileNames.h>
#include <vtkDICOMDirectory.h>
#include <vtkDICOMImageReader.h>
#include <vtkPNGWriter.h>

#include <vtkPolyDataMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkMarchingCubes.h>
#include <vtkSphereSource.h>
#include <vtkDICOMUtilities.h>
#include <vtkStringArray.h>
#include <vtkImageCast.h>

#include <itkSignedMaurerDistanceMapImageFilter.h>
#include <QDir>

SceneVtkData::SceneVtkData() :
    _backgroundColor{ 0.3, 0.3, 0.3 },
    _renderer{ vtkSmartPointer<vtkRenderer>::New() }
{
}

void SceneVtkData::InitSceneVTKData(vtkRenderWindow* renderWindow)
{
#ifndef _DEBUG
    vtkObject::GlobalWarningDisplayOff();
#endif

    _renderWindow = renderWindow;
    SetupRender();

    CreateRepresentations();
}

void SceneVtkData::SetupRender()
{
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    style->SetDefaultRenderer(_renderer);

    _renderWindow->GetInteractor()->SetInteractorStyle(style);
    _renderWindow->SetSize(_renderWindow->GetScreenSize());
    _renderWindow->AddRenderer(_renderer);
    _renderWindow->SetWindowName("MainWindow");

    _renderer->SetBackground(_backgroundColor);
}

void SceneVtkData::SetupGPU() const noexcept
{
    // Setup Volume property
    vtkNew<vtkColorTransferFunction> m_ptrColorFunction;
    vtkNew<vtkPiecewiseFunction> m_ptrOpacityFunction;
    volumeProperty->SetColor(m_ptrColorFunction);
    volumeProperty->SetScalarOpacity(m_ptrOpacityFunction);
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->ShadeOn();
    volumeProperty->SetAmbient(0.1);
    volumeProperty->SetDiffuse(0.8);
    volumeProperty->SetSpecular(0.25);
    volumeProperty->SetSpecularPower(40);

    LayersConfiguration::SetColorAndOpacityFunction(
        volumeProperty, viewSettings.lLevel, viewSettings.wLevel
    );
    _volume->SetProperty(volumeProperty);
}

void SceneVtkData::AddDataSet(vtkSmartPointer<vtkImageReader2> reader)
{
    RemoveDataSet();

    SetupReader(reader);

    _renderer->ResetCamera();
}

void SceneVtkData::RemoveDataSet() const
{
    vtkProp* volume = _renderer->GetVolumes()->GetLastProp();
    if (volume != nullptr)
    {
        _renderer->RemoveVolume(volume);
    }
}

void SceneVtkData::SetupReader(vtkSmartPointer<vtkImageReader2> reader)
{
}

bool SceneVtkData::OpenDirectory(QString directory)
{
    qInfo() << "Opening DICOM directory by path:" << directory;

    const QString path = Viewer::Utils::String::WithoutUrlPrefix(directory);
    const QDir root(path);
    const QFileInfoList files = root.entryInfoList(QDir::Files);
    auto dcmFiles = vtkSmartPointer<vtkStringArray>::New();
    for (const QFileInfo& info : files)
    {
        const QString currentPath = info.absoluteFilePath();
        std::string currentPathStd = currentPath.toStdString();
        if (vtkDICOMUtilities::IsDICOMFile(currentPathStd.c_str()))
        {
            dcmFiles->InsertNextValue(std::move(currentPathStd));
        }
    }

    if (dcmFiles->GetNumberOfValues() == 0)
    {
        qWarning() << "No valid DICOM files found in directory";
        return false;
    }

    vtkNew<vtkDICOMReader> reader;
    reader->SetFileNames(dcmFiles);
    reader->SetMemoryRowOrderToFileNative();
    reader->Update();
    if (reader->GetErrorCode() != 0 || reader->GetOutput()->GetNumberOfPoints() == 0)
    {
        return false;
    }

    vtkNew<vtkImageCast> cast;
    cast->SetInputConnection(reader->GetOutputPort());
    cast->SetOutputScalarTypeToFloat();
    cast->Update();

    vtk2itk = VTK2ITKFilterType::New();

    vtk2itk->SetInput(cast->GetOutput());
    vtk2itk->Update();
    return true;

}

bool SceneVtkData::OpenSingleFile(QString singleFile)
{
    vtkSmartPointer<vtkImageReader2> dataSet = nullptr;
    // Open file
    QString fileName = Viewer::Utils::String::WithoutUrlPrefix(singleFile);

    vtkNew<vtkDICOMReader> fileReader;
    fileReader->SetFileName(fileName.toStdString().c_str());
    fileReader->SetMemoryRowOrderToFileNative();
    fileReader->Update();

    return CheckReader(std::move(fileReader), dataSet);
}

bool SceneVtkData::CheckReader(
    vtkSmartPointer<vtkDICOMReader> reader, vtkSmartPointer<vtkImageReader2> dataSet
)
{
    if (reader->GetErrorCode() == 0 && reader->GetOutput()->GetNumberOfPoints() > 0)
    {
        dataSet = reader;
    }

    // Add data set to 3D view
    if (dataSet)
    {
        vtkNew<vtkImageCast> cast;
        cast->SetInputConnection(dataSet->GetOutputPort());
        cast->SetOutputScalarTypeToFloat();
        cast->Update();
        vtk2itk = VTK2ITKFilterType::New();
        vtk2itk->SetInput(cast->GetOutput());
        vtk2itk->Update();
        return true;
    }

    return false;
}

void SceneVtkData::CreateRepresentations() noexcept
{
    _representation =
        std::make_shared<Representation>(_renderWindow->GetInteractor(), _renderer.Get());
}
