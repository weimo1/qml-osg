#include "simpleosgrenderer.h"
#include "simpleosgviewer.h"
#include "mousehandler.h"
#include "uihandler.h"
#include "demoshader.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Camera>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Material>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Options>
#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventAdapter>
#include <osgGA/CameraManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osg/Program>
#include <osg/Shader>
#include <QDebug>
#include "shadercube.h"

SimpleOSGRenderer::SimpleOSGRenderer(SimpleOSGViewer::ViewType viewType)
    : m_initialized(false), m_viewType(viewType), m_mouseHandler(new MouseHandler()), m_uiHandler(new UIHandler())
{
    
}

SimpleOSGRenderer::~SimpleOSGRenderer()
{
    delete m_mouseHandler;
    delete m_uiHandler;
}



void SimpleOSGRenderer::initializeOSG(int width, int height)
{
    // 检查OpenGL上下文
    if (!QOpenGLContext::currentContext()) {
         qCritical() << "No OpenGL context available!";
        return;
    }
    
    try {
        // 创建Viewer
        m_viewer = new osgViewer::Viewer();
        
        // 创建根节点
        m_rootNode = new osg::Group();
        
        // 创建图形环境
        osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow = 
            new osgViewer::GraphicsWindowEmbedded(0, 0, width, height);
        
        // 设置相机
        osg::Camera* camera = m_viewer->getCamera();
        camera->setGraphicsContext(graphicsWindow.get());
        // 使用UIHandler来设置相机
        m_uiHandler->setupCameraForView(m_viewer, m_rootNode, width, height, m_viewType);
        camera->setClearColor(osg::Vec4(0.5f, 0.7f, 1.0f, 1.0f)); // 蓝色背景
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 激活OSG内置的uniform变量
        camera->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);
        
        // 设置渲染器特性
        m_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
        
        // 设置视图场景
        m_viewer->setSceneData(m_rootNode.get());
        
        // 添加简单场景
        createSimpleScene();
        
        // 启用轨迹球操作器，允许鼠标旋转 (所有视图类型)
        osg::ref_ptr<osgGA::TrackballManipulator> manipulator = new osgGA::TrackballManipulator;
        
        // 设置操作器参数以改善旋转体验
        manipulator->setVerticalAxisFixed(false);  // 不固定垂直轴
        manipulator->setAllowThrow(false);         // 禁止投掷效果，避免自动旋转
        
        m_viewer->setCameraManipulator(manipulator.get());
        
        osg::Vec3 center(0.0f, 0.0f, 0.0f);  

        
        // 获取场景的边界框，用于设置正确的旋转中心
        osg::ComputeBoundsVisitor boundsVisitor;
        m_rootNode->accept(boundsVisitor);
        osg::BoundingBox bb = boundsVisitor.getBoundingBox();
        
        if (bb.valid()) {
            // 使用场景的中心作为旋转中心
            center = bb.center();
        }
        
        // 设置操作器的home位置
        osg::Vec3 eye(0.0f, -5.0f, 0.0f);
        osg::Vec3 up(0.0f, 0.0f, 1.0f);
        manipulator->setHomePosition(eye, center, up);
        
        // 添加事件处理器
        m_viewer->addEventHandler(new osgViewer::StatsHandler);
        m_viewer->addEventHandler(new osgViewer::WindowSizeHandler);
        
        // 禁用自动编译
        m_viewer->setIncrementalCompileOperation(nullptr);
        
        // 初始化viewer
        m_viewer->realize();
        
        // checkGLError("After initializeOSG");
    }
    catch (const std::exception& e) {
        // qCritical() << "Error initializing OSG:" << e.what();
    }
    catch (...) {
        // qCritical() << "Unknown error initializing OSG";
    }
}



// 实现创建图形功能
void SimpleOSGRenderer::createShape()
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->createShape(m_viewer, m_rootNode, m_shapeNode);
        // 重置视图以适应新创建的立方体
        resetView(m_viewType);
    }
}

// 实现创建PBR场景功能
void SimpleOSGRenderer::createPBRScene()
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->createPBRScene(m_viewer, m_rootNode, m_shapeNode);
        // 重置视图以适应新创建的场景
        resetView(m_viewType);
        
        // 确保相机操作器正确设置
        osgGA::TrackballManipulator* manipulator = dynamic_cast<osgGA::TrackballManipulator*>(m_viewer->getCameraManipulator());
        if (manipulator) {
            // 设置操作器的home位置
            osg::Vec3d eye(0.0, -5.0, 0.0);
            osg::Vec3d center(0.0, 0.0, 0.0);
            osg::Vec3d up(0.0, 0.0, 1.0);
            manipulator->setHomePosition(eye, center, up);
        }
    }
}

// 实现创建大气渲染场景功能
void SimpleOSGRenderer::createAtmosphereScene()
{
    if (m_viewer && m_rootNode) {
        // 清除现有场景
        m_uiHandler->createAtmosphereScene(m_viewer, m_rootNode);
      
    }
}

// 新增：实现创建结合纹理和大气渲染的场景功能
void SimpleOSGRenderer::createTexturedAtmosphereScene()
{
    if (m_viewer && m_rootNode) {
        // 创建结合纹理和大气渲染的场景
        // 使用空字符串作为纹理路径，因为纹理路径在UIHandler中设置
        m_uiHandler->createTexturedAtmosphereScene(m_viewer, m_rootNode);
    }
}

// 新增：实现创建结合天空盒和大气渲染的场景功能
void SimpleOSGRenderer::createSkyboxAtmosphereScene()
{
    if (m_viewer && m_rootNode) {
        // 创建结合天空盒和大气渲染的场景
        m_uiHandler->createSkyboxAtmosphereScene(m_viewer, m_rootNode);
    }
}

// 新增：实现创建结合天空盒大气和PBR立方体的场景功能
void SimpleOSGRenderer::createSkyboxAtmosphereWithPBRScene()
{
    if (m_viewer && m_rootNode) {
        // 创建结合天空盒大气和PBR立方体的场景
        m_uiHandler->createSkyboxAtmosphereWithPBRScene(m_viewer, m_rootNode);
    }
}

// 实现重置视野功能
void SimpleOSGRenderer::resetView(SimpleOSGViewer::ViewType viewType)
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->resetView(m_viewer, m_rootNode, viewType);
    }
}

// 实现加载OSG文件功能
void SimpleOSGRenderer::loadOSGFile(const QString& fileName)
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->loadOSGFile(m_viewer, m_rootNode, fileName);
    }
}

// 添加回归主视角的函数
void SimpleOSGRenderer::resetToHomeView()
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->resetToHomeView(m_viewer, m_rootNode);
    }
}

// 添加OpenGL错误检查函数
void SimpleOSGRenderer::checkGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char* errorStr = "";
        switch (err) {
            case GL_INVALID_ENUM: errorStr = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errorStr = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: errorStr = "GL_OUT_OF_MEMORY"; break;
            default: errorStr = "Unknown error";
        }
    }
}

// 添加设置视图类型的方法
void SimpleOSGRenderer::setViewType(SimpleOSGViewer::ViewType viewType)
{
    if (m_viewType != viewType) {
        m_viewType = viewType;
        
        // 使用UIHandler来设置视图类型
        m_uiHandler->setViewType(m_viewer, m_rootNode, viewType);
    }
}


// 添加设置图形颜色的方法
void SimpleOSGRenderer::setShapeColor(float r, float g, float b, float a)
{
    // 使用保存的节点引用
    if (m_shapeNode.valid()) {
        m_uiHandler->setShapeColor(m_shapeNode, r, g, b, a);
        
        // 强制更新视图
        if (m_viewer) {
            m_viewer->advance();
            m_viewer->requestRedraw();
            
        }
    }
}

// 实现更改几何体颜色的方法
void SimpleOSGRenderer::changeGeometryColor(osg::Geometry* geom)
{
    if (!geom) return;
    
    // 生成随机颜色
    float r = static_cast<float>(rand()) / RAND_MAX;
    float g = static_cast<float>(rand()) / RAND_MAX;
    float b = static_cast<float>(rand()) / RAND_MAX;
    
    // 获取状态集
    osg::StateSet* stateset = geom->getOrCreateStateSet();
    
    // 更新颜色uniform
    osg::Uniform* colorUniform = stateset->getUniform("baseColor");
    if (colorUniform) {
        colorUniform->set(osg::Vec4(r, g, b, 1.0f));
    } else {
        // 如果没有baseColor uniform，创建一个新的
        stateset->addUniform(new osg::Uniform("baseColor", osg::Vec4(r, g, b, 1.0f)));
    }
    
    // 强制更新视图
    if (m_viewer) {
        m_viewer->advance();
        m_viewer->requestRedraw();
    }
}

// 实现创建使用新天空盒类的图形功能
void SimpleOSGRenderer::createShapeWithNewSkybox()
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->createShapeWithNewSkybox(m_viewer, m_rootNode, m_shapeNode);
        // 重置视图以适应新创建的立方体
        resetView(m_viewType);
    }
}

void SimpleOSGRenderer::createSimpleScene()
{
    // 创建一个自定义的立方体，每个面有不同的颜色
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    
    // 定义立方体的8个顶点
    osg::Vec3 vertices[] = {
        osg::Vec3(-0.5f, -0.5f, -0.5f), // 0
        osg::Vec3( 0.5f, -0.5f, -0.5f), // 1
        osg::Vec3( 0.5f,  0.5f, -0.5f), // 2
        osg::Vec3(-0.5f,  0.5f, -0.5f), // 3
        osg::Vec3(-0.5f, -0.5f,  0.5f), // 4
        osg::Vec3( 0.5f, -0.5f,  0.5f), // 5
        osg::Vec3( 0.5f,  0.5f,  0.5f), // 6
        osg::Vec3(-0.5f,  0.5f,  0.5f)  // 7
    };
    
    // 定义立方体的6个面，每个面由4个顶点组成
    int faces[][4] = {
        {0, 1, 2, 3}, // 底面 (z = -0.5)
        {4, 7, 6, 5}, // 顶面 (z = 0.5)
        {0, 4, 5, 1}, // 前面 (y = -0.5)
        {2, 6, 7, 3}, // 后面 (y = 0.5)
        {0, 3, 7, 4}, // 左面 (x = -0.5)
        {1, 5, 6, 2}  // 右面 (x = 0.5)
    };
    
    // 定义每个面的颜色
    osg::Vec4 colors[] = {
        osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f), // 红色 - 底面
        osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f), // 绿色 - 顶面
        osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f), // 蓝色 - 前面
        osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f), // 黄色 - 后面
        osg::Vec4(1.0f, 0.0f, 1.0f, 1.0f), // 品红 - 左面
        osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f)  // 青色 - 右面
    };
    
    // 为每个面创建一个几何体
    for (int i = 0; i < 6; ++i) {
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
        
        // 设置顶点
        osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array;
        for (int j = 0; j < 4; ++j) {
            vertexArray->push_back(vertices[faces[i][j]]);
        }
        geometry->setVertexArray(vertexArray);
        
        // 设置颜色
        osg::ref_ptr<osg::Vec4Array> colorArray = new osg::Vec4Array;
        colorArray->push_back(colors[i]);
        geometry->setColorArray(colorArray);
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        
        // 设置法线
        osg::ref_ptr<osg::Vec3Array> normalArray = new osg::Vec3Array;
        // 计算面的法线
        osg::Vec3 v1 = vertices[faces[i][1]] - vertices[faces[i][0]];
        osg::Vec3 v2 = vertices[faces[i][2]] - vertices[faces[i][0]];
        osg::Vec3 normal = v1 ^ v2; // 叉积
        normal.normalize();
        normalArray->push_back(normal);
        geometry->setNormalArray(normalArray);
        geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
        
        // 设置绘制模式为四边形
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
        
        // 禁用光照以确保颜色正确显示
        osg::StateSet* stateset = geometry->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        
        // 将几何体添加到Geode
        geode->addDrawable(geometry);
    }
    
    // 添加到根节点
    m_rootNode->addChild(geode);
}

void SimpleOSGRenderer::render()
{
    // 检查OpenGL上下文
    if (!QOpenGLContext::currentContext()) {
        return;
    }
    
    // 获取当前FBO的尺寸
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];
    
    // 初始化OSG（如果尚未初始化）
    if (!m_viewer) {
        initializeOSG(width, height);
    }
    
    // 更新viewer尺寸并渲染
    if (m_viewer && m_viewer->isRealized()) {
        // 检查视口尺寸是否改变
        osg::Viewport* currentViewport = m_viewer->getCamera()->getViewport();
        bool viewportChanged = !currentViewport || 
                              (currentViewport->width() != width || currentViewport->height() != height);
        
        // 如果视口改变，则更新相机设置
        if (viewportChanged) {
            m_viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
            
        }
        
        // 关键：设置正确的FBO ID
        if (QOpenGLContext::currentContext()) {
            GLuint fboId = QOpenGLContext::currentContext()->defaultFramebufferObject();
            if (m_viewer->getCamera()->getGraphicsContext()) {
                m_viewer->getCamera()->getGraphicsContext()->setDefaultFboId(fboId);
            }
        }
        
        // 重置关键状态 - 确保颜色能正确显示
        glDisable(GL_LIGHTING);  // 禁用固定功能管线光照
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);
        
        // 清除缓冲区 - 使用蓝色背景
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 使用OSG进行渲染
        m_viewer->frame();
        
        // 更新ViewManager中的相机参数，确保UI能获取到最新的相机位置
        if (m_uiHandler && m_uiHandler->getViewManager()) {
            m_uiHandler->getViewManager()->updateViewParametersFromManipulator(m_viewer);
        }
    } else {
        // 如果OSG未正确初始化，则显示蓝色背景
        glViewport(0, 0, width, height);
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

QOpenGLFramebufferObject *SimpleOSGRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

bool SimpleOSGRenderer::processEvent(const QEvent* event)
{
    // 所有视图类型都处理鼠标事件，不仅仅是主视图
    
    if (!m_viewer) {
        return false;
    }
    
    // 检查是否是Ctrl+左键点击，用于选择模型
    if (event->type() == QEvent::MouseButtonPress) {
        const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && (mouseEvent->modifiers() & Qt::ControlModifier)) {
            float x = static_cast<float>(mouseEvent->x());
            float y = static_cast<float>(mouseEvent->y());
            selectModel(static_cast<int>(x), static_cast<int>(y));
            return true;
        }
    }
    
    // 使用鼠标处理器处理其他事件，并传递ViewManager
    return m_mouseHandler->processEvent(event, m_viewer, m_viewType, m_uiHandler->getViewManager());
}

// 添加模型选择相关方法
void SimpleOSGRenderer::selectModel(int x, int y)
{
    if (!m_viewer || !m_rootNode) return;
    
    // 选择新的模型
    osg::Geometry* geom = pickGeometry(x, y);
    if (geom) {
        // 检查几何体是否支持颜色变化（通过检查是否有Shader相关的uniform）
        osg::StateSet* stateSet = geom->getStateSet();
        if (stateSet) {
            // 检查是否有baseColor uniform，这表明是使用Shader的几何体
            osg::Uniform* colorUniform = stateSet->getUniform("baseColor");
            if (colorUniform) {
                // 高亮显示选中的几何体
                highlightGeometry(geom);
                changeGeometryColor(geom);
            } else {
                // 如果没有baseColor uniform，创建一个新的
                stateSet->addUniform(new osg::Uniform("baseColor", osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
                // 高亮显示选中的几何体
                highlightGeometry(geom);
                changeGeometryColor(geom);
            }
        } else {
            // 如果没有状态集，创建一个新的
            osg::StateSet* newStateSet = geom->getOrCreateStateSet();
            newStateSet->addUniform(new osg::Uniform("baseColor", osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
            // 高亮显示选中的几何体
            highlightGeometry(geom);
            changeGeometryColor(geom);
        }
    }
}

// 实现拾取几何体的方法
osg::Geometry* SimpleOSGRenderer::pickGeometry(int x, int y)
{
    if (!m_viewer || !m_rootNode) return nullptr;
    
    // 获取视口尺寸
    osg::Viewport* viewport = m_viewer->getCamera()->getViewport();
    if (!viewport) return nullptr;
    
    // 创建线段相交器
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = 
        new osgUtil::LineSegmentIntersector(
            osgUtil::Intersector::WINDOW, 
            x, 
            viewport->height() - y  // OSG的Y轴方向与Qt相反
        );
    
    // 创建相交访问器
    osgUtil::IntersectionVisitor iv(intersector.get());
    
    // 执行相交检测
    m_viewer->getCamera()->accept(iv);
    
    // 检查是否有相交结果
    if (intersector->containsIntersections()) {
        // 获取第一个相交结果
        osgUtil::LineSegmentIntersector::Intersection intersection = 
            intersector->getFirstIntersection();
        
        // 尝试将相交的可绘制对象转换为几何体
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(intersection.drawable.get());
        return geom;
    }
    
    return nullptr;
}

// 实现设置可绘制对象颜色的方法
void SimpleOSGRenderer::setDrawableColor(osg::Geometry* geom, const osg::Vec4& color)
{
    if (!geom) return;
    
    // 获取颜色数组
    osg::Vec4Array* colors = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
    if (colors && colors->size() > 0) {
        // 修改第一个颜色值
        colors->front() = color;
        // 标记颜色数组为脏数据，需要更新
        colors->dirty();
    } else {
        // 如果没有颜色数组，创建一个新的
        osg::ref_ptr<osg::Vec4Array> newColors = new osg::Vec4Array(1);
        (*newColors)[0] = color;
        geom->setColorArray(newColors);
        geom->setColorBinding(osg::Geometry::BIND_OVERALL);
        // 标记颜色数组为脏数据，需要更新
        newColors->dirty();
    }
}

// 实现高亮几何体的方法
void SimpleOSGRenderer::highlightGeometry(osg::Geometry* geom)
{
    if (!geom) return;
    
    // 取消之前选中的几何体
    if (_lastDrawable.valid()) {
        // 获取之前选中几何体的状态集
        osg::StateSet* prevStateSet = _lastDrawable->getOrCreateStateSet();
        // 设置为未选中状态
        osg::Uniform* prevSelectedUniform = prevStateSet->getUniform("isSelected");
        if (prevSelectedUniform) {
            prevSelectedUniform->set(false);
        }
    }
    
    // 高亮新的几何体
    osg::StateSet* currentStateSet = geom->getOrCreateStateSet();
    osg::Uniform* currentSelectedUniform = currentStateSet->getUniform("isSelected");
    if (currentSelectedUniform) {
        currentSelectedUniform->set(true);
    }
    
    _lastDrawable = geom;
    
    // 强制更新视图
    if (m_viewer) {
        m_viewer->advance();
        m_viewer->requestRedraw();
    }
}

// 添加更新PBR材质的方法（包含Alpha参数）
void SimpleOSGRenderer::updatePBRMaterial(float albedoR, float albedoG, float albedoB, float albedoA,
                                        float metallic, float roughness, 
                                        float specular, float ao)
{
    if (m_rootNode.valid()) {
        // 创建访问器来遍历场景图并更新所有PBR球体的材质
        class PBRMaterialUpdater : public osg::NodeVisitor {
        public:
            osg::Vec4 albedo;  // 包含Alpha通道
            float metallic;
            float roughness;
            float specular;
            float ao;
            
            PBRMaterialUpdater(float albedoR, float albedoG, float albedoB, float albedoA,
                              float metallic, float roughness, 
                              float specular, float ao)
                : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
                  albedo(albedoR, albedoG, albedoB, albedoA),
                  metallic(metallic),
                  roughness(roughness),
                  specular(specular),
                  ao(ao)
            {
            }
            
            virtual void apply(osg::Geode& geode) {
                // 检查这个Geode是否包含PBR球体
                osg::StateSet* stateSet = geode.getStateSet();
                if (stateSet) {
                    // 检查是否存在PBR相关的uniform
                    if (stateSet->getUniform("albedo") || 
                        stateSet->getUniform("metallic") || 
                        stateSet->getUniform("roughness")) {
                        
                        // 更新基础颜色uniform（包含Alpha）
                        osg::Uniform* albedoUniform = stateSet->getUniform("albedo");
                        if (albedoUniform) {
                            albedoUniform->set(osg::Vec3(albedo.x(), albedo.y(), albedo.z()));
                        } else {
                            stateSet->addUniform(new osg::Uniform("albedo", osg::Vec3(albedo.x(), albedo.y(), albedo.z())));
                        }
                        
                        // 更新Alpha值（如果着色器支持）
                        osg::Uniform* alphaUniform = stateSet->getUniform("alpha");
                        if (alphaUniform) {
                            alphaUniform->set(albedo.w());
                        } else {
                            stateSet->addUniform(new osg::Uniform("alpha", albedo.w()));
                        }
                        
                        // 更新金属度uniform
                        osg::Uniform* metallicUniform = stateSet->getUniform("metallic");
                        if (metallicUniform) {
                            metallicUniform->set(metallic);
                        } else {
                            stateSet->addUniform(new osg::Uniform("metallic", metallic));
                        }
                        
                        // 更新粗糙度uniform
                        osg::Uniform* roughnessUniform = stateSet->getUniform("roughness");
                        if (roughnessUniform) {
                            roughnessUniform->set(roughness);
                        } else {
                            stateSet->addUniform(new osg::Uniform("roughness", roughness));
                        }
                        
                        // 更新环境光遮蔽uniform
                        osg::Uniform* aoUniform = stateSet->getUniform("ao");
                        if (aoUniform) {
                            aoUniform->set(ao);
                        } else {
                            stateSet->addUniform(new osg::Uniform("ao", ao));
                        }
                        
                        // 注意：specular在PBR中通常不作为uniform传递，因为它是在着色器中计算的
                        // 如果需要，可以添加其他uniform更新
                    }
                }
                
                traverse(geode);
            }
        };
        
        // 创建并应用访问器
        PBRMaterialUpdater updater(albedoR, albedoG, albedoB, albedoA, metallic, roughness, specular, ao);
        m_rootNode->accept(updater);
        
        // 强制更新视图
        if (m_viewer) {
            m_viewer->advance();
            m_viewer->requestRedraw();
        }
    }
}

// 添加更新大气参数的方法
void SimpleOSGRenderer::updateAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle)
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->updateAtmosphereParameters(m_viewer, m_rootNode, sunZenithAngle, sunAzimuthAngle);
    }
}

// 添加更新大气密度和太阳强度的方法
void SimpleOSGRenderer::updateAtmosphereDensityAndIntensity(float density, float intensity)
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->updateAtmosphereDensityAndIntensity(m_viewer, m_rootNode, density, intensity);
    }
}

// 添加更新米氏散射和瑞利散射的方法
void SimpleOSGRenderer::updateAtmosphereScattering(float mie, float rayleigh)
{
    if (m_viewer && m_rootNode) {
        m_uiHandler->updateAtmosphereScattering(m_viewer, m_rootNode, mie, rayleigh);
    }
}

// 添加更新SkyNode大气参数的方法
void SimpleOSGRenderer::updateSkyNodeAtmosphereParameters(float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG)
{
    if (m_viewer && m_rootNode && m_uiHandler) {
        // 使用DemoShader中的新方法来更新SkyNode大气参数
        if (m_uiHandler->getDemoShader() && m_uiHandler->getDemoShader()) {
            m_uiHandler->getDemoShader()->updateSkyNodeAtmosphereParameters(m_viewer, m_rootNode, turbidity, rayleigh, mieCoefficient, mieDirectionalG);
        }
    }
}

// 添加更新Textured Atmosphere参数的方法
void SimpleOSGRenderer::updateTexturedAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle, float exposure)
{
    if (m_rootNode.valid()) {
        m_uiHandler->updateTexturedAtmosphereParameters(m_viewer, m_rootNode, sunZenithAngle, sunAzimuthAngle, exposure);
    }
}
