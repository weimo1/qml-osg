import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: pbrControl
    
    // 属性定义
    property real albedoR: 1.0
    property real albedoG: 1.0
    property real albedoB: 1.0
    property real albedoA: 1.0
    property real metallic: 0.0
    property real roughness: 0.5
    property real specular: 0.5
    property real ao: 1.0
    
    // 定义信号，当材质属性改变时发出
    signal materialPropertiesChanged(real albedoR, real albedoG, real albedoB, real albedoA,
                                   real metallic, real roughness, 
                                   real specular, real ao)
    
    // 当任何属性改变时发出信号
    onAlbedoRChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onAlbedoGChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onAlbedoBChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onAlbedoAChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onMetallicChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onRoughnessChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onSpecularChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
                                pbrControl.metallic, pbrControl.roughness, 
                                pbrControl.specular, pbrControl.ao);
    }
    onAoChanged: function() {
        materialPropertiesChanged(pbrControl.albedoR, pbrControl.albedoG, pbrControl.albedoB, pbrControl.albedoA,
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
            spacing: 20
            
            // 基础颜色控制 - 使用圆形色盘
            GroupBox {
                title: "基础颜色 (Albedo)"
                Layout.fillWidth: true
                Layout.preferredHeight: 600  // 增加高度为新增的RGB输入框留出空间
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 15
                    
                    // 圆形色盘颜色选择器
                    ColorPicker {
                        id: colorPicker
                        Layout.fillWidth: true
                        Layout.preferredHeight: 550  // 增加高度
                        Layout.alignment: Qt.AlignHCenter
                        
                        onColorChanged: function(newColor) {
                            // 将选中的颜色转换为RGB并更新属性
                            pbrControl.albedoR = newColor.r;
                            pbrControl.albedoG = newColor.g;
                            pbrControl.albedoB = newColor.b;
                            pbrControl.albedoA = newColor.a;
                        }
                    }
                }
            }
            
            // PBR属性控制 - 横向布局
            GroupBox {

                
                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    columns: 3
                    rowSpacing: 15
                    columnSpacing: 15
                    
                    // 金属度
                    Text { 
                        text: "金属度:" 
                        Layout.alignment: Qt.AlignRight
                        font.pixelSize: 14
                    }
                    Slider {
                        id: metallicSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.metallic
                        onValueChanged: function() { pbrControl.metallic = metallicSlider.value; }
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (metallicSlider.value).toFixed(2)
                        Layout.preferredWidth: 50
                        font.pixelSize: 14
                    }
                    
                    // 粗糙度
                    Text { 
                        text: "粗糙度:" 
                        Layout.alignment: Qt.AlignRight
                        font.pixelSize: 14
                    }
                    Slider {
                        id: roughnessSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.roughness
                        onValueChanged: function() { pbrControl.roughness = roughnessSlider.value; }
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (roughnessSlider.value).toFixed(2)
                        Layout.preferredWidth: 50
                        font.pixelSize: 14
                    }
                    
                    // 镜面反射
                    Text { 
                        text: "镜面反射:" 
                        Layout.alignment: Qt.AlignRight
                        font.pixelSize: 14
                    }
                    Slider {
                        id: specularSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.specular
                        onValueChanged: function() { pbrControl.specular = specularSlider.value; }
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (specularSlider.value).toFixed(2)
                        Layout.preferredWidth: 50
                        font.pixelSize: 14
                    }
                    
                    // 环境光遮蔽
                    Text { 
                        text: "环境光遮蔽:" 
                        Layout.alignment: Qt.AlignRight
                        font.pixelSize: 14
                    }
                    Slider {
                        id: aoSlider
                        from: 0.0
                        to: 1.0
                        stepSize: 0.01
                        value: pbrControl.ao
                        onValueChanged: function() { pbrControl.ao = aoSlider.value; }
                        Layout.fillWidth: true
                    }
                    Text { 
                        text: (aoSlider.value).toFixed(2)
                        Layout.preferredWidth: 50
                        font.pixelSize: 14
                    }
                }
            }
            
            Item {
                Layout.fillHeight: true
            }
        }
    }
}