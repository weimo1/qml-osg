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
    
    // 添加摄像机位置属性
    Q_PROPERTY(double cameraX READ cameraX NOTIFY cameraPositionChanged)
    Q_PROPERTY(double cameraY READ cameraY NOTIFY cameraPositionChanged)
    Q_PROPERTY(double cameraZ READ cameraZ NOTIFY cameraPositionChanged)
    
    // 设置视图类型
    void setViewType(ViewType viewType);
    ViewType viewType() const;
    
    // 获取鼠标位置
    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }
    
    // 获取摄像机位置
    double cameraX() const { return m_cameraX; }
    double cameraY() const { return m_cameraY; }
    double cameraZ() const { return m_cameraZ; }
    
    // 添加获取相机Eye位置的方法
    Q_INVOKABLE QVector3D getCameraEye() const;
    Q_INVOKABLE QVector3D getCameraCenter() const;
    Q_INVOKABLE QVector3D getCameraUp() const;
    
signals:
    void viewTypeChanged();
    void mousePositionChanged();
    void cameraPositionChanged();
    
public slots:
    // 添加公共方法供QML调用
    void createShape();
    void createShapeWithNewSkybox();
    void createPBRScene();  // 添加PBR场景创建方法
    void createAtmosphereScene(); // 添加大气渲染场景创建方法
    void createTexturedAtmosphereScene(); // 添加结合纹理和大气渲染的场景创建方法
    void createSkyboxAtmosphereScene(); // 添加结合天空盒和大气渲染的场景创建方法
    void resetView();
    void resetToHomeView();  // 添加回归主视角的方法
    void loadOSGFile(const QString& fileName);
    void openFileSelector();  // 添加文件选择功能
    void setShapeColor(float r, float g, float b, float a = 1.0f);  // 添加设置图形颜色的方法
    void selectModel(int x, int y);  // 添加选择模型的方法
    
    // 添加更新PBR材质的方法（包含Alpha参数）
    void updatePBRMaterial(float albedoR, float albedoG, float albedoB, float albedoA,
                          float metallic, float roughness, 
                          float specular, float ao);
    
    // 添加更新大气参数的方法
    void updateAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle);
    
    // 添加更新大气密度和太阳强度的方法
    void updateAtmosphereDensityAndIntensity(float density, float intensity);
    
    // 添加更新米氏散射和瑞利散射的方法
    void updateAtmosphereScattering(float mie, float rayleigh);
    
    // 添加更新SkyNode大气参数的方法
    void updateSkyNodeAtmosphereParameters(float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG);
    
    // 添加更新Textured Atmosphere参数的方法
    void updateTexturedAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle, float exposure);
    
    // 添加实际调用渲染器的槽函数
    void invokeCreateShape();
    void invokeCreateShapeWithNewSkybox();
    void invokeCreatePBRScene();  // 添加PBR场景创建的槽函数
    void invokeCreateAtmosphereScene(); // 添加大气渲染场景创建的槽函数
    void invokeCreateTexturedAtmosphereScene(); // 添加结合纹理和大气渲染的场景创建的槽函数
    void invokeCreateSkyboxAtmosphereScene(); // 添加结合天空盒和大气渲染的场景创建的槽函数
    void invokeResetView();
    void invokeLoadOSGFile(const QString& fileName);
    void invokeSetViewType(ViewType viewType);  // 添加设置视图类型的槽函数
    void invokeResetToHomeView();  // 添加回归主视角的槽函数
    void updateCameraPosition();  // 添加更新摄像机位置的槽函数
    // 更新PBR材质的槽函数（包含Alpha参数）
    void invokeUpdatePBRMaterial(float albedoR, float albedoG, float albedoB, float albedoA,
                                float metallic, float roughness, 
                                float specular, float ao);
    // 更新大气参数的槽函数
    void invokeUpdateAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle);
    
    // 更新大气密度和太阳强度的槽函数
    void invokeUpdateAtmosphereDensityAndIntensity(float density, float intensity);

    // 添加更新米氏散射和瑞利散射的槽函数声明
    void invokeUpdateAtmosphereScattering(float mie, float rayleigh);
    
    // 添加更新SkyNode大气参数的槽函数声明
    void invokeUpdateSkyNodeAtmosphereParameters(float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG);
    
    // 添加更新Textured Atmosphere参数的槽函数声明
    void invokeUpdateTexturedAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle, float exposure);

signals:
    // 添加信号，用于通知QML打开文件对话框
    void requestFileDialog();
    void fileSelected(const QString& fileName);  // 文件选择完成信号

private:
    mutable SimpleOSGRenderer* m_renderer;  // 保存渲染器引用
    ViewType m_viewType;  // 视图类型
    int m_mouseX;  // 鼠标X坐标
    int m_mouseY;  // 鼠标Y坐标
    double m_cameraX;  // 摄像机X坐标
    double m_cameraY;  // 摄像机Y坐标
    double m_cameraZ;  // 摄像机Z坐标
};

#endif // SIMPLEOSGVIEWER_H