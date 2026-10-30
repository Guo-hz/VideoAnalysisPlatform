pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

import "../components"
import "../styles"

Item {
    id: page
    required property var recordingModel
    readonly property AppTheme theme: AppTheme {}

    function localUrl(path) {
        return "file:///" + path.replace(/\\/g, "/")
    }

    MediaPlayer {
        id: player
        audioOutput: AudioOutput {}
        videoOutput: output
        onErrorOccurred: page.errorLabel.text = errorString
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: page.theme.spacingM
        spacing: page.theme.spacingM

        ColumnLayout {
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            Toolbar {
                Layout.fillWidth: true
                title: qsTr("Playback")
                subtitle: qsTr("Indexed AVI recordings")
                accentColor: page.theme.primary
            }
            RowLayout {
                Layout.fillWidth: true
                Button { text: qsTr("Reindex"); onClicked: page.recordingModel.reindex() }
                Button { text: qsTr("Reload"); onClicked: page.recordingModel.reload() }
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: page.theme.spacingS
                model: page.recordingModel
                delegate: Rectangle {
                    id: recordingRow
                    required property int index
                    required property string recordingId
                    required property string sourceName
                    required property string path
                    required property string startedAt
                    width: ListView.view.width
                    height: 70
                    color: player.source.toString() === page.localUrl(recordingRow.path) ? "#e8f1ff" : page.theme.surface
                    border.color: page.theme.border
                    radius: page.theme.radiusS
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: page.theme.spacingS
                        Text { Layout.fillWidth: true; text: recordingRow.sourceName; color: page.theme.textPrimary; font.bold: true; elide: Text.ElideRight }
                        Text { Layout.fillWidth: true; text: recordingRow.startedAt; color: page.theme.textSecondary; font.pixelSize: page.theme.fontSmall; elide: Text.ElideMiddle }
                        Text { Layout.fillWidth: true; text: recordingRow.path; color: page.theme.textSecondary; font.pixelSize: page.theme.fontSmall; elide: Text.ElideMiddle }
                    }
                    TapHandler {
                        onTapped: {
                            player.source = page.localUrl(recordingRow.path)
                            player.play()
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Toolbar {
                Layout.fillWidth: true
                title: qsTr("Selected recording")
                subtitle: qsTr("Play, pause and seek")
                accentColor: page.theme.success
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0b1220"
                VideoOutput { id: output; anchors.fill: parent; fillMode: VideoOutput.PreserveAspectFit }
                Label {
                    anchors.centerIn: parent
                    visible: player.source.toString().length === 0
                    text: qsTr("Select a recording")
                    color: "#c8d0da"
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Button { text: qsTr("Play"); onClicked: player.play() }
                Button { text: qsTr("Pause"); onClicked: player.pause() }
                Button { text: qsTr("Stop"); onClicked: player.stop() }
                Button { text: qsTr("-10s"); onClicked: player.position = Math.max(0, player.position - 10000) }
                Button { text: qsTr("+10s"); onClicked: player.position = Math.min(player.duration, player.position + 10000) }
                Text { id: errorLabel; Layout.fillWidth: true; text: player.errorString; color: page.theme.danger; elide: Text.ElideRight }
            }
            Slider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, player.duration)
                value: player.position
                onMoved: player.position = value
            }
        }
    }
}
