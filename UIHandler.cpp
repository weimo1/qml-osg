#include "UIHandler.h"
#include "AtmosphereDemo.h"
#include "FullscreenAtmosphere.h"
#include "FastAtmosphere.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <osgDB/ReadFile>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
#include <osg/Camera>
#include <osgGA/CameraManipulator>
#include <iostream>
#include <osg/MatrixTransform>

UIHandler::UIHandler(QObject *parent)
    : QObject(parent)
{
}

UIHandler::~UIHandler()
{
}

void UIHandler::loadOSGFile(osgViewer::View* view, osg::Group* rootNode, const QString& fileName)
{
    if (!view || !rootNode) return;
    
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
        loadOSGFilesFromDirectory(view, rootNode, localFileName);
    } else {
        // 如果是单个文件，则加载该文件
        loadSingleOSGFile(view, rootNode, localFileName);
    }
}

// 加载单个OSG文件
void UIHandler::loadSingleOSGFile(osgViewer::View* view, osg::Group* rootNode, const QString& fileName)
{
    // 检查文件名是否为空
    if (fileName.isEmpty()) {
        emit fileLoadError(fileName, "File name is empty");
        return;
    }
    
    // 处理相对路径 - 尝试在当前工作目录和应用程序目录中查找文件
    QString fullPath = fileName;
    QFile file(fullPath);
    
    // 如果文件不存在，尝试在应用程序目录中查找
    if (!file.exists()) {
        QString appDir = QCoreApplication::applicationDirPath();
        fullPath = appDir + "/" + fileName;
        file.setFileName(fullPath);
        
        // 如果仍然不存在，尝试在应用程序目录的上一级目录中查找
        if (!file.exists()) {
            fullPath = appDir + "/../" + fileName;
            file.setFileName(fullPath);
        }
    }
    
    if (!file.exists()) {
        emit fileLoadError(fileName, "File does not exist: " + fullPath);
        return;
    }
    
    // 将QString转换为std::string
    std::string stdFileName = fullPath.toStdString();
    
    try {
        // 使用osgDB加载模型
        osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(stdFileName);
        
        if (loadedModel) {
            // 不再清空现有场景，直接添加加载的模型到场景
            rootNode->addChild(loadedModel);
            
            // 获取模型的包围球，用于计算合适的相机位置
            osg::BoundingSphere bs = loadedModel->getBound();
            double radius = bs.radius();
            osg::Vec3d center = bs.center();
            
            // 如果模型有有效的边界球，则调整相机位置确保能看到整个模型
            if (radius > 0) {
                // 计算合适的视距，确保模型完整显示
                double viewDistance = radius * 3.0;
                
                // 设置相机方向向上为Z轴
                osg::Vec3d up(0.0, 0.0, 1.0);
                
                // 从前方观察模型（稍微偏下的角度）
                osg::Vec3d viewDirection(0.0, -1.0, 0.3);
                viewDirection.normalize();
                
                // 相机位置 = 模型中心 + 视线方向 * 距离
                osg::Vec3d eye = center + viewDirection * viewDistance;
                
                // 获取当前相机操作器
                osgGA::CameraManipulator* manipulator = view->getCameraManipulator();
                if (manipulator) {
                    manipulator->setHomePosition(eye, center, up);
                    // 立即应用home位置
                    manipulator->home(0.0);
                }
                
                // 调整投影矩阵以适应模型大小
                osg::Camera* camera = view->getCamera();
                if (camera && camera->getViewport()) {
                    float aspectRatio = static_cast<float>(camera->getViewport()->width()) / 
                                       static_cast<float>(camera->getViewport()->height());
                    // 调整投影矩阵以适应模型大小，增加远裁剪面以防止模型消失
                    // 远裁剪面从radius * 100.0f调整为radius * 1000.0f以提供更大的可视范围
                    camera->setProjectionMatrixAsPerspective(
                        30.0f, aspectRatio, radius * 0.1f, radius * 1000.0f);
                }
            }
            // 注意：如果模型没有有效的边界球，我们不改变当前的相机位置
            
            // 强制更新视图
            view->requestRedraw();
            
            emit fileLoadSuccess(fileName);
        } else {
            emit fileLoadError(fileName, "Failed to load model");
        }
    }
    catch (const std::exception& e) {
        // 如果加载失败，不执行任何操作
        // 保持现有场景不变
        std::cout << "Exception while loading OSG file: " << e.what() << std::endl;
        emit fileLoadError(fileName, QString("Exception: ") + e.what());
    }
    catch (...) {
        // 如果加载失败，不执行任何操作
        // 保持现有场景不变
        std::cout << "Unknown exception while loading OSG file" << std::endl;
        emit fileLoadError(fileName, "Unknown exception occurred");
    }
    
}

void UIHandler::loadOSGFilesFromDirectory(osgViewer::View* view, osg::Group* rootNode, const QString& dirPath)
{
    if (!view || !rootNode) return;
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        std::cout << "Directory does not exist: " << dirPath.toStdString() << std::endl;
        emit fileLoadError(dirPath, "Directory does not exist");
        return;
    }

    // 获取目录中所有子目录
    QFileInfoList subDirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    
    if (subDirList.isEmpty()) {
        std::cout << "No subdirectories found in directory: " << dirPath.toStdString() << std::endl;
        emit fileLoadError(dirPath, "No subdirectories found");
        return;
    }

    // 记录加载前的子节点数量
    unsigned int initialChildCount = rootNode->getNumChildren();
    
    // 遍历每个子目录，只加载与目录同名的第一个OSG文件
    for (const QFileInfo& subDirInfo : subDirList) {
        QDir subDir(subDirInfo.absoluteFilePath());
        
        // 获取子目录名称
        QString subDirName = subDirInfo.fileName();
        
        // 支持的OSG文件扩展名
        QStringList filters;
        filters << "*.osg" << "*.osgt" << "*.osgb";
        
        // 获取子目录中所有匹配的文件
        QFileInfoList fileList = subDir.entryInfoList(filters, QDir::Files | QDir::Readable);
        
        // 查找与目录同名的文件
        for (const QFileInfo& fileInfo : fileList) {
            QString baseName = fileInfo.baseName(); // 获取不带扩展名的文件名
            if (baseName == subDirName) {
                // 找到与目录同名的文件，加载它并跳出循环
                loadSingleOSGFile(view, rootNode, fileInfo.absoluteFilePath());
                break;
            }
        }
    }
    
    // 检查是否成功加载了新模型
    if (rootNode->getNumChildren() > initialChildCount) {
        // 适应视图以显示所有加载的模型
      
        
        // 在所有文件加载完成后，将场景移动到原点
        moveSceneToOrigin(view, rootNode);
        fitToView(view, rootNode);
    }
}

void UIHandler::fitToView(osgViewer::View* view, osg::Group* rootNode)
{
    if (!view || !rootNode) return;
    
    // 计算整个场景的包围盒
    osg::ComputeBoundsVisitor boundsVisitor;
    rootNode->accept(boundsVisitor);
    osg::BoundingBox bb = boundsVisitor.getBoundingBox();
    
    // 如果包围盒有效，则调整相机位置以适应所有模型
    if (bb.valid())
    {
        // 获取当前相机操作器
        osgGA::CameraManipulator* manipulator = view->getCameraManipulator();
        if (manipulator)
        {
            // 设置新的home位置，使所有模型完整显示在视图中
            osg::Vec3 center = bb.center();
            osg::BoundingSphere bs(bb);
            float radius = bs.radius();
            
            // 设置更合适的相机距离，确保所有模型在视图中心且可见
            // 增加距离因子以确保模型完全可见
            float distanceFactor = 3.0f;
            osg::Vec3 eye = center + osg::Vec3(0, -radius * distanceFactor, radius * 0.5f);
            osg::Vec3 up(0.0f, 0.0f, 1.0f);
            
            // 设置home位置并移动到该位置
            manipulator->setHomePosition(eye, center, up);
            manipulator->home(0.0);
            
            // 强制更新视图
            view->requestRedraw();
            
            std::cout << "Scene bounds: center=(" << center.x() << ", " << center.y() << ", " << center.z() 
                      << "), radius=" << radius << std::endl;
            std::cout << "Camera position: eye=(" << eye.x() << ", " << eye.y() << ", " << eye.z() 
                      << "), center=(" << center.x() << ", " << center.y() << ", " << center.z() << ")" << std::endl;
        }
    }
}

void UIHandler::toggleLighting(osgViewer::View* view, osg::Group* rootNode, bool enabled)
{
    if (!rootNode) return;
    
    // 遍历场景图，启用或禁用光照
    osg::StateSet* stateSet = rootNode->getOrCreateStateSet();
    if (enabled) {
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        std::cout << "Lighting enabled" << std::endl;
    } else {
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        std::cout << "Lighting disabled" << std::endl;
    }
    
    // 强制更新视图
    if (view) {
        // 请求重绘
        // 注意：OSG View没有直接的requestRedraw方法，我们可以通过其他方式触发重绘
        // 例如，通过操作相机或场景来触发更新
        osg::Camera* camera = view->getCamera();
        if (camera) {
            // 通过修改相机的视图矩阵来触发重绘
            osg::Matrixd viewMatrix = camera->getViewMatrix();
            camera->setViewMatrix(viewMatrix);
        }
    }
}

void UIHandler::createAtmosphere(osg::Camera* camera, osg::Group* rootNode)
{
    if (!camera || !rootNode) {
        return;
    }

    // 创建大气渲染实例
    if (!m_fastAtmosphereDemo) {
        m_fastAtmosphereDemo = new FastAtmosphere();
    }

    osg::ref_ptr<osg::Group> pRender = new osg::Group;

    // 将原始场景的所有子节点移动到pRender中
    for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
        pRender->addChild(rootNode->getChild(i));
    }

    // 清空根节点
    rootNode->removeChildren(0, rootNode->getNumChildren());

    // 创建大气效果节点，并将RTT纹理传递给它
    osg::ref_ptr<osg::Node> atmosphereNode = m_fastAtmosphereDemo->createAtmosphere(pRender, camera);
    if (atmosphereNode.valid()) {
        // 将大气效果添加到场景
        rootNode->addChild(atmosphereNode);
        std::cout << "Atmosphere effect with RTT created successfully" << std::endl;
    }
}

void UIHandler::moveSceneToOrigin(osgViewer::View* view, osg::Group* rootNode)
{
    if (!view || !rootNode) return;
    
    // 使用ComputeBoundsVisitor计算整个场景的边界
    osg::ComputeBoundsVisitor boundVisitor;
    rootNode->accept(boundVisitor);
    osg::BoundingBox boundingBox = boundVisitor.getBoundingBox();
    
    // 如果边界框有效
    if (boundingBox.valid()) {
        // 计算场景的中心点
        osg::Vec3d sceneCenter = boundingBox.center();
        
        // 计算需要移动的偏移量，将场景中心移动到原点
        osg::Vec3d offset = -sceneCenter;
        
        // 创建矩阵变换节点
        osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
        osg::Matrixd matrix;
        matrix.setTrans(offset);
        transform->setMatrix(matrix);
        
        // 保存所有子节点
        osg::NodeList children;
        for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
            children.push_back(rootNode->getChild(i));
        }
        
        // 清空根节点
        rootNode->removeChildren(0, rootNode->getNumChildren());
        
        // 将所有子节点添加到变换节点中
        for (unsigned int i = 0; i < children.size(); ++i) {
            transform->addChild(children[i]);
        }
        
        // 将变换节点添加回根节点
        rootNode->addChild(transform);
        
        std::cout << "Scene moved to origin. Offset applied: (" 
                  << offset.x() << ", " << offset.y() << ", " << offset.z() << ")" << std::endl;
        
        // 更新相机视角，使其看向原点
        osgGA::CameraManipulator* manipulator = view->getCameraManipulator();
        if (manipulator) {
            // 设置新的home位置，使相机看向原点
            osg::Vec3d eye(0, -1000, 500);  // 相机位置
            osg::Vec3d center(0, 0, 0);     // 目标位置（原点）
            osg::Vec3d up(0, 0, 1);         // 上方向
            
            manipulator->setHomePosition(eye, center, up);
            manipulator->home(0.0);
            
            std::cout << "Camera updated to look at origin" << std::endl;
        }
    } else {
        std::cout << "Scene bounding box is invalid, cannot move to origin." << std::endl;
    }
}


void UIHandler::createFullscreenAtmosphere(osg::Camera* camera, osg::Group* rootNode)
{
     if (!camera || !rootNode) {
        return;
    }

    // 创建大气渲染实例
    if (!m_fullscreenAtmosphereDemo) {
        m_fullscreenAtmosphereDemo = new FullscreenAtmosphere();
          // 设置全屏大气效果
    }

    osg::ref_ptr<osg::Group> pRender = new osg::Group;

    // 将原始场景的所有子节点移动到pRender中
    for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
        pRender->addChild(rootNode->getChild(i));
    }

    // 清空根节点
    rootNode->removeChildren(0, rootNode->getNumChildren());


    osg::ref_ptr<osg::Node> atmosphereNode = m_fullscreenAtmosphereDemo->createAtmosphere(pRender,camera);
    if (atmosphereNode.valid()) {
        // 将大气效果添加到场景
        rootNode->addChild(atmosphereNode);
        std::cout << "Atmosphere effect with RTT created successfully" << std::endl;
    }
    
    std::cout << "Fullscreen atmosphere effect created successfully" << std::endl;
    
    // 请求重绘
   
}

void UIHandler::updateCloudParameters(float shapescale, float detailScale, float windSpeed,
                              float weatherScale, float curlStrength, float curlScale,
                              float erosionStrength, float windDirX, float windDirY, float windDirZ,
                              float weatherWindX, float weatherWindY)
{
    if (m_fullscreenAtmosphereDemo) {
        m_fullscreenAtmosphereDemo->updateCloudParameters(shapescale, detailScale, windSpeed,
                                                         weatherScale, curlStrength, curlScale,
                                                         erosionStrength, windDirX, windDirY, windDirZ,
                                                         weatherWindX, weatherWindY);
    }
}

void UIHandler::resetCloudParameters()
{
    if (m_fullscreenAtmosphereDemo) {
        m_fullscreenAtmosphereDemo->resetCloudParameters();
    }
}

void UIHandler::updateMousePosition(float x, float y)
{
    // 如果有FastAtmosphere实例，也更新其鼠标位置
    if (m_fastAtmosphereDemo) {
        m_fastAtmosphereDemo->updateMousePosition(x, y);
    }
}