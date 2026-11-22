#pragma once
#include <QObject>
#include <QPointer>
#include <QVariantList>
#include "Backend/Layering/AbstractSliceItem.hpp"
#include "Backend/SceneVtkItem.hpp"
#include "UI/AsyncQueue.hpp"
#include "LabelListModel.hpp"
#include "DentalInferenceJob.hpp"
#include <boost/signals2/connection.hpp>

namespace UI::Adapters
{
    class SegmentationAdapter : public QObject, public Viewer::UI::Adapters::AsyncQueue
    {
        Q_OBJECT
        Q_PROPERTY(Dicom::Viewer::Slicer::AbstractSliceItem* axialSlice READ axialSlice WRITE
                       setAxialSlice)
        Q_PROPERTY(Dicom::Viewer::Slicer::AbstractSliceItem* coronalSlice READ coronalSlice WRITE
                       setCoronalSlice)
        Q_PROPERTY(Dicom::Viewer::Slicer::AbstractSliceItem* sagittalSlice READ sagittalSlice WRITE
                       setSagittalSlice)
        Q_PROPERTY(SceneVtkItem* mainScene READ mainScene WRITE setMainScene)
        Q_PROPERTY(LabelListModel* labels READ labels CONSTANT)
        Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
        Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
        Q_PROPERTY(bool dentalRunning READ dentalRunning NOTIFY dentalChanged)
        Q_PROPERTY(QString dentalStatus READ dentalStatus NOTIFY dentalChanged)
        Q_PROPERTY(int tool READ tool WRITE setTool NOTIFY brushChanged)
        Q_PROPERTY(int brushShape READ brushShape WRITE setBrushShape NOTIFY brushChanged)
        Q_PROPERTY(int brushRadius READ brushRadius WRITE setBrushRadius NOTIFY brushChanged)

    signals:
        void dentalChanged();
        void readyChanged();
        void busyChanged();
        void brushChanged();
        void volumeOpened(QVariantList dimensions);
        void errorOccurred(QString message);


    public:
        SegmentationAdapter();
        ~SegmentationAdapter() override;
        bool dentalRunning() const;
        QString dentalStatus() const;
        Q_INVOKABLE void RunDentalSegmentation(QString device);
        Q_INVOKABLE void CancelDentalSegmentation();
        void setAxialSlice(Dicom::Viewer::Slicer::AbstractSliceItem* item);
        void setCoronalSlice(Dicom::Viewer::Slicer::AbstractSliceItem* item);
        void setSagittalSlice(Dicom::Viewer::Slicer::AbstractSliceItem* item);
        void setMainScene(SceneVtkItem* item);
        Dicom::Viewer::Slicer::AbstractSliceItem* axialSlice() const;
        Dicom::Viewer::Slicer::AbstractSliceItem* coronalSlice() const;
        Dicom::Viewer::Slicer::AbstractSliceItem* sagittalSlice() const;
        SceneVtkItem* mainScene() const;
        LabelListModel* labels();
        bool ready() const;
        bool busy() const;
        int tool() const;
        int brushShape() const;
        int brushRadius() const;
        void setTool(int value);
        void setBrushShape(int value);
        void setBrushRadius(int value);
        Q_INVOKABLE void OpenDicom(QString url, bool directory);

    private:
        void FinishDental(QString error = {});
        void Queue(Task task);
        void ApplyBrush();
        void SyncLabels();
        void RefreshSlices(); // Render thread only.
        QPointer<Dicom::Viewer::Slicer::AbstractSliceItem> axialSlice_, coronalSlice_,
            sagittalSlice_;
        QPointer<SceneVtkItem> mainScene_;
        LabelListModel labels_;
        // Styles retain the manager until their VTK scene is destroyed.
        std::shared_ptr<ManualSegmentationManager> manager_;
        std::vector<boost::signals2::scoped_connection> connections_;
        std::unique_ptr<DentalInferenceJob> dentalJob_;
        SegmentationMask::VTK2ITKFilterType::Pointer dentalSource_;
        bool dentalRunning_ = false;
        bool dentalCancelRequested_ = false;
        QString dentalStatus_;
        bool ready_ = false;
        bool busy_ = false;
        int tool_ = 0; // Navigate, paint, erase.
        int brushShape_ = 0;
        int brushRadius_ = 16;
    };
}
