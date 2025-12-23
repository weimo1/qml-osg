#include "uihandler.h"
#include <QFile>
#include <QDir>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include "SkyNode.h"
#include "AtmosphereDemo.h"
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include "AtmosphereDemo.h"

UIHandler::UIHandler()
    : m_viewManager()  // 显式初始化ViewManager
    , m_atmosphereDemo(nullptr)  // 显式初始化为nullptr
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

    // 获取目录中的所有子目录
    QFileInfoList subDirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    
    bool modelsLoaded = false;
    
    // 遍历每个子目录
    for (const QFileInfo& subDirInfo : subDirList) {
        QString subDirPath = subDirInfo.absoluteFilePath();
        QString subDirName = subDirInfo.fileName();
        
        qDebug() << "Checking subdirectory:" << subDirPath;
        
        // 检查子目录中是否存在与子目录同名的OSG文件
        QStringList osgExtensions = {"*.osg", "*.osgt", "*.osgb"};
        for (const QString& extension : osgExtensions) {
            QString targetFileName = subDirName + extension.mid(1); // 移除*号
            QString targetFilePath = subDirPath + "/" + targetFileName;
            
            QFileInfo targetFile(targetFilePath);
            if (targetFile.exists() && targetFile.isFile()) {
                qDebug() << "Found matching OSG file:" << targetFilePath;
                loadSingleOSGFile(viewer, rootNode, targetFilePath);
                modelsLoaded = true;
                break; // 找到匹配的文件后跳出循环，每个子目录只加载一个文件
            }
        }
    }
    
    // 如果加载了模型，则适应视图以显示所有加载的模型
    if (modelsLoaded) {
        fitToView(viewer, rootNode);
    }
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

// 新的大气渲染方法实现
void UIHandler::createNewAtmosphere(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (!viewer || !rootNode) {
        return;
    }

    // 创建新的大气渲染实例
    osg::ref_ptr<FullscreenAtmosphere> newAtmosphere = new FullscreenAtmosphere();
    newAtmosphere->setCamera(viewer->getCamera());

    // 将新的大气渲染节点添加到场景图的前面，确保它在所有其他对象之前渲染
    rootNode->addChild(newAtmosphere);

    qDebug() << "New fullscreen atmosphere effect created successfully";

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
    
    // 创建新的TransmiteLUT实例用于测试
    osg::ref_ptr<TransmiteLUT> transmiteLUT = new TransmiteLUT();
    transmiteLUT->setCamera(viewer->getCamera());
    
    // 清除现有的子节点
    rootNode->removeChildren(0, rootNode->getNumChildren());
    
    // 将TransmiteLUT节点添加到场景图
    rootNode->addChild(transmiteLUT);
    
    // 请求重绘
    if (viewer) {
        viewer->requestRedraw();
    }
}

// 创建并显示TransmiteLUT
void UIHandler::createTransmiteLUT(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
   osg::ref_ptr<TransmiteLUT> transmiteLUT = new TransmiteLUT();
   
   // 生成LUT纹理
   transmiteLUT->generateLUT();
   
   // 导出LUT纹理为图像文件
   transmiteLUT->exportLUT("LUT.png");
}

void UIHandler::exportLUT(const QString& filename)
{
    osg::ref_ptr<TransmiteLUT> transmiteLUT = new TransmiteLUT();
    transmiteLUT->generateLUT();
    
    // 等待足够长的时间确保渲染完成
    // osg::Timer_t startTick = osg::Timer::instance()->tick();
    // double elapsedTime = 0.0;
    // while (elapsedTime < 0.5) {  // 等待500毫秒确保渲染完成
    //     elapsedTime = osg::Timer::instance()->delta_s(startTick, osg::Timer::instance()->tick());
    // }
    

    transmiteLUT->exportLUT(filename.toStdString());
    // 强制从GPU读取数据并保存
    
}
