import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: colorPicker
    
    // 属性
    property color selectedColor: "#ff0000"
    property real hue: 0.0
    property real saturation: 1.0
    property real brightness: 1.0
    property real alpha: 1.0
    
    // 信号
    signal colorChanged(color newColor)
    
    width: 300
    height: 550
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 15
        
        // 标题
        Text {
            text: "颜色选择器"
            font.bold: true
            color: "#333333"
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 16
        }
        
        // HSV调色板
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            Layout.alignment: Qt.AlignHCenter
            
            // 饱和度-亮度选择器
            Rectangle {
                id: svSelector
                width: 200
                height: 200
                anchors.centerIn: parent
                color: "transparent"
                
                // 使用Canvas绘制饱和度-亮度选择器
                Canvas {
                    id: svCanvas
                    anchors.fill: parent
                    
                    onPaint: {
                        var ctx = getContext("2d");
                        var width = svCanvas.width;
                        var height = svCanvas.height;
                        
                        // 绘制饱和度-亮度渐变
                        // 基于当前色调创建渐变
                        var baseColor = Qt.hsva(colorPicker.hue, 1.0, 1.0, 1.0);
                        
                        // 创建从白色到基色的渐变（水平方向）
                        var whiteToColor = ctx.createLinearGradient(0, 0, width, 0);
                        whiteToColor.addColorStop(0, "white");
                        whiteToColor.addColorStop(1, baseColor);
                        
                        // 创建从透明到黑色的渐变（垂直方向）
                        var transparentToBlack = ctx.createLinearGradient(0, 0, 0, height);
                        transparentToBlack.addColorStop(0, "transparent");
                        transparentToBlack.addColorStop(1, "black");
                        
                        // 先绘制水平渐变
                        ctx.fillStyle = whiteToColor;
                        ctx.fillRect(0, 0, width, height);
                        
                        // 再绘制垂直渐变（混合模式）
                        ctx.globalCompositeOperation = "multiply";
                        ctx.fillStyle = transparentToBlack;
                        ctx.fillRect(0, 0, width, height);
                        
                        // 重置混合模式
                        ctx.globalCompositeOperation = "source-over";
                    }
                }
                
                // SV选择器点
                Rectangle {
                    id: svIndicator
                    width: 20
                    height: 20
                    radius: 10
                    border.color: "white"
                    border.width: 2
                    color: "transparent"
                    
                    // 使用锚点定位而不是直接设置x,y，提高响应性
                    x: colorPicker.saturation * (svSelector.width - width)
                    y: (1 - colorPicker.brightness) * (svSelector.height - height)
                    
                    // 添加边框阴影效果
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.color: "black"
                        border.width: 1
                        opacity: 0.5
                        scale: 1.2
                    }
                }
                
                // 饱和度-亮度选择区域
                MouseArea {
                    anchors.fill: parent
                    // 增加点击区域
                    anchors.margins: -20
                    
                    onPressed: function() {
                        handleSV(mouseX, mouseY);
                    }
                    
                    onPositionChanged: function() {
                        if (pressed) {
                            handleSV(mouseX, mouseY);
                        }
                    }
                    
                    function handleSV(x, y) {
                        // 限制在区域内
                        var posX = Math.max(0, Math.min(x, svSelector.width));
                        var posY = Math.max(0, Math.min(y, svSelector.height));
                        
                        // 计算饱和度和亮度
                        colorPicker.saturation = posX / svSelector.width;
                        colorPicker.brightness = 1 - (posY / svSelector.height);
                        
                        // 更新颜色
                        updateColor();
                    }
                }
            }
        }
        
        // 色调选择器
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.alignment: Qt.AlignHCenter
            
            // 色调条
            Rectangle {
                id: hueBar
                width: 200
                height: 20
                anchors.centerIn: parent
                color: "transparent"
                
                Canvas {
                    id: hueCanvas
                    anchors.fill: parent
                    
                    onPaint: {
                        var ctx = getContext("2d");
                        var width = hueCanvas.width;
                        var height = hueCanvas.height;
                        
                        // 创建色调渐变
                        var gradient = ctx.createLinearGradient(0, 0, width, 0);
                        for (var i = 0; i <= 6; i++) {
                            var hue = i / 6;
                            gradient.addColorStop(i / 6, Qt.hsva(hue, 1.0, 1.0, 1.0));
                        }
                        
                        ctx.fillStyle = gradient;
                        ctx.fillRect(0, 0, width, height);
                    }
                }
                
                // 色调指示器
                Rectangle {
                    id: hueIndicator
                    width: 12
                    height: 28
                    y: -4
                    color: "transparent"
                    border.color: "white"
                    border.width: 2
                    
                    // 使用锚点定位
                    x: colorPicker.hue * (hueBar.width - width)
                    
                    // 添加边框阴影效果
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: "black"
                        border.width: 1
                        opacity: 0.5
                        scale: 1.2
                    }
                }
                
                // 色调选择区域
                MouseArea {
                    anchors.fill: parent
                    // 增加点击区域
                    anchors.margins: -15
                    
                    onPressed: function() {
                        handleHue(mouseX);
                    }
                    
                    onPositionChanged: function() {
                        if (pressed) {
                            handleHue(mouseX);
                        }
                    }
                    
                    function handleHue(x) {
                        // 限制在区域内
                        var posX = Math.max(0, Math.min(x, hueBar.width));
                        
                        // 计算色调
                        colorPicker.hue = posX / hueBar.width;
                        
                        // 更新颜色
                        updateColor();
                        
                        // 重新绘制SV画布
                        svCanvas.requestPaint();
                    }
                }
            }
        }
        
        // 透明度选择器
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.alignment: Qt.AlignHCenter
            
            // 透明度条
            Rectangle {
                id: alphaBar
                width: 200
                height: 20
                anchors.centerIn: parent
                color: "transparent"
                
                Canvas {
                    id: alphaCanvas
                    anchors.fill: parent
                    
                    onPaint: {
                        var ctx = getContext("2d");
                        var width = alphaCanvas.width;
                        var height = alphaCanvas.height;
                        
                        // 创建棋盘背景（模拟透明效果）
                        ctx.fillStyle = "white";
                        ctx.fillRect(0, 0, width, height);
                        
                        // 绘制灰色棋盘
                        ctx.fillStyle = "#cccccc";
                        for (var x = 0; x < width; x += 10) {
                            for (var y = 0; y < height; y += 10) {
                                if ((x/10 + y/10) % 2 === 0) {
                                    ctx.fillRect(x, y, 10, 10);
                                }
                            }
                        }
                        
                        // 创建透明度渐变
                        var gradient = ctx.createLinearGradient(0, 0, width, 0);
                        gradient.addColorStop(0, "transparent");
                        gradient.addColorStop(1, Qt.hsva(colorPicker.hue, colorPicker.saturation, colorPicker.brightness, 1.0));
                        
                        ctx.fillStyle = gradient;
                        ctx.fillRect(0, 0, width, height);
                    }
                }
                
                // 透明度指示器
                Rectangle {
                    id: alphaIndicator
                    width: 12
                    height: 28
                    y: -4
                    color: "transparent"
                    border.color: "white"
                    border.width: 2
                    
                    // 使用锚点定位
                    x: colorPicker.alpha * (alphaBar.width - width)
                    
                    // 添加边框阴影效果
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: "black"
                        border.width: 1
                        opacity: 0.5
                        scale: 1.2
                    }
                }
                
                // 透明度选择区域
                MouseArea {
                    anchors.fill: parent
                    // 增加点击区域
                    anchors.margins: -15
                    
                    onPressed: function() {
                        handleAlpha(mouseX);
                    }
                    
                    onPositionChanged: function() {
                        if (pressed) {
                            handleAlpha(mouseX);
                        }
                    }
                    
                    function handleAlpha(x) {
                        // 限制在区域内
                        var posX = Math.max(0, Math.min(x, alphaBar.width));
                        
                        // 计算透明度
                        colorPicker.alpha = posX / alphaBar.width;
                        
                        // 更新颜色
                        updateColor();
                    }
                }
            }
        }
        
        // 直接RGB输入
        GroupBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            title: "直接输入RGB值"
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                Text { text: "R:"; color: "#666666" }
                TextField {
                    id: redInput
                    Layout.fillWidth: true
                    text: Math.round(colorPicker.selectedColor.r * 255)
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 0; top: 255 }
                    onTextChanged: function() {
                        if (text !== "" && !internalUpdate) {
                            var r = parseInt(text) / 255.0;
                            var newColor = Qt.rgba(r, colorPicker.selectedColor.g, colorPicker.selectedColor.b, colorPicker.alpha);
                            updateFromRgb(newColor);
                        }
                    }
                }
                
                Text { text: "G:"; color: "#666666" }
                TextField {
                    id: greenInput
                    Layout.fillWidth: true
                    text: Math.round(colorPicker.selectedColor.g * 255)
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 0; top: 255 }
                    onTextChanged: function() {
                        if (text !== "" && !internalUpdate) {
                            var g = parseInt(text) / 255.0;
                            var newColor = Qt.rgba(colorPicker.selectedColor.r, g, colorPicker.selectedColor.b, colorPicker.alpha);
                            updateFromRgb(newColor);
                        }
                    }
                }
                
                Text { text: "B:"; color: "#666666" }
                TextField {
                    id: blueInput
                    Layout.fillWidth: true
                    text: Math.round(colorPicker.selectedColor.b * 255)
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 0; top: 255 }
                    onTextChanged: function() {
                        if (text !== "" && !internalUpdate) {
                            var b = parseInt(text) / 255.0;
                            var newColor = Qt.rgba(colorPicker.selectedColor.r, colorPicker.selectedColor.g, b, colorPicker.alpha);
                            updateFromRgb(newColor);
                        }
                    }
                }
            }
        }
        
        // RGBA数值显示
        GridLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 80
            columns: 4
            rowSpacing: 8
            columnSpacing: 8
            
            Text { text: "R:"; color: "#666666" }
            SpinBox {
                id: redSpinBox
                Layout.fillWidth: true
                from: 0
                to: 255
                value: Math.round(colorPicker.selectedColor.r * 255)
                onValueChanged: function() {
                    if (!internalUpdate) {
                        var newColor = Qt.rgba(value / 255, colorPicker.selectedColor.g, colorPicker.selectedColor.b, colorPicker.alpha);
                        updateFromRgb(newColor);
                    }
                }
            }
            Text { text: "G:"; color: "#666666" }
            SpinBox {
                id: greenSpinBox
                Layout.fillWidth: true
                from: 0
                to: 255
                value: Math.round(colorPicker.selectedColor.g * 255)
                onValueChanged: function() {
                    if (!internalUpdate) {
                        var newColor = Qt.rgba(colorPicker.selectedColor.r, value / 255, colorPicker.selectedColor.b, colorPicker.alpha);
                        updateFromRgb(newColor);
                    }
                }
            }
            Text { text: "B:"; color: "#666666" }
            SpinBox {
                id: blueSpinBox
                Layout.fillWidth: true
                from: 0
                to: 255
                value: Math.round(colorPicker.selectedColor.b * 255)
                onValueChanged: function() {
                    if (!internalUpdate) {
                        var newColor = Qt.rgba(colorPicker.selectedColor.r, colorPicker.selectedColor.g, value / 255, colorPicker.alpha);
                        updateFromRgb(newColor);
                    }
                }
            }
            Text { text: "A:"; color: "#666666" }
            SpinBox {
                id: alphaSpinBox
                Layout.fillWidth: true
                from: 0
                to: 255
                value: Math.round(colorPicker.alpha * 255)
                onValueChanged: function() {
                    if (!internalUpdate) {
                        colorPicker.alpha = value / 255;
                        updateColor();
                    }
                }
            }
        }
        
        // RGBA值文本显示
        Text {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 30
            text: "RGBA: " + 
                  Math.round(colorPicker.selectedColor.r * 255) + ", " + 
                  Math.round(colorPicker.selectedColor.g * 255) + ", " + 
                  Math.round(colorPicker.selectedColor.b * 255) + ", " + 
                  Math.round(colorPicker.alpha * 255)
            color: "#333333"
            font.bold: true
            font.pixelSize: 14
        }
        
        // 颜色预览和十六进制值
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 50
            spacing: 10
            
            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 40
                radius: 8
                border.color: "#cccccc"
                border.width: 1
                
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "white" }
                    GradientStop { position: 1.0; color: "lightgray" }
                }
                
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: 6
                    color: colorPicker.selectedColor
                    border.color: "#999999"
                    border.width: 1
                }
            }
            
            Text {
                Layout.fillWidth: true
                text: "HEX: #" + 
                      (Math.round(colorPicker.selectedColor.r * 255)).toString(16).padStart(2, '0').toUpperCase() +
                      (Math.round(colorPicker.selectedColor.g * 255)).toString(16).padStart(2, '0').toUpperCase() +
                      (Math.round(colorPicker.selectedColor.b * 255)).toString(16).padStart(2, '0').toUpperCase()
                color: "#333333"
                font.bold: true
                font.pixelSize: 14
            }
        }
    }
    
    // 内部更新标志，防止循环更新
    property bool internalUpdate: false
    
    // 更新颜色
    function updateColor() {
        internalUpdate = true;
        
        // 计算最终颜色
        colorPicker.selectedColor = Qt.hsva(colorPicker.hue, colorPicker.saturation, colorPicker.brightness, colorPicker.alpha);
        
        // 触发颜色变化信号
        colorChanged(colorPicker.selectedColor);
        
        // 更新输入框和SpinBox值
        redInput.text = Math.round(colorPicker.selectedColor.r * 255).toString();
        greenInput.text = Math.round(colorPicker.selectedColor.g * 255).toString();
        blueInput.text = Math.round(colorPicker.selectedColor.b * 255).toString();
        redSpinBox.value = Math.round(colorPicker.selectedColor.r * 255);
        greenSpinBox.value = Math.round(colorPicker.selectedColor.g * 255);
        blueSpinBox.value = Math.round(colorPicker.selectedColor.b * 255);
        alphaSpinBox.value = Math.round(colorPicker.alpha * 255);
        
        internalUpdate = false;
    }
    
    // 从RGB更新HSV
    function updateFromRgb(newColor) {
        internalUpdate = true;
        
        // 转换RGB到HSV
        var r = newColor.r;
        var g = newColor.g;
        var b = newColor.b;
        
        var max = Math.max(r, g, b);
        var min = Math.min(r, g, b);
        var delta = max - min;
        
        // 计算亮度
        colorPicker.brightness = max;
        
        // 计算饱和度
        colorPicker.saturation = max === 0 ? 0 : delta / max;
        
        // 计算色调
        if (delta === 0) {
            colorPicker.hue = 0;
        } else {
            if (max === r) {
                colorPicker.hue = (((g - b) / delta) % 6) / 6;
            } else if (max === g) {
                colorPicker.hue = (((b - r) / delta) + 2) / 6;
            } else {
                colorPicker.hue = (((r - g) / delta) + 4) / 6;
            }
        }
        
        // 更新alpha
        colorPicker.alpha = newColor.a;
        
        // 更新颜色
        colorPicker.selectedColor = newColor;
        
        // 触发颜色变化信号
        colorChanged(colorPicker.selectedColor);
        
        // 更新控件位置
        svIndicator.x = colorPicker.saturation * (svSelector.width - svIndicator.width);
        svIndicator.y = (1 - colorPicker.brightness) * (svSelector.height - svIndicator.height);
        hueIndicator.x = colorPicker.hue * (hueBar.width - hueIndicator.width);
        alphaIndicator.x = colorPicker.alpha * (alphaBar.width - alphaIndicator.width);
        
        // 更新输入框和SpinBox值
        redInput.text = Math.round(colorPicker.selectedColor.r * 255).toString();
        greenInput.text = Math.round(colorPicker.selectedColor.g * 255).toString();
        blueInput.text = Math.round(colorPicker.selectedColor.b * 255).toString();
        redSpinBox.value = Math.round(colorPicker.selectedColor.r * 255);
        greenSpinBox.value = Math.round(colorPicker.selectedColor.g * 255);
        blueSpinBox.value = Math.round(colorPicker.selectedColor.b * 255);
        alphaSpinBox.value = Math.round(colorPicker.alpha * 255);
        
        // 重新绘制画布
        svCanvas.requestPaint();
        alphaCanvas.requestPaint();
        
        internalUpdate = false;
    }
    
    // 监听selectedColor变化
    onSelectedColorChanged: {
        if (!internalUpdate) {
            internalUpdate = true;
            redInput.text = Math.round(selectedColor.r * 255).toString();
            greenInput.text = Math.round(selectedColor.g * 255).toString();
            blueInput.text = Math.round(selectedColor.b * 255).toString();
            redSpinBox.value = Math.round(selectedColor.r * 255);
            greenSpinBox.value = Math.round(selectedColor.g * 255);
            blueSpinBox.value = Math.round(selectedColor.b * 255);
            alphaSpinBox.value = Math.round(alpha * 255);
            internalUpdate = false;
        }
    }
    
    Component.onCompleted: {
        // 初始化输入框和SpinBox值
        redInput.text = Math.round(selectedColor.r * 255).toString();
        greenInput.text = Math.round(selectedColor.g * 255).toString();
        blueInput.text = Math.round(selectedColor.b * 255).toString();
        redSpinBox.value = Math.round(selectedColor.r * 255);
        greenSpinBox.value = Math.round(selectedColor.g * 255);
        blueSpinBox.value = Math.round(selectedColor.b * 255);
        alphaSpinBox.value = Math.round(alpha * 255);
    }
}