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
#include "demoshader.h"  // 添加DemoShader头文件
#include "SkyNode.h"

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

void UIHandler::createAtmosphereScene(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (viewer && rootNode) {
        // 清除现有的场景
        rootNode->removeChildren(0, rootNode->getNumChildren());
        
        // 创建DemoShader实例
        m_demoShader = new DemoShader();
        
        
        // 创建大气渲染场景（使用全屏四边形）
        osg::ref_ptr<osg::Node> atmosphereScene = m_demoShader->createAtmosphereScene();
        
        // 将大气渲染场景添加到根节点
        if (atmosphereScene.valid()) {
            rootNode->addChild(atmosphereScene);
        } else {
            qDebug() << "Failed to create atmosphere scene";
            return;
        }
        
        // 设置默认的视图参数
        osg::Vec3d eye(0.0, 0.0, 1.0);  // 设置相机位置在原点附近
        osg::Vec3d center(0.0, 0.0, 0.0);
        osg::Vec3d up(0.0, 1.0, 0.0);
        m_viewManager.setViewParameters(eye, center, up);
        
        // 设置相机背景色为黑色，避免干扰大气渲染效果
        if (viewer->getCamera()) {
            viewer->getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        
        // 强制更新视图
        viewer->advance();
        viewer->requestRedraw();
        
        qDebug() << "Atmosphere scene created successfully";
    }
}

// 新增：创建结合纹理和大气渲染的场景
void UIHandler::createTexturedAtmosphereScene(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (viewer && rootNode) {
        // 清除现有的场景
        rootNode->removeChildren(0, rootNode->getNumChildren());
        
        // 创建DemoShader实例
        m_demoShader = new DemoShader();
        
        // 创建结合纹理和大气渲染的场景
        // 使用空字符串作为纹理路径，因为纹理路径在DemoShader中设置
        osg::ref_ptr<osg::Node> texturedAtmosphereScene = m_demoShader->createTexturedAtmosphereScene();
        
        // 将场景添加到根节点
        if (texturedAtmosphereScene.valid()) {
            rootNode->addChild(texturedAtmosphereScene);
        } else {
            qDebug() << "Failed to create textured atmosphere scene";
            return;
        }
        
        // 设置默认的视图参数
        osg::Vec3d eye(0.0, 0.0, 5.0);  // 设置相机位置
        osg::Vec3d center(0.0, 0.0, 0.0);
        osg::Vec3d up(0.0, 1.0, 0.0);
        m_viewManager.setViewParameters(eye, center, up);
        
        // 设置相机背景色为黑色，避免干扰大气渲染效果
        if (viewer->getCamera()) {
            viewer->getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        
        // 强制更新视图
        viewer->advance();
        viewer->requestRedraw();
        
        qDebug() << "Textured atmosphere scene created successfully";
    }
}

// 新增：创建结合天空盒和大气渲染的场景
void UIHandler::createSkyboxAtmosphereScene(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (viewer && rootNode) {
        // 清除现有的场景
        rootNode->removeChildren(0, rootNode->getNumChildren());
        
       
        
        // 创建结合天空盒和大气渲染的场景
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 450000.0)));
    geode->setCullingActive(false);
    osg::ref_ptr<SkyBoxThree> skybox = new SkyBoxThree(viewer->getCamera());
    skybox->setName("skybox");
    skybox->addChild(geode.get());
    
        
        // 将场景添加到根节点
        if (skybox.valid()) {
            rootNode->addChild(skybox);
        } else {
            qDebug() << "Failed to create skybox atmosphere scene";
            return;
        }
        
        // 设置默认的视图参数
        // osg::Vec3d eye(0.0, 0.0, 0.0);  // 设置相机位置
        // osg::Vec3d center(0.0, 0.0, 0.0);
        // osg::Vec3d up(0.0, 1.0, 0.0);
        // m_viewManager.setViewParameters(eye, center, up);
        
        // 设置相机背景色为黑色，避免干扰大气渲染效果
        if (viewer->getCamera()) {
            viewer->getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        
        // 强制更新视图
        viewer->advance();
        viewer->requestRedraw();
        
        qDebug() << "Skybox atmosphere scene created successfully";
    }
}

void UIHandler::updateAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode, 
                                         float sunZenithAngle, float sunAzimuthAngle)
{
    if (m_demoShader.valid()) {
        // 更新DemoShader中的参数
        m_demoShader->setSunZenithAngle(sunZenithAngle);
        m_demoShader->setSunAzimuthAngle(sunAzimuthAngle);
        
        // 调用DemoShader的更新函数来更新uniform变量
        // 修复：使用DemoShader中保存的状态集而不是尝试从Program获取
        osg::StateSet* atmosphereStateSet = m_demoShader->getAtmosphereStateSet();
        if (atmosphereStateSet) {
            // 根据场景类型选择合适的更新方法
            // 对于Textured Atmosphere场景，使用updateAtmosphereSceneUniforms
            m_demoShader->updateAtmosphereUniforms(atmosphereStateSet);
        }
    
        // 强制更新视图
        if (viewer) {
            viewer->advance();
            viewer->requestRedraw();
        }
    }
}

void UIHandler::updateTexturedAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                                float sunZenithAngle, float sunAzimuthAngle,
                                                float exposure)
{
    if (m_demoShader.valid()) {
        // 更新DemoShader中的参数
        m_demoShader->setSunZenithAngle(sunZenithAngle);
        m_demoShader->setSunAzimuthAngle(sunAzimuthAngle);
        m_demoShader->setExposure(exposure);
        
        qDebug() << "Updating Textured Atmosphere Parameters:";
        qDebug() << "  Sun Zenith Angle:" << sunZenithAngle;
        qDebug() << "  Sun Azimuth Angle:" << sunAzimuthAngle;
        qDebug() << "  Exposure:" << exposure;
        
        // 调用DemoShader的更新函数来更新uniform变量
        osg::StateSet* atmosphereStateSet = m_demoShader->getAtmosphereStateSet();
        if (atmosphereStateSet) {
            // 对于Textured Atmosphere场景，使用updateAtmosphereSceneUniforms
            m_demoShader->updateAtmosphereSceneUniforms(atmosphereStateSet);
        }
        
        // 强制更新视图
        if (viewer) {
            viewer->advance();
            viewer->requestRedraw();
        }
    }
}

// 添加更新大气密度和太阳强度的方法
void UIHandler::updateAtmosphereDensityAndIntensity(osgViewer::Viewer* viewer, osg::Group* rootNode, 
                                                   float density, float intensity)
{
    if (m_demoShader.valid()) {
        // 更新DemoShader中的参数
        m_demoShader->setAtmosphereDensity(density);
        m_demoShader->setSunIntensity(intensity);
        
        // 调用DemoShader的更新函数来更新uniform变量
        osg::StateSet* atmosphereStateSet = m_demoShader->getAtmosphereStateSet();
        if (atmosphereStateSet) {
            m_demoShader->updateAtmosphereUniforms(atmosphereStateSet);
        }
        
        // 强制更新视图
        if (viewer) {
            viewer->advance();
            viewer->requestRedraw();
        }
    }
}

// 添加更新米氏散射和瑞利散射的方法
void UIHandler::updateAtmosphereScattering(osgViewer::Viewer* viewer, osg::Group* rootNode, 
                                         float mie, float rayleigh)
{
    if (m_demoShader.valid()) {
        // 更新DemoShader中的参数
        m_demoShader->setMieScattering(mie);
        m_demoShader->setRayleighScattering(rayleigh);
        
        // 调用DemoShader的更新函数来更新uniform变量
        osg::StateSet* atmosphereStateSet = m_demoShader->getAtmosphereStateSet();
        if (atmosphereStateSet) {
            m_demoShader->updateAtmosphereUniforms(atmosphereStateSet);
        }
        
        // 强制更新视图
        if (viewer) {
            viewer->advance();
            viewer->requestRedraw();
        }
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

// 更新SkyNode中的大气参数
void UIHandler::updateSkyNodeAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                               float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG)
{
    if (!viewer || !rootNode) return;
    
    // 查找场景中的SkyBoxThree节点
    osg::Node* skyboxNode = nullptr;
    for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
        osg::Node* child = rootNode->getChild(i);
        if (child && child->getName() == "skybox") {
            skyboxNode = child;
            break;
        }
    }
    
    if (skyboxNode) {
        // 获取SkyBoxThree节点的状态集
        osg::StateSet* stateset = skyboxNode->getOrCreateStateSet();
        if (stateset) {
            // 更新uniform变量
            osg::Uniform* turbidityUniform = stateset->getUniform("turbidity");
            if (turbidityUniform) {
                turbidityUniform->set(turbidity);
            }
            
            osg::Uniform* rayleighUniform = stateset->getUniform("rayleigh");
            if (rayleighUniform) {
                rayleighUniform->set(rayleigh);
            }
            
            osg::Uniform* mieCoefficientUniform = stateset->getUniform("mieCoefficient");
            if (mieCoefficientUniform) {
                mieCoefficientUniform->set(mieCoefficient);
            }
            
            osg::Uniform* mieDirectionalGUniform = stateset->getUniform("mieDirectionalG");
            if (mieDirectionalGUniform) {
                mieDirectionalGUniform->set(mieDirectionalG);
            }
        }
    }
    
    // 强制更新视图
    if (viewer) {
        viewer->advance();
        viewer->requestRedraw();
    }
}
