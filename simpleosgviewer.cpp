#include "simpleosgviewer.h"
#include "simpleosgrenderer.h"
#include <QQuickWindow>
#include <QDebug>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QFileDialog>
#include <QCoreApplication>
#include <QMetaObject>

SimpleOSGViewer::SimpleOSGViewer(QQuickItem *parent)
    : QQuickFramebufferObject(parent), m_renderer(nullptr), m_viewType(MainView), m_mouseX(0), m_mouseY(0), m_cameraX(0.0), m_cameraY(0.0), m_cameraZ(0.0)
{
    setTextureFollowsItemSize(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    
    // 使用定时器定期更新，避免线程问题
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SimpleOSGViewer::update);
    connect(timer, &QTimer::timeout, this, &SimpleOSGViewer::updateCameraPosition);
    timer->start(16); // 约60 FPS
}

QQuickFramebufferObject::Renderer *SimpleOSGViewer::createRenderer() const
{
    m_renderer = new SimpleOSGRenderer(m_viewType);
    return m_renderer;
}

void SimpleOSGViewer::setViewType(ViewType viewType)
{
    if (m_viewType != viewType) {
        m_viewType = viewType;
        emit viewTypeChanged();
        
        // 通知渲染器更新视图类型
        if (m_renderer) {
            QMetaObject::invokeMethod(this, "invokeSetViewType", Qt::QueuedConnection, Q_ARG(SimpleOSGViewer::ViewType, viewType));
        }
        
        // 强制更新视图
        update();
    }
}

SimpleOSGViewer::ViewType SimpleOSGViewer::viewType() const
{
    return m_viewType;
}

// 添加获取相机Eye位置的方法
QVector3D SimpleOSGViewer::getCameraEye() const
{
    if (m_renderer) {
        ViewManager* viewManager = m_renderer->getViewManager();
        if (viewManager) {
            osg::Vec3d eye = viewManager->getEye();
            return QVector3D(eye.x(), eye.y(), eye.z());
        }
    }
    return QVector3D(0.0f, 0.0f, 0.0f);
}

QVector3D SimpleOSGViewer::getCameraCenter() const
{
    if (m_renderer) {
        ViewManager* viewManager = m_renderer->getViewManager();
        if (viewManager) {
            osg::Vec3d center = viewManager->getCenter();
            return QVector3D(center.x(), center.y(), center.z());
        }
    }
    return QVector3D(0.0f, 0.0f, 0.0f);
}

QVector3D SimpleOSGViewer::getCameraUp() const
{
    if (m_renderer) {
        ViewManager* viewManager = m_renderer->getViewManager();
        if (viewManager) {
            osg::Vec3d up = viewManager->getUp();
            return QVector3D(up.x(), up.y(), up.z());
        }
    }
    return QVector3D(0.0f, 0.0f, 1.0f);
}

void SimpleOSGViewer::mousePressEvent(QMouseEvent *event)
{
    // 更新鼠标位置
    m_mouseX = event->x();
    m_mouseY = event->y();
    emit mousePositionChanged();
    
    // 将所有鼠标按下事件传递给渲染器
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    // 不调用父类的mousePressEvent，避免事件被拦截
    // QQuickFramebufferObject::mousePressEvent(event);
    event->setAccepted(true); // 标记事件已被处理
}

void SimpleOSGViewer::mouseMoveEvent(QMouseEvent *event)
{
    // 更新鼠标位置
    m_mouseX = event->x();
    m_mouseY = event->y();
    emit mousePositionChanged();
    
    // 将所有鼠标移动事件传递给渲染器
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    event->setAccepted(true); // 标记事件已被处理
}

void SimpleOSGViewer::mouseReleaseEvent(QMouseEvent *event)
{
    // 更新鼠标位置
    m_mouseX = event->x();
    m_mouseY = event->y();
    emit mousePositionChanged();
    
    // 将所有鼠标释放事件传递给渲染器
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    event->setAccepted(true); // 标记事件已被处理
}

void SimpleOSGViewer::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 处理双击事件，特别是左键双击回归视角
    if (event->button() == Qt::LeftButton) {
        if (m_renderer) {
            // 调用回归视角功能
            QMetaObject::invokeMethod(this, "invokeResetView", Qt::QueuedConnection);
        }
    }
    
    // 将双击事件也传递给渲染器
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    event->setAccepted(true);
}

void SimpleOSGViewer::wheelEvent(QWheelEvent *event)
{
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    // 不调用父类的wheelEvent，避免事件被拦截
    // QQuickFramebufferObject::wheelEvent(event);
    event->setAccepted(true); // 标记事件已被处理
}

void SimpleOSGViewer::hoverMoveEvent(QHoverEvent *event)
{
    // 更新鼠标位置
    m_mouseX = event->pos().x();
    m_mouseY = event->pos().y();
    emit mousePositionChanged();
    
    // 将悬停事件也传递给渲染器
    if (m_renderer) {
        // 创建一个模拟的鼠标移动事件
        QMouseEvent mouseEvent(QEvent::MouseMove, event->pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        m_renderer->processEvent(&mouseEvent);
    }
    QQuickFramebufferObject::hoverMoveEvent(event);
}

// 实现创建图形功能
void SimpleOSGViewer::createShape()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeCreateShape", Qt::QueuedConnection);
    }
}

// 实现创建PBR场景功能
void SimpleOSGViewer::createPBRScene()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeCreatePBRScene", Qt::QueuedConnection);
    }
}

// 实现创建大气渲染场景功能
void SimpleOSGViewer::createAtmosphereScene()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeCreateAtmosphereScene", Qt::QueuedConnection);
    }
}

// 新增：实现创建结合纹理和大气渲染的场景功能
void SimpleOSGViewer::createTexturedAtmosphereScene()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeCreateTexturedAtmosphereScene", Qt::QueuedConnection);
    }
}

// 新增：实现创建结合天空盒和大气渲染的场景功能
void SimpleOSGViewer::createSkyboxAtmosphereScene()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeCreateSkyboxAtmosphereScene", Qt::QueuedConnection);
    }
}

// 新增：实现创建结合天空盒大气和PBR立方体的场景功能
void SimpleOSGViewer::createSkyboxAtmosphereWithPBRScene()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeCreateSkyboxAtmosphereWithPBRScene", Qt::QueuedConnection);
    }
}

// 实现重置视野功能
void SimpleOSGViewer::resetView()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeResetView", Qt::QueuedConnection);
    }
}

// 实现加载OSG文件功能
void SimpleOSGViewer::loadOSGFile(const QString& fileName)
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeLoadOSGFile", Qt::QueuedConnection, Q_ARG(QString, fileName));
    }
}

// 实现文件选择功能
void SimpleOSGViewer::openFileSelector()
{
    // 发送信号通知QML打开文件对话框
    emit requestFileDialog();
}

// 实现选择模型功能
void SimpleOSGViewer::selectModel(int x, int y)
{
    // 直接调用渲染器的方法
    if (m_renderer) {
        m_renderer->selectModel(x, y);
    }
}

// 实现设置图形颜色功能
void SimpleOSGViewer::setShapeColor(float r, float g, float b, float a)
{
    // 直接调用渲染器的方法
    if (m_renderer) {
        m_renderer->setShapeColor(r, g, b, a);
    }
}

// 添加实际调用渲染器的槽函数
void SimpleOSGViewer::invokeCreateShape()
{
    if (m_renderer) {
        m_renderer->createShape();
    }
}

void SimpleOSGViewer::invokeCreateShapeWithNewSkybox()
{
    if (m_renderer) {
        m_renderer->createShapeWithNewSkybox();
    }
}

void SimpleOSGViewer::invokeCreatePBRScene()
{
    if (m_renderer) {
        m_renderer->createPBRScene();
    }
}

void SimpleOSGViewer::invokeCreateAtmosphereScene()
{
    if (m_renderer) {
        m_renderer->createAtmosphereScene();
    }
}

// 实际调用渲染器重置视野的方法
void SimpleOSGViewer::invokeResetView()
{
    if (m_renderer) {
        m_renderer->resetView(m_viewType);
    }
}

// 添加回归主视角的方法
void SimpleOSGViewer::resetToHomeView()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeResetToHomeView", Qt::QueuedConnection);
    }
}

// 实际调用渲染器回归主视角的方法
void SimpleOSGViewer::invokeResetToHomeView()
{
    if (m_renderer) {
        m_renderer->resetToHomeView();
    }
    
    // 强制更新视图
    update();
}

// 实现创建使用新天空盒类的图形功能
void SimpleOSGViewer::createShapeWithNewSkybox()
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeCreateShapeWithNewSkybox", Qt::QueuedConnection);
    }
}

// 实际调用渲染器加载文件的方法
void SimpleOSGViewer::invokeLoadOSGFile(const QString& fileName)
{
    if (m_renderer) {
        m_renderer->loadOSGFile(fileName);
    }
}

// 实际调用渲染器设置视图类型的方法
void SimpleOSGViewer::invokeSetViewType(ViewType viewType)
{
    if (m_renderer) {
        m_renderer->setViewType(viewType);
    }
    // 强制更新视图
    update();
}

// 添加获取摄像机位置的函数
void SimpleOSGViewer::updateCameraPosition() {
    if (m_renderer) {
        ViewManager* viewManager = m_renderer->getViewManager();
        if (viewManager) {
            // 从操作器获取当前的相机参数
            viewManager->updateViewParametersFromManipulator(m_renderer->getViewer());
            
            osg::Vec3d eye = viewManager->getEye();
            m_cameraX = eye.x();
            m_cameraY = eye.y();
            m_cameraZ = eye.z();
            emit cameraPositionChanged();
        }
    }
}

// 添加更新PBR材质的方法（包含Alpha参数）
void SimpleOSGViewer::updatePBRMaterial(float albedoR, float albedoG, float albedoB, float albedoA,
                                       float metallic, float roughness, 
                                       float specular, float ao)
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeUpdatePBRMaterial", Qt::QueuedConnection,
                                 Q_ARG(float, albedoR), Q_ARG(float, albedoG), Q_ARG(float, albedoB), Q_ARG(float, albedoA),
                                 Q_ARG(float, metallic), Q_ARG(float, roughness),
                                 Q_ARG(float, specular), Q_ARG(float, ao));
    }
}

// 实际调用渲染器更新PBR材质的方法（包含Alpha参数）
void SimpleOSGViewer::invokeUpdatePBRMaterial(float albedoR, float albedoG, float albedoB, float albedoA,
                                             float metallic, float roughness, 
                                             float specular, float ao)
{
    if (m_renderer) {
        m_renderer->updatePBRMaterial(albedoR, albedoG, albedoB, albedoA, metallic, roughness, specular, ao);
    }
}

// 添加更新大气参数的方法
void SimpleOSGViewer::updateAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle)
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeUpdateAtmosphereParameters", Qt::QueuedConnection,
                                 Q_ARG(float, sunZenithAngle), Q_ARG(float, sunAzimuthAngle));
    }
}

// 实际调用渲染器更新大气参数的方法
void SimpleOSGViewer::invokeUpdateAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle)
{
    if (m_renderer) {
        m_renderer->updateAtmosphereParameters(sunZenithAngle, sunAzimuthAngle);
    }
}

// 添加更新大气密度和太阳强度的方法
void SimpleOSGViewer::updateAtmosphereDensityAndIntensity(float density, float intensity)
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeUpdateAtmosphereDensityAndIntensity", Qt::QueuedConnection,
                                 Q_ARG(float, density), Q_ARG(float, intensity));
    }
}

// 实际调用渲染器更新大气密度和太阳强度的方法
void SimpleOSGViewer::invokeUpdateAtmosphereDensityAndIntensity(float density, float intensity)
{
    if (m_renderer) {
        m_renderer->updateAtmosphereDensityAndIntensity(density, intensity);
    }
}

// 添加更新米氏散射和瑞利散射的方法
void SimpleOSGViewer::updateAtmosphereScattering(float mie, float rayleigh)
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeUpdateAtmosphereScattering", Qt::QueuedConnection,
                                 Q_ARG(float, mie), Q_ARG(float, rayleigh));
    }
}

// 实际调用渲染器更新米氏散射和瑞利散射的方法
void SimpleOSGViewer::invokeUpdateAtmosphereScattering(float mie, float rayleigh)
{
    if (m_renderer) {
        m_renderer->updateAtmosphereScattering(mie, rayleigh);
    }
}

// 添加更新SkyNode大气参数的方法
void SimpleOSGViewer::updateSkyNodeAtmosphereParameters(float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG, float sunZenithAngle, float sunAzimuthAngle)
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeUpdateSkyNodeAtmosphereParameters", Qt::QueuedConnection,
                                 Q_ARG(float, turbidity), Q_ARG(float, rayleigh),
                                 Q_ARG(float, mieCoefficient), Q_ARG(float, mieDirectionalG), 
                                 Q_ARG(float, sunZenithAngle), Q_ARG(float, sunAzimuthAngle));
    }
}

// 实际调用渲染器更新SkyNode大气参数的方法
void SimpleOSGViewer::invokeUpdateSkyNodeAtmosphereParameters(float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG, float sunZenithAngle, float sunAzimuthAngle)
{
    if (m_renderer) {
        m_renderer->updateSkyNodeAtmosphereParameters(turbidity, rayleigh, mieCoefficient, mieDirectionalG, sunZenithAngle, sunAzimuthAngle);
    }
}

// 添加更新Textured Atmosphere参数的方法
void SimpleOSGViewer::updateTexturedAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle, float exposure)
{
    // 使用QMetaObject::invokeMethod确保在GUI线程中调用
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeUpdateTexturedAtmosphereParameters", Qt::QueuedConnection,
                                 Q_ARG(float, sunZenithAngle), Q_ARG(float, sunAzimuthAngle), Q_ARG(float, exposure));
    }
}

// 实际调用渲染器更新Textured Atmosphere参数的方法
void SimpleOSGViewer::invokeUpdateTexturedAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle, float exposure)
{
    if (m_renderer) {
        m_renderer->updateTexturedAtmosphereParameters(sunZenithAngle, sunAzimuthAngle, exposure);
    }
}

// 实际调用渲染器创建结合纹理和大气渲染的场景的方法
void SimpleOSGViewer::invokeCreateTexturedAtmosphereScene()
{
    if (m_renderer) {
        m_renderer->createTexturedAtmosphereScene();
    }
}

// 实际调用渲染器创建结合天空盒和大气渲染的场景的方法
void SimpleOSGViewer::invokeCreateSkyboxAtmosphereScene()
{
    if (m_renderer) {
        m_renderer->createSkyboxAtmosphereScene();
    }
}

// 实际调用渲染器创建结合天空盒大气和PBR立方体的场景的方法
void SimpleOSGViewer::invokeCreateSkyboxAtmosphereWithPBRScene()
{
    if (m_renderer) {
        m_renderer->createSkyboxAtmosphereWithPBRScene();
    }
}
