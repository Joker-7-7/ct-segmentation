#include "Backend/SceneVtkItem.hpp"
#include "Backend/Layering/AxialSliceItem.hpp"

#include "Backend/Layering/CoronalSliceItem.hpp"
#include "Backend/Layering/SagittalSliceItem.hpp"

#include "UI/Adapters/SegmentationAdapter.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <QVTKRenderWindowAdapter.h>

extern "C"
{
    _declspec(dllexport) int NvOptimusEnablement = 1;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

using namespace Dicom::Viewer::Slicer;

int main(int argc, char* argv[])
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    QSurfaceFormat::setDefaultFormat(QVTKRenderWindowAdapter::defaultFormat());

    QGuiApplication app(argc, argv);

    qmlRegisterType<SceneVtkItem>("Vtk", 1, 0, "SceneVtkItem");
    qmlRegisterType<AxialSliceItem>("Vtk", 1, 0, "AxialSliceItem");
    qmlRegisterType<CoronalSliceItem>("Vtk", 1, 0, "CoronalSliceItem");
    qmlRegisterType<SagittalSliceItem>("Vtk", 1, 0, "SagittalSliceItem");

    qmlRegisterUncreatableType<LabelListModel>(
        "Vtk", 1, 0, "LabelListModel", "Owned by SegmentationAdapter"
    );
    qmlRegisterType<UI::Adapters::SegmentationAdapter>("Vtk", 1, 0, "SegmentationAdapter");

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/Gui/Main.qml"_qs);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (obj == nullptr && url == objUrl)
            {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );
    engine.load(url);

    return app.exec();
}
