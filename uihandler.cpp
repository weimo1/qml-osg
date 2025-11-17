#include "uihandler.h"
#include <QFile>
#include <QDir>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <QDebug>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include "AtmosphereDemo.h"

UIHandler::UIHandler()
    : m_atmosphereDemo(nullptr)  // 显式初始化为nullptr
{
    // 注意：我们不在构造函数中初始化AtmosphereDemo，而是在第一次使用时初始化
    // 这样可以避免在UIHandler构造时可能出现的问题
}

UIHandler::~UIHandler()
{
}

// 视图操作相关方法
void UIHandler::resetToHomeView(osgViewer::Viewer* viewer)
{
    if (viewer) {
        getViewManager()->resetToHomeView(viewer);
    }
}

void UIHandler::fitToView(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (viewer && rootNode) {
        getViewManager()->fitToView(viewer, rootNode);
    }
}

void UIHandler::setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType)
{
    if (viewer && rootNode) {
        getViewManager()->setViewType(viewer, rootNode, viewType);
    }
}

// 文件加载相关方法
void UIHandler::loadOSGFile(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& fileName)
{
    if (!viewer || !rootNode) return;
    
    // 处理QML传来的file:// URL格式
    QString localFileName = fileName;
    if (localFileName.startsWith("file://")) {
        // 移除file://前缀
        localFileName = localFileName.mid(7);
        // 处理Windows路径中的额外斜杠
        if (localFileName.startsWith("/")) {
            localFileName = localFileName.mid(1);
        }
    }

    QFileInfo fileInfo(localFileName);
    if (fileInfo.isDir()) {
        // 如果是目录，则遍历目录下的所有OSG相关文件
        loadOSGFilesFromDirectory(viewer, rootNode, localFileName);
    } else {
        // 如果是单个文件，则加载该文件
        loadSingleOSGFile(viewer, rootNode, localFileName);
    }
}

void UIHandler::loadSingleOSGFile(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& fileName)
{
    if (!viewer || !rootNode) return;
    
    // 将QString转换为std::string
    std::string stdFileName = fileName.toStdString();
    
    // 使用osgDB读取文件
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(stdFileName);
    
    if (loadedModel)
    {
        // 将加载的模型添加到场景图中
        rootNode->addChild(loadedModel);
        
        // 计算新添加模型的包围盒
        osg::ComputeBoundsVisitor boundsVisitor;
        loadedModel->accept(boundsVisitor);
        osg::BoundingBox bb = boundsVisitor.getBoundingBox();
        
        // 如果包围盒有效，则调整相机位置以适应新模型
        if (bb.valid())
        {
            // 获取当前相机操作器
            osgGA::CameraManipulator* manipulator = viewer->getCameraManipulator();
            if (manipulator)
            {
                // 设置新的home位置，使模型完整显示在视图中
                osg::Vec3 center = bb.center();
                osg::BoundingSphere bs(bb);
                float radius = bs.radius();
                
                // 设置合适的相机距离
                osg::Vec3 eye = center + osg::Vec3(0, -radius * 2.5f, radius);
                osg::Vec3 up(0.0f, 0.0f, 1.0f);
                
                manipulator->setHomePosition(eye, center, up);
                manipulator->home(0.0);
            }
        }
        
        qDebug() << "Successfully loaded OSG file:" << fileName;
    }
    else
    {
        qDebug() << "Failed to load OSG file:" << fileName;
    }
}

void UIHandler::loadOSGFilesFromDirectory(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& dirPath)
{
    if (!viewer || !rootNode) return;
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        qDebug() << "Directory does not exist:" << dirPath;
        return;
    }

    // 支持的OSG文件扩展名
    QStringList filters;
    filters << "*.osg" << "*.osgt" << "*.osgb";
    
    // 获取目录中所有匹配的文件
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    if (fileList.isEmpty()) {
        qDebug() << "No OSG files found in directory:" << dirPath;
        return;
    }

    // 遍历并加载所有文件
    for (const QFileInfo& fileInfo : fileList) {
        loadSingleOSGFile(viewer, rootNode, fileInfo.absoluteFilePath());
    }
    
    // 适应视图以显示所有加载的模型
    fitToView(viewer, rootNode);
}

// 光照控制相关方法
void UIHandler::toggleLighting(osgViewer::Viewer* viewer, osg::Group* rootNode, bool enabled)
{
    if (!rootNode) return;
    
    // 遍历场景图，启用或禁用光照
    osg::StateSet* stateSet = rootNode->getOrCreateStateSet();
    if (enabled) {
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        qDebug() << "Lighting enabled";
    } else {
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        qDebug() << "Lighting disabled";
    }
    
    // 强制更新视图
    if (viewer) {
        viewer->requestRedraw();
    }
}

// 模型选择相关方法
void UIHandler::selectModel(osgViewer::Viewer* viewer, int x, int y)
{
    if (!viewer) return;
    
    // 需要反转Y坐标，osg和qml的坐标系不同
    osg::Viewport* currentViewport = viewer->getCamera()->getViewport(); 
    if (!currentViewport) return;
    
    y = currentViewport->height() - y;
    
    // 创建一个线段求交器
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = 
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, x, y);
        
    // 创建求交访问器
    osgUtil::IntersectionVisitor iv(intersector.get());
    
    // 执行求交检测
    viewer->getCamera()->accept(iv);
    
    // 检查是否有交点
    if (intersector->containsIntersections())
    {
        // 获取第一个交点
        osgUtil::LineSegmentIntersector::Intersection intersection = intersector->getFirstIntersection();
        
        // 获取被选中的节点路径
        osg::NodePath nodePath = intersection.nodePath;
        
        // 遍历节点路径，查找可选择的几何体
        for (int i = nodePath.size() - 1; i >= 0; --i)
        {
            osg::Node* node = nodePath[i];
            osg::Geode* geode = dynamic_cast<osg::Geode*>(node);
            if (geode)
            {
                // 找到Geode节点，选中它
                osg::StateSet* ss = geode->getOrCreateStateSet();
                if (ss)
                {
                    ss->getOrCreateUniform("bSelect", osg::Uniform::BOOL)->set(true);
                }
                break;
            }
        }
    }
}

// 天空盒相关方法
void UIHandler::createSkyBox(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (!viewer || !rootNode) return;
    
    // 创建天空盒
    osg::Camera* camera = viewer->getCamera();
    if (camera) {
        m_skyBox = new SkyBoxThree(camera);
        rootNode->addChild(m_skyBox);
    }
}

// 大气渲染相关方法
void UIHandler::createAtmosphere(osgViewer::Viewer* viewer, osg::Group* rootNode)
{ 
    
    if (!viewer || !rootNode) {
        return;
    }
    
    // 创建大气渲染实例
    if (!m_atmosphereDemo) {
        m_atmosphereDemo = new AtmosphereDemo();
    }
    
    osg::ref_ptr<osg::Group> pRender = new osg::Group;

    for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
        pRender->addChild(rootNode->getChild(i));
    }
    
    rootNode->removeChildren(0, rootNode->getNumChildren());
    
    // 创建大气效果节点
    osg::ref_ptr<osg::Node> atmosphereNode = m_atmosphereDemo->createAtmosphere(pRender, viewer->getCamera(), osg::Vec4(0.5, 0.5, 0.5, 1));
    if (atmosphereNode.valid()) {
        // 将大气效果添加到场景的最前面，确保它在所有其他对象之前渲染
        rootNode->addChild(atmosphereNode);
        qDebug() << "Atmosphere effect created successfully";
    }
    
    if (viewer) {
        viewer->requestRedraw();
    }
}

// 添加专门用于测试MRT功能的函数
void UIHandler::testMRT(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (!viewer || !rootNode) {
        return;
    }
    
    printf("Starting MRT test...\n");
    
    // 创建一个简单的测试场景
    osg::ref_ptr<osg::Group> testScene = new osg::Group;
    
    // 创建一个立方体作为测试模型
    osg::ref_ptr<osg::Geode> cubeGeode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> cube = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0, 0, 0), 2.0f));
    cube->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)); // 红色立方体
    cubeGeode->addDrawable(cube);
    
    // 为立方体创建一个简单的着色器
    osg::ref_ptr<osg::StateSet> cubeStateSet = cubeGeode->getOrCreateStateSet();
    cubeStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    testScene->addChild(cubeGeode);
    
    // 创建一个简单的地面
    osg::ref_ptr<osg::Geode> groundGeode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> groundGeom = osg::createTexturedQuadGeometry(
        osg::Vec3(-5, -5, -1),
        osg::Vec3(10, 0, 0),
        osg::Vec3(0, 10, 0)
    );
    
    // 创建一个简单的绿色地面
    osg::ref_ptr<osg::Vec4Array> groundColors = new osg::Vec4Array;
    groundColors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f)); // 绿色
    groundGeom->setColorArray(groundColors);
    groundGeom->setColorBinding(osg::Geometry::BIND_OVERALL);
    
    groundGeode->addDrawable(groundGeom);
    
    // 为地面创建一个简单的着色器
    osg::ref_ptr<osg::StateSet> groundStateSet = groundGeode->getOrCreateStateSet();
    groundStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    testScene->addChild(groundGeode);
    
    printf("Created test scene with cube and ground\n");
    
    // 直接调用createRTTCamera静态函数来创建MRT相机
    osg::ref_ptr<osg::Texture2D> textures = new osg::Texture2D;
    osg::ref_ptr<osg::Camera> mrtCam = AtmosphereDemo::createRTTCamera(textures);
    mrtCam->addChild(testScene);
    
    // 创建一个四边形来显示纹理
    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    osg::Geometry* geom = osg::createTexturedQuadGeometry(
        osg::Vec3(-1.0f, -1.0f, 0.0f),
        osg::Vec3(2.0f, 0.0f, 0.0f),
        osg::Vec3(0.0f, 2.0f, 0.0f));
    
    geom->setVertexAttribArray(0, geom->getVertexArray(), osg::Array::BIND_PER_VERTEX);
    geom->setVertexAttribArray(1, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);
    
    osg::StateSet* ss = geom->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0, textures.get());
    
    // 创建简单的着色器程序
    osg::Program* program = new osg::Program;
    
    // 顶点着色器
    const char* vertexShaderSource = R"(
        #version 330
        layout(location = 0) in vec4 vertex;
        layout(location = 1) in vec2 texCoord;
        out vec2 v_texCoord;
        void main() {
            v_texCoord = texCoord;
            gl_Position = vertex;
        }
    )";
    
    // 片段着色器
    const char* fragmentShaderSource = R"(
        #version 330
        in vec2 v_texCoord;
        uniform sampler2D groundTexture;
        out vec4 color;
        void main() {
            // 采样RTT纹理
            vec4 rttColor = texture(groundTexture, v_texCoord);
            
            // 设置背景颜色（深蓝色）
            vec4 backgroundColor = vec4(0.0, 0.0, 0.5, 1.0);
            
            // 混合RTT纹理和背景颜色
            if (rttColor.a > 0.0) {
                // 将RTT颜色和背景颜色混合
                color = mix(backgroundColor, rttColor, rttColor.a);
            } else {
                // 只显示背景颜色
                color = backgroundColor;
            }
            
            // 为了更容易看到效果，我们可以添加一个边框
            if (v_texCoord.x < 0.05 || v_texCoord.x > 0.95 || v_texCoord.y < 0.05 || v_texCoord.y > 0.95) {
                color = vec4(1.0, 1.0, 0.0, 1.0); // 黄色边框
            }
        }
    )";
    
    osg::Shader* vertexShader = new osg::Shader(osg::Shader::VERTEX, vertexShaderSource);
    osg::Shader* fragmentShader = new osg::Shader(osg::Shader::FRAGMENT, fragmentShaderSource);
    
    program->addShader(vertexShader);
    program->addShader(fragmentShader);
    
    ss->setAttributeAndModes(program);
    ss->addUniform(new osg::Uniform("groundTexture", 0));
    
    quad->addDrawable(geom);
    
    // 构建场景图
    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(mrtCam.get());
    
    rootNode->addChild(root);
    
    printf("MRT test node added to scene\n");
    
    if (viewer) {
        viewer->requestRedraw();
    }
}

// 获取ViewManager实