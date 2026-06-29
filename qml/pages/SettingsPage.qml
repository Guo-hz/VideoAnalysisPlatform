import QtQuick

import "../components"
import "../styles"

PagePlaceholder {
    id: page

    title: qsTr("设置")
    subtitle: qsTr("系统配置页面占位")
    accentColor: page.theme.textSecondary
    items: [
        { "title": qsTr("基础设置"), "body": qsTr("后续配置语言、启动行为和显示选项。") },
        { "title": qsTr("存储设置"), "body": qsTr("后续配置截图、录像和数据库路径。") },
        { "title": qsTr("性能设置"), "body": qsTr("后续配置队列长度、限流和统计间隔。") },
        { "title": qsTr("日志设置"), "body": qsTr("后续配置日志等级和输出位置。") }
    ]
}
