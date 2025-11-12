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
    
    // 大气控制面板可见性
    property bool atmosphereControlVisible: false
    
    // 导航栏宽度
    property int navigationWidth: 80
    
    // 侧边栏宽度
    property int sidebarWidth: 250
    
    // 当前选中的导航项
    property string currentNavSelection: ""
    
    // 大气参数
    property real sunZenithAngle: 0.5
    property real sunAzimuthAngle: 0.0

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
                
                // 新增：大气渲染场景按钮
                Button {
                    text: "大气渲染"
                    background: Rectangle {
                        color: "#e74c3c"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "大气渲染"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Create Atmosphere Scene button clicked")
                        osgViewer.createAtmosphereScene()
                    }
                }
                
                // 新增：结合天空盒和大气渲染的场景按钮
                Button {
                    text: "天空盒大气"
                    background: Rectangle {
                        color: "#9b59b6"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "天空盒大气"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Create Skybox Atmosphere Scene button clicked")
                        osgViewer.createSkyboxAtmosphereScene()
                    }
                }
                
                // 新增：结合纹理和大气渲染的场景按钮
                Button {
                    text: "纹理大气"
                    background: Rectangle {
                        color: "#3498db"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "纹理大气"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Create Textured Atmosphere Scene button clicked")
                        // 直接调用，纹理路径在C++代码中设置
                        osgViewer.createTexturedAtmosphereScene()
                    }
                }
                
                // 新增：结合天空盒大气和PBR立方体的场景按钮
                Button {
                    text: "天空盒PBR"
                    background: Rectangle {
                        color: "#e67e22"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "天空盒PBR"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("Create Skybox Atmosphere with PBR Scene button clicked")
                        osgViewer.createSkyboxAtmosphereWithPBRScene()
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
    
    // 主要内容区域 - 使用绝对定位确保完全填充
    Item {
        id: mainContent
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: 0 // 精确匹配工具栏高度
        }
        
        // 左侧导航栏
        Rectangle {
            id: navigationBar
            width: navigationWidth
            height: parent.height
            color: "#2c3e50"
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            
            Column {
                anchors.fill: parent
                spacing: 0
                
                // 导航项
                Repeater {
                    model: ListModel {
                        ListElement { name: "场景"; icon: "\u{1F310}"; action: "scene" }  // 地球图标
                        ListElement { name: "材质"; icon: "\u{1F3A8}"; action: "material" }  // 调色板图标
                        ListElement { name: "视图"; icon: "\u{1F441}"; action: "view" }   // 眼睛图标
                        ListElement { name: "模型"; icon: "\u{1F532}"; action: "model" }  // 方块图标
                        ListElement { name: "光照"; icon: "\u{1F4A1}"; action: "light" }  // 灯泡图标
                        ListElement { name: "动画"; icon: "\u{1F3AC}"; action: "animation" } // 电影图标
                        ListElement { name: "粒子"; icon: "\u{2600}"; action: "particle" }  // 太阳图标
                        ListElement { name: "物理"; icon: "\u{1F30C}"; action: "physics" }  // 银河图标
                        ListElement { name: "SkyNode"; icon: "\u{1F324}"; action: "skynode" }  // 小太阳图标
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
            anchors.left: navigationBar.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            border.color: "#bdc3c7"
            border.width: 1
            
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
                            case "view": return "视图控制";
                            case "model": return "模型管理";
                            case "light": return "光照控制";
                            case "particle": return "云海大气控制";
                            default: return "功能面板";
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
                            case "particle": return cloudSeaAtmosphereControls;
                            case "skynode": return skyNodeAtmosphereControls;
                            case "physics": return physicsControls;  // 添加物理控制组件
                            default: return defaultControls;
                            }
                        }
                        width: parent.width
                    }
                }
            }
            
            // 视图控制组件
            Component {
                id: viewControls
                
                Column {
                    width: parent.width
                    spacing: 15
                    
                    // 视图切换按钮
                    Text {
                        text: "视图类型"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    // 使用Column代替Grid来避免布局循环问题
                    Column {
                        width: parent.width
                        spacing: 10
                        
                        Button {
                            text: "主视图"
                            width: parent.width
                            checkable: true
                            checked: currentViewType === SimpleOSGViewer.MainView
                            onClicked: {
                                currentViewType = SimpleOSGViewer.MainView;
                            }
                        }
                        
                        Button {
                            text: "前视图"
                            width: parent.width
                            checkable: true
                            checked: currentViewType === SimpleOSGViewer.FrontView
                            onClicked: {
                                currentViewType = SimpleOSGViewer.FrontView;
                            }
                        }
                        
                        Button {
                            text: "侧视图"
                            width: parent.width
                            checkable: true
                            checked: currentViewType === SimpleOSGViewer.SideView
                            onClicked: {
                                currentViewType = SimpleOSGViewer.SideView;
                            }
                        }
                        
                        Button {
                            text: "俯视图"
                            width: parent.width
                            checkable: true
                            checked: currentViewType === SimpleOSGViewer.TopView
                            onClicked: {
                                currentViewType = SimpleOSGViewer.TopView;
                            }
                        }
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 视图操作按钮
                    Text {
                        text: "视图操作"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Button {
                        text: "重置视野"
                        width: parent.width
                        onClicked: {
                            console.log("Reset View button clicked")
                            osgViewer.resetView()
                        }
                    }
                }
            }
            
            // 模型控制组件
            Component {
                id: modelControls
                
                Column {
                    width: parent.width
                    spacing: 15
                    
                    // 文件操作
                    Text {
                        text: "文件操作"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    TextField {
                        id: filePathInput
                        width: parent.width
                        height: 40  // 增加高度以便更好地显示路径
                        text: filePath
                        placeholderText: "请输入OSG文件路径..."
                        selectByMouse: true
                        font.pixelSize: 14  // 增加字体大小
                        verticalAlignment: TextInput.AlignVCenter  // 垂直居中对齐
                        onTextChanged: {
                            filePath = text
                        }
                    }
                    
                    Row {
                        width: parent.width
                        spacing: 10
                        
                        Button {
                            text: "加载"
                            Layout.fillWidth: true
                            onClicked: {
                                console.log("Load File button clicked, file path: " + filePath)
                                osgViewer.loadOSGFile(filePath)
                            }
                        }
                        
                        Button {
                            text: "选择"
                            Layout.fillWidth: true
                            onClicked: {
                                console.log("Select File button clicked")
                                osgViewer.openFileSelector()
                            }
                        }
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 模型操作
                    Text {
                        text: "模型操作"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Button {
                        text: "创建立方体"
                        width: parent.width
                        onClicked: {
                            console.log("Create Cube button clicked")
                            osgViewer.createShape()
                        }
                    }
                    
                    Button {
                        text: "创建PBR球体"
                        width: parent.width
                        onClicked: {
                            console.log("Create PBR Sphere button clicked")
                            osgViewer.createPBRScene()
                        }
                    }
                    
                    Button {
                        text: "创建天空盒"
                        width: parent.width
                        onClicked: {
                            console.log("Create Skybox button clicked")
                            osgViewer.createShapeWithNewSkybox()
                        }
                    }
                    
                    Button {
                        text: "创建大气效果"
                        width: parent.width
                        onClicked: {
                            console.log("Create Atmosphere Scene button clicked")
                            osgViewer.createAtmosphereScene()
                            atmosphereControlVisible = true;
                        }
                    }
                }
            }
            
            // 云层参数控制组件
            Component {
                id: lightControls
                
                Column {
                    width: parent.width
                    spacing: 15
                    
                    // 云密度控制
                    Text {
                        text: "云密度 (Cloud Density)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudDensitySlider
                        width: parent.width
                        from: 0.0
                        to: 10.0
                        value: 5.0
                        stepSize: 0.1
                        onValueChanged: {
                            osgViewer.updateVolumeCloudParameters(
                                sunZenithAngle, sunAzimuthAngle,
                                cloudDensitySlider.value, cloudHeightSlider.value,
                                densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
                                stepSizeSlider.value, maxStepsSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "密度: " + cloudDensitySlider.value.toFixed(2)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云高度控制
                    Text {
                        text: "云厚度 (Cloud Height)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudHeightSlider
                        width: parent.width
                        from: 100.0
                        to: 5000.0
                        value: 800.0
                        stepSize: 10.0
                        onValueChanged: {
                            osgViewer.updateVolumeCloudParameters(
                                sunZenithAngle, sunAzimuthAngle,
                                cloudDensitySlider.value, cloudHeightSlider.value,
                                densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
                                stepSizeSlider.value, maxStepsSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "厚度: " + cloudHeightSlider.value.toFixed(0) + " m"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层密度阈值控制
                    Text {
                        text: "云层密度阈值"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: densityThresholdSlider
                        width: parent.width
                        from: 0.1
                        to: 0.5
                        value: 0.3
                        onValueChanged: {
                            osgViewer.updateVolumeCloudParameters(
                                sunZenithAngle, sunAzimuthAngle,
                                cloudDensitySlider.value, cloudHeightSlider.value,
                                densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
                                stepSizeSlider.value, maxStepsSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "阈值: " + densityThresholdSlider.value.toFixed(2)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层对比度控制
                    Text {
                        text: "云层对比度"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: contrastSlider
                        width: parent.width
                        from: 1.0
                        to: 3.0
                        value: 2.5
                        onValueChanged: {
                            osgViewer.updateVolumeCloudParameters(
                                sunZenithAngle, sunAzimuthAngle,
                                cloudDensitySlider.value, cloudHeightSlider.value,
                                densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
                                stepSizeSlider.value, maxStepsSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "对比度: " + contrastSlider.value.toFixed(1)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层密度因子控制
                    Text {
                        text: "云层密度因子"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: densityFactorSlider
                        width: parent.width
                        from: 0.5
                        to: 3.0
                        value: 1.5
                        onValueChanged: {
                            osgViewer.updateVolumeCloudParameters(
                                sunZenithAngle, sunAzimuthAngle,
                                cloudDensitySlider.value, cloudHeightSlider.value,
                                densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
                                stepSizeSlider.value, maxStepsSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "因子: " + densityFactorSlider.value.toFixed(1)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层步长控制
                    Text {
                        text: "云层步长"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: stepSizeSlider
                        width: parent.width
                        from: 1.0
                        to: 10.0
                        value: 3.0
                        onValueChanged: {
                            osgViewer.updateVolumeCloudParameters(
                                sunZenithAngle, sunAzimuthAngle,
                                cloudDensitySlider.value, cloudHeightSlider.value,
                                densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
                                stepSizeSlider.value, maxStepsSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "步长: " + stepSizeSlider.value.toFixed(1)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层最大步数控制
                    Text {
                        text: "云层最大步数"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: maxStepsSlider
                        width: parent.width
                        from: 50
                        to: 500
                        value: 200
                        stepSize: 10
                        onValueChanged: {
                            osgViewer.updateVolumeCloudParameters(
                                sunZenithAngle, sunAzimuthAngle,
                                cloudDensitySlider.value, cloudHeightSlider.value,
                                densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
                                stepSizeSlider.value, maxStepsSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "最大步数: " + maxStepsSlider.value.toFixed(0)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            // SkyNode大气控制组件
            Component {
                id: skyNodeAtmosphereControls
                
                Column {
                    width: parent.width
                    spacing: 15
                    
                    // 大气密度控制 (对应turbidity)
                    Text {
                        text: "大气密度 (Turbidity)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyNodeTurbiditySlider
                        width: parent.width
                        from: 1.0
                        to: 10.0
                        value: 2.0
                        onValueChanged: {
                            osgViewer.updateSkyNodeAtmosphereParameters(
                                value, 
                                skyNodeRayleighSlider.value, 
                                skyNodeMieCoefficientSlider.value, 
                                skyNodeMieDirectionalGSlider.value,
                                skyNodeSunZenithAngleSlider.value,
                                skyNodeSunAzimuthAngleSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "密度: " + skyNodeTurbiditySlider.value.toFixed(1)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 瑞利散射控制 (对应rayleigh)
                    Text {
                        text: "瑞利散射 (Rayleigh)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyNodeRayleighSlider
                        width: parent.width
                        from: 0.1
                        to: 20.0  // 改为20.0
                        value: 1.0
                        stepSize: 0.1
                        onValueChanged: {
                            osgViewer.updateSkyNodeAtmosphereParameters(
                                skyNodeTurbiditySlider.value, 
                                value, 
                                skyNodeMieCoefficientSlider.value, 
                                skyNodeMieDirectionalGSlider.value,
                                skyNodeSunZenithAngleSlider.value,
                                skyNodeSunAzimuthAngleSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "系数: " + skyNodeRayleighSlider.value.toFixed(2)  // 改为2位小数
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 米氏散射控制 (对应mieCoefficient)
                    Text {
                        text: "米氏散射 (Mie Coefficient)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyNodeMieCoefficientSlider
                        width: parent.width
                        from: 0.001
                        to: 0.1
                        value: 0.005
                        onValueChanged: {
                            osgViewer.updateSkyNodeAtmosphereParameters(
                                skyNodeTurbiditySlider.value, 
                                skyNodeRayleighSlider.value, 
                                value, 
                                skyNodeMieDirectionalGSlider.value,
                                skyNodeSunZenithAngleSlider.value,
                                skyNodeSunAzimuthAngleSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "系数: " + skyNodeMieCoefficientSlider.value.toFixed(3)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 米氏散射方向性控制 (对应mieDirectionalG)
                    Text {
                        text: "米氏散射方向性 (Mie Directional G)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyNodeMieDirectionalGSlider
                        width: parent.width
                        from: 0.0
                        to: 1.0
                        value: 0.8
                        onValueChanged: {
                            osgViewer.updateSkyNodeAtmosphereParameters(
                                skyNodeTurbiditySlider.value, 
                                skyNodeRayleighSlider.value, 
                                skyNodeMieCoefficientSlider.value, 
                                value,
                                skyNodeSunZenithAngleSlider.value,
                                skyNodeSunAzimuthAngleSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "方向性: " + skyNodeMieDirectionalGSlider.value.toFixed(2)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 太阳天顶角度控制
                    Text {
                        text: "太阳天顶角度"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyNodeSunZenithAngleSlider
                        width: parent.width
                        from: 0.0
                        to: 2 * Math.PI  // 改为2 * Math.PI，即360度
                        value: 85.0 * Math.PI / 180.0  // 初始化为75度
                        onValueChanged: {
                            osgViewer.updateSkyNodeAtmosphereParameters(
                                skyNodeTurbiditySlider.value, 
                                skyNodeRayleighSlider.value, 
                                skyNodeMieCoefficientSlider.value, 
                                skyNodeMieDirectionalGSlider.value,
                                value,
                                skyNodeSunAzimuthAngleSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "角度: " + (skyNodeSunZenithAngleSlider.value * 180 / Math.PI).toFixed(1) + "°"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 太阳方位角度控制
                    Text {
                        text: "太阳方位角度"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyNodeSunAzimuthAngleSlider
                        width: parent.width
                        from: 0.0
                        to: 2 * Math.PI
                        value: 270.0 * Math.PI / 180.0  // 初始化为40度
                        onValueChanged: {
                            osgViewer.updateSkyNodeAtmosphereParameters(
                                skyNodeTurbiditySlider.value, 
                                skyNodeRayleighSlider.value, 
                                skyNodeMieCoefficientSlider.value, 
                                skyNodeMieDirectionalGSlider.value,
                                skyNodeSunZenithAngleSlider.value,
                                value
                            );
                        }
                    }
                    
                    Text {
                        text: "角度: " + (skyNodeSunAzimuthAngleSlider.value * 180 / Math.PI).toFixed(1) + "°"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            // 云海大气控制组件
            Component {
                id: cloudSeaAtmosphereControls
                
                Column {
                    width: parent.width
                    spacing: 15
                    
                    // 太阳天顶角度控制
                    Text {
                        text: "太阳天顶角度"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudSeaSunZenithAngleSlider
                        width: parent.width
                        from: 0.0
                        to: 2 * Math.PI  // 改为2 * Math.PI，即360度
                        value: 75.0 * Math.PI / 180.0
                        stepSize: 0.01
                        onValueChanged: {
                            osgViewer.updateSkyNodeCloudParameters(
                                value,
                                cloudSeaSunAzimuthAngleSlider.value,
                                cloudSeaCloudDensitySlider.value,
                                cloudSeaCloudHeightSlider.value,
                                cloudSeaCloudBaseHeightSlider.value,
                                cloudSeaCloudRangeMinSlider.value,
                                cloudSeaCloudRangeMaxSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "角度: " + (cloudSeaSunZenithAngleSlider.value * 180 / Math.PI).toFixed(1) + "°"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 太阳方位角度控制
                    Text {
                        text: "太阳方位角度"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudSeaSunAzimuthAngleSlider
                        width: parent.width
                        from: 0.0
                        to: 2 * Math.PI
                        value: 40.0 * Math.PI / 180.0
                        stepSize: 0.01
                        onValueChanged: {
                            osgViewer.updateSkyNodeCloudParameters(
                                cloudSeaSunZenithAngleSlider.value,
                                value,
                                cloudSeaCloudDensitySlider.value,
                                cloudSeaCloudHeightSlider.value,
                                cloudSeaCloudBaseHeightSlider.value,
                                cloudSeaCloudRangeMinSlider.value,
                                cloudSeaCloudRangeMaxSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "角度: " + (cloudSeaSunAzimuthAngleSlider.value * 180 / Math.PI).toFixed(1) + "°"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云密度控制
                    Text {
                        text: "云密度 (Cloud Density)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudSeaCloudDensitySlider
                        width: parent.width
                        from: 0.0
                        to: 10.0
                        value: 5.0
                        stepSize: 0.1
                        onValueChanged: {
                            osgViewer.updateSkyNodeCloudParameters(
                                cloudSeaSunZenithAngleSlider.value,
                                cloudSeaSunAzimuthAngleSlider.value,
                                value,
                                cloudSeaCloudHeightSlider.value,
                                cloudSeaCloudBaseHeightSlider.value,
                                cloudSeaCloudRangeMinSlider.value,
                                cloudSeaCloudRangeMaxSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "密度: " + cloudSeaCloudDensitySlider.value.toFixed(2)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云高度控制
                    Text {
                        text: "云厚度 (Cloud Height)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudSeaCloudHeightSlider
                        width: parent.width
                        from: 100.0
                        to: 5000.0
                        value: 800.0
                        stepSize: 10.0
                        onValueChanged: {
                            osgViewer.updateSkyNodeCloudParameters(
                                cloudSeaSunZenithAngleSlider.value,
                                cloudSeaSunAzimuthAngleSlider.value,
                                cloudSeaCloudDensitySlider.value,
                                value,
                                cloudSeaCloudBaseHeightSlider.value,
                                cloudSeaCloudRangeMinSlider.value,
                                cloudSeaCloudRangeMaxSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "厚度: " + cloudSeaCloudHeightSlider.value.toFixed(0) + " m"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层底部高度控制
                    Text {
                        text: "云层底部高度 (Cloud Base Height)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudSeaCloudBaseHeightSlider
                        width: parent.width
                        from: 0.0
                        to: 10000.0
                        value: 1500.0
                        stepSize: 10.0
                        onValueChanged: {
                            osgViewer.updateSkyNodeCloudParameters(
                                cloudSeaSunZenithAngleSlider.value,
                                cloudSeaSunAzimuthAngleSlider.value,
                                cloudSeaCloudDensitySlider.value,
                                cloudSeaCloudHeightSlider.value,
                                value,
                                cloudSeaCloudRangeMinSlider.value,
                                cloudSeaCloudRangeMaxSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "底部高度: " + cloudSeaCloudBaseHeightSlider.value.toFixed(0) + " m"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层近裁剪距离控制
                    Text {
                        text: "云层近裁剪距离 (Cloud Range Min)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudSeaCloudRangeMinSlider
                        width: parent.width
                        from: 0.0
                        to: 10000.0
                        value: 0.0
                        stepSize: 10.0
                        onValueChanged: {
                            osgViewer.updateSkyNodeCloudParameters(
                                cloudSeaSunZenithAngleSlider.value,
                                cloudSeaSunAzimuthAngleSlider.value,
                                cloudSeaCloudDensitySlider.value,
                                cloudSeaCloudHeightSlider.value,
                                cloudSeaCloudBaseHeightSlider.value,
                                value,
                                cloudSeaCloudRangeMaxSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "近裁剪: " + cloudSeaCloudRangeMinSlider.value.toFixed(0) + " m"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云层远裁剪距离控制
                    Text {
                        text: "云层远裁剪距离 (Cloud Range Max)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: cloudSeaCloudRangeMaxSlider
                        width: parent.width
                        from: 10000.0
                        to: 100000.0
                        value: 50000.0
                        stepSize: 100.0
                        onValueChanged: {
                            osgViewer.updateSkyNodeCloudParameters(
                                cloudSeaSunZenithAngleSlider.value,
                                cloudSeaSunAzimuthAngleSlider.value,
                                cloudSeaCloudDensitySlider.value,
                                cloudSeaCloudHeightSlider.value,
                                cloudSeaCloudBaseHeightSlider.value,
                                cloudSeaCloudRangeMinSlider.value,
                                value
                            );
                        }
                    }
                    
                    Text {
                        text: "远裁剪: " + cloudSeaCloudRangeMaxSlider.value.toFixed(0) + " m"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            // 物理控制组件（用于控制SkyCloud参数）
            Component {
                id: physicsControls
                
                Column {
                    width: parent.width
                    spacing: 15
                    
                    // 标题
                    Text {
                        text: "SkyCloud参数控制"
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
                    
                    // 云密度控制
                    Text {
                        text: "云密度 (Cloud Density)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyCloudDensitySlider
                        width: parent.width
                        from: 0.0
                        to: 200.0
                        value: 100.0
                        stepSize: 1.0
                        onValueChanged: {
                            osgViewer.updateSkyCloudParameters(
                                value,
                                skyCloudHeightSlider.value,
                                skyCloudCoverageThresholdSlider.value,
                                skyCloudDensityThresholdSlider.value,
                                skyCloudEdgeThresholdSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "密度: " + skyCloudDensitySlider.value.toFixed(2)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 云高度控制
                    Text {
                        text: "云高度 (Cloud Height)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyCloudHeightSlider
                        width: parent.width
                        from: 100.0
                        to: 5000.0
                        value: 1300.0
                        stepSize: 10.0
                        onValueChanged: {
                            osgViewer.updateSkyCloudParameters(
                                skyCloudDensitySlider.value,
                                value,
                                skyCloudCoverageThresholdSlider.value,
                                skyCloudDensityThresholdSlider.value,
                                skyCloudEdgeThresholdSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "高度: " + skyCloudHeightSlider.value.toFixed(0) + " m"
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 覆盖率阈值控制
                    Text {
                        text: "覆盖率阈值 (Coverage Threshold)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyCloudCoverageThresholdSlider
                        width: parent.width
                        from: 0.0
                        to: 1.0
                        value: 0.4
                        stepSize: 0.01
                        onValueChanged: {
                            osgViewer.updateSkyCloudParameters(
                                skyCloudDensitySlider.value,
                                skyCloudHeightSlider.value,
                                value,
                                skyCloudDensityThresholdSlider.value,
                                skyCloudEdgeThresholdSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "覆盖率: " + skyCloudCoverageThresholdSlider.value.toFixed(2)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 密度阈值控制
                    Text {
                        text: "密度阈值 (Density Threshold)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyCloudDensityThresholdSlider
                        width: parent.width
                        from: 0.0
                        to: 1.0
                        value: 0.07
                        stepSize: 0.01
                        onValueChanged: {
                            osgViewer.updateSkyCloudParameters(
                                skyCloudDensitySlider.value,
                                skyCloudHeightSlider.value,
                                skyCloudCoverageThresholdSlider.value,
                                value,
                                skyCloudEdgeThresholdSlider.value
                            );
                        }
                    }
                    
                    Text {
                        text: "密度阈值: " + skyCloudDensityThresholdSlider.value.toFixed(2)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 边缘阈值控制
                    Text {
                        text: "边缘阈值 (Edge Threshold)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    Slider {
                        id: skyCloudEdgeThresholdSlider
                        width: parent.width
                        from: 0.0
                        to: 0.1
                        value: 0.04
                        stepSize: 0.001
                        onValueChanged: {
                            osgViewer.updateSkyCloudParameters(
                                skyCloudDensitySlider.value,
                                skyCloudHeightSlider.value,
                                skyCloudCoverageThresholdSlider.value,
                                skyCloudDensityThresholdSlider.value,
                                value
                            );
                        }
                    }
                    
                    Text {
                        text: "边缘阈值: " + skyCloudEdgeThresholdSlider.value.toFixed(3)
                        font.pixelSize: 12
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#bdc3c7"
                    }
                    
                    // 调试按钮
                    Text {
                        text: "调试功能"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#34495e"
                    }
                    
                    // 重置按钮
                    Button {
                        text: "重置参数"
                        width: parent.width
                        onClicked: {
                            skyCloudDensitySlider.value = 100.0;
                            skyCloudHeightSlider.value = 1300.0;
                            skyCloudCoverageThresholdSlider.value = 0.4;
                            skyCloudDensityThresholdSlider.value = 0.07;
                            skyCloudEdgeThresholdSlider.value = 0.04;
                        }
                    }
                }
            }
            
            // 默认控制组件
            Component {
                id: defaultControls
                
                Column {
                    width: parent.width
                    spacing: 15
                    
                    Text {
                        text: "请选择一个功能选项"
                        font.pixelSize: 14
                        color: "#7f8c8d"
                        anchors.horizontalCenter: parent.horizontalCenter
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
                top: parent.top
                bottom: parent.bottom
            }
            
            // OSG视图区域 - 填充整个可用空间
            SimpleOSGViewer {
                id: osgViewer
                anchors.fill: parent
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
            // 通过window的属性访问filePath（不能直接访问filePathInput，因为不在同一作用域）
            window.filePath = selectedFile;
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
    
    // 添加更新云密度的函数
    function updateCloudDensity(value) {
        osgViewer.updateVolumeCloudParameters(
            sunZenithAngle, sunAzimuthAngle,
            cloudDensitySlider.value, cloudHeightSlider.value,
            densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
            stepSizeSlider.value, maxStepsSlider.value
        );
    }
    
    // 添加更新云高度的函数
    function updateCloudHeight(value) {
        osgViewer.updateVolumeCloudParameters(
            sunZenithAngle, sunAzimuthAngle,
            cloudDensitySlider.value, cloudHeightSlider.value,
            densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
            stepSizeSlider.value, maxStepsSlider.value
        );
    }
    
    // 添加更新云层密度阈值的函数
    function updateCloudDensityThreshold(value) {
        osgViewer.updateVolumeCloudParameters(
            sunZenithAngle, sunAzimuthAngle,
            cloudDensitySlider.value, cloudHeightSlider.value,
            densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
            stepSizeSlider.value, maxStepsSlider.value
        );
    }
    
    // 添加更新云层对比度的函数
    function updateCloudContrast(value) {
        osgViewer.updateVolumeCloudParameters(
            sunZenithAngle, sunAzimuthAngle,
            cloudDensitySlider.value, cloudHeightSlider.value,
            densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
            stepSizeSlider.value, maxStepsSlider.value
        );
    }
    
    // 添加更新云层密度因子的函数
    function updateCloudDensityFactor(value) {
        osgViewer.updateVolumeCloudParameters(
            sunZenithAngle, sunAzimuthAngle,
            cloudDensitySlider.value, cloudHeightSlider.value,
            densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
            stepSizeSlider.value, maxStepsSlider.value
        );
    }
    
    // 添加更新云层步长的函数
    function updateCloudStepSize(value) {
        osgViewer.updateVolumeCloudParameters(
            sunZenithAngle, sunAzimuthAngle,
            cloudDensitySlider.value, cloudHeightSlider.value,
            densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
            stepSizeSlider.value, maxStepsSlider.value
        );
    }
    
    // 添加更新云层最大步数的函数
    function updateCloudMaxSteps(value) {
        osgViewer.updateVolumeCloudParameters(
            sunZenithAngle, sunAzimuthAngle,
            cloudDensitySlider.value, cloudHeightSlider.value,
            densityThresholdSlider.value, contrastSlider.value, densityFactorSlider.value,
            stepSizeSlider.value, maxStepsSlider.value
        );
    }
}