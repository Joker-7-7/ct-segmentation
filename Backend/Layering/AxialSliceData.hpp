#pragma once

#include "AbstractSliceData.hpp"

namespace Dicom::Viewer::Panoramic
{
    class PanoramicViewData;
}

namespace Dicom::Viewer::Slicer
{
    /**
     *  VTK data for axial (XY) slice
     *
     *  <p>
     *  Used for layer-by-layer viewing of a DICOM file along the Z-axis
     *  <p>
     *
     * @code
     * auto renderWindow = vtkNew<vtkRenderWindow>();
     * auto slice = vtkNew<AxialSliceData>();
     * slice->InitAxialSliceData(renderWindow);
     * @endcode
     */
    class AxialSliceData final : public AbstractSliceData
    {
    public:
        /**
         * Reader for DICOM
         */
        //vtkSmartPointer<vtkImageReader2> reader;

        /**
         * Ctor
         */
        AxialSliceData();

        /**
         * Standard VTK macro
         */
        vtkTypeMacro(AxialSliceData, vtkObject);

        /**
         * VTK standard new method
         *
         * @return pointer to created object
         */
        static AxialSliceData* New();

        void SetupCamera() noexcept override;

        void SliceFromDicom() override;
    };
}
