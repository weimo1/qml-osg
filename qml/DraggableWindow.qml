import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    id: draggableWindow
    
    // 属性
    property alias titleText: titleText.text
    property alias content: contentLoader.sourceComponent
    property bool isCollapsed: false
    
    // 自定义信号
    signal windowClosed()
    
    // 默认属性
    width: 320
    height: 450
    flags: Qt.Window | Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint
    color: "transparent"
    
    // 拖动相关属性
    property point lastMousePos: Qt.point(0, 0)
    
    // 阴影背景
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "transparent"
        radius: 10
        
        // 主窗口背景
        Rectangle {
            id: windowBackground
            anchors.fill: parent
            anchors.margins: 5
            color: "white"
            border.color: "#cccccc"
            border.width: 1
            radius: 8
            
            // 窗口标题栏
            Rectangle {
                id: titleBar
                height: 30
                color: "#34495e"
                radius: 8
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 1
                anchors.topMargin: 1
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                
                // 标题文本
                Text {
                    id: titleText
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 10
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                }
                
                // 控制按钮
                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 5
                    spacing: 5
                    
                    // 折叠/展开按钮
                    Rectangle {
                        width: 20
                        height: 20
                        color: "#2c3e50"
                        radius: 3
                        
                        Text {
                            anchors.centerIn: parent
                            text: draggableWindow.isCollapsed ? "□" : "−"
                            color: "white"
                            font.pixelSize: 12
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                draggableWindow.isCollapsed = !draggableWindow.isCollapsed;
                                if (draggableWindow.isCollapsed) {
                                    draggableWindow.height = titleBar.height + 10;
                                } else {
                                    draggableWindow.height = 450; // 恢复原始高度
                                }
                            }
                        }
                    }
                    
                    // 关闭按钮
                    Rectangle {
                        width: 20
                        height: 20
                        color: "#e74c3c"
                        radius: 3
                        
                        Text {
                            anchors.centerIn: parent
                            text: "×"
                            color: "white"
                            font.pixelSize: 14
                            font.bold: true
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                draggableWindow.windowClosed(); // 发出自定义关闭信号
                                draggableWindow.close(); // 关闭窗口
                            }
                        }
                    }
                }
                
                // 拖动区域（排除按钮区域）
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 50 // 为按钮留出空间
                    onPressed: {
                        lastMousePos = Qt.point(mouseX, mouseY);
                    }
                    onMouseXChanged: {
                        var delta = mouseX - lastMousePos.x;
                        draggableWindow.x += delta;
                    }
                    onMouseYChanged: {
                        var delta = mouseY - lastMousePos.y;
                        draggableWindow.y += delta;
                    }
                }
            }
            
            // 内容区域
            Loader {
                id: contentLoader
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: titleBar.bottom
                anchors.bottom: parent.bottom
                anchors.margins: 10
                anchors.topMargin: 10  // 增加顶部边距，避免与标题栏重叠
                visible: !draggableWindow.isCollapsed
            }
        }
    }
}