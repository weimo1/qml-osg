#ifndef MOUSEHANDLER_H
#define MOUSEHANDLER_H

#include <QEvent>
#include <osgViewer/Viewer>
#include "simpleosgviewer.h"
#include "viewmanager.h"

class MouseHandler
{
public:
    MouseHandler();
    ~MouseHandler();

    bool processEvent(const QEvent* event, osgViewer::Viewer* viewer, 
                     SimpleOSGViewer::ViewType viewType, ViewManager* viewManager);

private:
    bool handleMouseMove(const QMouseEvent* event, osgViewer::Viewer* viewer, 
                        SimpleOSGViewer::ViewType viewType, ViewManager* viewManager);
    bool handleMousePress(const QMouseEvent* event, osgViewer::Viewer* viewer, 
                         SimpleOSGViewer::ViewType viewType, ViewManager* viewManager);
    bool handleMouseRelease(const QMouseEvent* event, osgViewer::Viewer* viewer, 
                           SimpleOSGViewer::ViewType viewType, ViewManager* viewManager);
    
    bool handleWheelEvent(const QEvent* event, osgViewer::Viewer* viewer, 
                           SimpleOSGViewer::ViewType viewType, ViewManager* viewManager);
};

#endif // MOUSEHANDLER_H