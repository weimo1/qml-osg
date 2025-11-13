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
#include <QKeyEvent>  // 添加键盘事件头文件

SimpleOSGViewer::SimpleOSGViewer(QQuickItem *parent)
    : QQuickFramebufferObject(parent), m_renderer(nullptr), m_viewType(MainView), m_mouseX(0), m_mouseY(0)
{
    setMirrorVertically(true);//osg和qml的Y轴朝向是反的
    setTextureFollowsItemSize(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);

    setFlag(QQuickItem::ItemIsFocusScope);  // 设置为焦点范围
    
    // 使用定时器定期更新，避免线程问题
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SimpleOSGViewer::update);
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

// 添加键盘按下事件处理函数
void SimpleOSGViewer::keyPressEvent(QKeyEvent *event)
{
    if (m_renderer) {
        // 将键盘事件传递给渲染器处理
        m_renderer->processEvent(event);
    }
    
    // 调用父类处理确保事件被正确处理
    QQuickFramebufferObject::keyPressEvent(event);
}

// 添加键盘释放事件处理函数
void SimpleOSGViewer::keyReleaseEvent(QKeyEvent *event)
{
    if (m_renderer) {
        // 将键盘事件传递给渲染器处理
        m_renderer->processEvent(event);
    }
    
    // 调用父类处理确保事件被正确处理
    QQuickFramebufferObject::keyReleaseEvent(event);
}

// 实现重置视图功能
void SimpleOSGViewer::resetToHomeView()
{
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeResetView", Qt::QueuedConnection);
    }
}

// 实现适应视图功能
void SimpleOSGViewer::fitToView()
{
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeFitToView", Qt::QueuedConnection);
    }
}

// 实现加载OSG文件功能
void SimpleOSGViewer::loadOSGFile(const QString& fileName)
{
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeLoadOSGFile", Qt::QueuedConnection, Q_ARG(QString, fileName));
    }
} 
 

// 添加光照控制功能
void SimpleOSGViewer::toggleLighting(bool enabled)
{
    if (m_renderer) {
        QMetaObject::invokeMethod(this, "invokeToggleLighting", Qt::QueuedConnection, Q_ARG(bool, enabled));
    }
}

// 添加invokeResetView槽函数的实现
void SimpleOSGViewer::invokeResetView()
{
    if (m_renderer) {
        m_renderer->resetToHomeView();
    }
}

// 添加invokeSetViewType槽函数的实现
void SimpleOSGViewer::invokeSetViewType(SimpleOSGViewer::ViewType viewType)
{
    if (m_renderer) {
        m_renderer->setViewType(viewType);
    }
}

// 添加invokeLoadOSGFile槽函数的实现
void SimpleOSGViewer::invokeLoadOSGFile(const QString& fileName)
{
    if (m_renderer) {
        m_renderer->loadOSGFile(fileName);
    }
}

// 添加invokeFitToView槽函数的实现
void SimpleOSGViewer::invokeFitToView()
{
    if (m_renderer) {
        m_renderer->fitToView();
    }
}

// 添加invokeToggleLighting槽函数的实现
void SimpleOSGViewer::invokeToggleLighting(bool enabled)
{
    if (m_renderer) {
        m_renderer->toggleLighting(enabled);
    }
}
