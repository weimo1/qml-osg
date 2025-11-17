#ifndef SIMPLEOSGRENDERER_H
#define SIMPLEOSGRENDERER_H

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QEvent>
#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Geode>

// 包含视图类型枚举
#include "simpleosgviewer.h"
  
#include "mousehandler.h"
#include "uihandler.h"

class SimpleOSGRenderer : public QQuickFramebufferObject::Renderer
{
public:
    SimpleOSGRenderer(SimpleOSGViewer::ViewType viewType = SimpleOSGViewer::MainView);
    ~SimpleOSGRenderer() override;

    void render() override;
    // 去掉const修饰符
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
    bool processEvent(const QEvent* event);
    
    // 添加获取viewer和viewManager的方法
    osgViewer::Viewer* getViewer() const { return m_viewer; }     

    void resetToHomeView();
    void fitToView();  // 添加适应视图功能

    void loadOSGFile(const QString& fileName);
    void setViewType(SimpleOSGViewer::ViewType viewType);
    
    void toggleLighting(bool enabled);  // 添加光照控制方法
    void createAtmosphere();  // 添加大气渲染方法
    void testMRT();  // 添加MRT测试方法

    // 添加模型选择相关方法
    void selectModel(int x, int y);
    osg::ref_ptr<osg::Drawable>  pickGeometry(int x, int y);  
    
    // 获取UIHandler实例
    UIHandler* getUIHandler() { return m_uiHandler; }
    
    // 设置viewer引用
    void setViewer(SimpleOSGViewer* viewer) { m_osgViewer = viewer; }
    
private:
    void initializeOSG(int width, int height);
    void checkGLError(const char* location);
    osg::ref_ptr<osg::Geode> createShape(osg::Vec3 center); // 添加createShape函数声明

    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<osg::Group> m_rootNode;
    bool m_initialized;    
    MouseHandler* m_mouseHandler;
    UIHandler* m_uiHandler;
    SimpleOSGViewer* m_osgViewer;  // 添加对SimpleOSGViewer的引用
};

#endif // SIMPLEOSGRENDERER_H