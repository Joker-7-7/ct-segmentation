import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vtk 1.0 as Vtk
import "qrc:/Gui" as App

Pane {
    id: root
    required property Vtk.SegmentationAdapter segmentation
    property color selectedColor: "#FD0007"
    enabled: segmentation.ready && !segmentation.busy
    ColumnLayout {
        anchors.fill: parent
        Label {
            text: qsTr("Labels")
            font.pixelSize: 20
            font.bold: true
        }
        Label {
            text: qsTr("Choose a color, then add a label.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        GridLayout {
            columns: 10
            rowSpacing: 3
            columnSpacing: 3
            Repeater {
                model: root.segmentation.labels.palette
                Rectangle {
                    required property color modelData
                    width: 22
                    height: 22
                    color: modelData
                    border.width: root.selectedColor === modelData ? 3 : 1
                    border.color: root.selectedColor === modelData ? "white" : "#555555"
                    Text {
                        anchors.centerIn: parent
                        text: "•"
                        color: parent.modelData.hslLightness > 0.5 ? "black" : "white"
                        visible: {
                            // Reevaluate used marks on structural model changes.
                            const unused = marks.revision;
                            return root.segmentation.labels.containsColor(parent.modelData);
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.selectedColor = parent.modelData
                    }
                }
            }
        }
        App.GrayButton {
            text: qsTr("Add / select label")
            Layout.fillWidth: true
            onClicked: root.segmentation.labels.add(root.selectedColor)
        }
        TextField {
            id: search
            placeholderText: qsTr("Search labels")
            Layout.fillWidth: true
        }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: root.segmentation.labels
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                id: row
                required property string caption
                required property color labelColor
                required property int colorIndex
                required property bool labelVisible
                readonly property bool matches: caption.toLowerCase().indexOf(search.text.toLowerCase()) >= 0
                width: list.width
                height: matches ? 78 : 0
                visible: matches
                radius: 4
                color: root.segmentation.labels.activeLabel === colorIndex ? "#42576e" : "#333638"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    RowLayout {
                        Rectangle {
                            width: 18
                            height: 18
                            color: row.labelColor
                        }
                        TextField {
                            text: row.caption
                            Layout.fillWidth: true
                            selectByMouse: true
                            onEditingFinished: root.segmentation.labels.rename(row.colorIndex, text)
                        }
                    }
                    RowLayout {
                        CheckBox {
                            text: qsTr("Visible")
                            checked: row.labelVisible
                            onClicked: root.segmentation.labels.setVisible(row.colorIndex, checked)
                        }
                        App.GrayButton {
                            text: qsTr("Select")
                            onClicked: root.segmentation.labels.select(row.colorIndex)
                        }
                        App.GrayButton {
                            text: "×"
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Delete label and its voxels")
                            onClicked: {
                                deleteDialog.labelId = row.colorIndex;
                                deleteDialog.open();
                            }
                        }
                    }
                }
            }
        }
        Label {
            text: qsTr("Brush overwrites other labels. Eraser removes only the selected label. Hidden labels cannot be painted.")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.7
        }
    }
    QtObject {
        id: marks
        property int revision: 0
    }
    Connections {
        target: root.segmentation.labels
        function onLabelsChanged() {
            ++marks.revision;
        }
    }
    Dialog {
        id: deleteDialog
        property int labelId: 0
        anchors.centerIn: parent
        title: qsTr("Delete this label and its voxels?")
        modal: true
        footer: RowLayout {
            App.GrayButton {
                text: qsTr("Cancel")
                onClicked: deleteDialog.reject()
            }
            App.GrayButton {
                text: qsTr("Delete")
                onClicked: deleteDialog.accept()
            }
        }
        onAccepted: root.segmentation.labels.remove(labelId)
    }
}
