import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../styles"

Button {
    id: root

    property bool selected: false

    readonly property AppTheme theme: AppTheme {}

    implicitHeight: 44
    leftPadding: theme.spacingM
    rightPadding: theme.spacingM

    background: Rectangle {
        radius: root.theme.radiusM
        color: root.selected ? root.theme.navSelected : (root.hovered ? "#303c49" : "transparent")

        Rectangle {
            width: 3
            height: parent.height - root.theme.spacingM
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            radius: 2
            color: root.selected ? root.theme.primary : "transparent"
        }
    }

    contentItem: RowLayout {
        spacing: root.theme.spacingS

        Text {
            Layout.fillWidth: true
            text: root.text
            color: root.selected ? root.theme.navTitle : root.theme.navText
            font.pixelSize: root.theme.fontBody
            font.bold: root.selected
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}
