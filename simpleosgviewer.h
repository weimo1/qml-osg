#ifndef SIMPLEOSGVIEWER_H
#define SIMPLEOSGVIEWER_H

#include <QQuickFramebufferObject>
#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventAdapter>

class SimpleOSGRenderer;

class SimpleOSGViewer : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(ViewType viewType READ viewType WRITE setViewType NOTIFY viewTypeChanged)
    // 添加相机位置属性
    Q_PROPERTY(double cameraX READ cameraX NOTIFY cameraPositionChanged)
    Q_PROPERTY(double cameraY READ cameraY NOTIFY cameraPositionChanged)
    Q_PROPERTY(double cameraZ READ cameraZ NOTIFY cameraPositionChanged)

public:
    explicit SimpleOSGViewer(QQuickItem *parent = nullptr);
    Renderer *createRenderer() const override;
    
    // 添加鼠标事件处理函数
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;  // 添加双击事件处理
    void wheelEvent(QWheelEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    
    // 添加键盘事件处理函数
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    
    // 添加视图类型枚举
    enum ViewType {
        MainView,    // 主视图
        FrontView,   // 前视图
        SideView,    // 侧视图
        TopView      // 俯视图
    };
    Q_ENUM(ViewType)
    
    // 添加鼠标位置属性
    Q_PROPERTY(int mouseX READ mouseX NOTIFY mousePositionChanged)
    Q_PROPERTY(int mouseY READ mouseY NOTIFY mousePositionChanged)
  
    
    // 设置视图类型
    void setViewType(ViewType viewType);
    ViewType viewType() const;
    
    // 获取鼠标位置
    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }
    
    // 获取相机位置
    double cameraX() const { return m_cameraX; }
    double cameraY() const { return m_cameraY; }
    double cameraZ() const { return m_cameraZ; }
    
public slots:
    // 添加公共方法供QML调用
    void loadOSGFile(const QString& fileName);
    void resetToHomeView();
    void fitToView();
    void toggleLighting(bool enabled);  // 添加光照控制方法
    void createAtmosphere();  // 添加大气渲染方法
    void testMRT();  // 添加MRT测试方法
    
    // 添加内部调用的槽函数
    void invokeResetView();
    void invokeSetViewType(ViewType viewType);
    void invokeLoadOSGFile(const QString& fileName);
    void invokeFitToView();
    void invokeToggleLighting(bool enabled);  // 添加光照控制槽函数
    void invokeCreateAtmosphere();  // 添加大气渲染槽函数
    void invokeTestMRT();  // 添加MRT测试槽函数

    // 更新相机位置的方法
    void updateCameraPosition(double x, double y, double z);

signals:
    void viewTypeChanged();
    void mousePositionChanged();
    void cameraPositionChanged();
    
private:
    mutable SimpleOSGRenderer* m_renderer;  // 保存渲染器引用
    ViewType m_viewType;  // 视图类型
    int m_mouseX;  // 鼠标X坐标
    int m_mouseY;  // 鼠标Y坐标
    double m_cameraX;  // 相机X坐标
    double m_cameraY;  // 相机Y坐标
    double m_cameraZ;  // 相机Z坐标
 
};

#endif // SIMPLEOSGVIEWER_H