pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "components"
import "pages"
import "styles"

ApplicationWindow {
    id: root

    width: 1280
    height: 720
    minimumWidth: 960
    minimumHeight: 600
    visible: true
    title: qsTr("实时多路视频流智能分析平台")

    required property var videoSourceModel
    required property var alarmEventModel
    required property var streamSessionManager
    required property var performanceModel
    required property var recordingModel
    required property var appPaths
    required property var settingsController

    readonly property AppTheme theme: AppTheme {}
    readonly property var navigationItems: [
        { "title": qsTr("首页") },
        { "title": qsTr("视频墙") },
        { "title": qsTr("视频源") },
        { "title": qsTr("告警中心") },
        { "title": qsTr("回放") },
        { "title": qsTr("设置") }
    ]
    property int currentPageIndex: 0

    color: theme.windowBackground

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            color: root.theme.navBackground

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: root.theme.spacingM
                spacing: root.theme.spacingS

                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: root.theme.spacingS
                    Layout.bottomMargin: root.theme.spacingL
                    text: qsTr("视频分析平台")
                    color: root.theme.navTitle
                    font.pixelSize: root.theme.fontTitle
                    font.bold: true
                    elide: Text.ElideRight
                }

                Repeater {
                    model: root.navigationItems

                    NavigationItem {
                        required property var modelData
                        required property int index

                        Layout.fillWidth: true
                        text: modelData.title
                        selected: root.currentPageIndex === index
                        onClicked: root.currentPageIndex = index
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.theme.windowBackground

            StackLayout {
                anchors.fill: parent
                currentIndex: root.currentPageIndex

                DashboardPage {
                    streamSessionManager: root.streamSessionManager
                    performanceModel: root.performanceModel
                    alarmModel: root.alarmEventModel
                    recordingModel: root.recordingModel
                }
                VideoWallPage {
                    streamSessionManager: root.streamSessionManager
                    performanceModel: root.performanceModel
                    sourceModel: root.videoSourceModel
                }
                SourceManagerPage {
                    sourceModel: root.videoSourceModel
                    streamSessionManager: root.streamSessionManager
                }
                AlarmCenterPage {
                    alarmModel: root.alarmEventModel
                }
                PlaybackPage {
                    recordingModel: root.recordingModel
                }
                SettingsPage {
                    appPaths: root.appPaths
                    settingsController: root.settingsController
                }
            }
        }
    }
}
