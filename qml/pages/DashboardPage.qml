import QtQuick

import "../components"
import "../styles"

PagePlaceholder {
    id: page

    title: qsTr("首页")
    subtitle: qsTr("系统运行概览占位")
    accentColor: page.theme.primary
    items: [
        { "title": qsTr("运行状态"), "body": qsTr("后续显示系统在线状态、运行时长和核心模块状态。") },
        { "title": qsTr("视频源概览"), "body": qsTr("后续汇总视频源数量、在线数量和异常数量。") },
        { "title": qsTr("告警摘要"), "body": qsTr("后续展示最新告警数量和未确认告警数量。") },
        { "title": qsTr("性能摘要"), "body": qsTr("后续展示 FPS、队列长度和处理耗时等关键指标。") }
    ]
}
