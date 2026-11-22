#include "DentalInferenceJob.hpp"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <cmath>
#include <limits>

DentalInferenceJob::DentalInferenceJob(QObject* parent) :
    QObject(parent)
{
    connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
        const auto text = QString::fromUtf8(process_.readAllStandardOutput()).trimmed();
        if (progress && text.isEmpty() == false)
        {
            progress(text.section('\n', -1));
        }
    });
    connect(&process_, &QProcess::readyReadStandardError, this, [this] {
        processError_ += QString::fromUtf8(process_.readAllStandardError());
        processError_ = processError_.right(8192);
    });
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
        {
            finish({}, process_.errorString());
        }
    });
    connect(
        &process_,
        qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this,
        [this](int code, QProcess::ExitStatus status) {
            if (active_ == false)
            {
                return;
            }
            if (cancelled_)
            {
                finish({}, {});
                return;
            }
            processError_ += QString::fromUtf8(process_.readAllStandardError());
            if (status != QProcess::NormalExit || code != 0)
            {
                finish(
                    {},
                    QString("Dental inference failed (exit %1).\n%2")
                        .arg(code)
                        .arg(processError_.right(8192))
                );
                return;
            }
            loadResult();
        }
    );
}

DentalInferenceJob::~DentalInferenceJob()
{
    disconnect(&process_, nullptr, this, nullptr);
    if (process_.state() != QProcess::NotRunning)
    {
        process_.kill();
        process_.waitForFinished(-1);
    }
    join(); // queued callbacks are discarded when QObject is destroyed
}

void DentalInferenceJob::join()
{
    if (io_.joinable())
    {
        io_.join();
    }
}

std::vector<LabelMetaData> DentalInferenceJob::labels()
{
    // Use existing palette identities so imported structures remain editable.
    const std::array<QRgb, 5> colors{ 0xF1BFC1, 0xC4DAF1, 0xFEE188, 0xFEF0C0, 0xD26958 };
    const std::array<const char*, 5> names{
        "Upper Skull", "Mandible", "Upper Teeth", "Lower Teeth", "Mandibular canal"
    };
    std::vector<LabelMetaData> result;
    for (int i = 0; i < 5; ++i)
    {
        const QColor color(colors[i]);
        result.push_back(
            { LabelMetaData::EncodeColor(color), QString::fromLatin1(names[i]), color, true }
        );
    }
    return result;
}

void DentalInferenceJob::start(Image::Pointer snapshot, QString modelDirectory, QString device)
{
    if (active_)
    {
        return;
    }
    active_ = true;
    cancelled_ = false;
    processError_.clear();
    source_ = snapshot;
    try
    {
        const QString program = QCoreApplication::applicationDirPath() + "/DentalInference.exe";
        if (modelDirectory.isEmpty())
        {
            modelDirectory = QDir(QCoreApplication::applicationDirPath()).filePath("dental-model");
        }
        if (snapshot == nullptr || QFileInfo::exists(program) == false)
        {
            finish(
                {},
                "DentalInference.exe is missing. Build with SEGMENTATION_BUILD_DENTAL_INFERENCE=ON."
            );
            return;
        }
        for (const auto* file : { "dental.onnx", "plans.json", "dataset.json", "gaussian.bin" })
        {
            if (QFileInfo::exists(QDir(modelDirectory).filePath(file)) == false)
            {
                finish(
                    {}, QString("Missing model file: %1").arg(QDir(modelDirectory).filePath(file))
                );
                return;
            }
        }
        directory_ =
            std::make_unique<QTemporaryDir>(QDir::tempPath() + "/DentalSegmentator-XXXXXX");
        if (directory_->isValid() == false)
        {
            finish({}, "Cannot create a temporary inference directory.");
            return;
        }
        if (progress)
        {
            progress("Preparing CT...");
        }
        const auto input = directory_->filePath("input.nii.gz"),
                   output = directory_->filePath("output.nii.gz");
        join();
        io_ = std::thread([this, snapshot, input, output, program, modelDirectory, device] {
            QString error;
            try
            {
                auto writer = itk::ImageFileWriter<Image>::New();
                writer->SetFileName(input.toStdString());
                writer->SetInput(snapshot);
                writer->Update();
            }
            catch (const std::exception& e)
            {
                error = QString::fromUtf8(e.what());
            }
            QMetaObject::invokeMethod(
                this,
                [this, error, input, output, program, modelDirectory, device] {
                    join();
                    if (cancelled_ || error.isEmpty() == false)
                    {
                        finish({}, error);
                        return;
                    }
                    process_.start(program, { input, output, modelDirectory, device });
                },
                Qt::QueuedConnection
            );
        });
    }
    catch (const std::exception& e)
    {
        finish({}, QString::fromUtf8(e.what()));
    }
}

void DentalInferenceJob::cancel()
{
    if (active_ == false)
    {
        return;
    }
    cancelled_ = true;
    if (process_.state() != QProcess::NotRunning)
    {
        process_.kill();
    }
    if (progress)
    {
        progress("Cancelling...");
    }
}

void DentalInferenceJob::finish(Mask::Pointer mask, QString error)
{
    if (active_ == false)
    {
        return;
    }
    join();
    active_ = false;
    source_ = nullptr;
    directory_.reset();
    if (completed)
    {
        completed(mask, std::move(error), cancelled_);
    }
}

void DentalInferenceJob::loadResult()
{
    if (progress)
    {
        progress("Loading segmentation...");
    }
    const auto path = directory_->filePath("output.nii.gz");
    const auto source = source_;
    const auto metadata = labels();
    join();
    io_ = std::thread([this, path, source, metadata] {
        QString error;
        Mask::Pointer result;
        try
        {
            using LabelImage = itk::Image<unsigned short, 3>;
            auto reader = itk::ImageFileReader<LabelImage>::New();
            reader->SetFileName(path.toStdString());
            reader->Update();
            const auto image = reader->GetOutput();
            if (image->GetLargestPossibleRegion() != source->GetLargestPossibleRegion())
            {
                throw std::runtime_error("Inference mask dimensions do not match the CT");
            }
            for (int a = 0; a < 3; ++a)
            {
                if (std::abs(image->GetSpacing()[a] - source->GetSpacing()[a]) > 1e-5 ||
                    std::abs(image->GetOrigin()[a] - source->GetOrigin()[a]) > 1e-3)
                {
                    throw std::runtime_error("Inference mask geometry does not match the CT");
                }
                for (int b = 0; b < 3; ++b)
                {
                    if (std::abs(image->GetDirection()[a][b] - source->GetDirection()[a][b]) > 1e-5)
                    {
                        throw std::runtime_error("Inference mask orientation does not match the CT"
                        );
                    }
                }
            }
            const auto region = source->GetLargestPossibleRegion();
            const auto size = region.GetSize();
            const auto start = region.GetIndex();
            if (size[0] > std::numeric_limits<unsigned short>::max())
            {
                throw std::runtime_error("CT rows exceed the mask format capacity");
            }
            result = Mask::New();
            result->CopyInformation(source);
            result->SetRegions(region);
            result->Allocate(true);
            result->SetOnTheFlyCleanup(true);
            const auto pixels = image->GetBufferPointer();
            for (std::size_t z = 0; z < size[2]; ++z)
            {
                for (std::size_t y = 0; y < size[1]; ++y)
                {
                    Mask::BufferType::IndexType index;
                    index[0] = start[1] + y;
                    index[1] = start[2] + z;
                    auto& line = result->GetBuffer()->GetPixel(index);
                    line.clear();
                    for (std::size_t x = 0; x < size[0]; ++x)
                    {
                        const auto value = pixels[(z * size[1] + y) * size[0] + x];
                        if (value > 5)
                        {
                            throw std::runtime_error(
                                "Inference returned an unknown anatomical label"
                            );
                        }
                        const unsigned short mapped = value ? metadata[value - 1].label : 0;
                        if (line.empty() == false && line.back().second == mapped)
                        {
                            ++line.back().first;
                        }
                        else
                        {
                            line.emplace_back(1, mapped);
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            error = QString::fromUtf8(e.what());
            result = nullptr;
        }
        QMetaObject::invokeMethod(
            this,
            [this, result, error] {
                finish(cancelled_ ? Mask::Pointer{} : result, error);
            },
            Qt::QueuedConnection
        );
    });
}
