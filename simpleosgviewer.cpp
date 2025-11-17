#include "simpleosgviewer.h"
#include "simpleosgrenderer.h"
#include <QOpenGLFramebufferObject>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QTimer>

SimpleOSGViewer::SimpleOSGViewer(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_renderer(nullptr)
    , m_viewType(MainView)
    , m_mouseX(0)
    , m_mouseY(0)
{
     setMirrorVertically(true);//osg和qml的Y轴朝向是反的
    setTextureFollowsItemSize(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);

    setFlag(QQuickItem::ItemIsFocusScope);  // 设置为焦点范围
    
    // 使用定时器定期更新，避免线程问题
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SimpleOSGViewer::update);
    timer->start(16);
}

QQuickFramebufferObject::Renderer *SimpleOSGViewer::createRenderer() const
{
    // 创建渲染器实例
    SimpleOSGViewer* self = const_cast<SimpleOSGViewer*>(this);
    self->m_renderer = new SimpleOSGRenderer(m_viewType);
    // 设置viewer引用
    self->m_renderer->setViewer(self);
    return self->m_renderer;
}

// 添加鼠标事件处理函数
void SimpleOSGViewer::mousePressEvent(QMouseEvent *event)
{
    m_mouseX = event->x();
    m_mouseY = event->y();
    emit mousePositionChanged();
    
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

void SimpleOSGViewer::mouseMoveEvent(QMouseEvent *event)
{
    m_mouseX = event->x();
    m_mouseY = event->y();
    emit mousePositionChanged();
    
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

void SimpleOSGViewer::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseX = event->x();
    m_mouseY = event->y();
    emit mousePositionChanged();
    
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

void SimpleOSGViewer::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 双击事件处理
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

void SimpleOSGViewer::wheelEvent(QWheelEvent *event)
{
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

void SimpleOSGViewer::keyPressEvent(QKeyEvent *event)
{
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

void SimpleOSGViewer::keyReleaseEvent(QKeyEvent *event)
{
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

void SimpleOSGViewer::hoverMoveEvent(QHoverEvent *event)
{
    m_mouseX = event->pos().x();
    m_mouseY = event->pos().y();
    emit mousePositionChanged();
    
    if (m_renderer) {
        m_renderer->processEvent(event);
    }
    
    // 不要设置event->setAccepted(true)，让事件继续传播
    // event->setAccepted(true);
}

// 视图类型属性的getter和setter
SimpleOSGViewer::ViewType SimpleOSGViewer::viewType() const
{
    return m_viewType;
}

void SimpleOSGViewer::setViewType(ViewType viewType)
{
    if (m_viewType != viewType) {
        m_viewType = viewType;
        emit viewTypeChanged();
    }
}

// 添加公共方法供QML调用
void SimpleOSGViewer::loadOSGFile(const QString& fileName)
{
    if (m_renderer) {
        m_renderer->loadOSGFile(fileName);
    }
}

void SimpleOSGViewer::resetToHomeView()
{
    if (m_renderer) {
        m_renderer->resetToHomeView();
    }
}

void SimpleOSGViewer::fitToView()
{
    if (m_renderer) {
        m_renderer->fitToView();
    }
}

void SimpleOSGViewer::toggleLighting(bool enabled)
{
    if (m_renderer) {
        m_renderer->toggleLighting(enabled);
    }
}

void SimpleOSGViewer::createAtmosphere()
{
    if (m_renderer) {
        m_renderer->createAtmosphere();
    }
}

// 添加MRT测试方法
void SimpleOSGViewer::testMRT()
{
    if (m_renderer) {
        m_renderer->testMRT();
    }
}

// 添加内部调用的槽函数
void SimpleOSGViewer::invokeResetView()
{
    resetToHomeView();
}

void SimpleOSGViewer::invokeSetViewType(ViewType viewType)
{
    setViewType(viewType);
    if (m_renderer) {
        m_renderer->setViewType(viewType);
    }
}

void SimpleOSGViewer::invokeLoadOSGFile(const QString& fileName)
{
    loadOSGFile(fileName);
}

void SimpleOSGViewer::invokeFitToView()
{
    fitToView();
}

void SimpleOSGViewer::invokeToggleLighting(bool enabled)
{
    toggleLighting(enabled);
}

void SimpleOSGViewer::invokeCreateAtmosphere()
{
    createAtmosphere();
}

// 添加MRT测试槽函数
void SimpleOSGViewer::invokeTestMRT()
{
    testMRT();
}

// 更新相机位置的方法
void SimpleOSGViewer::updateCameraPosition(double x, double y, double z)
{
    if (m_cameraX != x || m_cameraY != y || m_cameraZ != z) {
        m_cameraX = x;
        m_cameraY = y;
        m_cameraZ = z;
        emit cameraPositionChanged();
    }
}