pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"
import "../styles"

Item {
    id: page
    required property var alarmModel
    readonly property AppTheme theme: AppTheme {}

    function levelColor(level) {
        return level === "Critical" ? page.theme.danger
             : level === "Warning" ? page.theme.warning : page.theme.primary
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: page.theme.spacingM
        spacing: page.theme.spacingS

        RowLayout {
            Layout.fillWidth: true
            Toolbar {
                Layout.fillWidth: true
                title: qsTr("Alarm center")
                subtitle: qsTr("Filter, confirm and inspect persisted alarm events")
                accentColor: page.theme.danger
            }
            Button { text: qsTr("Add test"); onClicked: page.alarmModel.addTestAlarm() }
            Button { text: qsTr("Reload"); onClicked: page.alarmModel.reload() }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: page.theme.spacingS
            CheckBox { id: deleteMediaBox; text: qsTr("Delete unreferenced snapshot") }
            TextField {
                Layout.fillWidth: true
                placeholderText: qsTr("Source name, key or id")
                onTextChanged: page.alarmModel.sourceFilter = text
            }
            ComboBox {
                model: [qsTr("All types"), "MotionDetected", "IntrusionDetected", "PersonDetected", "VehicleDetected", "ManualTest"]
                onActivated: page.alarmModel.eventTypeFilter = currentIndex === 0 ? "" : currentText
            }
            ComboBox {
                model: [qsTr("All states"), qsTr("Unconfirmed"), qsTr("Confirmed")]
                onActivated: page.alarmModel.confirmedFilter = currentIndex === 0 ? -1 : currentIndex - 1
            }
            Button {
                text: qsTr("Clear filters")
                onClicked: {
                    page.alarmModel.sourceFilter = ""
                    page.alarmModel.eventTypeFilter = ""
                    page.alarmModel.confirmedFilter = -1
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: page.alarmModel.lastError.length > 0
            text: page.alarmModel.lastError
            color: page.theme.danger
            wrapMode: Text.Wrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: page.theme.surface
            border.color: page.theme.border
            radius: page.theme.radiusM

            ListView {
                id: alarmList
                anchors.fill: parent
                anchors.margins: page.theme.spacingS
                clip: true
                spacing: page.theme.spacingS
                model: page.alarmModel

                delegate: Rectangle {
                    id: row
                    required property int index
                    required property int alarmId
                    required property int sourceId
                    required property string sourceName
                    required property string eventType
                    required property string level
                    required property string message
                    required property string snapshotPath
                    required property bool confirmed
                    required property string timestamp
                    width: alarmList.width
                    height: 116
                    color: row.confirmed ? page.theme.surfaceMuted : "#fff7ed"
                    border.color: row.confirmed ? page.theme.border : page.levelColor(row.level)
                    radius: page.theme.radiusS

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: page.theme.spacingS
                        spacing: page.theme.spacingS
                        Rectangle {
                            Layout.preferredWidth: 4
                            Layout.fillHeight: true
                            color: page.levelColor(row.level)
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("%1 | %2 | %3").arg(row.eventType).arg(row.sourceName).arg(row.timestamp)
                                color: page.theme.textPrimary
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: row.message
                                color: page.theme.textSecondary
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: row.snapshotPath.length > 0 ? qsTr("Snapshot: %1").arg(row.snapshotPath) : qsTr("No snapshot")
                                color: page.theme.textSecondary
                                font.pixelSize: page.theme.fontSmall
                                elide: Text.ElideMiddle
                            }
                        }
                        ColumnLayout {
                            Button {
                                text: row.confirmed ? qsTr("Confirmed") : qsTr("Confirm")
                                enabled: !row.confirmed
                                onClicked: page.alarmModel.confirmAlarm(row.index)
                            }
                            Button {
                                text: qsTr("Delete")
                                onClicked: page.alarmModel.removeAlarmWithMedia(row.index, deleteMediaBox.checked)
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: page.alarmModel.count === 0
                    text: qsTr("No matching alarms")
                    color: page.theme.textSecondary
                }
            }
        }
    }
}
