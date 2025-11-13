#include "mousehandler.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <osgGA/GUIEventAdapter>
#include <osgViewer/ViewerEventHandlers>
#include <QDebug>

MouseHandler::MouseHandler()
{
}

MouseHandler::~MouseHandler()
{
}

bool MouseHandler::processEvent(const QEvent* event, osgViewer::Viewer* viewer, 
                               SimpleOSGViewer::ViewType viewType, ViewManager* viewManager)
{
    if (!event || !viewer) {
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseMove:
        return handleMouseMove(static_cast<const QMouseEvent*>(event), viewer, viewType, viewManager);
    case QEvent::MouseButtonPress:
        return handleMousePress(static_cast<const QMouseEvent*>(event), viewer, viewType, viewManager);
    case QEvent::MouseButtonRelease:
        return handleMouseRelease(static_cast<const QMouseEvent*>(event), viewer, viewType, viewManager);
    case QEvent::Wheel:
        return handleWheelEvent(static_cast<const QWheelEvent*>(event), viewer, viewType, viewManager);
    default:
        return false;
    }
}

bool MouseHandler::handleMouseMove(const QMouseEvent* event, osgViewer::Viewer* viewer, 
                                  SimpleOSGViewer::ViewType viewType, ViewManager* viewManager)
{
    if (!event || !viewer) {
        return false;
    }

    // 获取viewer的事件队列
    osgViewer::Viewer::Contexts contexts;
    viewer->getContexts(contexts);
    
    if (contexts.empty()) {
        return false;
    }
    
    osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(contexts[0]);
    if (!gw) {
        return false;
    }
    
    osgGA::EventQueue* queue = gw->getEventQueue();
    if (!queue) {
        return false;
    }
    
    // 处理鼠标移动事件
    float x = static_cast<float>(event->x());
    float y = static_cast<float>(event->y());
    queue->mouseMotion(x, y);

    // 更新视图管理器中的相机参数
    if (viewManager) {
        viewManager->updateViewParametersFromManipulator(viewer);
    }

    return true;
}

bool MouseHandler::handleMousePress(const QMouseEvent* event, osgViewer::Viewer* viewer, 
                                   SimpleOSGViewer::ViewType viewType, ViewManager* viewManager)
{
    if (!event || !viewer) {
        return false;
    }

    // 获取viewer的事件队列
    osgViewer::Viewer::Contexts contexts;
    viewer->getContexts(contexts);
    
    if (contexts.empty()) {
        return false;
    }
    
    osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(contexts[0]);
    if (!gw) {
        return false;
    }
    
    osgGA::EventQueue* queue = gw->getEventQueue();
    if (!queue) {
        return false;
    }
    
    // 处理鼠标按下事件
    float x = static_cast<float>(event->x());
    float y = static_cast<float>(event->y());
    queue->mouseButtonPress(x, y, event->button());

    // 更新视图管理器中的相机参数
    if (viewManager) {
        viewManager->updateViewParametersFromManipulator(viewer);
    }

    return true;
}

bool MouseHandler::handleMouseRelease(const QMouseEvent* event, osgViewer::Viewer* viewer, 
                                     SimpleOSGViewer::ViewType viewType, ViewManager* viewManager)
{
    if (!event || !viewer) {
        return false;
    }

    // 获取viewer的事件队列
    osgViewer::Viewer::Contexts contexts;
    viewer->getContexts(contexts);
    
    if (contexts.empty()) {
        return false;
    }
    
    osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(contexts[0]);
    if (!gw) {
        return false;
    }
    
    osgGA::EventQueue* queue = gw->getEventQueue();
    if (!queue) {
        return false;
    }
    
    // 处理鼠标释放事件
    float x = static_cast<float>(event->x());
    float y = static_cast<float>(event->y());
    queue->mouseButtonRelease(x, y, event->button());

    // 更新视图管理器中的相机参数
    if (viewManager) {
        viewManager->updateViewParametersFromManipulator(viewer);
    }

    return true;
}

bool MouseHandler::handleWheelEvent(const QEvent* event, osgViewer::Viewer* viewer, 
                                   SimpleOSGViewer::ViewType viewType, ViewManager* viewManager)
{
    if (!event || !viewer) {
        return false;
    }

    // 将QEvent转换为QWheelEvent
    const QWheelEvent* wheelEvent = static_cast<const QWheelEvent*>(event);

    // 获取viewer的事件队列
    osgViewer::Viewer::Contexts contexts;
    viewer->getContexts(contexts);
    
    if (contexts.empty()) {
        return false;
    }
    
    osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(contexts[0]);
    if (!gw) {
        return false;
    }
    
    osgGA::EventQueue* queue = gw->getEventQueue();
    if (!queue) {
        return false;
    }
    
    // 处理滚轮事件
    // Qt6中使用angleDelta
    int delta = wheelEvent->angleDelta().y();
    
    if (delta > 0) {
        queue->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
    } else {
        queue->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
    }

    

    return true;
}