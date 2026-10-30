pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var streamSession
    required property var videoItem
    property bool editingEnabled: false
    property bool showExistingRules: true
    property color roiColor: "#38bdf8"
    property color draftColor: "#22c55e"

    readonly property bool hasVideo: videoItem
                                     && videoItem.visible
                                     && videoItem.paintedWidth > 0
                                     && videoItem.paintedHeight > 0
    readonly property real paintedX: hasVideo ? videoItem.x + (videoItem.width - videoItem.paintedWidth) / 2 : 0
    readonly property real paintedY: hasVideo ? videoItem.y + (videoItem.height - videoItem.paintedHeight) / 2 : 0
    readonly property real paintedWidth: hasVideo ? videoItem.paintedWidth : 0
    readonly property real paintedHeight: hasVideo ? videoItem.paintedHeight : 0

    property bool dragging: false
    property real startX: 0
    property real startY: 0
    property real currentX: 0
    property real currentY: 0

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function isInsidePaintedArea(x, y) {
        return root.hasVideo
                && x >= root.paintedX
                && y >= root.paintedY
                && x <= root.paintedX + root.paintedWidth
                && y <= root.paintedY + root.paintedHeight
    }

    function normalizedDraftRect() {
        var left = Math.min(root.startX, root.currentX)
        var top = Math.min(root.startY, root.currentY)
        var width = Math.abs(root.currentX - root.startX)
        var height = Math.abs(root.currentY - root.startY)
        return {
            x: (left - root.paintedX) / root.paintedWidth,
            y: (top - root.paintedY) / root.paintedHeight,
            width: width / root.paintedWidth,
            height: height / root.paintedHeight
        }
    }

    Repeater {
        model: root.showExistingRules || root.editingEnabled ? root.streamSession.roiRules : []

        Rectangle {
            id: roiRect

            required property var modelData

            x: root.paintedX + modelData.x * root.paintedWidth
            y: root.paintedY + modelData.y * root.paintedHeight
            width: Math.max(2, modelData.width * root.paintedWidth)
            height: Math.max(2, modelData.height * root.paintedHeight)
            z: 2
            visible: root.hasVideo && modelData.enabled && (root.showExistingRules || root.editingEnabled)
            color: root.showExistingRules ? "#2238bdf8" : "transparent"
            border.color: root.showExistingRules ? root.roiColor : "transparent"
            border.width: 2

            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 4
                visible: root.showExistingRules
                text: roiRect.modelData.ruleName
                color: "#e0f2fe"
                font.pixelSize: 11
                font.bold: true
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 3
                width: 18
                height: 18
                radius: 9
                z: 4
                visible: root.editingEnabled
                color: "#dc2626"

                Text {
                    anchors.centerIn: parent
                    text: "X"
                    color: "white"
                    font.pixelSize: 10
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.streamSession.removeRoiRule(roiRect.modelData.index)
                }
            }
        }
    }

    Rectangle {
        x: Math.min(root.startX, root.currentX)
        y: Math.min(root.startY, root.currentY)
        width: Math.abs(root.currentX - root.startX)
        height: Math.abs(root.currentY - root.startY)
        z: 3
        visible: root.dragging
        color: "#3322c55e"
        border.color: root.draftColor
        border.width: 2
    }

    MouseArea {
        anchors.fill: parent
        z: 1
        enabled: root.editingEnabled && root.hasVideo
        acceptedButtons: Qt.LeftButton

        onPressed: function(mouse) {
            if (!root.isInsidePaintedArea(mouse.x, mouse.y)) {
                root.dragging = false
                return
            }

            root.dragging = true
            root.startX = root.clamp(mouse.x, root.paintedX, root.paintedX + root.paintedWidth)
            root.startY = root.clamp(mouse.y, root.paintedY, root.paintedY + root.paintedHeight)
            root.currentX = root.startX
            root.currentY = root.startY
        }

        onPositionChanged: function(mouse) {
            if (!root.dragging) {
                return
            }

            root.currentX = root.clamp(mouse.x, root.paintedX, root.paintedX + root.paintedWidth)
            root.currentY = root.clamp(mouse.y, root.paintedY, root.paintedY + root.paintedHeight)
        }

        onReleased: function(mouse) {
            if (!root.dragging) {
                return
            }

            root.currentX = root.clamp(mouse.x, root.paintedX, root.paintedX + root.paintedWidth)
            root.currentY = root.clamp(mouse.y, root.paintedY, root.paintedY + root.paintedHeight)

            var width = Math.abs(root.currentX - root.startX)
            var height = Math.abs(root.currentY - root.startY)
            if (width >= 8 && height >= 8) {
                var rect = root.normalizedDraftRect()
                root.streamSession.addRoiRect(rect.x, rect.y, rect.width, rect.height)
            }

            root.dragging = false
        }

        onCanceled: root.dragging = false
    }
}
