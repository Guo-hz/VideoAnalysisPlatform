pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"
import "../styles"

Item {
    id: page
    required property var appPaths
    required property var settingsController
    readonly property AppTheme theme: AppTheme {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: page.theme.spacingM
        spacing: page.theme.spacingM

        Toolbar {
            Layout.fillWidth: true
            title: qsTr("Settings")
            subtitle: qsTr("Storage, disk warning and model availability")
            accentColor: page.theme.textSecondary
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: page.theme.spacingM

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: page.theme.surface
                border.color: page.theme.border
                radius: page.theme.radiusM
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: page.theme.spacingM
                    spacing: page.theme.spacingS

                    Text { text: qsTr("Managed directories"); color: page.theme.textPrimary; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("Database: %1").arg(page.appPaths.databasePath); color: page.theme.textSecondary; elide: Text.ElideMiddle }
                        Button { text: qsTr("Open"); onClicked: page.settingsController.openDirectory(page.appPaths.databasePath) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("Snapshots: %1").arg(page.appPaths.snapshotDirectory); color: page.theme.textSecondary; elide: Text.ElideMiddle }
                        Button { text: qsTr("Open"); onClicked: page.settingsController.openDirectory(page.appPaths.snapshotDirectory) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("Recordings: %1").arg(page.appPaths.recordingDirectory); color: page.theme.textSecondary; elide: Text.ElideMiddle }
                        Button { text: qsTr("Open"); onClicked: page.settingsController.openDirectory(page.appPaths.recordingDirectory) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("Logs: %1").arg(page.appPaths.logDirectory); color: page.theme.textSecondary; elide: Text.ElideMiddle }
                        Button { text: qsTr("Open"); onClicked: page.settingsController.openDirectory(page.appPaths.logDirectory) }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                color: page.theme.surface
                border.color: page.theme.border
                radius: page.theme.radiusM
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: page.theme.spacingM
                    spacing: page.theme.spacingM
                    Text { text: qsTr("Runtime controls"); color: page.theme.textPrimary; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("Disk warning threshold (%)"); color: page.theme.textSecondary; wrapMode: Text.Wrap }
                        SpinBox {
                            from: 1
                            to: 50
                            value: page.settingsController.diskWarningThresholdPercent
                            onValueModified: page.settingsController.diskWarningThresholdPercent = value
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("YOLOX model: %1").arg(page.settingsController.isModelPresent("object_detection_yolox_2022nov.onnx") ? qsTr("available") : qsTr("not installed"))
                        color: page.theme.textSecondary
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("The app warns on low disk space and stops only the affected snapshot or recording request.")
                        color: page.theme.textSecondary
                        wrapMode: Text.Wrap
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
