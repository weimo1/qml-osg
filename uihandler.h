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
#include "demoshader.h"  // 添加DemoShader头文件
#include "viewmanager.h"

class SkyNode;
class UIHandler
{
public:
    UIHandler();
    ~UIHandler();
    
    void createShape(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode);
    void createShapeWithNewSkybox(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode);
    void createPBRScene(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode);  // 添加PBR场景创建函数
    void createAtmosphereScene(osgViewer::Viewer* viewer, osg::Group* rootNode);  // 添加大气渲染场景创建函数
    void createTexturedAtmosphereScene(osgViewer::Viewer* viewer, osg::Group* rootNode); // 添加结合纹理和大气渲染的场景创建函数
    void createSkyboxAtmosphereScene(osgViewer::Viewer* viewer, osg::Group* rootNode); // 添加结合天空盒和大气渲染的场景创建函数
    void resetView(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType);
    void resetToHomeView(osgViewer::Viewer* viewer, osg::Group* rootNode);
    void loadOSGFile(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& fileName);
    void setShapeColor(osg::Geode* shapeNode, float r, float g, float b, float a);
    
    // 添加相机相关的函数
    void setupCameraForView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height, SimpleOSGViewer::ViewType viewType);
    void setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType);
    
    // 获取视图管理器，用于在UI中显示相机位置信息
    ViewManager* getViewManager() { return &m_viewManager; }
    
    // 更新大气渲染参数
    void updateAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode, 
                                  float sunZenithAngle, float sunAzimuthAngle);
    
    // 更新大气密度和太阳强度
    void updateAtmosphereDensityAndIntensity(osgViewer::Viewer* viewer, osg::Group* rootNode, 
                                           float density, float intensity);
    
    // 更新米氏散射和瑞利散射
    void updateAtmosphereScattering(osgViewer::Viewer* viewer, osg::Group* rootNode, 
                                  float mie, float rayleigh);
    
    // 更新Textured Atmosphere场景参数
    void updateTexturedAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                          float sunZenithAngle, float sunAzimuthAngle,
                                          float exposure);
    
    // 更新SkyNode中的大气参数
    void updateSkyNodeAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                        float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG);
    
private:
    ViewManager m_viewManager;
    osg::ref_ptr<DemoShader> m_demoShader;  // 保存DemoShader引用
};

#endif // UIHANDLER_H