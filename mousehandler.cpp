#include "mousehandler.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>  // 添加键盘事件头文件
#include <osgGA/GUIEventAdapter>
#include <osgViewer/ViewerEventHandlers>
#include <QDebug>

MouseHandler::MouseHandler()
{
}

MouseHandler::~MouseHandler()
{
}

bool MouseHandler::processEvent(const QEvent* event, osgViewer::Viewer* viewer)
{
    if (!event || !viewer) {
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseMove:
        return handleMouseMove(static_cast<const QMouseEvent*>(event), viewer);
    case QEvent::MouseButtonPress:
        return handleMousePress(static_cast<const QMouseEvent*>(event), viewer);
    case QEvent::MouseButtonRelease:
        return handleMouseRelease(static_cast<const QMouseEvent*>(event), viewer);
    case QEvent::Wheel:
        return handleWheelEvent(static_cast<const QWheelEvent*>(event), viewer);
    case QEvent::KeyPress:
        return handleKeyPress(static_cast<const QKeyEvent*>(event), viewer);
    case QEvent::KeyRelease:
        return handleKeyRelease(static_cast<const QKeyEvent*>(event), viewer);
    default:
        return false;
    }
}

bool MouseHandler::handleMouseMove(const QMouseEvent* event, osgViewer::Viewer* viewer)
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
    
    // 获取视口高度以正确转换Y坐标
    osg::Viewport* viewport = viewer->getCamera()->getViewport();
    if (viewport) {
        y = viewport->height() - y;  // OSG使用左下角为原点，Qt使用左上角为原点
    }
    
    // 传递鼠标移动事件给OSG
    queue->mouseMotion(x, y);
    
    return true;
}

bool MouseHandler::handleMousePress(const QMouseEvent* event, osgViewer::Viewer* viewer)
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
    
    // 获取视口高度以正确转换Y坐标
    osg::Viewport* viewport = viewer->getCamera()->getViewport();
    if (viewport) {
        y = viewport->height() - y;  // OSG使用左下角为原点，Qt使用左上角为原点
    }
    
    // 传递鼠标按下事件给OSG
    queue->mouseButtonPress(x, y, event->button());
 
    return true;
}

bool MouseHandler::handleMouseRelease(const QMouseEvent* event, osgViewer::Viewer* viewer)
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
    
    // 获取视口高度以正确转换Y坐标
    osg::Viewport* viewport = viewer->getCamera()->getViewport();
    if (viewport) {
        y = viewport->height() - y;  // OSG使用左下角为原点，Qt使用左上角为原点
    }
    
    // 传递鼠标释放事件给OSG
    queue->mouseButtonRelease(x, y, event->button());
    
    return true;
}

bool MouseHandler::handleWheelEvent(const QEvent* event, osgViewer::Viewer* viewer)
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

// 添加键盘按下事件处理函数
bool MouseHandler::handleKeyPress(const QKeyEvent* event, osgViewer::Viewer* viewer)
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
    
    // 处理键盘按下事件
    int key = event->key();
    QString text = event->text();
    
    // 将Qt键值转换为OSG键值
    osgGA::GUIEventAdapter::KeySymbol osgKey =(osgGA::GUIEventAdapter::KeySymbol)0;
    switch (key) {
    case Qt::Key_Space:
        osgKey = osgGA::GUIEventAdapter::KEY_Space;
        break;
    case Qt::Key_Escape:
        osgKey = osgGA::GUIEventAdapter::KEY_Escape;
        break;
    case Qt::Key_Return:
        osgKey = osgGA::GUIEventAdapter::KEY_Return;
        break;
    case Qt::Key_Tab:
        osgKey = osgGA::GUIEventAdapter::KEY_Tab;
        break;
    case Qt::Key_Backspace:
        osgKey = osgGA::GUIEventAdapter::KEY_BackSpace;
        break;
    case Qt::Key_Left:
        osgKey = osgGA::GUIEventAdapter::KEY_Left;
        break;
    case Qt::Key_Right:
        osgKey = osgGA::GUIEventAdapter::KEY_Right;
        break;
    case Qt::Key_Up:
        osgKey = osgGA::GUIEventAdapter::KEY_Up;
        break;
    case Qt::Key_Down:
        osgKey = osgGA::GUIEventAdapter::KEY_Down;
        break;
    case Qt::Key_PageUp:
        osgKey = osgGA::GUIEventAdapter::KEY_Page_Up;
        break;
    case Qt::Key_PageDown:
        osgKey = osgGA::GUIEventAdapter::KEY_Page_Down;
        break;
    case Qt::Key_Home:
        osgKey = osgGA::GUIEventAdapter::KEY_Home;
        break;
    case Qt::Key_End:
        osgKey = osgGA::GUIEventAdapter::KEY_End;
        break;
    case Qt::Key_Delete:
        osgKey = osgGA::GUIEventAdapter::KEY_Delete;
        break;
    case Qt::Key_F1:
        osgKey = osgGA::GUIEventAdapter::KEY_F1;
        break;
    case Qt::Key_F2:
        osgKey = osgGA::GUIEventAdapter::KEY_F2;
        break;
    case Qt::Key_F3:
        osgKey = osgGA::GUIEventAdapter::KEY_F3;
        break;
    case Qt::Key_F4:
        osgKey = osgGA::GUIEventAdapter::KEY_F4;
        break;
    case Qt::Key_F5:
        osgKey = osgGA::GUIEventAdapter::KEY_F5;
        break;
    case Qt::Key_F6:
        osgKey = osgGA::GUIEventAdapter::KEY_F6;
        break;
    case Qt::Key_F7:
        osgKey = osgGA::GUIEventAdapter::KEY_F7;
        break;
    case Qt::Key_F8:
        osgKey = osgGA::GUIEventAdapter::KEY_F8;
        break;
    case Qt::Key_F9:
        osgKey = osgGA::GUIEventAdapter::KEY_F9;
        break;
    case Qt::Key_F10:
        osgKey = osgGA::GUIEventAdapter::KEY_F10;
        break;
    case Qt::Key_F11:
        osgKey = osgGA::GUIEventAdapter::KEY_F11;
        break;
    case Qt::Key_F12:
        osgKey = osgGA::GUIEventAdapter::KEY_F12;
        break;
    default:   
        if (!text.isEmpty())
        {
			char code = text[0].toLatin1();
			if ((code >= 'a' && code <= 'z') || (code >= 'A' && code <= 'Z') ||	(code >= '0' && code <= '9'))
                osgKey=(osgGA::GUIEventAdapter::KeySymbol)code;
        }
        break;
    }
    
    // 如果有有效的OSG键值，则使用它
    if (osgKey !=  0) {
        queue->keyPress(osgKey);
    }
    
    return true;
}

// 添加键盘释放事件处理函数
bool MouseHandler::handleKeyRelease(const QKeyEvent* event, osgViewer::Viewer* viewer)
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
    
    // 处理键盘释放事件
    int key = event->key();
    QString text = event->text();
    
    // 将Qt键值转换为OSG键值
    osgGA::GUIEventAdapter::KeySymbol osgKey = (osgGA::GUIEventAdapter::KeySymbol)0;
    switch (key) {
    case Qt::Key_Space:
        osgKey = osgGA::GUIEventAdapter::KEY_Space;
        break;
    case Qt::Key_Escape:
        osgKey = osgGA::GUIEventAdapter::KEY_Escape;
        break;
    case Qt::Key_Return:
        osgKey = osgGA::GUIEventAdapter::KEY_Return;
        break;
    case Qt::Key_Tab:
        osgKey = osgGA::GUIEventAdapter::KEY_Tab;
        break;  
    case Qt::Key_Backspace:
        osgKey = osgGA::GUIEventAdapter::KEY_BackSpace;
        break;
    case Qt::Key_Left:
        osgKey = osgGA::GUIEventAdapter::KEY_Left;
        break;
    case Qt::Key_Right:
        osgKey = osgGA::GUIEventAdapter::KEY_Right;
        break;
    case Qt::Key_Up:
        osgKey = osgGA::GUIEventAdapter::KEY_Up;
        break;
    case Qt::Key_Down:
        osgKey = osgGA::GUIEventAdapter::KEY_Down;
        break;
    case Qt::Key_PageUp:
        osgKey = osgGA::GUIEventAdapter::KEY_Page_Up;
        break;
    case Qt::Key_PageDown:
        osgKey = osgGA::GUIEventAdapter::KEY_Page_Down;
        break;
    case Qt::Key_Home:
        osgKey = osgGA::GUIEventAdapter::KEY_Home;
        break;
    case Qt::Key_End:
        osgKey = osgGA::GUIEventAdapter::KEY_End;
        break;
    case Qt::Key_Delete:
        osgKey = osgGA::GUIEventAdapter::KEY_Delete;
        break;
    case Qt::Key_F1:
        osgKey = osgGA::GUIEventAdapter::KEY_F1;
        break;
    case Qt::Key_F2:
        osgKey = osgGA::GUIEventAdapter::KEY_F2;
        break;
    case Qt::Key_F3:
        osgKey = osgGA::GUIEventAdapter::KEY_F3;
        break;
    case Qt::Key_F4:
        osgKey = osgGA::GUIEventAdapter::KEY_F4;
        break;
    case Qt::Key_F5:
        osgKey = osgGA::GUIEventAdapter::KEY_F5;
        break;
    case Qt::Key_F6:
        osgKey = osgGA::GUIEventAdapter::KEY_F6;
        break;
    case Qt::Key_F7:
        osgKey = osgGA::GUIEventAdapter::KEY_F7;
        break;
    case Qt::Key_F8:
        osgKey = osgGA::GUIEventAdapter::KEY_F8;
        break;
    case Qt::Key_F9:
        osgKey = osgGA::GUIEventAdapter::KEY_F9;
        break;
    case Qt::Key_F10:
        osgKey = osgGA::GUIEventAdapter::KEY_F10;
        break;
    case Qt::Key_F11:
        osgKey = osgGA::GUIEventAdapter::KEY_F11;
        break;
    case Qt::Key_F12:
        osgKey = osgGA::GUIEventAdapter::KEY_F12;
        break;
    default:
    {
		if (!text.isEmpty())
		{
			char code = text[0].toLatin1();
			if ((code >= 'a' && code <= 'z') || (code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9'))
				osgKey = (osgGA::GUIEventAdapter::KeySymbol)code;
		}
		break;
    }
 
    }
    
    // 如果有有效的OSG键值，则使用它
    if (osgKey != 0) {
        queue->keyRelease(osgKey);
    }
    
    return true;
}

osg::ref_ptr<osg::Drawable> MouseHandler::pickModel(int dx, int dy, osgViewer::Viewer* viewer)
{
    if (nullptr == viewer) {
        return  nullptr;
    }
     
    int dEps = 5;
    osg::ref_ptr<osgUtil::PolytopeIntersector> picker = new osgUtil::PolytopeIntersector(osgUtil::Intersector::WINDOW, dx - dEps, dy - dEps, dx + dEps, dy + dEps);
    picker->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);

    osgUtil::IntersectionVisitor iv(picker);
    viewer->getCamera()->accept(iv);
    if (picker->containsIntersections())
    {
        osgUtil::PolytopeIntersector::Intersection f1 = picker->getFirstIntersection();
        if (!f1.nodePath.empty())
        {
            if (f1.drawable) {
                return f1.drawable;
            }
        }
    }

    return nullptr;
}
 
