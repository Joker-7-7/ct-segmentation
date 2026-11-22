#ifndef SCENE_VTK_ITEM_HPP
#define SCENE_VTK_ITEM_HPP

#include "SceneVtkData.hpp"
#include "Backend/ThirdParty/QQuickVtkItem.h"

namespace Dicom::Viewer::Slicer
{
    class AxialSliceItem;
}

class SceneVtkItem : public QQuickVtkItem
{
    Q_OBJECT
    Q_PROPERTY(Dicom::Viewer::Slicer::AxialSliceItem* axialScene READ AxialSceneView WRITE
                   SetAxialSceneView)
public:
    vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override;

    Q_INVOKABLE void OnOpenFileClicked(QString singleFile);

    Q_INVOKABLE void OnOpenDirectoryClicked(QString directory);

    Q_INVOKABLE void OnResetCameraClicked();

    Q_INVOKABLE void OnTeethConfigClicked();

    Q_INVOKABLE void OnSolidConfigClicked();

    Q_INVOKABLE void OnSkinConfigClicked();

    Q_INVOKABLE void OnJitteringModeClicked();

    Q_INVOKABLE void OnSliderChanged(int value, QString sliderType);

    /**
     * Link to the main scene
     *
     * @param item main scene
     */
    Dicom::Viewer::Slicer::AxialSliceItem* AxialSceneView() const
    {
        return axialSliceItem;
    }
    void SetAxialSceneView(Dicom::Viewer::Slicer::AxialSliceItem* item) noexcept;

signals:
    void showMessageBox();

public:
    Dicom::Viewer::Slicer::AxialSliceItem* axialSliceItem = nullptr;
    SceneVtkData* sceneData = nullptr;

private:
    void mouseEnter() override;
};

#endif
