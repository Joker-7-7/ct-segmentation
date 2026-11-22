#ifndef LAYERS_CONFIGURATION_HPP
#define LAYERS_CONFIGURATION_HPP

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolumeProperty.h>

namespace LayersConfiguration
{
    void SetColorAndOpacityFunction(vtkVolumeProperty* volumeProperty, double wl, double ww);
    void SetModelXraySettings(vtkVolumeProperty* properties, double wl, double ww);
    void SetMPRXraySettings(vtkVolumeProperty* properties, double wl, double ww);
}

#endif
