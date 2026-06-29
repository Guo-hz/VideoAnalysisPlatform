import QtQuick

import "../components"
import "../styles"

PagePlaceholder {
    id: page

    title: qsTr("回放")
    subtitle: qsTr("历史回放页面占位")
    accentColor: page.theme.primary
    items: [
        { "title": qsTr("录像列表"), "body": qsTr("后续展示本地录像和事件片段。") },
        { "title": qsTr("时间选择"), "body": qsTr("后续按日期和时间范围查找历史内容。") },
        { "title": qsTr("回放窗口"), "body": qsTr("后续展示选中录像内容。") },
        { "title": qsTr("关联告警"), "body": qsTr("后续从告警事件跳转到对应片段。") }
    ]
}
