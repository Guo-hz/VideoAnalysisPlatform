import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    required property var startupController

    width: 720
    height: 420
    minimumWidth: 560
    minimumHeight: 360
    visible: true
    title: qsTr("视频分析平台 - 启动失败")
    color: "#f4f6f8"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 18

        Label {
            Layout.fillWidth: true
            text: qsTr("无法初始化应用数据")
            color: "#17202a"
            font.pixelSize: 24
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.startupController.errorMessage
            color: "#b42318"
            font.pixelSize: 15
            wrapMode: Text.Wrap
            verticalAlignment: Text.AlignTop
        }

        Label {
            Layout.fillWidth: true
            text: root.startupController.dataDirectory
            color: "#475467"
            elide: Text.ElideMiddle
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10

            Button {
                text: qsTr("打开数据目录")
                onClicked: root.startupController.openDataDirectory()
            }
            Button {
                text: qsTr("退出")
                onClicked: root.startupController.quit()
            }
            Button {
                text: qsTr("重试")
                highlighted: true
                onClicked: root.startupController.retry()
            }
        }
    }
}
