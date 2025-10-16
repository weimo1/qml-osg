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
    
    // 全屏状态
    property bool isFullScreen: false
    
    // 当前视图类型
    property int currentViewType: SimpleOSGViewer.MainView  // 使用枚举值
    
    // 文件路径输入框
    property string filePath: "test.osgt"
    
    // 控制面板可见性
    property bool controlPanelVisible: true
    
    // PBR控制面板可见性
    property bool pbrControlVisible: false
    
 
    // 工具栏
    header: Rectangle {
        height: 60
        color: "#34495e"
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 15
            
            // 标题
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
            
            // 主要功能按钮
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
                        osgViewer.createShape()
                    }
                }
                
                Button {
                    text: "PBR球体"
                    background: Rectangle {
                        color: "#9b59b6"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "PBR球体"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Create PBR Sphere button clicked")
                        osgViewer.createPBRScene()
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
                        osgViewer.resetView()
                    }
                }
                
                Button {
                    text: "PBR控制"
                    checkable: true
                    checked: pbrControlVisible
                    background: Rectangle {
                        color: pbrControlVisible ? "#e67e22" : "#7f8c8d"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "PBR控制"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        pbrControlVisible = !pbrControlVisible;
                    }
                }
                
                // 新增：使用新天空盒类创建图形的按钮
                Button {
                    text: "新天空盒"
                    background: Rectangle {
                        color: "#1abc9c"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "新天空盒"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Create Shape with New Skybox button clicked")
                        osgViewer.createShapeWithNewSkybox()
                    }
                }
                
               
            }
            
            // 状态指示器
            Rectangle {
                width: 100
                height: 30
                color: "#27ae60"
                radius: 15
                
                Row {
                    anchors.centerIn: parent
                    spacing: 5
                    
                    Rectangle {
                        width: 10
                        height: 10
                        color: "#2ecc71"
                        radius: 5
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    
                    Text {
                        text: "就绪"
                        color: "white"
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
    
    // 主要内容区域
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 60  // 为工具栏留出空间
        spacing: 10
        
        // OSG视图区域 - 横屏显示
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 15
            color: "transparent"
            border.color: "#bdc3c7"
            border.width: 2
            radius: 8
            
            // 添加内边框效果
            Rectangle {
                anchors.fill: parent
                anchors.margins: 2
                color: "transparent"
                border.color: "#7f8c8d"
                border.width: 1
                radius: 6
                
                // OSG视图
                SimpleOSGViewer {
                    id: osgViewer
                    anchors.fill: parent
                    anchors.margins: 5
                    viewType: currentViewType
                    
                    // 监听视图类型变化
                    onViewTypeChanged: {
                        console.log("OSG Viewer view type changed to:", viewType);
                    }
                    
                    // 双击全屏和点选择
                    MouseArea {
                        anchors.fill: parent
                        onPressed: function(mouse) {
                            // Ctrl+左键点击选择模型
                            if (mouse.button === Qt.LeftButton && (mouse.modifiers & Qt.ControlModifier)) {
                                // 将鼠标坐标转换为OSG视图坐标
                                var point = mapToItem(osgViewer, mouse.x, mouse.y);
                                osgViewer.selectModel(point.x, point.y);
                                mouse.accepted = true;
                            } else {
                                // 不处理任何鼠标按下事件，全部传递给OSG视图
                                mouse.accepted = false;
                            }
                        }
                        onDoubleClicked: function(mouse) {
                            if (mouse.button === Qt.LeftButton) {
                                // 左键双击回归视角
                                osgViewer.resetToHomeView();
                                mouse.accepted = true; // 左键双击被处理
                            } else {
                                mouse.accepted = false; // 其他双击事件传递给OSG视图
                            }
                        }
                        onReleased: function(mouse) {
                            // 不处理任何鼠标释放事件，全部传递给OSG视图
                            mouse.accepted = false;
                        }
                        onPositionChanged: function(mouse) {
                            // 不处理任何鼠标移动事件，全部传递给OSG视图
                            mouse.accepted = false;
                        }
                        onWheel: function(wheel) {
                            // 不处理任何滚轮事件，全部传递给OSG视图
                            wheel.accepted = false;
                        }
                        // 确保事件能够正确传递
                        propagateComposedEvents: true
                    }

                    onRequestFileDialog: {
                        fileDialog.open();
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
                
                // 添加摄像机位置显示（左上角）
                Text {
                    id: cameraPosition
                    text: "相机: x=" + osgViewer.cameraX.toFixed(2) + " y=" + osgViewer.cameraY.toFixed(2) + " z=" + osgViewer.cameraZ.toFixed(2)
                    color: "white"
                    font.pixelSize: 12
                    x: 10
                    y: 10
                    z: 100  // 确保在最上层显示
                }
            }
        }
        
        // 底部控制面板
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            Layout.margins: 15
            color: "white"
            border.color: "#bdc3c7"
            border.width: 1
            radius: 8
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                // 文件操作和视图切换
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20
                    
                    // 文件操作
                    GroupBox {
                        title: "文件操作"
                        Layout.fillWidth: true
                        Layout.preferredWidth: 300
                        
                        RowLayout {
                            anchors.fill: parent
                            spacing: 10
                            
                            TextField {
                                id: filePathInput
                                Layout.fillWidth: true
                                text: filePath
                                placeholderText: "请输入OSG文件路径..."
                                selectByMouse: true
                                font.pixelSize: 12
                                onTextChanged: {
                                    filePath = text
                                }
                            }
                            
                            Button {
                                text: "加载"
                                Layout.preferredWidth: 60
                                onClicked: {
                                    console.log("Load File button clicked, file path: " + filePath)
                                    osgViewer.loadOSGFile(filePath)
                                }
                            }
                            
                            Button {
                                text: "选择"
                                Layout.preferredWidth: 60
                                onClicked: {
                                    console.log("Select File button clicked")
                                    osgViewer.openFileSelector()
                                }
                            }
                        }
                    }
                    
                    // 视图切换
                    GroupBox {
                        title: "视图切换"
                        Layout.fillWidth: true
                        Layout.preferredWidth: 300
                        
                        RowLayout {
                            anchors.fill: parent
                            spacing: 10
                            
                            Button {
                                text: "主视图"
                                checkable: true
                                checked: currentViewType === SimpleOSGViewer.MainView
                                onClicked: {
                                    currentViewType = SimpleOSGViewer.MainView;
                                }
                            }
                            
                            Button {
                                text: "前视图"
                                checkable: true
                                checked: currentViewType === SimpleOSGViewer.FrontView
                                onClicked: {
                                    currentViewType = SimpleOSGViewer.FrontView;
                                }
                            }
                            
                            Button {
                                text: "侧视图"
                                checkable: true
                                checked: currentViewType === SimpleOSGViewer.SideView
                                onClicked: {
                                    currentViewType = SimpleOSGViewer.SideView;
                                }
                            }
                            
                            Button {
                                text: "俯视图"
                                checkable: true
                                checked: currentViewType === SimpleOSGViewer.TopView
                                onClicked: {
                                    currentViewType = SimpleOSGViewer.TopView;
                                }
                            }
                        }
                    }
                    
                    Item {
                        Layout.fillWidth: true
                    }
                }
                
                // 操作提示
                Rectangle {
                    Layout.fillWidth: true
                    height: 30
                    color: "#f8f9fa"
                    border.color: "#e9ecef"
                    border.width: 1
                    radius: 5
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 20
                        
                        Text {
                            text: "鼠标左键拖拽: 旋转视图"
                            color: "#7f8c8d"
                            font.pixelSize: 11
                        }
                        
                        Text {
                            text: "鼠标滚轮: 缩放视图"
                            color: "#7f8c8d"
                            font.pixelSize: 11
                        }
                        
                        Text {
                            text: "鼠标右键拖拽: 平移视图"
                            color: "#7f8c8d"
                            font.pixelSize: 11
                        }
                        
                        Text {
                            text: "Ctrl+左键点击: 选择模型"
                            color: "#7f8c8d"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }
    }
    
    // PBR材质控制面板（浮动窗口）
    DraggableWindow {
        id: pbrControlWindow
        width: 400
        height: 300
        visible: pbrControlVisible
        titleText: "PBR材质控制"
        
        onVisibleChanged: {
            if (visible) {
                // 设置窗口位置在主窗口旁边
                x = window.x + window.width - width - 50
                y = window.y + 100
                raise();
                requestActivate();
            }
        }
        
        onWindowClosed: {
            pbrControlVisible = false;
        }
        
        // 设置内容
        content: Component {
            PBRMaterialControl {
                id: pbrControl
                anchors.fill: parent
                
                onMaterialPropertiesChanged: function(albedoR, albedoG, albedoB, albedoA,
                                                   metallic, roughness, 
                                                   specular, ao) {
                    // 当材质属性改变时，更新OSG场景中的材质
                    osgViewer.updatePBRMaterial(albedoR, albedoG, albedoB, albedoA, metallic, roughness, specular, ao);
                }
            }
        }
    }
    
    // 文件选择对话框
    FileDialog {
        id: fileDialog
        title: "请选择OSG文件"
        nameFilters: ["OSG文件 (*.osg *.osgt *.osgb)", "所有文件 (*)"]
        onAccepted: {
            // 获取选择的文件路径并更新文本框
            var selectedFile = fileDialog.selectedFile.toString();
            // 移除 "file:///" 前缀
            if (selectedFile.startsWith("file:///")) {
                selectedFile = selectedFile.substring(8);
            }
            filePathInput.text = selectedFile;
            filePath = selectedFile;
            console.log("选择的文件: " + selectedFile);
        }
    }
    
    // 全屏切换函数
    function toggleFullScreen() {
        if (isFullScreen) {
            window.showNormal();
            isFullScreen = false;
        } else {
            window.showFullScreen();
            isFullScreen = true;
        }
    }
    
    // 获取视图类型名称
    function getViewTypeName(viewType) {
        switch(viewType) {
        case SimpleOSGViewer.MainView: return "主视图";
        case SimpleOSGViewer.FrontView: return "前视图";
        case SimpleOSGViewer.SideView: return "侧视图";
        case SimpleOSGViewer.TopView: return "俯视图";
        default: return "未知视图";
        }
    }
}