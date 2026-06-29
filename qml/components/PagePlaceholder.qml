pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../styles"

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    property color accentColor: theme.primary
    property var items: []

    readonly property AppTheme theme: AppTheme {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.spacingL
        spacing: root.theme.spacingL

        Toolbar {
            Layout.fillWidth: true
            title: root.title
            subtitle: root.subtitle
            accentColor: root.accentColor
        }

        GridLayout {
            Layout.fillWidth: true
            columns: root.width < 780 ? 1 : 2
            columnSpacing: root.theme.spacingM
            rowSpacing: root.theme.spacingM

            Repeater {
                model: root.items

                Rectangle {
                    id: card

                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    color: root.theme.surface
                    radius: root.theme.radiusM
                    border.color: root.theme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.theme.spacingM
                        spacing: root.theme.spacingS

                        Text {
                            Layout.fillWidth: true
                            text: card.modelData.title
                            color: root.theme.textPrimary
                            font.pixelSize: root.theme.fontSubtitle
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: card.modelData.body
                            color: root.theme.textSecondary
                            font.pixelSize: root.theme.fontBody
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
