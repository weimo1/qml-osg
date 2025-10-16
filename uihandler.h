#ifndef UIHANDLER_H
#define UIHANDLER_H

#include <QString>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgViewer/Viewer>
#include "simpleosgviewer.h"
#include "shadercube.h"
#include "shaderpbr.h"  // 添加PBR头文件
#include "viewmanager.h"

class UIHandler
{
public:
    UIHandler();
    ~UIHandler();
    
    void createShape(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode);
    void createPBRScene(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode);  // 添加PBR场景创建函数
    void resetView(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType);
    void resetToHomeView(osgViewer::Viewer* viewer, osg::Group* rootNode);
    void loadOSGFile(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& fileName);
    void setShapeColor(osg::Geode* shapeNode, float r, float g, float b, float a);
    
    // 添加相机相关的函数
    void setupCameraForView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height, SimpleOSGViewer::ViewType viewType);
    void setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType);
    
    // 获取视图管理器，用于在UI中显示相机位置信息
    ViewManager* getViewManager() { return &m_viewManager; }
    
private:
    ViewManager m_viewManager;
};

#endif // UIHANDLER_H