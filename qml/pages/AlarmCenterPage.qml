import QtQuick

import "../components"
import "../styles"

PagePlaceholder {
    id: page

    title: qsTr("告警中心")
    subtitle: qsTr("告警事件展示页面占位")
    accentColor: page.theme.danger
    items: [
        { "title": qsTr("最新告警"), "body": qsTr("后续展示实时告警事件列表。") },
        { "title": qsTr("筛选条件"), "body": qsTr("后续按视频源、事件类型和时间范围筛选。") },
        { "title": qsTr("告警详情"), "body": qsTr("后续显示截图、事件说明和确认状态。") },
        { "title": qsTr("处理操作"), "body": qsTr("后续支持确认和删除告警。") }
    ]
}
