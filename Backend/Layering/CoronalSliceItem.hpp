#pragma once

#include "Backend/Layering/AbstractSliceItem.hpp"
#include "Backend/ThirdParty/QQuickVtkItem.h"
#include "Backend/SceneVtkItem.hpp"

class vtkRenderWindow;
class vtkImageReader2;

class SceneVtkItem;

namespace Dicom::Viewer::Slicer
{
    class CoronalSliceData;

    /**
     * QML coronal slice item
     *
     * <p>
     * Used to render the coronal slices
     * <p>
     *
     * @code
     * Vtk.CoronalSliceItem {
     *    id: coronalSliceItem
     *    width: 850
     *    height: 620
     * }
     * @endcode
     */
    class CoronalSliceItem : public AbstractSliceItem
    {
        Q_OBJECT
    public:
        /**
         * Ctor
         *
         * @param parent parent object
         */
        explicit CoronalSliceItem(QQuickVtkItem* parent = nullptr);

        /**
         * Scene creation and data binding
         *
         * @param  renderWindow graphical user interface where renderers draw their images
         * @return              the object associated with the VTK render window
         */
        vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override;
    };
}
