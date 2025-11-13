#include "simpleosgrenderer.h"
#include "simpleosgviewer.h" 
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
#include "osg/Shape"
#include "SkyNode.h"
 

enum NM_NODEMASK
{
	NM_ALL = 0xffffffff,
	NM_NULL = 0,
	NM_WORK_PLANE = 1 << 4, //工作面
	NM_HUD = 1 << 5, //DbViewUI3D::getPreRenderNode()
	NM_SNAP = 1 << 6,  //这一位也不能选中
	NM_NO_PICK = 1 << 7,
	NM_NO_SHOW = 1 << 8, //★★★★不显示，但可以选中， 一般很少用
	NM_NO_PICK_NO_SHOW = 0, 
	NM_TERRAIN = 1 << 9, //GIS
	NM_PLACEHOLDER = 1 << 10,
	NM_TEXT = 1 << 11,						
	NM_SKY = 1 << 12,
	NM_DEBUG = 1<< 13,  //调试使用的对象
	NM_OBJECT_3D = 1<< 14, //3d模型
	NM_TIN = 1 << 15, //GIS的TIN
	NM_INEDITABLE = 1 << 16,//不可编辑
	NM_3D_NOTE = 1 << 17,   //三维注释
	NM_NO_DELETE = 1 << 18,  //不可删除
	NM_InstanceObb = 1 << 19, //obb包围盒，不可见，但是可以被选中
	//可以捕捉，但不可选中
	NM_SNAP_BUT_NO_PICK = (NM_NO_PICK | NM_SNAP)
};


SimpleOSGRenderer::SimpleOSGRenderer(SimpleOSGViewer::ViewType viewType)
    : m_initialized(false)
{
    m_mouseHandler = new MouseHandler;    
}

SimpleOSGRenderer::~SimpleOSGRenderer()
{
    delete m_mouseHandler;    
}


osg::ref_ptr<osg::Program> g_batchProgram = nullptr;


class TileSetCB : public osg::StateSet::Callback
{
public:
	explicit TileSetCB(osg::Camera* camera) : pCamera_(camera)
	{
	}

	virtual void operator()(osg::StateSet* ss, osg::NodeVisitor* nv)
	{
		preocess(ss);
	}

	void preocess(osg::StateSet* ss)
	{
		if (pCamera_)
		{
			osg::Vec3f eye, center, up;
			pCamera_->getViewMatrixAsLookAt(eye, center, up);
			ss->getOrCreateUniform("viewPos", osg::Uniform::FLOAT_VEC3)->set(eye);

			osg::Matrixf vieMat = pCamera_->getViewMatrix();
			osg::Matrixf viewInverse = vieMat.inverse(vieMat);
			ss->getOrCreateUniform("viewInverse", osg::Uniform::FLOAT_MAT4)->set(viewInverse);

		}
	}
private:
	osg::Camera* pCamera_ = nullptr;
};


osg::ref_ptr<osg::Geode> SimpleOSGRenderer::createShape(osg::Vec3 center)
{ 
		// 创建一个自定义的立方体，每个面有不同的颜色
		osg::ref_ptr<osg::Geode> geode = new osg::Geode;

		// 定义立方体的8个顶点
		osg::Vec3 vertices[] = {
			osg::Vec3(-0.5f, -0.5f, -0.5f), // 0
			osg::Vec3(0.5f, -0.5f, -0.5f), // 1
			osg::Vec3(0.5f,  0.5f, -0.5f), // 2
			osg::Vec3(-0.5f,  0.5f, -0.5f), // 3
			osg::Vec3(-0.5f, -0.5f,  0.5f), // 4
			osg::Vec3(0.5f, -0.5f,  0.5f), // 5
			osg::Vec3(0.5f,  0.5f,  0.5f), // 6
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
				vertexArray->push_back(vertices[faces[i][j]]+center );
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

			geometry->setVertexAttribArray(0, geometry->getVertexArray(), osg::Array::BIND_PER_VERTEX);
            geometry->setVertexAttribArray(1, geometry->getNormalArray(), osg::Array::BIND_OVERALL);
            geometry->setVertexAttribArray(2, geometry->getColorArray(),  osg::Array::BIND_OVERALL);
		}

        //
		osg::StateSet* stateset = geode->getOrCreateStateSet();
		if (stateset)
		{
			if (g_batchProgram == nullptr)
			{
				g_batchProgram = new osg::Program;		

				osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX,   "E:/qml/shader/Obj.vert");
				pV->setName("pv");
				osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, "E:/qml/shader/Obj.frag");
				pF->setName("pF");

				g_batchProgram->addShader(pV);				
				g_batchProgram->addShader(pF);
			}
			stateset->setAttribute(g_batchProgram);

			TileSetCB* pCB = new TileSetCB(m_viewer->getCamera() );
            stateset->setUpdateCallback(pCB);
		}

        return geode;

 
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
        osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow =      new osgViewer::GraphicsWindowEmbedded(0, 0, width, height);
        
        // 设置相机
        osg::Camera* camera = m_viewer->getCamera();
        camera->setGraphicsContext(graphicsWindow.get());      
        
        camera->setClearColor(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f)); // 深灰色背景
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 激活OSG内置的uniform变量
        camera->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);
        
        // 设置渲染器特性
        m_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
        
        // 设置视图场景
        m_viewer->setSceneData(m_rootNode.get());        
    
        for (int i = 0; i < 5; i++) {
			m_rootNode->addChild(createShape( osg::Vec3(4*i,0,0) ));
        }
        camera->setProjectionMatrixAsPerspective(30.0f, static_cast<float>(width) / height, 1.0f, 1000000.0f);
                
      
        osg::Vec3 center(0.0f, 0.0f, 0.0f);          
        
        // 获取场景的边界框，用于设置正确的旋转中心
        osg::ComputeBoundsVisitor boundsVisitor;
        boundsVisitor.setTraversalMask(~NM_SKY ); 
        m_rootNode->accept(boundsVisitor);
        osg::BoundingBox bb = boundsVisitor.getBoundingBox();
        
        if (bb.valid()) {
            // 使用场景的中心作为旋转中心
            center = bb.center();
        }
        
        // 设置操作器的home位置
        osg::Vec3 eye(0.0f, 0.0f, 10.0f);
        osg::Vec3 up(0.0f, 0.0f, 1.0f);

        
        // 启用轨迹球操作器，允许鼠标旋转 (所有视图类型)
        osg::ref_ptr<osgGA::TrackballManipulator> manipulator = new osgGA::TrackballManipulator;
            
        // 优化操纵器设置以改善旋转体验
        manipulator->setVerticalAxisFixed(true);  // 固定垂直轴，使左右旋转更自然
        manipulator->setAllowThrow(false);  // 禁止抛掷效果，使控制更精确
    //    manipulator->setRotationMode(osgGA::TrackballManipulator::TRACKBALL);  // 使用轨迹球模式
            
        m_viewer->setCameraManipulator(manipulator.get());
            
        manipulator->setHomePosition(eye, center, up);        
     
        // 设置鼠标事件处理的视口尺寸
  
        if (camera) {
            camera->setViewport(0, 0, width, height);
        }
        
        // 添加事件处理器
        m_viewer->addEventHandler(new osgViewer::StatsHandler);
        m_viewer->addEventHandler(new osgViewer::WindowSizeHandler);
        
        // 禁用自动编译
        m_viewer->setIncrementalCompileOperation(nullptr);
        
        // 初始化viewer
        m_viewer->realize();        
        
        // 再次设置视口以确保正确初始化
        if (camera) {
            camera->setViewport(0, 0, width, height);
        }
    }
    catch (const std::exception& e) {
        // qCritical() << "Error initializing OSG:" << e.what();
    }
    catch (...) {
        // qCritical() << "Unknown error initializing OSG";
    }
}


// 添加OpenGL错误检查函数
void SimpleOSGRenderer::checkGLError(const char* location) 
{
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


void SimpleOSGRenderer::render()
{
    // 检查OpenGL上下文
    if (!QOpenGLContext::currentContext()) {
        return;
    }
    
    // 获取当前FBO的尺寸
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width  = viewport[2];
    int height = viewport[3];
    
    // 初始化OSG（如果尚未初始化）
    if (!m_viewer) {
        initializeOSG(width, height);
    }
    
    // 更新viewer尺寸并渲染
    if (m_viewer && m_viewer->isRealized()) 
    {
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
        // 使用OSG进行渲染
        m_viewer->frame();
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
    if (event->type() == QEvent::MouseButtonPress) 
    {
        const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton && (mouseEvent->modifiers() & Qt::ControlModifier)) {

            float x = static_cast<float>(mouseEvent->x());
            float y = static_cast<float>(mouseEvent->y());
            selectModel(x, y);
            return true;
        }
    }
     
    // 使用鼠标处理器处理其他事件，并传递ViewManager
    return m_mouseHandler->processEvent(event, m_viewer);
}

// 添加模型选择相关方法
void SimpleOSGRenderer::selectModel(int x, int y)
{
    if (!m_viewer || !m_rootNode) return;
    
    //需要反转Y坐标，osg和qml的坐标系不同
	osg::Viewport* currentViewport = m_viewer->getCamera()->getViewport(); 
    y = currentViewport->height() - y;
     // 选择新的模型
    osg::ref_ptr<osg::Drawable> geom = pickGeometry(x, y);
    if (geom) {
        // 检查几何体是否支持颜色变化（通过检查是否有Shader相关的uniform）
        osg::StateSet* ss = geom->getOrCreateStateSet( );
        if (ss)
        {
			ss->getOrCreateUniform("bSelect", osg::Uniform::BOOL)->set(true);
        }
     }
}

// 实现拾取几何体的方法
osg::ref_ptr<osg::Drawable>  SimpleOSGRenderer::pickGeometry(int x, int y)
{
    if (!m_viewer || !m_rootNode) return nullptr;

    return  m_mouseHandler->pickModel(x, y, m_viewer);
}

 
// 实现重置到主视角的功能
void SimpleOSGRenderer::resetToHomeView()
{
    if (m_viewer && m_uiHandler) {
        m_uiHandler->resetToHomeView(m_viewer);
    }
}
void SimpleOSGRenderer::loadOSGFile(const QString& fileName){
    if (m_viewer && m_rootNode && m_uiHandler)
    {
        m_uiHandler->loadOSGFile(m_viewer, m_rootNode, fileName);
    }
}
// 实现适应视图功能
void SimpleOSGRenderer::fitToView()
{
    if (m_viewer && m_rootNode && m_uiHandler) {
        m_uiHandler->fitToView(m_viewer, m_rootNode);
    }
}

// 实现设置视图类型的功能
void SimpleOSGRenderer::setViewType(SimpleOSGViewer::ViewType viewType)
{
    if (m_viewer && m_rootNode && m_uiHandler) {
        m_uiHandler->setViewType(m_viewer, m_rootNode, viewType);
    }
}

// 实现光照控制功能
void SimpleOSGRenderer::toggleLighting(bool enabled)
{
    if (m_viewer && m_rootNode && m_uiHandler) {
        m_uiHandler->toggleLighting(m_viewer, m_rootNode, enabled);
    }
}