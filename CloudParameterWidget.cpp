#include "CloudParameterWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QPushButton>

CloudParameterWidget::CloudParameterWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("云参数控制");
    resize(400, 600);
    
    createControls();
}

void CloudParameterWidget::createControls()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QLabel* title = new QLabel("云参数控制", this);
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);
    
    // 创建云形状参数组
    QGroupBox* shapeGroup = new QGroupBox("云形状参数", this);
    QGridLayout* shapeLayout = new QGridLayout(shapeGroup);
    
    // 云形状缩放
    m_shapeScaleLabel = new QLabel("云形状缩放: 0.0003", shapeGroup);
    shapeLayout->addWidget(m_shapeScaleLabel, 0, 0);
    
    m_shapeScaleSlider = new QSlider(Qt::Horizontal, shapeGroup);
    m_shapeScaleSlider->setRange(1, 100000);  // 映射到0.00001 - 1.0
    m_shapeScaleSlider->setValue(30);  // 0.0003
    shapeLayout->addWidget(m_shapeScaleSlider, 0, 1);
    
    m_shapeScaleSpinBox = new QDoubleSpinBox(shapeGroup);
    m_shapeScaleSpinBox->setRange(0.00001, 1.0);
    m_shapeScaleSpinBox->setValue(0.0003);
    m_shapeScaleSpinBox->setSingleStep(0.00001);
    m_shapeScaleSpinBox->setDecimals(5);
    shapeLayout->addWidget(m_shapeScaleSpinBox, 0, 2);
    
    // 云细节缩放
    m_detailScaleLabel = new QLabel("云细节缩放: 0.00100", shapeGroup);
    shapeLayout->addWidget(m_detailScaleLabel, 1, 0);
    
    m_detailScaleSlider = new QSlider(Qt::Horizontal, shapeGroup);
    m_detailScaleSlider->setRange(1, 10000);  // 映射到0.0001 - 1.0
    m_detailScaleSlider->setValue(10);  // 0.001
    shapeLayout->addWidget(m_detailScaleSlider, 1, 1);
    
    m_detailScaleSpinBox = new QDoubleSpinBox(shapeGroup);
    m_detailScaleSpinBox->setRange(0.00001, 1.0);
    m_detailScaleSpinBox->setValue(0.001);
    m_detailScaleSpinBox->setSingleStep(0.0001);
    m_detailScaleSpinBox->setDecimals(5);
    shapeLayout->addWidget(m_detailScaleSpinBox, 1, 2);
    
    mainLayout->addWidget(shapeGroup);
    
    // 创建风参数组
    QGroupBox* windGroup = new QGroupBox("风参数", this);
    QGridLayout* windLayout = new QGridLayout(windGroup);
    
    // 风速
    m_windSpeedLabel = new QLabel("风速: 10.00", windGroup);
    windLayout->addWidget(m_windSpeedLabel, 0, 0);
    
    m_windSpeedSlider = new QSlider(Qt::Horizontal, windGroup);
    m_windSpeedSlider->setRange(0, 500);  // 映射到0.0 - 50.0
    m_windSpeedSlider->setValue(100);  // 10.0
    windLayout->addWidget(m_windSpeedSlider, 0, 1);
    
    m_windSpeedSpinBox = new QDoubleSpinBox(windGroup);
    m_windSpeedSpinBox->setRange(0.0, 50.0);
    m_windSpeedSpinBox->setValue(10.0);
    m_windSpeedSpinBox->setSingleStep(0.1);
    m_windSpeedSpinBox->setDecimals(2);
    windLayout->addWidget(m_windSpeedSpinBox, 0, 2);
    
    // 风向X
    m_windDirXLabel = new QLabel("风向X: 1.00", windGroup);
    windLayout->addWidget(m_windDirXLabel, 1, 0);
    
    m_windDirXSlider = new QSlider(Qt::Horizontal, windGroup);
    m_windDirXSlider->setRange(-100, 100);  // 映射到-1.0 - 1.0
    m_windDirXSlider->setValue(100);  // 1.0
    windLayout->addWidget(m_windDirXSlider, 1, 1);
    
    m_windDirXSpinBox = new QDoubleSpinBox(windGroup);
    m_windDirXSpinBox->setRange(-1.0, 1.0);
    m_windDirXSpinBox->setValue(1.0);
    m_windDirXSpinBox->setSingleStep(0.01);
    m_windDirXSpinBox->setDecimals(2);
    windLayout->addWidget(m_windDirXSpinBox, 1, 2);
    
    // 风向Y
    m_windDirYLabel = new QLabel("风向Y: 0.00", windGroup);
    windLayout->addWidget(m_windDirYLabel, 2, 0);
    
    m_windDirYSlider = new QSlider(Qt::Horizontal, windGroup);
    m_windDirYSlider->setRange(-100, 100);  // 映射到-1.0 - 1.0
    m_windDirYSlider->setValue(0);  // 0.0
    windLayout->addWidget(m_windDirYSlider, 2, 1);
    
    m_windDirYSpinBox = new QDoubleSpinBox(windGroup);
    m_windDirYSpinBox->setRange(-1.0, 1.0);
    m_windDirYSpinBox->setValue(0.0);
    m_windDirYSpinBox->setSingleStep(0.01);
    m_windDirYSpinBox->setDecimals(2);
    windLayout->addWidget(m_windDirYSpinBox, 2, 2);
    
    // 风向Z
    m_windDirZLabel = new QLabel("风向Z: 0.00", windGroup);
    windLayout->addWidget(m_windDirZLabel, 3, 0);
    
    m_windDirZSlider = new QSlider(Qt::Horizontal, windGroup);
    m_windDirZSlider->setRange(-100, 100);  // 映射到-1.0 - 1.0
    m_windDirZSlider->setValue(0);  // 0.0
    windLayout->addWidget(m_windDirZSlider, 3, 1);
    
    m_windDirZSpinBox = new QDoubleSpinBox(windGroup);
    m_windDirZSpinBox->setRange(-1.0, 1.0);
    m_windDirZSpinBox->setValue(0.0);
    m_windDirZSpinBox->setSingleStep(0.01);
    m_windDirZSpinBox->setDecimals(2);
    windLayout->addWidget(m_windDirZSpinBox, 3, 2);
    
    mainLayout->addWidget(windGroup);
    
    // 创建天气参数组
    QGroupBox* weatherGroup = new QGroupBox("天气参数", this);
    QGridLayout* weatherLayout = new QGridLayout(weatherGroup);
    
    // 天气缩放
    m_weatherScaleLabel = new QLabel("天气缩放: 0.0001", weatherGroup);
    weatherLayout->addWidget(m_weatherScaleLabel, 0, 0);
    
    m_weatherScaleSlider = new QSlider(Qt::Horizontal, weatherGroup);
    m_weatherScaleSlider->setRange(1, 1000);  // 映射到0.00001 - 0.01
    m_weatherScaleSlider->setValue(5);  // 0.00005
    weatherLayout->addWidget(m_weatherScaleSlider, 0, 1);
    
    m_weatherScaleSpinBox = new QDoubleSpinBox(weatherGroup);
    m_weatherScaleSpinBox->setRange(0.00001, 0.01);
    m_weatherScaleSpinBox->setValue(0.00005);
    m_weatherScaleSpinBox->setSingleStep(0.00001);
    m_weatherScaleSpinBox->setDecimals(5);
    weatherLayout->addWidget(m_weatherScaleSpinBox, 0, 2);
    
    // 天气风X
    m_weatherWindXLabel = new QLabel("天气风X: 0.01", weatherGroup);
    weatherLayout->addWidget(m_weatherWindXLabel, 1, 0);
    
    m_weatherWindXSlider = new QSlider(Qt::Horizontal, weatherGroup);
    m_weatherWindXSlider->setRange(-100, 100);  // 映射到-1.0 - 1.0
    m_weatherWindXSlider->setValue(1);  // 0.01
    weatherLayout->addWidget(m_weatherWindXSlider, 1, 1);
    
    m_weatherWindXSpinBox = new QDoubleSpinBox(weatherGroup);
    m_weatherWindXSpinBox->setRange(-1.0, 1.0);
    m_weatherWindXSpinBox->setValue(0.01);
    m_weatherWindXSpinBox->setSingleStep(0.01);
    m_weatherWindXSpinBox->setDecimals(2);
    weatherLayout->addWidget(m_weatherWindXSpinBox, 1, 2);
    
    // 天气风Y
    m_weatherWindYLabel = new QLabel("天气风Y: 0.00", weatherGroup);
    weatherLayout->addWidget(m_weatherWindYLabel, 2, 0);
    
    m_weatherWindYSlider = new QSlider(Qt::Horizontal, weatherGroup);
    m_weatherWindYSlider->setRange(-100, 100);  // 映射到-1.0 - 1.0
    m_weatherWindYSlider->setValue(0);  // 0.0
    weatherLayout->addWidget(m_weatherWindYSlider, 2, 1);
    
    m_weatherWindYSpinBox = new QDoubleSpinBox(weatherGroup);
    m_weatherWindYSpinBox->setRange(-1.0, 1.0);
    m_weatherWindYSpinBox->setValue(0.0);
    m_weatherWindYSpinBox->setSingleStep(0.01);
    m_weatherWindYSpinBox->setDecimals(2);
    weatherLayout->addWidget(m_weatherWindYSpinBox, 2, 2);
    
    mainLayout->addWidget(weatherGroup);
    
    // 创建漩涡和侵蚀参数组
    QGroupBox* effectGroup = new QGroupBox("效果参数", this);
    QGridLayout* effectLayout = new QGridLayout(effectGroup);
    
    // 漩涡强度
    m_curlStrengthLabel = new QLabel("漩涡强度: 100.00", effectGroup);
    effectLayout->addWidget(m_curlStrengthLabel, 0, 0);
    
    m_curlStrengthSlider = new QSlider(Qt::Horizontal, effectGroup);
    m_curlStrengthSlider->setRange(0, 200);  // 映射到0.0 - 200.0
    m_curlStrengthSlider->setValue(100);  // 100.0
    effectLayout->addWidget(m_curlStrengthSlider, 0, 1);
    
    m_curlStrengthSpinBox = new QDoubleSpinBox(effectGroup);
    m_curlStrengthSpinBox->setRange(0.0, 200.0);
    m_curlStrengthSpinBox->setValue(100.0);
    m_curlStrengthSpinBox->setSingleStep(1.0);
    m_curlStrengthSpinBox->setDecimals(2);
    effectLayout->addWidget(m_curlStrengthSpinBox, 0, 2);
    
    // 漩涡缩放
    m_curlScaleLabel = new QLabel("漩涡缩放: 0.01", effectGroup);
    effectLayout->addWidget(m_curlScaleLabel, 1, 0);
    
    m_curlScaleSlider = new QSlider(Qt::Horizontal, effectGroup);
    m_curlScaleSlider->setRange(1, 1000);  // 映射到0.001 - 1.0
    m_curlScaleSlider->setValue(10);  // 0.01
    effectLayout->addWidget(m_curlScaleSlider, 1, 1);
    
    m_curlScaleSpinBox = new QDoubleSpinBox(effectGroup);
    m_curlScaleSpinBox->setRange(0.001, 1.0);
    m_curlScaleSpinBox->setValue(0.01);
    m_curlScaleSpinBox->setSingleStep(0.001);
    m_curlScaleSpinBox->setDecimals(3);
    effectLayout->addWidget(m_curlScaleSpinBox, 1, 2);
    
    // 侵蚀强度
    m_erosionStrengthLabel = new QLabel("侵蚀强度: 0.50", effectGroup);
    effectLayout->addWidget(m_erosionStrengthLabel, 2, 0);
    
    m_erosionStrengthSlider = new QSlider(Qt::Horizontal, effectGroup);
    m_erosionStrengthSlider->setRange(0, 100);  // 映射到0.0 - 1.0
    m_erosionStrengthSlider->setValue(50);  // 0.5
    effectLayout->addWidget(m_erosionStrengthSlider, 2, 1);
    
    m_erosionStrengthSpinBox = new QDoubleSpinBox(effectGroup);
    m_erosionStrengthSpinBox->setRange(0.0, 1.0);
    m_erosionStrengthSpinBox->setValue(0.5);
    m_erosionStrengthSpinBox->setSingleStep(0.01);
    m_erosionStrengthSpinBox->setDecimals(2);
    effectLayout->addWidget(m_erosionStrengthSpinBox, 2, 2);
    
    mainLayout->addWidget(effectGroup);
    
    // 创建重置按钮
    QPushButton* resetButton = new QPushButton("重置为默认值", this);
    resetButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; border: none; padding: 10px; border-radius: 5px; font-size: 14px; }"
                              "QPushButton:hover { background-color: #d32f2f; }"
                              "QPushButton:pressed { background-color: #b71c1c; }");
    mainLayout->addWidget(resetButton);
    
    // 连接重置按钮信号
    connect(resetButton, &QPushButton::clicked, this, &CloudParameterWidget::onResetButtonClicked);
    
    // 连接信号槽
    connect(m_shapeScaleSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.00001;  // 调整比例
        if (value == 30) val = 0.0003;  // 特殊处理初始值
        else val = value * 0.00001;
        m_shapeScaleSpinBox->setValue(val);
        m_shapeScaleLabel->setText(QString("云形状缩放: %1").arg(val, 0, 'f', 5));
        onParameterChanged();
    });
    
    connect(m_shapeScaleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.00001);
        if (value == 0.0003) sliderValue = 30;  // 特殊处理初始值
        m_shapeScaleSlider->setValue(sliderValue);
        m_shapeScaleLabel->setText(QString("云形状缩放: %1").arg(value, 0, 'f', 5));
        onParameterChanged();
    });
    
    connect(m_detailScaleSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.0001;  // 调整比例
        if (value == 10) val = 0.001;  // 特殊处理初始值
        else val = value * 0.0001;
        m_detailScaleSpinBox->setValue(val);
        m_detailScaleLabel->setText(QString("云细节缩放: %1").arg(val, 0, 'f', 5));
        onParameterChanged();
    });
    
    connect(m_detailScaleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.0001);
        if (value == 0.001) sliderValue = 10;  // 特殊处理初始值
        m_detailScaleSlider->setValue(sliderValue);
        m_detailScaleLabel->setText(QString("云细节缩放: %1").arg(value, 0, 'f', 5));
        onParameterChanged();
    });
    
    connect(m_windSpeedSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.1;
        m_windSpeedSpinBox->setValue(val);
        m_windSpeedLabel->setText(QString("风速: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_windSpeedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.1);
        m_windSpeedSlider->setValue(sliderValue);
        m_windSpeedLabel->setText(QString("风速: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_windDirXSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.01;
        m_windDirXSpinBox->setValue(val);
        m_windDirXLabel->setText(QString("风向X: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_windDirXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.01);
        m_windDirXSlider->setValue(sliderValue);
        m_windDirXLabel->setText(QString("风向X: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_windDirYSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.01;
        m_windDirYSpinBox->setValue(val);
        m_windDirYLabel->setText(QString("风向Y: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_windDirYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.01);
        m_windDirYSlider->setValue(sliderValue);
        m_windDirYLabel->setText(QString("风向Y: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_windDirZSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.01;
        m_windDirZSpinBox->setValue(val);
        m_windDirZLabel->setText(QString("风向Z: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_windDirZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.01);
        m_windDirZSlider->setValue(sliderValue);
        m_windDirZLabel->setText(QString("风向Z: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_weatherScaleSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.00001;
        if (value == 5) val = 0.00005;  // 特殊处理初始值
        else val = value * 0.00001;
        m_weatherScaleSpinBox->setValue(val);
        m_weatherScaleLabel->setText(QString("天气缩放: %1").arg(val, 0, 'f', 5));
        onParameterChanged();
    });
    
    connect(m_weatherScaleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.00001);
        if (value == 0.00005) sliderValue = 5;  // 特殊处理初始值
        m_weatherScaleSlider->setValue(sliderValue);
        m_weatherScaleLabel->setText(QString("天气缩放: %1").arg(value, 0, 'f', 5));
        onParameterChanged();
    });
    
    connect(m_weatherWindXSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.01;
        m_weatherWindXSpinBox->setValue(val);
        m_weatherWindXLabel->setText(QString("天气风X: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_weatherWindXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.01);
        m_weatherWindXSlider->setValue(sliderValue);
        m_weatherWindXLabel->setText(QString("天气风X: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_weatherWindYSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.01;
        m_weatherWindYSpinBox->setValue(val);
        m_weatherWindYLabel->setText(QString("天气风Y: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_weatherWindYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.01);
        m_weatherWindYSlider->setValue(sliderValue);
        m_weatherWindYLabel->setText(QString("天气风Y: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_curlStrengthSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 1.0;
        m_curlStrengthSpinBox->setValue(val);
        m_curlStrengthLabel->setText(QString("漩涡强度: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_curlStrengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 1.0);
        m_curlStrengthSlider->setValue(sliderValue);
        m_curlStrengthLabel->setText(QString("漩涡强度: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_curlScaleSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.001;
        if (value == 10) val = 0.01;  // 特殊处理初始值
        else val = value * 0.001;
        m_curlScaleSpinBox->setValue(val);
        m_curlScaleLabel->setText(QString("漩涡缩放: %1").arg(val, 0, 'f', 3));
        onParameterChanged();
    });
    
    connect(m_curlScaleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.001);
        if (value == 0.01) sliderValue = 10;  // 特殊处理初始值
        m_curlScaleSlider->setValue(sliderValue);
        m_curlScaleLabel->setText(QString("漩涡缩放: %1").arg(value, 0, 'f', 3));
        onParameterChanged();
    });
    
    connect(m_erosionStrengthSlider, &QSlider::valueChanged, this, [this](int value) {
        double val = value * 0.01;
        m_erosionStrengthSpinBox->setValue(val);
        m_erosionStrengthLabel->setText(QString("侵蚀强度: %1").arg(val, 0, 'f', 2));
        onParameterChanged();
    });
    
    connect(m_erosionStrengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        int sliderValue = static_cast<int>(value / 0.01);
        m_erosionStrengthSlider->setValue(sliderValue);
        m_erosionStrengthLabel->setText(QString("侵蚀强度: %1").arg(value, 0, 'f', 2));
        onParameterChanged();
    });
    
    // 初始化参数值
    onParameterChanged();
    

}

void CloudParameterWidget::onResetButtonClicked()
{
    // 重置所有参数到默认值
    m_shapeScaleSpinBox->setValue(0.0003);
    m_detailScaleSpinBox->setValue(0.001);
    m_windSpeedSpinBox->setValue(10.0);
    m_weatherScaleSpinBox->setValue(0.00005);
    m_curlStrengthSpinBox->setValue(100.0);
    m_curlScaleSpinBox->setValue(0.01);
    m_erosionStrengthSpinBox->setValue(0.5);
    m_windDirXSpinBox->setValue(1.0);
    m_windDirYSpinBox->setValue(0.0);
    m_windDirZSpinBox->setValue(0.0);
    m_weatherWindXSpinBox->setValue(0.01);
    m_weatherWindYSpinBox->setValue(0.0);
    
    // 触发参数变化信号
    onParameterChanged();
}

void CloudParameterWidget::onParameterChanged()
{
    emit cloudParametersChanged(
        static_cast<float>(m_shapeScaleSpinBox->value()),
        static_cast<float>(m_detailScaleSpinBox->value()),
        static_cast<float>(m_windSpeedSpinBox->value()),
        static_cast<float>(m_weatherScaleSpinBox->value()),
        static_cast<float>(m_curlStrengthSpinBox->value()),
        static_cast<float>(m_curlScaleSpinBox->value()),
        static_cast<float>(m_erosionStrengthSpinBox->value()),
        static_cast<float>(m_windDirXSpinBox->value()),
        static_cast<float>(m_windDirYSpinBox->value()),
        static_cast<float>(m_windDirZSpinBox->value()),
        static_cast<float>(m_weatherWindXSpinBox->value()),
        static_cast<float>(m_weatherWindYSpinBox->value())
    );
}