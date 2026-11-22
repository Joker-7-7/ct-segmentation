#include "SegmentationAdapter.hpp"
#include <QMetaObject>
#include <algorithm>
#include <exception>
#include <QUrl>
#include <itkImageDuplicator.h>

using namespace UI::Adapters;
SegmentationAdapter::SegmentationAdapter() :
    manager_(std::make_shared<ManualSegmentationManager>())
{
    dentalJob_ = std::make_unique<DentalInferenceJob>();
    dentalJob_->progress = [this](QString message) {
        dentalStatus_ = std::move(message);
        emit dentalChanged();
    };
    dentalJob_->completed =
        [this](DentalInferenceJob::Mask::Pointer mask, QString error, bool cancelled) {
            if (cancelled || dentalCancelRequested_)
            {
                dentalStatus_ = "Cancelled";
                FinishDental();
                return;
            }
            if (error.isEmpty() == false || mask == nullptr)
            {
                FinishDental(error.isEmpty() ? "No segmentation was produced." : error);
                return;
            }
            if (mainScene_ == nullptr)
            {
                FinishDental("The CT view has been closed.");
                return;
            }
            auto metadata = DentalInferenceJob::labels();
            dentalRunning_ =
                false; // Inference is finished; the following commit is not cancellable.
            dentalStatus_ = "Applying segmentation...";
            emit dentalChanged();
            Queue([this, mask, metadata, source = dentalSource_]() mutable {
                if (mainScene_ == nullptr || mainScene_->sceneData == nullptr ||
                    mainScene_->sceneData->vtk2itk != source)
                {
                    QMetaObject::invokeMethod(
                        this,
                        [this] {
                            FinishDental("The CT changed before the result could be applied.");
                        },
                        Qt::QueuedConnection
                    );
                    return;
                }
                auto guiMetadata = metadata;
                // The previous labels survive every failure before this commit.
                manager_->ClearSurfaces();
                manager_->storage.mask = mask;
                manager_->metadata.labels = std::move(metadata);
                manager_->metadata.activeLabel = manager_->metadata.labels.front().label;
                QString surfaceError;
                try
                {
                    manager_->RenderMask();
                }
                catch (const std::exception& e)
                {
                    surfaceError = QString::fromUtf8(e.what());
                }
                RefreshSlices();
                QMetaObject::invokeMethod(
                    this,
                    [this, metadata = std::move(guiMetadata), surfaceError]() mutable {
                        labels_.replace(std::move(metadata));
                        dentalStatus_ = surfaceError.isEmpty()
                                            ? "Segmentation complete."
                                            : "Segmentation applied; 3D surface failed.";
                        FinishDental();
                        if (surfaceError.isEmpty() == false)
                        {
                            emit errorOccurred(surfaceError);
                        }
                    },
                    Qt::QueuedConnection
                );
            });
        };
    connect(&labels_, &LabelListModel::labelsChanged, this, &SegmentationAdapter::SyncLabels);
    connect(&labels_, &LabelListModel::activeLabelChanged, this, [this] {
        const auto label = labels_.activeLabel();
        Queue([this, label] {
            manager_->metadata.activeLabel = static_cast<unsigned short>(label);
        });
        if (label == 0)
        {
            setTool(0);
        }
    });
    connect(&labels_, &LabelListModel::labelRemoved, this, [this](int label) {
        Queue([this, label] {
            manager_->RemoveLabel(static_cast<unsigned short>(label));
            RefreshSlices();
        });
    });
}

bool SegmentationAdapter::dentalRunning() const
{
    return dentalRunning_;
}

QString SegmentationAdapter::dentalStatus() const
{
    return dentalStatus_;
}

void SegmentationAdapter::setAxialSlice(Dicom::Viewer::Slicer::AbstractSliceItem* item)
{
    axialSlice_ = item;
}

void SegmentationAdapter::setCoronalSlice(Dicom::Viewer::Slicer::AbstractSliceItem* item)
{
    coronalSlice_ = item;
}

void SegmentationAdapter::setSagittalSlice(Dicom::Viewer::Slicer::AbstractSliceItem* item)
{
    sagittalSlice_ = item;
}

Dicom::Viewer::Slicer::AbstractSliceItem* SegmentationAdapter::axialSlice() const
{
    return axialSlice_;
}

Dicom::Viewer::Slicer::AbstractSliceItem* SegmentationAdapter::coronalSlice() const
{
    return coronalSlice_;
}

Dicom::Viewer::Slicer::AbstractSliceItem* SegmentationAdapter::sagittalSlice() const
{
    return sagittalSlice_;
}

SceneVtkItem* SegmentationAdapter::mainScene() const
{
    return mainScene_;
}

LabelListModel* SegmentationAdapter::labels()
{
    return &labels_;
}

bool SegmentationAdapter::ready() const
{
    return ready_;
}

bool SegmentationAdapter::busy() const
{
    return busy_;
}

int SegmentationAdapter::tool() const
{
    return tool_;
}

int SegmentationAdapter::brushShape() const
{
    return brushShape_;
}

int SegmentationAdapter::brushRadius() const
{
    return brushRadius_;
}

void SegmentationAdapter::Queue(Task task)
{
    if (mainScene_ == nullptr)
    {
        return;
    }
    QPointer<SegmentationAdapter> guard(this);
    execute([guard, task = std::move(task)] {
        if (guard)
        {
            task();
        }
    });
}

void SegmentationAdapter::setMainScene(SceneVtkItem* item)
{
    if (busy_ && item != mainScene_)
    {
        return;
    }
    mainScene_ = item;
    initialize(item);
}

void SegmentationAdapter::RefreshSlices()
{
    for (auto item : { axialSlice_.data(), coronalSlice_.data(), sagittalSlice_.data() })
    {
        if (item && item->GetData())
        {
            item->GetData()->GetPaintStyle()->RefreshLabels();
        }
    }
    // dispatch_async and QQuickItem::update belong to the GUI thread.
    QMetaObject::invokeMethod(
        this,
        [this] {
            for (auto item : { axialSlice_.data(), coronalSlice_.data(), sagittalSlice_.data() })
            {
                if (item)
                {
                    item->forceUpdate();
                }
            }
            if (mainScene_)
            {
                mainScene_->forceUpdate();
            }
        },
        Qt::QueuedConnection
    );
}

void SegmentationAdapter::SyncLabels()
{
    auto snapshot = labels_.items();
    Queue([this, snapshot = std::move(snapshot)] {
        manager_->metadata.labels = snapshot;
        manager_->UpdateLabelAppearance();
        RefreshSlices();
    });
}

void SegmentationAdapter::setTool(int value)
{
    if (value < 0 || value > 2 || (value != 0 && (ready_ == false || labels_.activeLabel() == 0)))
    {
        return;
    }
    if (tool_ == value)
    {
        return;
    }
    tool_ = value;
    emit brushChanged();
    ApplyBrush();
}

void SegmentationAdapter::setBrushShape(int value)
{
    if (value < 0 || value >= 6 || value == brushShape_)
    {
        return;
    }
    brushShape_ = value;
    emit brushChanged();
    ApplyBrush();
}

void SegmentationAdapter::setBrushRadius(int value)
{
    value = std::clamp(value, 1, 32);
    if (value == brushRadius_)
    {
        return;
    }
    brushRadius_ = value;
    emit brushChanged();
    ApplyBrush();
}

void SegmentationAdapter::ApplyBrush()
{
    if (ready_ == false)
    {
        return;
    }
    Queue([this, tool = tool_, shape = brushShape_, radius = brushRadius_] {
        for (auto item : { axialSlice_.data(), coronalSlice_.data(), sagittalSlice_.data() })
        {
            if (item == nullptr || item->GetData() == nullptr)
            {
                continue;
            }
            auto data = item->GetData();
            data->GetPaintStyle()->Configure(shape, radius, tool == 2);
            if (tool == 0)
            {
                data->EnableImageInteractionStyle();
            }
            else
            {
                data->EnablePaintBrushInteractorStyle();
            }
        }
        RefreshSlices();
    });
}

void SegmentationAdapter::OpenDicom(QString url, bool directory)
{
    if (busy_ || mainScene_ == nullptr || axialSlice_ == nullptr || coronalSlice_ == nullptr ||
        sagittalSlice_ == nullptr)
    {
        return;
    }
    setTool(0);
    ready_ = false;
    busy_ = true;
    emit readyChanged();
    emit busyChanged();
    Queue([this, url = std::move(url), directory] {
        QString error;
        QVariantList dimensions;
        try
        {
            if (mainScene_->sceneData == nullptr || axialSlice_->GetData() == nullptr ||
                coronalSlice_->GetData() == nullptr || sagittalSlice_->GetData() == nullptr)
            {
                throw std::runtime_error("Views are not initialized yet. Please try again.");
            }
            auto scene = mainScene_->sceneData;
            const bool opened = directory ? scene->OpenDirectory(url) : scene->OpenSingleFile(url);
            if (opened == false || scene->vtk2itk == nullptr)
            {
                throw std::runtime_error("Cannot open this DICOM volume.");
            }
            manager_->scene.sceneRenderer = scene->_renderer;
            manager_->CreateEmptyMask(scene->vtk2itk);
            manager_->metadata.labels.clear();
            manager_->metadata.activeLabel = 0;
            connections_.clear();
            connections_.emplace_back(manager_->maskChanged.connect([this] {
                try
                {
                    manager_->RenderMask();
                }
                catch (const std::exception& e)
                {
                    const QString error = QString::fromUtf8(e.what());
                    QMetaObject::invokeMethod(
                        this,
                        [this, error] {
                            emit errorOccurred(error);
                        },
                        Qt::QueuedConnection
                    );
                }
                RefreshSlices();
            }));
            const auto size = scene->vtk2itk->GetOutput()->GetLargestPossibleRegion().GetSize();
            for (int i = 0; i < 3; ++i)
            {
                dimensions.push_back(static_cast<int>(size[i]));
            }
            for (auto item : { axialSlice_.data(), coronalSlice_.data(), sagittalSlice_.data() })
            {
                auto data = item->GetData();
                auto style = data->GetPaintStyle();
                style->SetManager(manager_);
                connections_.emplace_back(style->radiusStepRequested.connect([this](int delta) {
                    QMetaObject::invokeMethod(
                        this,
                        [this, delta] {
                            setBrushRadius(brushRadius_ + delta);
                        },
                        Qt::QueuedConnection
                    );
                }));
                data->AddDataSet(scene->vtk2itk);
                data->EnableImageInteractionStyle();
            }
            RefreshSlices();
        }
        catch (const std::exception& e)
        {
            error = QString::fromUtf8(e.what());
        }
        QMetaObject::invokeMethod(
            this,
            [this, dimensions, error] {
                busy_ = false;
                ready_ = error.isEmpty();
                emit busyChanged();
                emit readyChanged();
                if (error.isEmpty() == false)
                {
                    emit errorOccurred(error);
                    return;
                }
                labels_.clear();
                labels_.add(QColor(0xFD0007));
                emit volumeOpened(dimensions);
                ApplyBrush();
            },
            Qt::QueuedConnection
        );
    });
}

SegmentationAdapter::~SegmentationAdapter()
{
    dentalJob_.reset();
}

void SegmentationAdapter::FinishDental(QString error)
{
    dentalRunning_ = false;
    dentalSource_ = nullptr;
    busy_ = false;
    if (error.isEmpty() == false)
    {
        dentalStatus_ = "Segmentation failed";
    }
    emit busyChanged();
    emit dentalChanged();
    if (error.isEmpty() == false)
    {
        emit errorOccurred(error);
    }
}

void SegmentationAdapter::CancelDentalSegmentation()
{
    if (dentalRunning_ == false)
    {
        return;
    }
    dentalCancelRequested_ = true;
    dentalJob_->cancel();
}

void SegmentationAdapter::RunDentalSegmentation(QString device)
{
    if (ready_ == false || busy_ || mainScene_ == nullptr)
    {
        return;
    }
    setTool(0);
    busy_ = true;
    dentalRunning_ = true;
    dentalCancelRequested_ = false;
    dentalStatus_ = "Copying CT...";
    emit busyChanged();
    emit dentalChanged();
    Queue([this, device] {
        DentalInferenceJob::Image::Pointer snapshot;
        SegmentationMask::VTK2ITKFilterType::Pointer source;
        QString error;
        try
        {
            if (mainScene_ == nullptr || mainScene_->sceneData == nullptr ||
                mainScene_->sceneData->vtk2itk == nullptr)
            {
                throw std::runtime_error("No CT volume is available.");
            }
            // VTK owns the source buffer. Deep-copy it before handing it to IO.
            source = mainScene_->sceneData->vtk2itk;
            auto copy = itk::ImageDuplicator<DentalInferenceJob::Image>::New();
            copy->SetInputImage(mainScene_->sceneData->vtk2itk->GetOutput());
            copy->Update();
            snapshot = copy->GetOutput();
            snapshot->DisconnectPipeline();
        }
        catch (const std::exception& e)
        {
            error = QString::fromUtf8(e.what());
        }
        QMetaObject::invokeMethod(
            this,
            [this, snapshot, source, error, device] {
                if (error.isEmpty() == false)
                {
                    FinishDental(error);
                    return;
                }
                if (dentalCancelRequested_)
                {
                    dentalStatus_ = "Cancelled";
                    FinishDental();
                    return;
                }
                dentalSource_ = source;
                dentalJob_->start(snapshot, {}, device);
            },
            Qt::QueuedConnection
        );
    });
}
