#pragma once

#include "AbstractSliceData.hpp"

namespace Dicom::Viewer::Slicer
{
    /**
     *  VTK data for sagittal (YZ) slice
     *
     *  <p>
     *  Used for layer-by-layer viewing of a DICOM file along the X-axis
     *  <p>
     *
     * @code
     * auto renderWindow = vtkNew<vtkRenderWindow>();
     * auto slice = vtkNew<SagittalSliceData>();
     * slice->Init(renderWindow);
     * @endcode
     */
    class SagittalSliceData final : public AbstractSliceData
    {
    public:
        /**
         * Ctor
         */
        SagittalSliceData();

        /**
         * Standard VTK macro
         */
        vtkTypeMacro(SagittalSliceData, vtkObject);

        /**
         * VTK standard new method
         *
         * @return pointer to created object
         */
        static SagittalSliceData* New();

        void SetupCamera() noexcept override;

        void SliceFromDicom() override;
    };
}
