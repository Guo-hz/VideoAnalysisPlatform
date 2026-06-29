import QtQuick

import "../components"
import "../styles"

PagePlaceholder {
    id: page

    title: qsTr("视频源")
    subtitle: qsTr("视频源管理页面占位")
    accentColor: page.theme.warning
    items: [
        { "title": qsTr("视频源列表"), "body": qsTr("后续展示已配置的视频源名称、类型和状态。") },
        { "title": qsTr("新增入口"), "body": qsTr("后续提供添加本地文件、摄像头和 RTSP 源的入口。") },
        { "title": qsTr("编辑信息"), "body": qsTr("后续支持修改名称、地址和启用状态。") },
        { "title": qsTr("错误信息"), "body": qsTr("后续展示最近一次连接或解析错误。") }
    ]
}
