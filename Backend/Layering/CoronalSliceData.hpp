#pragma once

#include "AbstractSliceData.hpp"

namespace Dicom::Viewer::Slicer
{
    /**
     *  VTK data for coronal (XZ) slice
     *
     *  <p>
     *  Used for layer-by-layer viewing of a DICOM file along the Y-axis
     *  <p>
     *
     * @code
     * auto renderWindow = vtkNew<vtkRenderWindow>();
     * auto slice = vtkNew<CoronalSliceData>();
     * slice->Init(renderWindow);
     * @endcode
     */
    class CoronalSliceData final : public AbstractSliceData
    {
    public:
        /**
         * Ctor
         */
        CoronalSliceData();

        /**
         * Standard VTK macro
         */
        vtkTypeMacro(CoronalSliceData, vtkObject);

        /**
         * VTK standard new method
         *
         * @return pointer to created object
         */
        static CoronalSliceData* New();

        void SetupCamera() noexcept override;

        void SliceFromDicom() override;
    };
}
