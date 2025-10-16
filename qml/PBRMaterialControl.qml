import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: pbrControl
    
    // 属性定义
    property real albedoR: 1.0
    property real albedoG: 1.0
    property real albedoB: 1.0
    property real metallic: 0.0
    property real roughness: 0.5
    property real specular: 0.5
    property real ao: 1.0
    
    // 定义信号，当材质属性改变时发出
    signal materialPropertiesChanged(real albedoR, real albedoG, real albedoB, 
                                   real metallic, real roughness, 
                                   real specular, real ao)
    
    // 当任何属性改变时发出信号
    onAlbedoRChanged: {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onAlbedoGChanged: {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onAlbedoBChanged: {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onMetallicChanged: {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onRoughnessChanged: {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onSpecularChanged: {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onAoChanged: {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    
    // 控件填充父元素
    anchors.fill: parent
    
    // 滚动视图以适应所有内容
    ScrollView {
        anchors.fill: parent
        clip: true
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 15
            
            // 基础颜色控制 (RGB) - 横向布局
            GroupBox {
                title: "基础颜色 (Albedo)"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    rowSpacing: 10
                    columnSpacing: 10
                    
                    // R通道
                    Text { 
                        text: "R:" 
                        Layout.alignment: Qt.AlignRight
                    }
                    Slider {
                        id: albedoRSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.albedoR
                        onValueChanged: pbrControl.albedoR = value
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (albedoRSlider.value).toFixed(2)
                        Layout.preferredWidth: 40
                    }
                    
                    // G通道
                    Text { 
                        text: "G:" 
                        Layout.alignment: Qt.AlignRight
                    }
                    Slider {
                        id: albedoGSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.albedoG
                        onValueChanged: pbrControl.albedoG = value
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (albedoGSlider.value).toFixed(2)
                        Layout.preferredWidth: 40
                    }
                    
                    // B通道
                    Text { 
                        text: "B:" 
                        Layout.alignment: Qt.AlignRight
                    }
                    Slider {
                        id: albedoBSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.albedoB
                        onValueChanged: pbrControl.albedoB = value
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (albedoBSlider.value).toFixed(2)
                        Layout.preferredWidth: 40
                    }
                }
            }
            
            // PBR属性控制 - 横向布局
            GroupBox {
                title: "PBR属性"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    rowSpacing: 10
                    columnSpacing: 10
                    
                    // 金属度
                    Text { 
                        text: "金属度:" 
                        Layout.alignment: Qt.AlignRight
                    }
                    Slider {
                        id: metallicSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.metallic
                        onValueChanged: pbrControl.metallic = value
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (metallicSlider.value).toFixed(2)
                        Layout.preferredWidth: 40
                    }
                    
                    // 粗糙度
                    Text { 
                        text: "粗糙度:" 
                        Layout.alignment: Qt.AlignRight
                    }
                    Slider {
                        id: roughnessSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.roughness
                        onValueChanged: pbrControl.roughness = value
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (roughnessSlider.value).toFixed(2)
                        Layout.preferredWidth: 40
                    }
                    
                    // 镜面反射
                    Text { 
                        text: "镜面反射:" 
                        Layout.alignment: Qt.AlignRight
                    }
                    Slider {
                        id: specularSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.specular
                        onValueChanged: pbrControl.specular = value
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (specularSlider.value).toFixed(2)
                        Layout.preferredWidth: 40
                    }
                    
                    // 环境光遮蔽
                    Text { 
                        text: "环境光遮蔽:" 
                        Layout.alignment: Qt.AlignRight
                    }
                    Slider {
                        id: aoSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.ao
                        onValueChanged: pbrControl.ao = value
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (aoSlider.value).toFixed(2)
                        Layout.preferredWidth: 40
                    }
                }
            }
            
            // 颜色预览
            GroupBox {
                title: "颜色预览"
                Layout.fillWidth: true
                
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    color: Qt.rgba(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, 1.0)
                    border.color: "#cccccc"
                    border.width: 1
                    radius: 4
                    
                    Text {
                        anchors.centerIn: parent
                        text: "RGB(" + 
                              Math.round(pbrControl.albedoR * 255) + ", " + 
                              Math.round(pbrControl.albedoG * 255) + ", " + 
                              Math.round(pbrControl.albedoB * 255) + ")"
                        color: (pbrControl.albedoR * 0.299 + pbrControl.albedoG * 0.587 + pbrControl.albedoB * 0.114) > 0.5 ? "black" : "white"
                    }
                }
            }
            
            Item {
                Layout.fillHeight: true
            }
        }
    }
}