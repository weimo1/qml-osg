#ifndef CLOUDPARAMETERWIDGET_H
#define CLOUDPARAMETERWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDoubleSpinBox>

class CloudParameterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CloudParameterWidget(QWidget *parent = nullptr);

signals:
    void cloudParametersChanged(float shapescale, float detailScale, float windSpeed,
                               float weatherScale, float curlStrength, float curlScale,
                               float erosionStrength, float windDirX, float windDirY, float windDirZ,
                               float weatherWindX, float weatherWindY);

private slots:
    void onParameterChanged();
    void onResetButtonClicked();

private:
    void createControls();

    // 云形状参数
    QSlider* m_shapeScaleSlider;
    QLabel* m_shapeScaleLabel;
    QDoubleSpinBox* m_shapeScaleSpinBox;

    // 云细节参数
    QSlider* m_detailScaleSlider;
    QLabel* m_detailScaleLabel;
    QDoubleSpinBox* m_detailScaleSpinBox;

    // 风速参数
    QSlider* m_windSpeedSlider;
    QLabel* m_windSpeedLabel;
    QDoubleSpinBox* m_windSpeedSpinBox;

    // 天气参数
    QSlider* m_weatherScaleSlider;
    QLabel* m_weatherScaleLabel;
    QDoubleSpinBox* m_weatherScaleSpinBox;

    // 漩涡强度参数
    QSlider* m_curlStrengthSlider;
    QLabel* m_curlStrengthLabel;
    QDoubleSpinBox* m_curlStrengthSpinBox;

    // 漩涡缩放参数
    QSlider* m_curlScaleSlider;
    QLabel* m_curlScaleLabel;
    QDoubleSpinBox* m_curlScaleSpinBox;

    // 侵蚀强度参数
    QSlider* m_erosionStrengthSlider;
    QLabel* m_erosionStrengthLabel;
    QDoubleSpinBox* m_erosionStrengthSpinBox;

    // 风向参数
    QSlider* m_windDirXSlider;
    QLabel* m_windDirXLabel;
    QDoubleSpinBox* m_windDirXSpinBox;

    QSlider* m_windDirYSlider;
    QLabel* m_windDirYLabel;
    QDoubleSpinBox* m_windDirYSpinBox;

    QSlider* m_windDirZSlider;
    QLabel* m_windDirZLabel;
    QDoubleSpinBox* m_windDirZSpinBox;

    // 天气风参数
    QSlider* m_weatherWindXSlider;
    QLabel* m_weatherWindXLabel;
    QDoubleSpinBox* m_weatherWindXSpinBox;

    QSlider* m_weatherWindYSlider;
    QLabel* m_weatherWindYLabel;
    QDoubleSpinBox* m_weatherWindYSpinBox;
};

#endif // CLOUDPARAMETERWIDGET_H