pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import "../styles"

Rectangle {
    id: root

    required property var streamSession
    property string title: qsTr("Channel")
    property bool compact: false
    property bool roiEditing: false
    property bool useRenderItem: true
    property string snapshotMessage: ""
    property bool snapshotSucceeded: true
    property string recordingMessage: ""
    property bool recordingSucceeded: true
    readonly property AppTheme theme: AppTheme {}

    color: "#111827"
    radius: theme.radiusM
    border.color: theme.border
    clip: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0b1220"

            VideoRenderItem {
                id: renderItem
                anchors.fill: parent
                anchors.margins: root.theme.spacingS
                streamSession: root.streamSession
                visible: root.useRenderItem && root.streamSession.frameSource.length > 0
            }

            Image {
                id: fallbackImage
                anchors.fill: parent
                anchors.margins: root.theme.spacingS
                source: root.streamSession.frameSource
                fillMode: Image.PreserveAspectFit
                cache: false
                visible: !root.useRenderItem && source.toString().length > 0
            }

            RoiEditor {
                anchors.fill: parent
                streamSession: root.streamSession
                videoItem: root.useRenderItem ? renderItem : fallbackImage
                editingEnabled: root.roiEditing
                showExistingRules: true
            }

            Text {
                anchors.centerIn: parent
                visible: root.streamSession.frameSource.length === 0
                text: qsTr("Select a source and press Play")
                color: "#c8d0da"
                font.pixelSize: root.compact ? root.theme.fontSmall : root.theme.fontBody
            }

            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: root.theme.spacingS
                text: root.title
                color: "#f8fafc"
                font.pixelSize: root.theme.fontSmall
                font.bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.compact ? 92 : 168
            color: root.theme.surface

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: root.theme.spacingS
                spacing: root.theme.spacingXs

                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.theme.spacingS

                    Text {
                        text: root.streamSession.status
                        color: root.streamSession.status === "Error" ? root.theme.danger : root.theme.textSecondary
                        font.pixelSize: root.theme.fontSmall
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.streamSession.sourceName.length > 0 ? root.streamSession.sourceName : root.title
                        color: root.theme.textPrimary
                        font.pixelSize: root.theme.fontSmall
                        elide: Text.ElideMiddle
                    }

                    Text {
                        text: qsTr("FPS %1").arg(root.streamSession.fps.toFixed(1))
                        color: root.theme.textSecondary
                        font.pixelSize: root.theme.fontSmall
                    }

                    Text {
                        visible: root.streamSession.recording
                        text: qsTr("REC")
                        color: root.theme.danger
                        font.pixelSize: root.theme.fontSmall
                        font.bold: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.theme.spacingXs

                    Button {
                        text: root.compact ? qsTr("File") : qsTr("Select file")
                        onClicked: fileDialog.open()
                    }
                    Button {
                        text: root.compact ? qsTr("Play") : qsTr("Play")
                        enabled: root.streamSession.status !== "Playing"
                        onClicked: root.streamSession.play()
                    }
                    Button {
                        text: root.compact ? qsTr("Pause") : qsTr("Pause")
                        enabled: root.streamSession.status === "Playing"
                        onClicked: root.streamSession.pause()
                    }
                    Button {
                        text: root.compact ? qsTr("Stop") : qsTr("Stop")
                        enabled: root.streamSession.status !== "Idle"
                        onClicked: root.streamSession.stop()
                    }
                    Button {
                        text: root.compact ? qsTr("Shot") : qsTr("Snapshot")
                        enabled: root.streamSession.frameSource.length > 0 && !root.streamSession.snapshotBusy
                        onClicked: {
                            root.snapshotSucceeded = true
                            root.snapshotMessage = qsTr("Saving snapshot...")
                            root.streamSession.takeSnapshot("jpg", "manual")
                        }
                    }
                    Button {
                        text: root.streamSession.recording ? qsTr("Stop REC") : qsTr("Record")
                        enabled: root.streamSession.recording || root.streamSession.status === "Playing"
                        onClicked: {
                            root.recordingSucceeded = true
                            root.recordingMessage = root.streamSession.recording
                                    ? qsTr("Stopping recording...")
                                    : qsTr("Starting recording...")
                            if (root.streamSession.recording) {
                                root.streamSession.stopRecording()
                            } else {
                                root.streamSession.startRecording()
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.streamSession.lastError.length > 0
                             || root.recordingMessage.length > 0
                             || root.snapshotMessage.length > 0
                    text: root.streamSession.lastError.length > 0
                          ? root.streamSession.lastError
                          : root.recordingMessage.length > 0 ? root.recordingMessage : root.snapshotMessage
                    color: root.streamSession.lastError.length > 0 || !root.recordingSucceeded || !root.snapshotSucceeded
                           ? root.theme.danger : root.theme.textSecondary
                    font.pixelSize: root.theme.fontSmall
                    elide: Text.ElideRight
                }

                RowLayout {
                    visible: !root.compact
                    Layout.fillWidth: true
                    spacing: root.theme.spacingS

                    TextField {
                        id: networkUrl
                        Layout.fillWidth: true
                        placeholderText: qsTr("rtsp:// or https:// direct media URL")
                        selectByMouse: true
                    }
                    Button {
                        text: qsTr("Open RTSP")
                        onClicked: {
                            if (root.streamSession.openRtspUrl(networkUrl.text)) {
                                root.streamSession.play()
                            }
                        }
                    }
                    Button {
                        text: qsTr("Open HTTP")
                        onClicked: {
                            if (root.streamSession.openHttpUrl(networkUrl.text)) {
                                root.streamSession.play()
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: root.streamSession
        function onSnapshotFinished(success, filePath, errorMessage) {
            root.snapshotSucceeded = success
            root.snapshotMessage = success ? qsTr("Snapshot: %1").arg(filePath) : errorMessage
        }
        function onRecordingResult(success, filePath, errorMessage) {
            root.recordingSucceeded = success
            root.recordingMessage = success ? qsTr("Recording: %1").arg(filePath) : errorMessage
        }
        function onFrameSourceChanged() {
            if (root.streamSession.frameSource.length === 0) {
                root.roiEditing = false
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Select local video")
        nameFilters: [qsTr("Video files (*.mp4 *.avi *.mkv)")]
        onAccepted: {
            if (root.streamSession.openLocalFile(selectedFile)) {
                root.streamSession.play()
            }
        }
    }
}
