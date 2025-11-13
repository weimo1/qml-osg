#ifndef VIEWMANAGER_H
#define VIEWMANAGER_H

#include <osg/Vec3d>
#include <osgViewer/Viewer>
#include <osg/Group>
#include "simpleosgviewer.h"

class ViewManager
{
public:
    ViewManager();
    ~ViewManager();

    // 视图相关的公共方法
    void setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType);
    void resetToHomeView(osgViewer::Viewer* viewer);
    void fitToView(osgViewer::Viewer* viewer, osg::Group* rootNode);
    
    // 相机操作相关方法
    void setupCameraForView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height, SimpleOSGViewer::ViewType viewType);
    void updateViewParametersFromManipulator(osgViewer::Viewer* viewer);

    // 获取视图参数
    osg::Vec3d getEye() const { return m_eye; }
    osg::Vec3d getCenter() const { return m_center; }
    osg::Vec3d getUp() const { return m_up; }

private:
    // 视图相关的辅助函数
    void setupFrontView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
    void setupSideView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
    void setupTopView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
    void setupMainView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
    
    // 设置视图参数
    void setViewParameters(osg::Vec3d eye, osg::Vec3d center, osg::Vec3d up);

    // 视图参数
    osg::Vec3d m_eye;
    osg::Vec3d m_center;
    osg::Vec3d m_up;
};

#endif // VIEWMANAGER_H