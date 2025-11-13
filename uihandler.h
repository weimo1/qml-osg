#ifndef UIHANDLER_H
#define UIHANDLER_H

#include <QObject>
#include <QString>
#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Geode>
#include "simpleosgviewer.h"
#include "SkyNode.h"
#include "viewmanager.h"

class UIHandler : public QObject
{
    Q_OBJECT

public:
    UIHandler();
    ~UIHandler();

    // 视图操作相关方法
    void resetToHomeView(osgViewer::Viewer* viewer);
    void fitToView(osgViewer::Viewer* viewer, osg::Group* rootNode);
    void setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType);
    
    // 文件加载相关方法
    void loadOSGFile(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& fileName);
    void loadSingleOSGFile(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& fileName);
    void loadOSGFilesFromDirectory(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& dirPath);
    
    // 光照控制相关方法
    void toggleLighting(osgViewer::Viewer* viewer, osg::Group* rootNode, bool enabled);
    
    // 模型选择相关方法
    void selectModel(osgViewer::Viewer* viewer, int x, int y);
    
    // 天空盒相关方法
    void createSkyBox(osgViewer::Viewer* viewer, osg::Group* rootNode);
    
    // 获取ViewManager实例
    ViewManager* getViewManager();

private:
    ViewManager m_viewManager;
    osg::ref_ptr<SkyBoxThree> m_skyBox;
};

#endif // UIHANDLER_H