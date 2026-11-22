#include "LayersConfiguration.hpp"

namespace LayersConfiguration
{
    struct ColorFunctions final
    {
    public:
        vtkColorTransferFunction* color;
        vtkPiecewiseFunction* opacity;
        vtkPiecewiseFunction* gradient;

        ColorFunctions(vtkVolumeProperty* properties) noexcept :
            color{ properties->GetRGBTransferFunction() },
            opacity{ properties->GetScalarOpacity() },
            gradient{ properties->GetGradientOpacity() }
        {
            Reset();
        }

    private:
        void Reset() noexcept
        {
            color->RemoveAllPoints();
            opacity->RemoveAllPoints();
            gradient->RemoveAllPoints();
            gradient->AddPoint(0, 0.0);
            gradient->AddPoint(90, 0.5);
            gradient->AddPoint(100, 1.0);
        }
    };

    void SetColorAndOpacityFunction(vtkVolumeProperty* volumeProperty, double wl, double ww)
    {
        vtkColorTransferFunction* colorFunction = volumeProperty->GetRGBTransferFunction();
        colorFunction->RemoveAllPoints();
        vtkPiecewiseFunction* opacityFunction = volumeProperty->GetScalarOpacity();
        opacityFunction->RemoveAllPoints();

        colorFunction->SetColorSpaceToRGB();
        colorFunction->AddRGBPoint(wl - ww / 2.0, 0.0, 0.0, 0.0);
        colorFunction->AddRGBPoint(
            wl - ww / 2.0 + 94.0 * (ww / 255.0), 1.0, 21.0 / 255.0, 27.0 / 255.0
        );
        colorFunction->AddRGBPoint(
            wl - ww / 2.0 + 147.0 * (ww / 255.0), 1.0, 176.0 / 255.0, 9.0 / 255.0
        );
        colorFunction->AddRGBPoint(
            wl - ww / 2.0 + 201.0 * (ww / 255.0), 1.0, 241.0 / 255.0, 39.0 / 255.0
        );
        colorFunction->AddRGBPoint(wl - ww / 2.0 + 255.0 * (ww / 255.0), 1.0, 1.0, 1.0);

        opacityFunction->AddPoint(wl - ww / 2.0, 0.0);
        opacityFunction->AddPoint(wl + ww / 2.0, 1.0);
    }

    void SetMPRXraySettings(vtkVolumeProperty* properties, double wl = 1745, double ww = 1650)
    {
        auto [color, opacity, gradient] = ColorFunctions{ properties };

        color->AddRGBSegment(0.0, 1.0, 1.0, 1.0, 255.0, 1.0, 1.0, 1.0);
        opacity->AddSegment(wl - 0.5 * ww, 0.0, wl + 0.5 * ww, 1.0);

        properties->SetAmbient(0.9);
        properties->SetDiffuse(0.9);
        properties->SetSpecular(0.1);
        properties->SetSpecularPower(1);
    }

    void SetModelXraySettings(vtkVolumeProperty* properties, double wl = 1745, double ww = 1650)
    {
        auto [color, opacity, gradient] = ColorFunctions{ properties };

        color->AddRGBPoint(0.0, 0.1, 0.1, 0.1);
        color->AddRGBPoint(800, 0.6, 0.6, 0.6);
        color->AddRGBPoint(900, 0.65, 0.65, 0.65);
        color->AddRGBPoint(1000, 0.7, 0.7, 0.7);
        color->AddRGBPoint(1200, 0.75, 0.75, 0.75);
        color->AddRGBPoint(2000, 0.8, 0.8, 0.8);

        opacity->AddPoint(wl, 0.0);
        opacity->AddPoint(ww, 1.0);

        properties->SetAmbient(0.9);
        properties->SetDiffuse(0.9);
        properties->SetSpecular(0.1);
        properties->SetSpecularPower(1);
    }
}
