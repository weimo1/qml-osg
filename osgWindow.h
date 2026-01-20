#ifndef OSGWINDOW_H
#define OSGWINDOW_H

#include <QOpenGLWidget>
#include <QTimer>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/StateAttribute>

// Forward declaration
class CloudParameterWidget;

class GraphicsWindowQt : public QOpenGLWidget
{
    Q_OBJECT

public:
    GraphicsWindowQt(QWidget* parent = nullptr);
    ~GraphicsWindowQt();
    
    // Public methods to get camera and mouse position
    osg::Vec3 getCameraPosition() const;
    QPoint getMousePosition() const { return m_mousePos; }
    
    // Public method to update cloud parameters
    void updateCloudParameters(float shapescale, float detailScale, float windSpeed,
                             float weatherScale, float curlStrength, float curlScale,
                             float erosionStrength, float windDirX, float windDirY, float windDirZ,
                             float weatherWindX, float weatherWindY);
    
    // 重置云参数
    void resetCloudParameters();
    
    // Public methods to access OSG components
    osgViewer::View* getView() const;
    osg::Group* getRootNode() const;

signals:
    void cameraPositionChanged(const QString& pos);
    void mousePositionChanged(const QString& pos);
    void loadFileRequested();
    void toggleLightingRequested(bool enabled);
    void createAtmosphereRequested();  // 添加大气效果信号
    void createFullscreenAtmosphereRequested();  // 添加全屏大气效果信号
    void updateCloudParametersRequested(float shapescale, float detailScale, float windSpeed,
                                      float weatherScale, float curlStrength, float curlScale,
                                      float erosionStrength, float windDirX, float windDirY, float windDirZ,
                                      float weatherWindX, float weatherWindY);  // 更新云参数信号
    void resetCloudParametersRequested();  // 重置云参数信号
    void updateMousePositionRequested(float x, float y);  // 更新鼠标位置信号
    void createFullscreenTriangleRequested();  // 全屏红色三角形信号

protected:
    // QOpenGLWidget overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    // Event handlers
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    
    // Widget events
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onLoadFileButtonClicked();
    void onToggleLightingButtonClicked();
    void onCreateAtmosphereButtonClicked();  // 添加大气效果槽函数
    void onCreateFullscreenAtmosphereButtonClicked();  // 添加全屏大气效果槽函数
    void onCloudParameterButtonClicked();  // 添加云参数控制槽函数
    void onCreateFullscreenTriangleButtonClicked();  // 添加全屏红色三角形槽函数

private:
    QTimer m_timer;
    QPoint m_mousePos;
    
    // OSG components
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_graphicsWindow;
    osg::ref_ptr<osgViewer::CompositeViewer> m_viewer;
    osg::ref_ptr<osg::Group> m_root;
    
    // UI components
    QWidget* m_buttonContainer = nullptr;
    QPushButton* m_loadFileButton = nullptr;
    QPushButton* m_toggleLightingButton = nullptr;
    QPushButton* m_createAtmosphereButton = nullptr;  // 大气效果按钮
    QPushButton* m_createFullscreenAtmosphereButton = nullptr;  // 全屏大气效果按钮
    QPushButton* m_cloudParameterButton = nullptr;  // 云参数控制按钮
    QPushButton* m_createFullscreenTriangleButton = nullptr;  // 全屏红色三角形按钮

    
    // Helper functions
    void moveSceneToOrigin();
    void setupOSG(int width, int height);
    void createSimpleScene();
    osgViewer::GraphicsWindow* createGraphicsContext();
    osgGA::EventQueue* getEventQueue() const;
    void updateCameraPosition();
    void createUI();
    
    // Button state
    bool m_mouseOverButton;
    bool _firstFrame=true;
};

#endif // OSGWINDOW_H