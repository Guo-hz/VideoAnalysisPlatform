pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../styles"

Rectangle {
    id: root

    required property var performanceModel
    readonly property AppTheme theme: AppTheme {}

    color: theme.surface
    border.color: theme.border
    radius: theme.radiusM

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.spacingM
        spacing: root.theme.spacingS

        Text {
            Layout.fillWidth: true
            text: qsTr("性能")
            color: root.theme.textPrimary
            font.pixelSize: root.theme.fontSubtitle
            font.bold: true
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingS

            Text {
                Layout.preferredWidth: 96
                text: qsTr("sourceName")
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontSmall
                elide: Text.ElideRight
            }

            Text {
                Layout.preferredWidth: 42
                text: qsTr("fps")
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontSmall
                horizontalAlignment: Text.AlignRight
            }

            Text {
                Layout.preferredWidth: 72
                text: qsTr("decodeTimeMs")
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontSmall
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }

            Text {
                Layout.preferredWidth: 42
                text: qsTr("queueSize")
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontSmall
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }

            Text {
                Layout.preferredWidth: 42
                text: qsTr("droppedFrames")
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontSmall
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("status")
                color: root.theme.textSecondary
                font.pixelSize: root.theme.fontSmall
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.border
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: root.theme.spacingS
            model: root.performanceModel

            delegate: Rectangle {
                id: row

                required property string sourceName
                required property real fps
                required property real decodeTimeMs
                required property int queueSize
                required property int droppedFrames
                required property string status

                width: ListView.view.width
                height: 32
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    spacing: root.theme.spacingS

                    Text {
                        Layout.preferredWidth: 96
                        text: row.sourceName
                        color: root.theme.textPrimary
                        font.pixelSize: root.theme.fontSmall
                        elide: Text.ElideMiddle
                    }

                    Text {
                        Layout.preferredWidth: 42
                        text: row.fps.toFixed(1)
                        color: root.theme.textPrimary
                        font.pixelSize: root.theme.fontSmall
                        horizontalAlignment: Text.AlignRight
                    }

                    Text {
                        Layout.preferredWidth: 72
                        text: row.decodeTimeMs.toFixed(1)
                        color: root.theme.textPrimary
                        font.pixelSize: root.theme.fontSmall
                        horizontalAlignment: Text.AlignRight
                    }

                    Text {
                        Layout.preferredWidth: 42
                        text: row.queueSize
                        color: root.theme.textPrimary
                        font.pixelSize: root.theme.fontSmall
                        horizontalAlignment: Text.AlignRight
                    }

                    Text {
                        Layout.preferredWidth: 42
                        text: row.droppedFrames
                        color: root.theme.textPrimary
                        font.pixelSize: root.theme.fontSmall
                        horizontalAlignment: Text.AlignRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: row.status
                        color: root.theme.textPrimary
                        font.pixelSize: root.theme.fontSmall
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
