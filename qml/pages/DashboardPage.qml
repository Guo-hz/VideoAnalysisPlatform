pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"
import "../styles"

Item {
    id: page
    required property var streamSessionManager
    required property var performanceModel
    required property var alarmModel
    required property var recordingModel
    readonly property AppTheme theme: AppTheme {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: page.theme.spacingM
        spacing: page.theme.spacingM

        Toolbar {
            Layout.fillWidth: true
            title: qsTr("Dashboard")
            subtitle: qsTr("Live status, active alarms and recording index")
            accentColor: page.theme.primary
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: page.theme.spacingS
            Repeater {
                model: [
                    { title: qsTr("Channels"), value: page.streamSessionManager.count },
                    { title: qsTr("Alarms"), value: page.alarmModel.count },
                    { title: qsTr("Recordings"), value: page.recordingModel.count }
                ]
                Rectangle {
                    id: card
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 76
                    color: page.theme.surface
                    border.color: page.theme.border
                    radius: page.theme.radiusM
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: page.theme.spacingM
                        Text { text: card.modelData.title; color: page.theme.textSecondary }
                        Text { text: card.modelData.value; color: page.theme.textPrimary; font.pixelSize: page.theme.fontTitle; font.bold: true }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: page.theme.surface
            border.color: page.theme.border
            radius: page.theme.radiusM
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: page.theme.spacingM
                Text { text: qsTr("Channel status"); color: page.theme.textPrimary; font.bold: true }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: page.streamSessionManager.count
                    spacing: page.theme.spacingS
                    delegate: Rectangle {
                        id: statusRow
                        required property int index
                        width: ListView.view.width
                        height: 54
                        color: page.theme.surfaceMuted
                        radius: page.theme.radiusS
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: page.theme.spacingS
                            Text { text: qsTr("Channel %1").arg(statusRow.index + 1); color: page.theme.textPrimary; font.bold: true }
                            Text { Layout.fillWidth: true; text: page.streamSessionManager.sessionAt(statusRow.index).sourceName; color: page.theme.textSecondary; elide: Text.ElideMiddle }
                            Text { text: page.streamSessionManager.sessionAt(statusRow.index).status; color: page.theme.textPrimary }
                            Text { text: qsTr("FPS %1").arg(page.streamSessionManager.sessionAt(statusRow.index).fps.toFixed(1)); color: page.theme.textSecondary }
                        }
                    }
                }
            }
        }
    }
}
