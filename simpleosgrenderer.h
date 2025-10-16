#ifndef SIMPLEOSGRENDERER_H
#define SIMPLEOSGRENDERER_H

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QEvent>
#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <osg/Group>

// 包含视图类型枚举
#include "simpleosgviewer.h"
#include "viewmanager.h"
#include "uihandler.h"

// 前向声明
class MouseHandler;

class SimpleOSGRenderer : public QQuickFramebufferObject::Renderer
{
public:
    SimpleOSGRenderer(SimpleOSGViewer::ViewType viewType = SimpleOSGViewer::MainView);
    ~SimpleOSGRenderer() override;

    void render() override;
    // 去掉const修饰符
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
    bool processEvent(const QEvent* event);
    
    // 添加功能方法
    void createShape();
    void createPBRScene();  // 添加PBR场景创建方法
    void resetView(SimpleOSGViewer::ViewType viewType = SimpleOSGViewer::MainView);
    void resetToHomeView();  // 添加回归主视角的函数
    void loadOSGFile(const QString& fileName);
    void setViewType(SimpleOSGViewer::ViewType viewType);  // 添加设置视图类型的方法
    void setShapeColor(float r, float g, float b, float a = 1.0f);  // 添加设置图形颜色的方法
    
    // 添加选择相关方法
    void selectModel(int x, int y);  // 添加选择模型的方法
    osg::Geometry* pickGeometry(int x, int y);  // 添加拾取几何体的方法
    
    // 获取视图管理器，用于在UI中显示相机位置信息
    ViewManager* getViewManager() { return m_uiHandler->getViewManager(); }
    
    // 获取Viewer，用于在UI中访问Viewer
    osgViewer::Viewer* getViewer() { return m_viewer.get(); }
    
private:
    void initializeOSG(int width, int height);
    void createSimpleScene();
    void checkGLError(const char* location);
    
    // 添加Shader相关方法
    void setDrawableColor(osg::Geometry* geom, const osg::Vec4& color);
    void highlightGeometry(osg::Geometry* geom);  // 添加高亮几何体的方法
    void changeGeometryColor(osg::Geometry* geom);  // 添加更改几何体颜色的方法

    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<osg::Group> m_rootNode;
    bool m_initialized;
    SimpleOSGViewer::ViewType m_viewType;  // 视图类型
    
    // 添加选择相关成员变量
    osg::ref_ptr<osg::Geometry> _lastDrawable;
    
    // 保存对创建的图形节点的引用
    osg::ref_ptr<osg::Geode> m_shapeNode;
    
    // 添加处理器成员变量
    MouseHandler* m_mouseHandler;
    UIHandler* m_uiHandler;
};

#endif // SIMPLEOSGRENDERER_H