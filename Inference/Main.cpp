#include "Volume.hpp"
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <onnxruntime_cxx_api.h>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#ifdef _WIN32
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace
{
    using Image = itk::Image<float, 3>;
    using Mask = itk::Image<unsigned short, 3>;
    using Dental::Shape;
    using Dental::Volume;
    constexpr Shape Patch{ 128, 160, 112 };
    constexpr int ClassCount = 6;
    struct ModelConfig
    {
        double mean;
        double standardDeviation;
        double clipLow;
        double clipHigh;
        std::array<double, 3> spacing;
    };

    QJsonObject ReadJson(const QString& path)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly) == false)
        {
            throw std::runtime_error("Cannot open model metadata");
        }
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || doc.isObject() == false)
        {
            throw std::runtime_error("Invalid model metadata");
        }
        return doc.object();
    }

    ModelConfig ReadAndValidateModelConfig(const QString& directory)
    {
        const auto plans = ReadJson(directory + "/plans.json");
        const auto configuration = plans["configurations"].toObject()["3d_fullres"].toObject();
        if (plans["dataset_name"].toString() != "Dataset111_453CT" ||
            plans["transpose_forward"].toArray() != QJsonArray{ 1, 0, 2 } ||
            configuration["patch_size"].toArray() !=
                QJsonArray{ Patch[0], Patch[1], Patch[2] } ||
            configuration["normalization_schemes"].toArray() != QJsonArray{ "CTNormalization" })
        {
            throw std::runtime_error("Unsupported DentalSegmentator model configuration");
        }
        const auto stats =
            plans["foreground_intensity_properties_per_channel"].toObject()["0"].toObject();
        constexpr ModelConfig supported{ 1273.709228515625,
                                         558.4849853515625,
                                         -110,
                                         3067,
                                         { 0.4492189884185791,
                                           0.31200000643730164,
                                           0.4492189884185791 } };
        ModelConfig model{ stats["mean"].toDouble(),
                           stats["std"].toDouble(),
                           stats["percentile_00_5"].toDouble(),
                           stats["percentile_99_5"].toDouble(),
                           {} };
        if (model.mean != supported.mean ||
            model.standardDeviation != supported.standardDeviation ||
            model.clipLow != supported.clipLow || model.clipHigh != supported.clipHigh)
        {
            throw std::runtime_error("Unsupported model normalization");
        }
        const auto spacing = configuration["spacing"].toArray();
        if (spacing.size() != 3)
        {
            throw std::runtime_error("Missing model spacing");
        }
        for (int i = 0; i < 3; ++i)
        {
            model.spacing[i] = spacing[i].toDouble();
            if (model.spacing[i] != supported.spacing[i])
            {
                throw std::runtime_error("Unsupported model spacing");
            }
        }
        const auto labels = ReadJson(directory + "/dataset.json")["labels"].toObject();
        if (labels != QJsonObject{ { "background", 0 },
                                   { "Upper Skull", 1 },
                                   { "Mandible", 2 },
                                   { "Upper Teeth", 3 },
                                   { "Lower Teeth", 4 },
                                   { "Mandibular canal", 5 } })
        {
            throw std::runtime_error("Unsupported model labels");
        }
        return model;
    }

    std::size_t Offset(const Shape& s, int a, int b, int c)
    {
        return (static_cast<std::size_t>(a) * s[1] + b) * s[2] + c;
    }

    struct CropRegion
    {
        Shape origin;
        Shape shape;
    };

    CropRegion FindCropRegion(const Image* image)
    {
        const auto size = image->GetLargestPossibleRegion().GetSize();
        for (int i = 0; i < 3; ++i)
        {
            if (size[i] == 0 || size[i] > std::numeric_limits<int>::max() ||
                std::isfinite(image->GetSpacing()[i]) == false || image->GetSpacing()[i] <= 0)
            {
                throw std::runtime_error("Invalid CT dimensions or spacing");
            }
        }
        const Shape full{ static_cast<int>(size[1]),
                          static_cast<int>(size[2]),
                          static_cast<int>(size[0]) };
        Shape lower = full, upper{ 0, 0, 0 };
        const auto source = image->GetBufferPointer();
        for (int y = 0; y < full[0]; ++y)
        {
            for (int z = 0; z < full[1]; ++z)
            {
                for (int x = 0; x < full[2]; ++x)
                {
                    const auto value =
                        source[(static_cast<std::size_t>(z) * size[1] + y) * size[0] + x];
                    if (std::isfinite(value) == false)
                    {
                        throw std::runtime_error("CT contains non-finite intensities");
                    }
                    if (value != 0)
                    {
                        const Shape p{ y, z, x };
                        for (int a = 0; a < 3; ++a)
                        {
                            lower[a] = std::min(lower[a], p[a]);
                            upper[a] = std::max(upper[a], p[a] + 1);
                        }
                    }
                }
            }
        }
        Shape cropped;
        for (int a = 0; a < 3; ++a)
        {
            cropped[a] = upper[a] - lower[a];
        }
        return { lower, cropped };
    }

    Volume NormalizeCrop(const Image* image, const CropRegion& crop, const ModelConfig& model)
    {
        const auto size = image->GetLargestPossibleRegion().GetSize();
        Volume input{ crop.shape, std::vector<float>(Dental::Count(crop.shape)) };
        const float clipLow = static_cast<float>(model.clipLow);
        const float clipHigh = static_cast<float>(model.clipHigh);
        const float mean = static_cast<float>(model.mean);
        const float standardDeviation = static_cast<float>(model.standardDeviation);
        const auto source = image->GetBufferPointer();
        for (int y = 0; y < crop.shape[0]; ++y)
        {
            for (int z = 0; z < crop.shape[1]; ++z)
            {
                for (int x = 0; x < crop.shape[2]; ++x)
                {
                    const float v = source
                        [(static_cast<std::size_t>(z + crop.origin[1]) * size[1] +
                          y + crop.origin[0]) * size[0] + x + crop.origin[2]];
                    input.data[Offset(crop.shape, y, z, x)] =
                        (std::clamp(v, clipLow, clipHigh) - mean) / standardDeviation;
                }
            }
        }
        return input;
    }

    struct ResamplePlan
    {
        Shape resampled;
        Shape padded;
        Shape pad;
        int separateAxis;
    };

    ResamplePlan MakeResamplePlan(
        const CropRegion& crop, const Image* image, const ModelConfig& model
    )
    {
        const std::array<double, 3> spacing{ image->GetSpacing()[1],
                                             image->GetSpacing()[2],
                                             image->GetSpacing()[0] };
        ResamplePlan plan{};
        for (int a = 0; a < 3; ++a)
        {
            const double n = std::nearbyint(crop.shape[a] * spacing[a] / model.spacing[a]);
            if (n < 1 || n > std::numeric_limits<int>::max())
            {
                throw std::runtime_error("Unsupported resampled dimensions");
            }
            plan.resampled[a] = static_cast<int>(n);
        }
        // A unique low-resolution axis is resampled separately in nnU-Net.
        plan.separateAxis = -1;
        const auto limits = std::minmax_element(spacing.begin(), spacing.end());
        if (*limits.second / *limits.first > 3 &&
            std::count(spacing.begin(), spacing.end(), *limits.second) == 1)
        {
            plan.separateAxis = static_cast<int>(limits.second - spacing.begin());
        }
        for (int a = 0; a < 3; ++a)
        {
            plan.padded[a] = std::max(plan.resampled[a], Patch[a]);
            plan.pad[a] = (plan.padded[a] - plan.resampled[a]) / 2;
        }
        const auto voxelCount = Dental::Count(plan.padded);
        // Explicit bound before allocating probability volumes; bad_alloc is also reported.
        if (voxelCount > (std::size_t{ 8 } << 30) / ((ClassCount + 1) * sizeof(float)))
        {
            throw std::runtime_error(
                "CT requires more than 8 GiB of inference buffers; reduce the field of view"
            );
        }
        return plan;
    }

    void ValidateSession(const Ort::Session& session)
    {
        if (session.GetInputCount() != 1 || session.GetOutputCount() != 1 ||
            session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape() !=
                std::vector<int64_t>{ 1, 1, Patch[0], Patch[1], Patch[2] } ||
            session.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape() !=
                std::vector<int64_t>{ 1, ClassCount, Patch[0], Patch[1], Patch[2] })
        {
            throw std::runtime_error("Invalid ONNX input/output contract");
        }
        if (session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetElementType() !=
                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
            session.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetElementType() !=
                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
        {
            throw std::runtime_error("ONNX tensors must contain float32 logits and input voxels");
        }
    }

    std::vector<float> LoadGaussian(const QString& directory)
    {
        const auto patchCount = Dental::Count(Patch);
        std::vector<float> gaussian(patchCount);
        QFile gaussianFile(directory + "/gaussian.bin");
        if (gaussianFile.open(QIODevice::ReadOnly) == false ||
            gaussianFile.size() != static_cast<qint64>(patchCount * sizeof(float)) ||
            gaussianFile.read(reinterpret_cast<char*>(gaussian.data()), gaussianFile.size()) !=
                gaussianFile.size())
        {
            throw std::runtime_error("Missing gaussian.bin; run the model exporter");
        }
        for (float weight : gaussian)
        {
            if (std::isfinite(weight) == false || weight <= 0)
            {
                throw std::runtime_error("Invalid Gaussian weights");
            }
        }
        return gaussian;
    }

    void RemoveIslands(Mask* mask)
    {
        using Binary = itk::Image<unsigned char, 3>;
        using Components = itk::Image<unsigned int, 3>;
        const double voxelVolume =
            mask->GetSpacing()[0] * mask->GetSpacing()[1] * mask->GetSpacing()[2];
        const double minimum = std::ceil(60.0 / voxelVolume);
        if (std::isfinite(minimum) == false || minimum > std::numeric_limits<unsigned long>::max())
        {
            throw std::runtime_error("Invalid voxel volume");
        }
        const auto count = mask->GetLargestPossibleRegion().GetNumberOfPixels();
        auto pixels = mask->GetBufferPointer();
        for (unsigned short label = 1; label <= 4; ++label)
        {
            auto binary = Binary::New();
            binary->CopyInformation(mask);
            binary->SetRegions(mask->GetLargestPossibleRegion());
            binary->Allocate();
            for (std::size_t i = 0; i < count; ++i)
            {
                binary->GetBufferPointer()[i] = pixels[i] == label;
            }
            auto components = itk::ConnectedComponentImageFilter<Binary, Components>::New();
            components->SetInput(binary);
            components->SetFullyConnected(false);
            auto relabel = itk::RelabelComponentImageFilter<Components, Components>::New();
            relabel->SetInput(components->GetOutput());
            relabel->SetMinimumObjectSize(static_cast<unsigned long>(minimum));
            relabel->Update();
            const auto kept = relabel->GetOutput()->GetBufferPointer();
            for (std::size_t i = 0; i < count; ++i)
            {
                if (pixels[i] == label && kept[i] == false)
                {
                    pixels[i] = 0;
                }
            }
        }
    }

    Mask::Pointer Predict(Image* image, const QString& directory, const QString& device)
    {
        const auto model = ReadAndValidateModelConfig(directory);
        const auto crop = FindCropRegion(image);
        auto input = NormalizeCrop(image, crop, model);
        const auto plan = MakeResamplePlan(crop, image, model);
        const auto& cropped = crop.shape;
        const auto& lower = crop.origin;
        const auto& resampled = plan.resampled;
        const auto& padded = plan.padded;
        const auto& pad = plan.pad;
        const auto separateAxis = plan.separateAxis;
        const auto voxelCount = Dental::Count(padded);
        std::cout << "Preparing volume" << std::endl;
        input = Dental::Resize(input, resampled, 3, separateAxis);
        Ort::Env environment(ORT_LOGGING_LEVEL_WARNING, "DentalSegmentator");
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(4);
        if (device == "cuda")
        {
            OrtCUDAProviderOptions cudaOptions{};
            cudaOptions.device_id = 0;
            try
            {
                options.AppendExecutionProvider_CUDA(cudaOptions);
            }
            catch (const Ort::Exception& error)
            {
                throw std::runtime_error(
                    std::string("Cannot initialize CUDA GPU: ") + error.what()
                );
            }
        }
        const auto modelPath = (directory + "/dental.onnx").toStdWString();
        Ort::Session session(environment, modelPath.c_str(), options);
        ValidateSession(session);
        const auto patchCount = Dental::Count(Patch);
        const auto gaussian = LoadGaussian(directory);
        std::vector<Ort::Float16_t> accumulated(ClassCount * voxelCount, Ort::Float16_t(0.f)),
            weights(voxelCount, Ort::Float16_t(0.f));
        std::vector<float> patch(patchCount);
        const std::array<int64_t, 5> dimensions{ 1, 1, Patch[0], Patch[1], Patch[2] };
        auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        auto tensor = Ort::Value::CreateTensor<float>(
            memory, patch.data(), patch.size(), dimensions.data(), dimensions.size()
        );
        const auto aSteps = Dental::Steps(padded[0], Patch[0]),
                   bSteps = Dental::Steps(padded[1], Patch[1]),
                   cSteps = Dental::Steps(padded[2], Patch[2]);
        const auto total = aSteps.size() * bSteps.size() * cSteps.size();
        std::size_t done = 0;
        const char* inputs[]{ "image" };
        const char* outputs[]{ "logits" };
        for (int a : aSteps)
        {
            for (int b : bSteps)
            {
                for (int c : cSteps)
                {
                    for (int i = 0; i < Patch[0]; ++i)
                    {
                        for (int j = 0; j < Patch[1]; ++j)
                        {
                            for (int k = 0; k < Patch[2]; ++k)
                            {
                                const int y = a + i - pad[0], z = b + j - pad[1],
                                          x = c + k - pad[2];
                                patch[Offset(Patch, i, j, k)] =
                                    y >= 0 && y < resampled[0] && z >= 0 && z < resampled[1] &&
                                            x >= 0 && x < resampled[2]
                                        ? input.data[Offset(resampled, y, z, x)]
                                        : 0.f;
                            }
                        }
                    }
                    auto prediction =
                        session.Run(Ort::RunOptions{ nullptr }, inputs, &tensor, 1, outputs, 1);
                    if (prediction[0].GetTensorTypeAndShapeInfo().GetElementCount() !=
                        ClassCount * patchCount)
                    {
                        throw std::runtime_error("Unexpected ONNX output tensor size");
                    }
                    const auto logits = prediction[0].GetTensorData<float>();
                    for (int i = 0; i < Patch[0]; ++i)
                    {
                        for (int j = 0; j < Patch[1]; ++j)
                        {
                            for (int k = 0; k < Patch[2]; ++k)
                            {
                                const auto local = Offset(Patch, i, j, k),
                                           global = Offset(padded, a + i, b + j, c + k);
                                weights[global] =
                                    Ort::Float16_t(weights[global].ToFloat() + gaussian[local]);
                                for (int label = 0; label < ClassCount; ++label)
                                {
                                    const float value = logits[label * patchCount + local];
                                    if (std::isfinite(value) == false)
                                    {
                                        throw std::runtime_error(
                                            "Inference produced non-finite logits"
                                        );
                                    }
                                    auto& sum = accumulated[label * voxelCount + global];
                                    sum = Ort::Float16_t(sum.ToFloat() + value * gaussian[local]);
                                }
                            }
                        }
                    }
                    std::cout << "Blocks " << ++done << "/" << total << std::endl;
                }
            }
        }
        // Resample logits before argmax; resampling a discrete mask changes boundaries.
        std::vector<float> best(Dental::Count(cropped), -std::numeric_limits<float>::infinity());
        std::vector<unsigned short> labels(best.size(), 0);
        for (int label = 0; label < ClassCount; ++label)
        {
            Volume probabilities{ resampled, std::vector<float>(Dental::Count(resampled)) };
            for (int y = 0; y < resampled[0]; ++y)
            {
                for (int z = 0; z < resampled[1]; ++z)
                {
                    for (int x = 0; x < resampled[2]; ++x)
                    {
                        const auto global = Offset(padded, y + pad[0], z + pad[1], x + pad[2]);
                        const float value = Ort::Float16_t(
                                                accumulated[label * voxelCount + global].ToFloat() /
                                                weights[global].ToFloat()
                        )
                                                .ToFloat();
                        if (std::isfinite(value) == false)
                        {
                            throw std::runtime_error("Non-finite accumulated logits");
                        }
                        probabilities.data[Offset(resampled, y, z, x)] = value;
                    }
                }
            }
            probabilities = Dental::Resize(probabilities, cropped, 1, separateAxis);
            for (std::size_t i = 0; i < best.size(); ++i)
            {
                const float value = Ort::Float16_t(probabilities.data[i]).ToFloat();
                if (value > best[i])
                {
                    best[i] = value;
                    labels[i] = static_cast<unsigned short>(label);
                }
            }
        }
        auto result = Mask::New();
        result->CopyInformation(image);
        result->SetRegions(image->GetLargestPossibleRegion());
        result->Allocate(true);
        auto output = result->GetBufferPointer();
        const auto size = image->GetLargestPossibleRegion().GetSize();
        for (int y = 0; y < cropped[0]; ++y)
        {
            for (int z = 0; z < cropped[1]; ++z)
            {
                for (int x = 0; x < cropped[2]; ++x)
                {
                    output
                        [(static_cast<std::size_t>(z + lower[1]) * size[1] + y + lower[0]) *
                             size[0] +
                         x + lower[2]] = labels[Offset(cropped, y, z, x)];
                }
            }
        }
        std::cout << "Removing small islands" << std::endl;
        RemoveIslands(result);
        return result;
    }
} // namespace
int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if ((args.size() != 4 && args.size() != 5) ||
        (args.size() == 5 && args[4] != "cpu" && args[4] != "cuda"))
    {
        std::cerr
            << "Usage: DentalInference input.nii.gz output.nii.gz model-directory [cpu|cuda]\n";
        return 2;
    }
    try
    {
        auto reader = itk::ImageFileReader<Image>::New();
        reader->SetFileName(args[1].toStdString());
        reader->Update();
        auto result = Predict(reader->GetOutput(), args[3], args.size() == 5 ? args[4] : "cpu");
        auto writer = itk::ImageFileWriter<Mask>::New();
        writer->SetFileName(args[2].toStdString());
        writer->SetInput(result);
        writer->Update();
        std::cout << "Finished" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
