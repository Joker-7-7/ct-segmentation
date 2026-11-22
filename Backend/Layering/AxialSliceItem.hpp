#pragma once

#include "Backend/ThirdParty/QQuickVtkItem.h"
#include "Backend/SceneVtkItem.hpp"
#include "AbstractSliceItem.hpp"

class vtkRenderWindow;
class vtkImageReader2;

namespace Dicom::Viewer::Slicer
{
    class AxialSliceData;

    /**
     * QML axial slice item
     *
     * <p>
     * Used to render the axial slices
     * <p>
     *
     * @code
     * Vtk.AxialSliceItem {
     *    id: axialSliceItem
     *    width: 850
     *    height: 620
     * }
     * @endcode
     */
    class AxialSliceItem : public AbstractSliceItem
    {
        Q_OBJECT
    public:
        Q_INVOKABLE void AutoSpline();

        /**
         * Ctor
         *
         * @param parent parent object
         */
        explicit AxialSliceItem(QQuickVtkItem* parent = nullptr);

        /**
         * Scene creation and data binding
         *
         * @param  renderWindow graphical user interface where renderers draw their images
         * @return              the object associated with the VTK render window
         */
        vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override;

        Q_INVOKABLE void ChangeInteractorStyle();

        /**
         * Reset MPR spline button 
         */
        Q_INVOKABLE void OnResetSplineClicked();

        /**
         * Panoramic view update button click
         */
        Q_INVOKABLE void OnPanoramicViewUpdateClicked();
    };
}
