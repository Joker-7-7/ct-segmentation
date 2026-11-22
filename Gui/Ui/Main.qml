import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Vtk 1.0 as Vtk
import "qrc:/Gui" as App

ApplicationWindow {
    id: root
    width: 1400
    height: 900
    minimumWidth: 1120
    minimumHeight: 700
    visible: true
    title: qsTr("Segmentation")
    color: "#202225"
    palette.window: "#272a2e"
    palette.windowText: "white"
    palette.base: "#35383d"
    palette.text: "white"
    palette.button: "#41464c"
    palette.buttonText: "white"
    property var dimensions: [1, 1, 1]
    header: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 6
            App.GrayButton {
                text: qsTr("Open DICOM folder")
                enabled: !adapter.busy
                onClicked: folder.open()
            }
            App.GrayButton {
                text: qsTr("Open DICOM file")
                enabled: !adapter.busy
                onClicked: file.open()
            }
            Label {
                text: adapter.ready ? qsTr("Left mouse: paint · Wheel: brush radius") : qsTr("Open a DICOM volume to begin")
            }
            Item {
                Layout.fillWidth: true
            }
            BusyIndicator {
                running: adapter.busy
                visible: running
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 6
            ButtonGroup {
                id: dentalDevices
            }
            App.GrayButton {
                id: cpuDevice
                text: "CPU"
                checkable: true
                checked: true
                ButtonGroup.group: dentalDevices
                enabled: !adapter.busy
            }
            App.GrayButton {
                id: gpuDevice
                text: "RTX (CUDA)"
                checkable: true
                ButtonGroup.group: dentalDevices
                enabled: !adapter.busy
            }
            App.GrayButton {
                text: qsTr("Segment whole CT")
                enabled: adapter.ready && !adapter.busy
                onClicked: replaceDialog.open()
            }
            App.GrayButton {
                text: qsTr("Cancel")
                visible: adapter.dentalRunning
                onClicked: adapter.CancelDentalSegmentation()
            }
            Label {
                text: adapter.dentalStatus
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
        App.ToolBar {
            Layout.fillWidth: true
            segmentation: adapter
        }
    }
    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            rows: 2
            uniformCellWidths: true
            uniformCellHeights: true
            columnSpacing: 8
            rowSpacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Label {
                    text: qsTr("Axial")
                }
                Vtk.AxialSliceItem {
                    id: axial
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    mainScene: scene
                }
                Slider {
                    id: axialSlider
                    Layout.fillWidth: true
                    from: 0
                    to: root.dimensions[2] - 1
                    stepSize: 1
                    enabled: adapter.ready && !adapter.busy
                    onValueChanged: if (adapter.ready)
                        axial.OnSliderChanged(Math.round(value))
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Label {
                    text: qsTr("Coronal")
                }
                Vtk.CoronalSliceItem {
                    id: coronal
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    mainScene: scene
                }
                Slider {
                    id: coronalSlider
                    Layout.fillWidth: true
                    from: 0
                    to: root.dimensions[1] - 1
                    stepSize: 1
                    enabled: adapter.ready && !adapter.busy
                    onValueChanged: if (adapter.ready)
                        coronal.OnSliderChanged(Math.round(value))
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Label {
                    text: qsTr("Sagittal")
                }
                Vtk.SagittalSliceItem {
                    id: sagittal
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    mainScene: scene
                }
                Slider {
                    id: sagittalSlider
                    Layout.fillWidth: true
                    from: 0
                    to: root.dimensions[0] - 1
                    stepSize: 1
                    enabled: adapter.ready && !adapter.busy
                    onValueChanged: if (adapter.ready)
                        sagittal.OnSliderChanged(Math.round(value))
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Label {
                    text: qsTr("3D labels")
                }
                Vtk.SceneVtkItem {
                    id: scene
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    axialScene: axial
                }
                Item {
                    Layout.preferredHeight: axialSlider.implicitHeight
                }
            }
        }
        App.SegmentationsBrowser {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            segmentation: adapter
        }
    }
    Vtk.SegmentationAdapter {
        id: adapter
        axialSlice: axial
        coronalSlice: coronal
        sagittalSlice: sagittal
        mainScene: scene
        onVolumeOpened: function (size) {
            root.dimensions = size;
            axialSlider.value = Math.floor(size[2] / 2);
            coronalSlider.value = Math.floor(size[1] / 2);
            sagittalSlider.value = Math.floor(size[0] / 2);
        }
        onErrorOccurred: function (message) {
            errorDialog.text = message;
            errorDialog.open();
        }
    }
    FolderDialog {
        id: folder
        onAccepted: adapter.OpenDicom(selectedFolder.toString(), true)
    }
    FileDialog {
        id: file
        nameFilters: [qsTr("DICOM files (*.dcm)"), qsTr("All files (*)")]
        onAccepted: adapter.OpenDicom(selectedFile.toString(), false)
    }
    MessageDialog {
        id: replaceDialog
        title: qsTr("Dental segmentation")
        text: qsTr("Segment the whole CT on %1? A successful result replaces the current labels and mask. Cancellation or failure preserves them.").arg(gpuDevice.checked ? gpuDevice.text : cpuDevice.text)
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: adapter.RunDentalSegmentation(gpuDevice.checked ? "cuda" : "cpu")
    }
    MessageDialog {
        id: errorDialog
        title: qsTr("Segmentation")
    }
}
