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

UIHandler::UIHandler()
{
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
    
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 450000)));
    geode->setCullingActive(false);

    m_skyBox = new SkyBoxThree(viewer->getCamera());
    m_skyBox->setName("skybox");
    m_skyBox->addChild(geode.get());
    rootNode->addChild(m_skyBox);
}

// 获取ViewManager实例
ViewManager* UIHandler::getViewManager()
{
    return &m_viewManager;
}