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
    required property var sourceModel
    readonly property AppTheme theme: AppTheme {}
    property int layoutMode: 4
    property int focusedChannel: 0
    property bool roiEditing: false
    readonly property var focusedSession: page.streamSessionManager.sessionAt(page.focusedChannel)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: page.theme.spacingM
        spacing: page.theme.spacingM

        RowLayout {
            Layout.fillWidth: true
            spacing: page.theme.spacingS

            Toolbar {
                Layout.fillWidth: true
                title: qsTr("Video wall")
                subtitle: qsTr("Four persistent channels with source binding")
                accentColor: page.theme.success
            }

            Button {
                text: qsTr("1 tile")
                checkable: true
                checked: page.layoutMode === 1
                onClicked: page.layoutMode = 1
            }
            Button {
                text: qsTr("4 tiles")
                checkable: true
                checked: page.layoutMode === 4
                onClicked: page.layoutMode = 4
            }
            Button {
                text: qsTr("Stop all")
                onClicked: page.streamSessionManager.stopAll()
            }
            Button {
                text: qsTr("Stop recordings")
                onClicked: page.streamSessionManager.stopAllRecordings()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: page.theme.spacingM

            GridLayout {
                id: tileGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: page.layoutMode === 1 ? 1 : 2
                rows: page.layoutMode === 1 ? 1 : 2
                rowSpacing: page.theme.spacingS
                columnSpacing: page.theme.spacingS

                Repeater {
                    model: page.streamSessionManager.count
                    VideoTile {
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 0
                        Layout.minimumHeight: 0
                        visible: page.layoutMode === 4 || index === page.focusedChannel
                        streamSession: page.streamSessionManager.sessionAt(index)
                        title: qsTr("Channel %1").arg(index + 1)
                        compact: page.layoutMode === 4
                        roiEditing: page.layoutMode === 1 && index === page.focusedChannel && page.roiEditing
                        TapHandler {
                            onTapped: page.focusedChannel = index
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.preferredWidth: 300
                Layout.minimumWidth: 250
                Layout.fillHeight: true
                spacing: page.theme.spacingS

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 184
                    color: page.theme.surface
                    border.color: page.theme.border
                    radius: page.theme.radiusM

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: page.theme.spacingS
                        spacing: page.theme.spacingS

                        Text {
                            text: qsTr("Focused channel %1").arg(page.focusedChannel + 1)
                            color: page.theme.textPrimary
                            font.bold: true
                        }

                        ComboBox {
                            id: sourcePicker
                            Layout.fillWidth: true
                            model: page.sourceModel
                            textRole: "name"
                            valueRole: "sourceId"
                            enabled: page.sourceModel.count > 0
                        }

                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Bind and play")
                            enabled: sourcePicker.currentIndex >= 0
                            onClicked: {
                                if (page.streamSessionManager.bindSource(page.focusedChannel, sourcePicker.currentValue)) {
                                    page.focusedSession.play()
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            CheckBox {
                                text: qsTr("Motion")
                                checked: page.focusedSession ? page.focusedSession.motionDetectionEnabled : false
                                onToggled: if (page.focusedSession) page.focusedSession.motionDetectionEnabled = checked
                            }
                            CheckBox {
                                text: qsTr("AI")
                                checked: page.focusedSession ? page.focusedSession.aiDetectionEnabled : false
                                onToggled: if (page.focusedSession) page.focusedSession.aiDetectionEnabled = checked
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 126
                    color: page.theme.surface
                    border.color: page.theme.border
                    radius: page.theme.radiusM

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: page.theme.spacingS
                        spacing: page.theme.spacingXs

                        Text {
                            text: qsTr("Analysis and ROI")
                            color: page.theme.textPrimary
                            font.bold: true
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: qsTr("Min area"); color: page.theme.textSecondary }
                            SpinBox {
                                Layout.fillWidth: true
                                from: 1
                                to: 20000
                                value: page.focusedSession ? page.focusedSession.minTargetArea : 500
                                onValueModified: if (page.focusedSession) page.focusedSession.minTargetArea = value
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Draw ROI")
                                checkable: true
                                checked: page.roiEditing
                                enabled: page.layoutMode === 1 && !!page.focusedSession
                                onToggled: page.roiEditing = checked
                            }
                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Clear ROI")
                                enabled: page.focusedSession && page.focusedSession.roiRules.length > 0
                                onClicked: if (page.focusedSession) page.focusedSession.clearRoiRules()
                            }
                        }
                    }
                }

                PerformancePanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    performanceModel: page.performanceModel
                }
            }
        }
    }
}
