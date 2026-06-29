import QtQuick
import QtQuick.Layouts

import "../styles"

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property color accentColor: theme.primary

    readonly property AppTheme theme: AppTheme {}

    implicitHeight: subtitle.length > 0 ? 78 : 60
    color: theme.surface
    border.color: theme.border
    radius: theme.radiusM

    RowLayout {
        anchors.fill: parent
        anchors.margins: root.theme.spacingM
        spacing: root.theme.spacingM

        Rectangle {
            Layout.preferredWidth: 4
            Layout.fillHeight: true
            radius: 2
            color: root.accentColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingXs

            Text {
                Layout.fillWidth: true
                text: root.title
                color: root.theme.textPrimary
                font.pixelSize: root.theme.fontTitle
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontBody
                elide: Text.ElideRight
            }
        }
    }
}
