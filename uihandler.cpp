#include "uihandler.h"
#include "viewmanager.h"
#include <QFile>
#include <QDir>
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
#include "shaderpbr.h"  // 添加PBR头文件
#include "skybox.h"


UIHandler::UIHandler()
{
}

UIHandler::~UIHandler()
{
}

void UIHandler::createShape(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode)
{
    if (viewer && rootNode) {
        // 清除现有的场景
        rootNode->removeChildren(0, rootNode->getNumChildren());
        
        // 创建天空盒，使用相对路径（基于应用程序目录）
        std::string resourcePath = QDir::currentPath().toStdString() + "/../../resource";
        osg::ref_ptr<osg::Node> skyBox = ShaderCube::createSkyBox(resourcePath);
        if (skyBox.valid()) {
            rootNode->addChild(skyBox);
        }
        
        // 使用Shader创建立方体
        osg::ref_ptr<osg::Node> shaderCube = ShaderCube::createCube(1.0f);
        
        // 更新节点引用
        shapeNode = dynamic_cast<osg::Geode*>(shaderCube.get());
        
        // 将Shader立方体添加到根节点
        if (shaderCube.valid()) {
            rootNode->addChild(shaderCube);
        }
        
        // 强制更新视图
        viewer->advance();
        viewer->requestRedraw();
    }
}

void UIHandler::createShapeWithNewSkybox(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode)
{
    if (viewer && rootNode) {
        // 清除现有的场景
        rootNode->removeChildren(0, rootNode->getNumChildren());
        
        // 创建使用新类的天空盒，使用相对路径（基于应用程序目录）
        std::string resourcePath = QDir::currentPath().toStdString() + "/../../resource";
        osg::ref_ptr<osg::Node> skyBox = ShaderCube::createSkyBoxWithNewClass(resourcePath);
        if (skyBox.valid()) {
            rootNode->addChild(skyBox);
        }
        
        // 使用Shader创建立方体
        osg::ref_ptr<osg::Node> shaderCube = ShaderCube::createCube(1.0f);
        
        // 更新节点引用
        shapeNode = dynamic_cast<osg::Geode*>(shaderCube.get());
        
        // 将Shader立方体添加到根节点
        if (shaderCube.valid()) {
            rootNode->addChild(shaderCube);
        }
        
        // 强制更新视图
        viewer->advance();
        viewer->requestRedraw();
    }
}

void UIHandler::createPBRScene(osgViewer::Viewer* viewer, osg::Group* rootNode, osg::ref_ptr<osg::Geode>& shapeNode)
{
    if (viewer && rootNode) {
        // 清除现有的场景
        rootNode->removeChildren(0, rootNode->getNumChildren());
        
        // 创建带PBR效果和天空盒的场景
        osg::ref_ptr<osg::Node> pbrScene = ShaderPBR::createPBRSceneWithSkybox(1.0f);
        
        // 更新节点引用
        shapeNode = nullptr; // PBR场景不使用单个Geode节点
        
        // 将PBR场景添加到根节点
        if (pbrScene.valid()) {
            rootNode->addChild(pbrScene);
        }
        
        // 设置默认的视图参数
        osg::Vec3d eye(0.0, -5.0, 0.0);
        osg::Vec3d center(0.0, 0.0, 0.0);
        osg::Vec3d up(0.0, 0.0, 1.0);
        m_viewManager.setViewParameters(eye, center, up);
        
        // 强制更新视图
        viewer->advance();
        viewer->requestRedraw();
    }
}

void UIHandler::resetView(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType)
{
    if (viewer && rootNode) {
        // 获取当前视口大小
        osg::Camera* camera = viewer->getCamera();
        if (camera) {
            osg::Viewport* viewport = camera->getViewport();
            if (viewport) {
                int width = viewport->width();
                int height = viewport->height();
                resetToHomeView(viewer, rootNode);
                // 强制更新视图
                viewer->advance();
                viewer->requestRedraw();
            } else {
                // 如果没有视口，使用默认尺寸
                resetToHomeView(viewer, rootNode);
                viewer->advance();
                viewer->requestRedraw();
            }
        } else {
            // 如果没有相机，使用默认尺寸
            resetToHomeView(viewer, rootNode);
            viewer->advance();
            viewer->requestRedraw();
        }
    }
}

void UIHandler::resetToHomeView(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (viewer && rootNode) {
        // 使用操作器的home方法重置视图
        osgGA::CameraManipulator* manipulator = viewer->getCameraManipulator();
        if (manipulator) {
            try {
                // 调用操作器的home方法重置视图
                manipulator->home(0.0);
                
                // 更新ViewManager中的相机参数
                m_viewManager.updateViewParametersFromManipulator(viewer);
            }
            catch (...) {
                // 如果操作器调用失败，使用默认设置
                osg::Vec3d eye(0.0, -5.0, 0.0);
                osg::Vec3d center(0.0, 0.0, 0.0);
                osg::Vec3d up(0.0, 0.0, 1.0);
                m_viewManager.setViewParameters(eye, center, up);
            }
        } else {
            // 如果没有操作器，使用默认设置
            osg::Vec3d eye(0.0, -5.0, 0.0);
            osg::Vec3d center(0.0, 0.0, 0.0);
            osg::Vec3d up(0.0, 0.0, 1.0);
            m_viewManager.setViewParameters(eye, center, up);
        }
        
        // 强制更新视图
        viewer->advance();
        viewer->requestRedraw();
    }
}

void UIHandler::loadOSGFile(osgViewer::Viewer* viewer, osg::Group* rootNode, const QString& fileName)
{
    if (viewer && rootNode) {
        // 检查文件名是否为空
        if (fileName.isEmpty()) {
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
            return;
        }
        
        // 将QString转换为std::string
        std::string stdFileName = fullPath.toStdString();
        
        try {
            // 使用osgDB加载模型
            osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(stdFileName);
            
            if (loadedModel) {
                // 清除现有的场景
                rootNode->removeChildren(0, rootNode->getNumChildren());
                
                // 添加加载的模型到场景
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
                    
                    // 更新视图管理器中的相机参数
                    m_viewManager.setViewParameters(eye, center, up);
                    
                    // 同时更新操作器的home位置，确保视角正确
                    osgGA::TrackballManipulator* manipulator = dynamic_cast<osgGA::TrackballManipulator*>(viewer->getCameraManipulator());
                    if (manipulator) {
                        manipulator->setHomePosition(eye, center, up);
                        // 立即应用home位置
                        manipulator->home(0.0);
                    }
                    
                    // 调整投影矩阵以适应模型大小
                    float aspectRatio = static_cast<float>(viewer->getCamera()->getViewport()->width()) / 
                                       static_cast<float>(viewer->getCamera()->getViewport()->height());
                    viewer->getCamera()->setProjectionMatrixAsPerspective(
                        30.0f, aspectRatio, radius * 0.1f, radius * 100.0f);
                } else {
                    // 如果模型没有有效的边界球，使用默认位置
                    osg::Vec3d eye(0.0, -10.0, 2.0);
                    osg::Vec3d center(0.0, 0.0, 0.0);
                    osg::Vec3d up(0.0, 0.0, 1.0);
                    
                    // 更新视图管理器中的相机参数
                    m_viewManager.setViewParameters(eye, center, up);
                    
                    // 同时更新操作器的home位置，确保视角正确
                    osgGA::TrackballManipulator* manipulator = dynamic_cast<osgGA::TrackballManipulator*>(viewer->getCameraManipulator());
                    if (manipulator) {
                        manipulator->setHomePosition(eye, center, up);
                        // 立即应用home位置
                        manipulator->home(0.0);
                    }
                }
                
                // 强制更新视图
                viewer->advance();
                viewer->requestRedraw();
            }
        }
        catch (const std::exception& e) {
            // 如果加载失败，重新创建默认场景
            rootNode->removeChildren(0, rootNode->getNumChildren());
            viewer->advance();
            viewer->requestRedraw();
        }
        catch (...) {
            // 如果加载失败，重新创建默认场景
            rootNode->removeChildren(0, rootNode->getNumChildren());
            viewer->advance();
            viewer->requestRedraw();
        }
    }
}

void UIHandler::setShapeColor(osg::Geode* shapeNode, float r, float g, float b, float a)
{
    // 使用保存的节点引用
    if (shapeNode) {
        // 获取状态集
        osg::StateSet* stateset = shapeNode->getOrCreateStateSet();
        
        // 更新颜色uniform
        osg::Uniform* colorUniform = stateset->getUniform("baseColor");
        if (colorUniform) {
            colorUniform->set(osg::Vec4(r, g, b, a));
        } else {
            stateset->addUniform(new osg::Uniform("baseColor", osg::Vec4(r, g, b, a)));
        }
    }
}

// 添加相机相关的函数实现
void UIHandler::setupCameraForView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height, SimpleOSGViewer::ViewType viewType)
{
    m_viewManager.setupCameraForView(viewer, rootNode, width, height, viewType);
}

void UIHandler::setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType)
{
    m_viewManager.setViewType(viewer, rootNode, viewType);
}