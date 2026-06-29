import QtQuick

import "../components"
import "../styles"

PagePlaceholder {
    id: page

    title: qsTr("视频墙")
    subtitle: qsTr("多路视频展示区域占位")
    accentColor: page.theme.success
    items: [
        { "title": qsTr("视频窗口 1"), "body": qsTr("后续显示视频画面、状态和基础控制。") },
        { "title": qsTr("视频窗口 2"), "body": qsTr("当前阶段只保留静态占位。") },
        { "title": qsTr("视频窗口 3"), "body": qsTr("后续支持 1/4 宫格布局切换。") },
        { "title": qsTr("视频窗口 4"), "body": qsTr("暂不接入播放、截图或检测逻辑。") }
    ]
}
