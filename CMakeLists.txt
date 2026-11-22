cmake_minimum_required(VERSION 3.5)

project(Segmentation VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources/temp)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Quick
    Network
)

find_package(Boost 1.85.0 REQUIRED)
find_package(VTK COMPONENTS REQUIRED)
find_package(ITK REQUIRED COMPONENTS)

include( ${ITK_USE_FILE} ) 

set(Sources "")
set(Headers "")

file(GLOB_RECURSE backend_headers CONFIGURE_DEPENDS Backend/*.h*)
list(APPEND Headers ${backend_headers})

file(GLOB_RECURSE backend_sources CONFIGURE_DEPENDS Backend/*.cpp)
list(APPEND Sources ${backend_sources})

file(GLOB_RECURSE ui_headers UI/*.h*)
list(APPEND Headers ${ui_headers})

file(GLOB_RECURSE ui_sources UI/*.cpp)
list(APPEND Sources ${ui_sources})

file (GLOB         QtResources *.qrc)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/Gui/Assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} WIN32 ${Sources}  ${QtResources} ${Headers})

set(Boost_INCLUDE_DIR $ENV{BOOST_ROOT})
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
                                PRIVATE Qt6::Core Qt6::Quick Qt6::Network
                                PRIVATE ${VTK_LIBRARIES}
                                PRIVATE ${ITK_LIBRARIES}
)

vtk_module_autoinit(
    TARGETS ${PROJECT_NAME}
    MODULES ${VTK_LIBRARIES}
)

option(SEGMENTATION_BUILD_TESTS "Build label-system regression tests" OFF)
if(SEGMENTATION_BUILD_TESTS)
    enable_testing()
    add_executable(SegmentationLabelsTest
        Tests/LabelsTest.cpp
        Backend/Overlay/PaintbrushInteractorStyle.cpp
        Backend/Overlay/BrushCursorController.cpp
        Backend/Overlay/BrushPaintContext.cpp
        Backend/Overlay/PaintbrushSettings.cpp
        Backend/Overlay/BrushWatershedPipeline.cpp
        Backend/GraphicPrimitives/Vector3D.cpp
        UI/Adapters/LabelListModel.cpp
        UI/Adapters/LabelListModel.hpp
        Backend/Segmentation/LabelMetaData.cpp
        Backend/Segmentation/ManualSegmentationManager.cpp
        Backend/Segmentation/SegmentationMask.cpp
        Backend/Segmentation/LabelMetadataStore.cpp
        Backend/Segmentation/LabelRegistry.cpp
        Backend/Segmentation/LabelMeshBuilder.cpp)
    target_include_directories(SegmentationLabelsTest PRIVATE ${Boost_INCLUDE_DIR})
    target_link_libraries(SegmentationLabelsTest PRIVATE Qt6::Core Qt6::Gui ${VTK_LIBRARIES} ${ITK_LIBRARIES})
    vtk_module_autoinit(TARGETS SegmentationLabelsTest MODULES ${VTK_LIBRARIES})
    add_test(NAME segmentation-labels COMMAND SegmentationLabelsTest)
endif()

set(ONNXRUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/LocalAssets/dental-model/onnxruntime-win-x64-1.20.1" CACHE PATH "Extracted ONNX Runtime C++ distribution (GPU package required for CUDA)")
set(DENTAL_INFERENCE_DEFAULT OFF)
if(EXISTS "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h")
    set(DENTAL_INFERENCE_DEFAULT ON)
endif()
option(SEGMENTATION_BUILD_DENTAL_INFERENCE "Build native DentalSegmentator inference" ${DENTAL_INFERENCE_DEFAULT})
if(SEGMENTATION_BUILD_DENTAL_INFERENCE)
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATHS "${ONNXRUNTIME_ROOT}/include" REQUIRED)
    find_library(ONNXRUNTIME_LIBRARY onnxruntime PATHS "${ONNXRUNTIME_ROOT}/lib" REQUIRED)
    add_executable(DentalInference Inference/Main.cpp Inference/Volume.cpp)
    add_dependencies(${PROJECT_NAME} DentalInference)
    set(DENTAL_MODEL_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/LocalAssets/dental-model/exported" CACHE PATH "Exported dental ONNX model directory")
    set(DENTAL_MODEL_SOURCE_DIRECTORY "${DENTAL_MODEL_DIRECTORY}")
    if(NOT DENTAL_MODEL_SOURCE_DIRECTORY)
        set(DENTAL_MODEL_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/LocalAssets/dental-model/exported")
    endif()
    if(EXISTS "${DENTAL_MODEL_SOURCE_DIRECTORY}/dental.onnx")
        add_custom_target(DentalModelFiles ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/dental-model"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DENTAL_MODEL_SOURCE_DIRECTORY}/dental.onnx"
                "${DENTAL_MODEL_SOURCE_DIRECTORY}/plans.json"
                "${DENTAL_MODEL_SOURCE_DIRECTORY}/dataset.json"
                "${DENTAL_MODEL_SOURCE_DIRECTORY}/gaussian.bin"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/dental-model"
            COMMENT "Copying dental model next to Segmentation"
            VERBATIM)
    else()
        message(WARNING "Dental model not found at ${DENTAL_MODEL_SOURCE_DIRECTORY}; segmentation will require the model in the application directory")
    endif()
    target_include_directories(DentalInference PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
    target_link_libraries(DentalInference PRIVATE Qt6::Core ${ITK_LIBRARIES} "${ONNXRUNTIME_LIBRARY}")
    if(WIN32)
        add_custom_target(DentalOnnxRuntime ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll" "$<TARGET_FILE_DIR:DentalInference>"
            VERBATIM)
        if(EXISTS "${ONNXRUNTIME_ROOT}/lib/onnxruntime_providers_cuda.dll")
            add_custom_target(DentalCudaProvider ALL
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ONNXRUNTIME_ROOT}/lib/onnxruntime_providers_shared.dll" "$<TARGET_FILE_DIR:DentalInference>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ONNXRUNTIME_ROOT}/lib/onnxruntime_providers_cuda.dll" "$<TARGET_FILE_DIR:DentalInference>"
                VERBATIM)
            set(DENTAL_CUDA_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/LocalAssets/dental-cuda-runtime" CACHE PATH "CUDA and cuDNN runtime DLLs for the dental inference worker")
            set(DENTAL_CUDA_DLLS
                "${DENTAL_CUDA_RUNTIME_ROOT}/nvidia/cuda_runtime/bin/cudart64_12.dll"
                "${DENTAL_CUDA_RUNTIME_ROOT}/nvidia/cublas/bin/cublas64_12.dll"
                "${DENTAL_CUDA_RUNTIME_ROOT}/nvidia/cublas/bin/cublasLt64_12.dll"
                "${DENTAL_CUDA_RUNTIME_ROOT}/nvidia/cufft/bin/cufft64_11.dll")
            file(GLOB DENTAL_CUDNN_DLLS "${DENTAL_CUDA_RUNTIME_ROOT}/nvidia/cudnn/bin/cudnn*.dll")
            list(APPEND DENTAL_CUDA_DLLS ${DENTAL_CUDNN_DLLS})
            if(EXISTS "${DENTAL_CUDA_RUNTIME_ROOT}/nvidia/cudnn/bin/cudnn64_9.dll")
                add_custom_target(DentalCudaRuntime
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DENTAL_CUDA_DLLS} "$<TARGET_FILE_DIR:DentalInference>"
                    COMMENT "Copying CUDA and cuDNN runtime DLLs")
                add_dependencies(DentalCudaRuntime DentalInference)
                add_dependencies(${PROJECT_NAME} DentalCudaRuntime)
            else()
                message(WARNING "CUDA runtime not found at ${DENTAL_CUDA_RUNTIME_ROOT}; CUDA inference will not start without these DLLs")
            endif()
        endif()
    endif()
endif()
if(SEGMENTATION_BUILD_TESTS)
    add_executable(DentalResampleProbe Tests/DentalResampleProbe.cpp Inference/Volume.cpp)
endif()
if(SEGMENTATION_BUILD_TESTS AND SEGMENTATION_BUILD_DENTAL_INFERENCE)
    add_executable(DentalJobTest Tests/DentalJobTest.cpp
        UI/Adapters/DentalInferenceJob.cpp UI/Adapters/DentalInferenceJob.hpp
        Backend/Segmentation/LabelMetaData.cpp)
    target_link_libraries(DentalJobTest PRIVATE Qt6::Core Qt6::Gui ${ITK_LIBRARIES} ${VTK_LIBRARIES})
    target_include_directories(DentalJobTest PRIVATE ${Boost_INCLUDE_DIR})
    add_dependencies(DentalJobTest DentalInference)
endif()
