import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Controls
import QtQuick.Layouts
import Vtk 1.0 as Vtk
import "qrc:/Gui" as App

Controls.ToolBar {
    id: root
    required property Vtk.SegmentationAdapter segmentation
    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        ButtonGroup {
            id: tools
        }
        Repeater {
            model: [qsTr("Navigate"), qsTr("Brush"), qsTr("Eraser")]
            App.GrayButton {
                required property int index
                required property string modelData
                text: modelData
                checkable: true
                ButtonGroup.group: tools
                checked: root.segmentation.tool === index
                enabled: root.segmentation.ready && !root.segmentation.busy && (index === 0 || root.segmentation.labels.activeLabel !== 0)
                onClicked: root.segmentation.tool = index
            }
        }
        ToolSeparator {}
        Label {
            text: qsTr("Shape")
        }
        ComboBox {
            model: [qsTr("Circle"), qsTr("Square"), qsTr("Adaptive 2D"), qsTr("Sphere"), qsTr("Cube"), qsTr("Adaptive 3D")]
            currentIndex: root.segmentation.brushShape
            enabled: !root.segmentation.busy
            onActivated: root.segmentation.brushShape = currentIndex
            Layout.preferredWidth: 145
        }
        Label {
            text: qsTr("Radius (voxels)")
        }
        SpinBox {
            from: 1
            to: 32
            value: root.segmentation.brushRadius
            enabled: !root.segmentation.busy
            onValueModified: root.segmentation.brushRadius = value
        }
        Item {
            Layout.fillWidth: true
        }
    }
}
