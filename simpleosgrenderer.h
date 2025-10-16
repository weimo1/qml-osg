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
    
    // 添加获取viewer和viewManager的方法
    osgViewer::Viewer* getViewer() const { return m_viewer; }
    ViewManager* getViewManager() const { return m_uiHandler->getViewManager(); }
    
    // 添加实际调用渲染器的槽函数
    void createShape();
    void createShapeWithNewSkybox();
    void createPBRScene();  // 添加PBR场景创建的槽函数
    void resetView(SimpleOSGViewer::ViewType viewType);
    void resetToHomeView();
    void loadOSGFile(const QString& fileName);
    void setViewType(SimpleOSGViewer::ViewType viewType);
    void setShapeColor(float r, float g, float b, float a = 1.0f);
    
    // 添加更新PBR材质的方法（包含Alpha参数）
    void updatePBRMaterial(float albedoR, float albedoG, float albedoB, float albedoA,
                          float metallic, float roughness, 
                          float specular, float ao);
    
    // 添加模型选择相关方法
    void selectModel(int x, int y);
    osg::Geometry* pickGeometry(int x, int y);
    void setDrawableColor(osg::Geometry* geom, const osg::Vec4& color);
    void highlightGeometry(osg::Geometry* geom);
    void changeGeometryColor(osg::Geometry* geom);

private:
    void initializeOSG(int width, int height);
    void createSimpleScene();
    void checkGLError(const char* location);

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