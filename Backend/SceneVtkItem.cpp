#include "SceneVtkItem.hpp"
#include "Backend/Layering/AxialSliceItem.hpp"
#include "Backend/Layering/AxialSliceData.hpp"

#include <vtkRenderWindow.h>

vtkStandardNewMacro(SceneVtkData);

using namespace Dicom::Viewer::Slicer;

QQuickVtkItem::vtkUserData SceneVtkItem::initializeVTK(vtkRenderWindow* renderWindow)
{
    auto vtkScene = vtkNew<SceneVtkData>();
    sceneData = vtkScene;
    sceneData->InitSceneVTKData(renderWindow);
    return vtkScene;
}

void SceneVtkItem::OnOpenDirectoryClicked(QString directory)
{
    std::function<void(vtkRenderWindow*, vtkUserData)> openDirectory(
        [this, directory](vtkRenderWindow*, const vtkUserData&) {
            if (sceneData->OpenDirectory(directory) == false)
            {
                emit showMessageBox();
            }
        }
    );
    QQuickVtkItem::dispatch_async(openDirectory);
}

void SceneVtkItem::OnOpenFileClicked(QString singleFile)
{
    std::function<void(vtkRenderWindow*, vtkUserData)> openFile(
        [this, singleFile](vtkRenderWindow*, const vtkUserData&) {
            if (sceneData->OpenSingleFile(singleFile) == false)
            {
                emit showMessageBox();
            }
        }
    );
    QQuickVtkItem::dispatch_async(openFile);
}

void SceneVtkItem::OnResetCameraClicked()
{
    std::function<void(vtkRenderWindow*, vtkUserData)> zoomExtent(
        [this](vtkRenderWindow*, const vtkUserData&) {

        }
    );
    QQuickVtkItem::dispatch_async(zoomExtent);
}

void SceneVtkItem::OnSliderChanged(int value, QString sliderType)
{
    std::function<void(vtkRenderWindow*, vtkUserData)> sliderChanged(
        [this, value, sliderType](vtkRenderWindow*, const vtkUserData&) {
            if (sliderType == "W")
            {
                sceneData->viewSettings.wLevel = value;
            }
            else if (sliderType == "L")
            {
                sceneData->viewSettings.lLevel = value;
            }
        }
    );
    QQuickVtkItem::dispatch_async(sliderChanged);
}

void SceneVtkItem::SetAxialSceneView(AxialSliceItem* item) noexcept
{
    if (item == nullptr || axialSliceItem == item)
    {
        return;
    }
    axialSliceItem = item;
}

void SceneVtkItem::mouseEnter()
{
}

void SceneVtkItem::OnTeethConfigClicked()
{
}

void SceneVtkItem::OnSolidConfigClicked()
{
}

void SceneVtkItem::OnSkinConfigClicked()
{
}

void SceneVtkItem::OnJitteringModeClicked()
{
}
