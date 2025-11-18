import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import OSGViewer 1.0

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: "OSG 3D Viewer"
    
    // 属性定义
    property bool isFullScreen: false
    property string filePath: "test.osgt"
    property bool controlPanelVisible: true
    property bool pbrControlVisible: false
    property bool atmosphereControlVisible: false
    property int navigationWidth: 80
    property int sidebarWidth: 250
    property string currentNavSelection: ""
    property real sunZenithAngle: 0.5
    property real sunAzimuthAngle: 0.0

    // 头部工具栏
    header: Rectangle {
        height: 60
        color: "#34495e"
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 15
            
            // 应用标题
            Text {
                text: "OSG 3D Viewer"
                color: "white"
                font.pixelSize: 20
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            // 工具按钮区域
            Row {
                spacing: 10
                
                Button {
                    text: "创建图形"
                    background: Rectangle {
                        color: "#3498db"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "创建图形"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Create Shape button clicked")
                        // 调用创建图形功能
                    }
                }
                
                // 添加加载模型按钮
                Button {
                    text: "加载模型"
                    background: Rectangle {
                        color: "#9b59b6"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "加载模型"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Load Model button clicked")
                        fileDialog.open()
                    }
                }
                
                // 添加加载目录按钮
                Button {
                    text: "加载目录"
                    background: Rectangle {
                        color: "#8e44ad"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "加载目录"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Load Directory button clicked")
                        folderDialog.open()
                    }
                }

                Button {
                    text: "重置视野"
                    background: Rectangle {
                        color: "#27ae60"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "重置视野"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Reset View button clicked")
                        osgViewer.resetToHomeView()
                    }
                } 
                
                // 添加光照控制按钮
                Button {
                    id: toolbarLightingButton
                    property bool lightingEnabled: true
                    text: toolbarLightingButton.lightingEnabled ? "Disable Lighting" : "Enable Lighting"
                    background: Rectangle {
                        color: toolbarLightingButton.lightingEnabled ? "#e74c3c" : "#27ae60"
                        radius: 4
                    }
                    contentItem: Text {
                        text: toolbarLightingButton.lightingEnabled ? "Disable Lighting" : "Enable Lighting"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        toolbarLightingButton.lightingEnabled = !toolbarLightingButton.lightingEnabled
                        console.log("Toggle lighting clicked, enabled:", toolbarLightingButton.lightingEnabled)
                        osgViewer.toggleLighting(toolbarLightingButton.lightingEnabled)
                    }
                }
                
                // 添加大气渲染按钮
                Button {
                    text: "大气渲染"
                    background: Rectangle {
                        color: "#f39c12"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "大气渲染"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Atmosphere rendering button clicked")
                        osgViewer.createAtmosphere()
                    }
                }
                
                // 添加MRT测试按钮
                Button {
                    text: "MRT测试"
                    background: Rectangle {
                        color: "#9b59b6"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "MRT测试"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("MRT test button clicked")
                        osgViewer.testMRT()
                    }
                }
            }
            
            // 添加文件对话框
            FileDialog {
                id: fileDialog
                title: "请选择OSG文件"
                nameFilters: ["OSG文件 (*.osg *.osgt *.osgb)", "所有文件 (*)"]
                onAccepted: {
                    console.log("选择的文件: " + fileDialog.selectedFile)
                    // 调用OSG视图组件加载文件
                    osgViewer.loadOSGFile(fileDialog.selectedFile)
                }
            }
            
            // 添加目录对话框
            FolderDialog {
                id: folderDialog
                title: "请选择包含OSG文件的目录"
                onAccepted: {
                    console.log("选择的目录: " + folderDialog.selectedFolder)
                    // 调用OSG视图组件加载目录
                    osgViewer.loadOSGFile(folderDialog.selectedFolder)
                }
            }

        }
    }
    
    // 主要内容区域
    Item {
        id: mainContent
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: 0  // 确保没有顶部边距
        }
        
        // 工具栏下方的分隔线
        Rectangle {
            id: toolbarSeparator
            height: 1
            color: "#bdc3c7"
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
        }
        
        // 左侧导航栏
        Rectangle {
            id: navigationBar
            width: navigationWidth
            height: parent.height
            color: "#2c3e50"
            anchors {
                left: parent.left
                top: toolbarSeparator.bottom  // 紧接在分隔线下方
                bottom: parent.bottom
            }
            
            Column {
                anchors.fill: parent
                spacing: 0
                
                // 导航项
                Repeater {
                    model: ListModel {
                        ListElement { name: "场景"; icon: "\u{1F310}"; action: "scene" }                         
                        ListElement { name: "视图"; icon: "\u{1F441}"; action: "view" }
                        ListElement { name: "模型"; icon: "\u{1F38F}"; action: "model" }
                    }
                    
                    Rectangle {
                        width: parent.width
                        height: 80
                        color: mouseArea.containsMouse || currentNavSelection === model.action ? "#34495e" : "transparent"
                        
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                console.log(model.name + " clicked");
                                // 二次点击收起侧边栏
                                if (currentNavSelection === model.action) {
                                    currentNavSelection = "";  // 收起侧边栏
                                } else {
                                    currentNavSelection = model.action;  // 展开侧边栏
                                }
                            }
                        }
                        
                        Column {
                            anchors.centerIn: parent
                            spacing: 5
                            
                            Text {
                                text: model.icon
                                color: "white"
                                font.pixelSize: 24
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            
                            Text {
                                text: model.name
                                color: "white"
                                font.pixelSize: 12
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
                
                Item {
                    Layout.fillHeight: true
                    Rectangle {
                        anchors.fill: parent
                        color: "#2c3e50"
                    }
                }
            }
        }
        
        // 侧边栏 - 显示选中导航项的详细功能
        Rectangle {
            id: sidebar
            width: currentNavSelection !== "" ? sidebarWidth : 0
            height: parent.height
            color: "#f8f9fa"
            anchors {
                left: navigationBar.right
                top: toolbarSeparator.bottom  // 确保从分隔线下方开始
                bottom: parent.bottom
            }
            border.color: "#bdc3c7"
            border.width: 1
            Behavior on width { NumberAnimation { duration: 200 } } // 添加动画效果
            
            // 侧边栏内容根据选中的导航项显示
            ScrollView {
                anchors.fill: parent
                clip: true
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15
                    
                    // 标题
                    Text {
                        text: {
                          switch(currentNavSelection) {
                            case "view": return "View Controls";
                            case "model": return "Model Management";
                            case "light": return "Lighting Control";
                            default: return "Control Panel";
                            }
                        }
                        font.pixelSize: 18
                        font.bold: true
                        color: "#2c3e50"
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 根据选中的导航项显示不同的内容
                    Loader {
                        sourceComponent: {
                            switch(currentNavSelection) {
                            case "view": return viewControls;
                            case "model": return modelControls;
                            case "light": return lightControls;
                            case "skynode": return skyNodeAtmosphereControls;
                            default: return defaultControls;
                            }
                        }
                        width: parent.width
                    }
                }
            }
        }
        
        // 右侧内容区域
        Item {
            id: contentArea
            anchors {
                left: sidebar.right
                right: parent.right
                top: toolbarSeparator.bottom  // 确保从分隔线下方开始
                bottom: parent.bottom
            }
            
            // OSG视图区域 - 填充整个可用空间
            SimpleOSGViewer {
                id: osgViewer
                anchors.fill: parent
                focus: true  // 启用焦点
                
                // 添加键盘事件处理
                Keys.onPressed: function(event) {
                    // 将键盘事件传递给OSG视图组件
                    console.log("Key pressed:", event.key)
                    // 在这里可以添加特定的键盘事件处理逻辑
                }
          
                // 双击全屏和点选择
                MouseArea {
                    anchors.fill: parent
                    onPressed: function(mouse) {
                        mouse.accepted = false;
                        // 当鼠标点击时，确保OSG视图获得焦点
                        osgViewer.forceActiveFocus();
                    }
                    onDoubleClicked: function(mouse) {
                         mouse.accepted = false;                 
                    }
                    onReleased: function(mouse) {
                        mouse.accepted = false;
                    }
                    onPositionChanged: function(mouse) {
                        mouse.accepted = false;
                    }
                    onWheel: function(wheel) {
                        wheel.accepted = false;
                    }
                    propagateComposedEvents: true
                }         
            }
            
            // 添加鼠标位置显示（右下角）
            Text {
                id: mousePosition
                text: "鼠标: x=" + osgViewer.mouseX + " y=" + osgViewer.mouseY
                color: "white"
                font.pixelSize: 12
                x: parent.width - width - 10
                y: parent.height - height - 10
                z: 100  // 确保在最上层显示
            }
            
            // 添加相机位置显示（右上角）
            Text {
                id: cameraPosition
                text: "相机: x=" + osgViewer.cameraX.toFixed(2) + " y=" + osgViewer.cameraY.toFixed(2) + " z=" + osgViewer.cameraZ.toFixed(2)
                color: "white"
                font.pixelSize: 12
                x: parent.width - width - 10
                y: 10
                z: 100  // 确保在最上层显示
            }
        }
    } 
    
    // 视图控制组件
    Component {
        id: viewControls
        
        Column {
            width: parent.width
            spacing: 15
            
            Text {
                text: "视图控制"
                font.pixelSize: 16
                font.bold: true
                color: "#2c3e50"
            }
            
            // 视图类型选择
            Text {
                text: "视图类型"
                font.pixelSize: 14
                color: "#34495e"
            }
            
            // 使用Grid布局优化按钮排列
            Grid {
                columns: 2
                spacing: 10
                width: parent.width
                
                Button {
                    text: "主视图"
                    width: Math.max(80, (parent.width - parent.spacing) / 2)
                    background: Rectangle {
                        color: osgViewer.viewType === 0 ? "#3498db" : "#ecf0f1"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "主视图"
                        color: osgViewer.viewType === 0 ? "white" : "#2c3e50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        osgViewer.setViewType(0)  // MainView
                    }
                }
                
                Button {
                    text: "前视图"
                    width: Math.max(80, (parent.width - parent.spacing) / 2)
                    background: Rectangle {
                        color: osgViewer.viewType === 1 ? "#3498db" : "#ecf0f1"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "前视图"
                        color: osgViewer.viewType === 1 ? "white" : "#2c3e50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        osgViewer.setViewType(1)  // FrontView
                    }
                }
                
                Button {
                    text: "侧视图"
                    width: Math.max(80, (parent.width - parent.spacing) / 2)
                    background: Rectangle {
                        color: osgViewer.viewType === 2 ? "#3498db" : "#ecf0f1"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "侧视图"
                        color: osgViewer.viewType === 2 ? "white" : "#2c3e50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        osgViewer.setViewType(2)  // SideView
                    }
                }
                
                Button {
                    text: "俯视图"
                    width: Math.max(80, (parent.width - parent.spacing) / 2)
                    background: Rectangle {
                        color: osgViewer.viewType === 3 ? "#3498db" : "#ecf0f1"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "俯视图"
                        color: osgViewer.viewType === 3 ? "white" : "#2c3e50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        osgViewer.setViewType(3)  // TopView
                    }
                }
            }
            
            // 视图操作按钮
            Text {
                text: "视图操作"
                font.pixelSize: 14
                color: "#34495e"
            }
            
            // 使用Column布局垂直排列操作按钮
            Column {
                width: parent.width
                spacing: 10
                
                Button {
                    text: "重置视野"
                    width: parent.width
                    background: Rectangle {
                        color: "#27ae60"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "重置视野"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        osgViewer.resetToHomeView()
                    }
                }
                
                Button {
                    text: "适应视图"
                    width: parent.width
                    background: Rectangle {
                        color: "#f39c12"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "适应视图"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        osgViewer.fitToView()
                    }
                }
            }
        }
    }
    
    // 模型控制组件（占位符）
    Component {
        id: modelControls
        
        Column {
            width: parent.width
            spacing: 15
            
            Text {
                text: "模型管理"
                font.pixelSize: 16
                font.bold: true
                color: "#2c3e50"
            }
            
            // 添加模型加载功能
            Button {
                text: "加载模型"
                width: parent.width
                background: Rectangle {
                    color: "#9b59b6"
                    radius: 4
                }
                contentItem: Text {
                    text: "加载模型"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    console.log("Load Model from sidebar clicked")
                    fileDialog.open()
                }
            }
            
            // 添加目录加载功能
            Button {
                text: "加载目录"
                width: parent.width
                background: Rectangle {
                    color: "#8e44ad"
                    radius: 4
                }
                contentItem: Text {
                    text: "加载目录"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    console.log("Load Directory from sidebar clicked")
                    folderDialog.open()
                }
            }
            
            // 添加重置视野功能
            Button {
                text: "重置视野"
                width: parent.width
                background: Rectangle {
                    color: "#27ae60"
                    radius: 4
                }
                contentItem: Text {
                    text: "重置视野"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    console.log("Reset View from sidebar clicked")
                    osgViewer.resetToHomeView()
                }
            }
            
            // 添加适应视图功能
            Button {
                text: "适应视图"
                width: parent.width
                background: Rectangle {
                    color: "#f39c12"
                    radius: 4
                }
                contentItem: Text {
                    text: "适应视图"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    console.log("Fit to View from sidebar clicked")
                    osgViewer.fitToView()
                }
            }
        }
    }
    
    // 光照控制组件（占位符）
    Component {
        id: lightControls
        
        Column {
            width: parent.width
            spacing: 15
            
            Text {
                text: "光照控制"
                font.pixelSize: 16
                font.bold: true
                color: "#2c3e50"
            }
            
            Text {
                text: "光照控制功能将在后续版本中实现"
                font.pixelSize: 14
                color: "#7f8c8d"
            }
        }
    }
    
    // 天空节点大气控制组件（占位符）
    Component {
        id: skyNodeAtmosphereControls
        
        Column {
            width: parent.width
            spacing: 15
            
            Text {
                text: "天空大气控制"
                font.pixelSize: 16
                font.bold: true
                color: "#2c3e50"
            }
            
            Text {
                text: "天空大气控制功能将在后续版本中实现"
                font.pixelSize: 14
                color: "#7f8c8d"
            }
        }
    }
    
    // 默认控制组件（占位符）
    Component {
        id: defaultControls
        
        Column {
            width: parent.width
            spacing: 15
            
            Text {
                text: "功能面板"
                font.pixelSize: 16
                font.bold: true
                color: "#2c3e50"
            }
            
            Text {
                text: "请选择左侧导航栏中的功能"
                font.pixelSize: 14
                color: "#7f8c8d"
            }
        }
    }
}