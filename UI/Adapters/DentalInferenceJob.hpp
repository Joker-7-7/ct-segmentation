#pragma once
#include "Backend/Segmentation/ManualSegmentationManager.hpp"
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <thread>
#include <functional>

// GUI-thread controller. IO runs in a joined worker; inference is a killable
// native process. No mutable image data or VTK objects are shared with it.
class DentalInferenceJob final : public QObject
{
    Q_OBJECT
public:
    using Image = SegmentationMask::ImageType3D;
    using Mask = SegmentationMask::RLE3Type;
    explicit DentalInferenceJob(QObject* parent = nullptr);
    ~DentalInferenceJob() override;
    void start(Image::Pointer snapshot, QString modelDirectory, QString device);
    void cancel();
    static std::vector<LabelMetaData> labels();
    std::function<void(Mask::Pointer, QString, bool)> completed;
    std::function<void(QString)> progress;

private:
    void finish(Mask::Pointer mask, QString error);
    void loadResult();
    void join();
    QProcess process_;
    std::unique_ptr<QTemporaryDir> directory_;
    Image::Pointer source_;
    std::thread io_;
    QString processError_;
    bool active_ = false;
    bool cancelled_ = false;
};
