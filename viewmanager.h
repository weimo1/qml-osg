#ifndef VIEWMANAGER_H
#define VIEWMANAGER_H

#include <osg/Vec3d>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osgViewer/Viewer>
#include "simpleosgviewer.h"

class ViewManager
{
public:
    ViewManager();
    ~ViewManager();
    
    // 设置视图参数
    void setViewParameters(osg::Vec3d eye, osg::Vec3d center, osg::Vec3d up);
    
    // 从操作器获取当前视图参数
    void updateViewParametersFromManipulator(osgViewer::Viewer* viewer);
    
    // 获取视图参数
    osg::Vec3d getEye() const { return m_eye; }
    osg::Vec3d getCenter() const { return m_center; }
    osg::Vec3d getUp() const { return m_up; }
    
    // 根据视图类型设置相机
    void setupCameraForView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height, SimpleOSGViewer::ViewType viewType);
    
    // 设置特定视图类型
    void setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType);
    
    // 更新投影（用于滚轮缩放）- 保持函数名但只处理透视投影
    void updateOrthographicProjection(osgViewer::Viewer* viewer, int width, int height);
    
private:
    // 视图参数
    osg::Vec3d m_eye;
    osg::Vec3d m_center;
    osg::Vec3d m_up;
    
    // 视图相关的辅助函数
    void setupFrontView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
    void setupSideView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
    void setupTopView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
    void setupMainView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height);
};

#endif // VIEWMANAGER_H