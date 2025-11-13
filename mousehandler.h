#ifndef MOUSEHANDLER_H
#define MOUSEHANDLER_H

#include <QEvent>
#include <osgViewer/Viewer>
#include "simpleosgviewer.h"
 

class MouseHandler
{
public:
    MouseHandler();
    ~MouseHandler();

    bool processEvent(const QEvent* event, osgViewer::Viewer* viewer);

    osg::ref_ptr<osg::Drawable> pickModel(int x, int y, osgViewer::Viewer* viewer);
private:
    bool handleMouseMove(const QMouseEvent* event, osgViewer::Viewer* viewer);
    bool handleMousePress(const QMouseEvent* event, osgViewer::Viewer* viewer);
    bool handleMouseRelease(const QMouseEvent* event, osgViewer::Viewer* viewer);
    
    bool handleWheelEvent(const QEvent* event, osgViewer::Viewer* viewer);
    
    // 添加键盘事件处理函数
    bool handleKeyPress(const QKeyEvent* event, osgViewer::Viewer* viewer);
    bool handleKeyRelease(const QKeyEvent* event, osgViewer::Viewer* viewer);
};

#endif // MOUSEHANDLER_H