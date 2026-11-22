#include "UI/Adapters/DentalInferenceJob.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <itkImageFileReader.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 4)
    {
        std::cerr << "Usage: DentalJobTest input.nii.gz model-directory expected.nii.gz\n";
        return 2;
    }
    try
    {
        auto reader = itk::ImageFileReader<DentalInferenceJob::Image>::New();
        reader->SetFileName(app.arguments()[1].toStdString());
        reader->Update();
        DentalInferenceJob::Image::Pointer input = reader->GetOutput();
        input->DisconnectPipeline();
        using LabelImage = itk::Image<unsigned short, 3>;
        auto reference = itk::ImageFileReader<LabelImage>::New();
        reference->SetFileName(app.arguments()[3].toStdString());
        reference->Update();
        for (int scenario = 0; scenario < 4; ++scenario)
        {
            DentalInferenceJob job;
            QEventLoop loop;
            bool finished = false, cancelled = false;
            QString error;
            DentalInferenceJob::Mask::Pointer result;
            job.completed = [&](auto mask, auto message, bool wasCancelled) {
                result = mask;
                error = message;
                cancelled = wasCancelled;
                finished = true;
                loop.quit();
            };
            job.progress = [&](const QString& message) {
                if (scenario == 2 && message.contains("Preparing volume"))
                {
                    job.cancel();
                }
            };
            job.start(
                input,
                scenario == 0 ? "missing-model-directory-for-test" : app.arguments()[2],
                "cpu"
            );
            if (scenario == 1)
            {
                job.cancel();
            }
            QTimer deadline;
            deadline.setSingleShot(true);
            QObject::connect(&deadline, &QTimer::timeout, &loop, [&] {
                job.cancel();
                loop.quit();
            });
            deadline.start(120000);
            if (finished == false)
            {
                loop.exec();
            }
            if (finished == false)
            {
                throw std::runtime_error("Job deadline expired");
            }
            if (scenario == 0 && (result || error.isEmpty() || cancelled))
            {
                throw std::runtime_error("Missing model error was lost");
            }
            if ((scenario == 1 || scenario == 2) && (result || cancelled == false))
            {
                throw std::runtime_error("Cancellation returned a result");
            }
            if (scenario == 3)
            {
                if (result == nullptr || error.isEmpty() == false || cancelled)
                {
                    throw std::runtime_error(error.toStdString());
                }
                const auto metadata = DentalInferenceJob::labels();
                const auto size = reference->GetOutput()->GetLargestPossibleRegion().GetSize();
                const auto expected = reference->GetOutput()->GetBufferPointer();
                for (std::size_t z = 0; z < size[2]; ++z)
                {
                    for (std::size_t y = 0; y < size[1]; ++y)
                    {
                        DentalInferenceJob::Mask::BufferType::IndexType row;
                        row[0] = y;
                        row[1] = z;
                        std::size_t x = 0;
                        for (const auto& run : result->GetBuffer()->GetPixel(row))
                        {
                            if (run.first == 0 || x + run.first > size[0])
                            {
                                throw std::runtime_error("Invalid RLE span");
                            }
                            for (int k = 0; k < run.first; ++k, ++x)
                            {
                                const auto label = expected[(z * size[1] + y) * size[0] + x];
                                if (run.second != (label ? metadata.at(label - 1).label : 0))
                                {
                                    throw std::runtime_error("Imported mask differs from reference"
                                    );
                                }
                            }
                        }
                        if (x != size[0])
                        {
                            throw std::runtime_error("Incomplete RLE row");
                        }
                    }
                }
            }
        }
        std::cout << "Passed missing-model, IO cancellation, process cancellation, and complete "
                     "RLE import tests\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
