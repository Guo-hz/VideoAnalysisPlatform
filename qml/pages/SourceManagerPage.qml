pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtMultimedia

import "../components"
import "../styles"

Item {
    id: page

    required property var sourceModel
    required property var streamSessionManager
    readonly property AppTheme theme: AppTheme {}
    property int selectedRow: -1
    property string testMessage: ""
    property bool testSucceeded: true

    function clearEditor() {
        page.selectedRow = -1
        nameField.text = ""
        endpointField.text = ""
        usernameField.text = ""
        passwordField.text = ""
        typePicker.currentIndex = 0
        enabledBox.checked = true
        page.testMessage = ""
    }

    function loadEditor(row) {
        const source = page.sourceModel.sourceAt(row)
        if (!source || source.sourceId === undefined) {
            clearEditor()
            return
        }
        page.selectedRow = row
        nameField.text = source.name
        endpointField.text = source.url
        usernameField.text = ""
        passwordField.text = ""
        typePicker.currentIndex = Math.max(0, typePicker.find(source.type))
        enabledBox.checked = source.enabled
        page.testMessage = qsTr("Credentials are never read back; leave fields empty to keep them.")
    }

    Connections {
        target: page.sourceModel
        function onSourceTestFinished(sourceId, success, message) {
            page.testSucceeded = success
            page.testMessage = message
        }
    }

    MediaDevices { id: mediaDevices }

    RowLayout {
        anchors.fill: parent
        anchors.margins: page.theme.spacingM
        spacing: page.theme.spacingM

        ColumnLayout {
            Layout.preferredWidth: 390
            Layout.minimumWidth: 320
            Layout.fillHeight: true
            spacing: page.theme.spacingS

            Toolbar {
                Layout.fillWidth: true
                title: qsTr("Video sources")
                subtitle: qsTr("LocalFile, Camera, RTSP and HTTP direct media")
                accentColor: page.theme.primary
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: page.theme.surface
                border.color: page.theme.border
                radius: page.theme.radiusM

                ListView {
                    id: sourceList
                    anchors.fill: parent
                    anchors.margins: page.theme.spacingS
                    clip: true
                    spacing: page.theme.spacingXs
                    model: page.sourceModel

                    delegate: Rectangle {
                        id: sourceRow
                        required property int index
                        required property int sourceId
                        required property string name
                        required property string type
                        required property string url
                        required property bool enabled
                        required property string status
                        required property string lastError

                        width: sourceList.width
                        height: 66
                        radius: page.theme.radiusS
                        color: page.selectedRow === index ? "#e8f1ff" : page.theme.surfaceMuted
                        border.color: page.selectedRow === index ? page.theme.primary : page.theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: page.theme.spacingS
                            spacing: page.theme.spacingS
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text {
                                    Layout.fillWidth: true
                                    text: sourceRow.name
                                    color: page.theme.textPrimary
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("%1 | %2 | %3").arg(sourceRow.type).arg(sourceRow.status).arg(sourceRow.enabled ? qsTr("Enabled") : qsTr("Disabled"))
                                    color: page.theme.textSecondary
                                    font.pixelSize: page.theme.fontSmall
                                    elide: Text.ElideMiddle
                                }
                            }
                            Button {
                                text: qsTr("Edit")
                                onClicked: page.loadEditor(sourceRow.index)
                            }
                        }
                        TapHandler { onTapped: page.loadEditor(sourceRow.index) }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: page.sourceModel.count === 0
                        text: qsTr("No sources yet")
                        color: page.theme.textSecondary
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: page.theme.spacingS

            Toolbar {
                Layout.fillWidth: true
                title: page.selectedRow >= 0 ? qsTr("Edit source") : qsTr("Add source")
                subtitle: qsTr("Network credentials are stored in Windows Credential Manager")
                accentColor: page.theme.success
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
                    spacing: page.theme.spacingS

                    Label { text: qsTr("Name") }
                    TextField { id: nameField; Layout.fillWidth: true; placeholderText: qsTr("Front gate") }

                    Label { text: qsTr("Type") }
                    ComboBox {
                        id: typePicker
                        Layout.fillWidth: true
                        model: ["LocalFile", "Camera", "Rtsp", "Http"]
                    }

                    Label { text: typePicker.currentText === "Camera" ? qsTr("Device index") : qsTr("Endpoint") }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: endpointField
                            Layout.fillWidth: true
                            placeholderText: typePicker.currentText === "Camera" ? qsTr("0") : qsTr("Path or direct URL")
                        }
                        ComboBox {
                            visible: typePicker.currentText === "Camera"
                            Layout.preferredWidth: 180
                            model: mediaDevices.videoInputs
                            textRole: "description"
                            onActivated: endpointField.text = String(currentIndex)
                        }
                        Button {
                            visible: typePicker.currentText === "LocalFile"
                            text: qsTr("Browse")
                            onClicked: fileDialog.open()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: typePicker.currentText === "Rtsp" || typePicker.currentText === "Http"
                        TextField { id: usernameField; Layout.fillWidth: true; placeholderText: qsTr("Username (optional)") }
                        TextField { id: passwordField; Layout.fillWidth: true; placeholderText: qsTr("Password (optional)"); echoMode: TextInput.Password }
                    }

                    CheckBox { id: enabledBox; text: qsTr("Enabled"); checked: true }

                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: page.selectedRow >= 0 ? qsTr("Save") : qsTr("Add")
                            onClicked: {
                                if (page.selectedRow >= 0) {
                                    if (page.sourceModel.updateSource(page.selectedRow, nameField.text, typePicker.currentText, endpointField.text, usernameField.text, passwordField.text)) {
                                        page.sourceModel.setSourceEnabled(page.selectedRow, enabledBox.checked)
                                    }
                                } else {
                                    const newRow = page.sourceModel.addSource(nameField.text, typePicker.currentText, endpointField.text, usernameField.text, passwordField.text)
                                    if (newRow >= 0) {
                                        if (!enabledBox.checked) page.sourceModel.setSourceEnabled(newRow, false)
                                        page.clearEditor()
                                    }
                                }
                            }
                        }
                        Button { text: qsTr("New"); onClicked: page.clearEditor() }
                        Button {
                            text: qsTr("Delete")
                            enabled: page.selectedRow >= 0
                            onClicked: {
                                if (page.sourceModel.removeSource(page.selectedRow)) page.clearEditor()
                            }
                        }
                        Button {
                            text: qsTr("Test")
                            enabled: page.selectedRow >= 0
                            onClicked: page.sourceModel.testSource(page.selectedRow)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: page.testMessage.length > 0 || page.sourceModel.lastError.length > 0
                        text: page.sourceModel.lastError.length > 0 ? page.sourceModel.lastError : page.testMessage
                        color: page.sourceModel.lastError.length > 0 || !page.testSucceeded ? page.theme.danger : page.theme.textSecondary
                        wrapMode: Text.Wrap
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: page.theme.border }

                    Label { text: qsTr("Bind selected source") }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox { id: channelPicker; model: [qsTr("Channel 1"), qsTr("Channel 2"), qsTr("Channel 3"), qsTr("Channel 4")] }
                        Button {
                            text: qsTr("Bind")
                            enabled: page.selectedRow >= 0
                            onClicked: {
                                const source = page.sourceModel.sourceAt(page.selectedRow)
                                if (source.sourceId > 0) {
                                    page.streamSessionManager.bindSource(channelPicker.currentIndex, source.sourceId)
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: page.streamSessionManager.lastError
                        color: page.theme.danger
                        visible: text.length > 0
                        wrapMode: Text.Wrap
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Select local video")
        nameFilters: [qsTr("Video files (*.mp4 *.avi *.mkv)")]
        onAccepted: endpointField.text = selectedFile.toString().replace("file:///", "")
    }
}
