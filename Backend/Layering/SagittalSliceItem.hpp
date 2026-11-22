#pragma once

#include "Backend/ThirdParty/QQuickVtkItem.h"
#include "Backend/SceneVtkItem.hpp"
#include "Backend/Layering/AbstractSliceItem.hpp"

class vtkRenderWindow;
class vtkImageReader2;

namespace Dicom::Viewer::Slicer
{
    class SagittalSliceData;

    /**
     * QML sagittal slice item
     *
     * <p>
     * Used to render the sagittal slices
     * <p>
     *
     * @code
     * Vtk.SagittalSliceItem {
     *    id: sagittalSliceItem
     *    width: 850
     *    height: 620
     * }
     * @endcode
     */
    class SagittalSliceItem : public AbstractSliceItem
    {
        Q_OBJECT
    public:
        /**
         * Ctor
         *
         * @param parent parent object
         */
        explicit SagittalSliceItem(QQuickVtkItem* parent = nullptr);

        /**
         * Scene creation and data binding
         *
         * @param  renderWindow graphical user interface where renderers draw their images
         * @return              the object associated with the VTK render window
         */
        vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override;
    };
}
