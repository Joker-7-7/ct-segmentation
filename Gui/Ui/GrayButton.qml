import QtQuick
import QtQuick.Controls as Controls

Controls.Button {
    id: root
    implicitHeight: 34
    implicitWidth: Math.max(60, label.implicitWidth + 24)
    contentItem: Text {
        id: label
        text: root.text
        font: root.font
        color: root.enabled ? "#ffffff" : "#b8bdc2"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 4
        color: !root.enabled ? "#3b3e42" : root.down ? "#363f49" : root.checked ? "#53677a" : root.hovered ? "#5c6269" : "#4a4f55"
        border.color: root.activeFocus ? "#90b9dc" : "#697078"
        border.width: 1
    }
}
