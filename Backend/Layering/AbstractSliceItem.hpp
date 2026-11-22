#pragma once

#include "Backend/ThirdParty/QQuickVtkItem.h"
#include "Backend/SceneVtkItem.hpp"
#include "AbstractSliceData.hpp"

namespace Dicom::Viewer::Slicer
{
    class AbstractSliceItem : public QQuickVtkItem
    {

        Q_OBJECT
        Q_PROPERTY(SceneVtkItem* mainScene READ SceneView WRITE SetSceneView)
    public:
        Q_INVOKABLE void SetUpperThreshold(int value);

        Q_INVOKABLE void SetLowerThreshold(int value);

        /**
         * Ctor
         *
         * @param parent parent object
         */
        explicit AbstractSliceItem(QQuickVtkItem* parent = nullptr);

        /**
         * Link to the main scene
         *
         * @param item main scene
         */
        SceneVtkItem* SceneView() const
        {
            return sceneItem_;
        }
        void SetSceneView(SceneVtkItem* item) noexcept;

        /**
         * Update slice when moving the slider
         *
         * @param value      value to update
         */
        Q_INVOKABLE void OnSliderChanged(int value);

        Q_INVOKABLE void ChangeFilterView();

        /**
         * Add a data to the scene
         *
         * @param data the data set to add
         */
        void AddData(SceneVtkData::VTK2ITKFilterType::Pointer data) noexcept;

        /**
         * Open DICOM to coronal slice view
         */
        Q_INVOKABLE void OnOpenClicked();

        /**
         * Return max slice number from DICOM file
         *
         * @return max number
         */
        Q_INVOKABLE int GetMaxSliceNumber() const noexcept;

        Q_INVOKABLE void ChangeBrushShape();

        Q_INVOKABLE void EnableImageInteraction();

        Q_INVOKABLE void EnablePaintBrushInteraction();

        AbstractSliceData* GetData() const noexcept;
    signals:
        /**
         * Signal of successful opening of the DICOM file to update the slider
         */
        void dicomOpened();

    protected:
        void mouseEnter() override {};

        /**
         * VTK data for render
         */
        AbstractSliceData* sliceData_ = nullptr;

        /**
         * Qml main scene item for render
         */
        SceneVtkItem* sceneItem_ = nullptr;
    };
}
