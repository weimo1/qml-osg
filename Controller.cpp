#include "Controller.h"
#include <iostream>
#include <QMessageBox>
#include <QDebug>
#include "UIHandler.h"
#include "AtmosphereDemo.h"
#include "FullscreenAtmosphere.h"
#include "FastAtmosphere.h"
#include "UEatmosphere.h"

Controller::Controller(GraphicsWindowQt* viewWidget, QObject *parent)
    : QObject(parent)
    , m_viewWidget(viewWidget)
    , m_uiHandler(new UIHandler(this))
    , m_fullscreenAtmosphereDemo(new FullscreenAtmosphere)
    , ueAtmosphereDemo(new UEatmosphere)
{
    if (!m_viewWidget) {
        qDebug() << "Error: viewWidget is null";
        return;
    }

    // 连接UI组件的信号到控制器的槽
    connect(m_viewWidget, &GraphicsWindowQt::loadFileRequested, this, &Controller::onLoadFileRequested);
    connect(m_viewWidget, &GraphicsWindowQt::toggleLightingRequested, this, &Controller::onToggleLightingRequested);
    connect(m_viewWidget, &GraphicsWindowQt::createAtmosphereRequested, this, &Controller::onCreateAtmosphereRequested);
    connect(m_viewWidget, &GraphicsWindowQt::createFullscreenAtmosphereRequested, this, &Controller::onCreateFullscreenAtmosphereRequested);
    connect(m_viewWidget, &GraphicsWindowQt::updateCloudParametersRequested, this, &Controller::onUpdateCloudParameters);
    connect(m_viewWidget, &GraphicsWindowQt::resetCloudParametersRequested, this, &Controller::onResetCloudParameters);
    connect(m_viewWidget, &GraphicsWindowQt::updateMousePositionRequested, this, &Controller::onUpdateMousePosition);
    connect(m_viewWidget, &GraphicsWindowQt::createFullscreenTriangleRequested, this, &Controller::onCreateFullscreenTriangleRequested);
    
    // 连接UIHandler的信号到控制器的槽
    connect(m_uiHandler, &UIHandler::fileLoadSuccess, this, &Controller::onFileLoadSuccess);
    connect(m_uiHandler, &UIHandler::fileLoadError, this, &Controller::onFileLoadError);
}

Controller::~Controller()
{
}

void Controller::onLoadFileRequested()
{
    std::cout << "Load file requested" << std::endl;
    
    // 获取当前的视图和根节点
    osgViewer::View* view = m_viewWidget->getView();
    if (view) {
        osg::Group* rootNode = m_viewWidget->getRootNode();
        
        if (rootNode) {
            // 使用UIHandler加载文件
            QString fileName = "path_to_your_model.osg"; // 这里应该是实际的文件路径
            m_uiHandler->loadOSGFile(view, rootNode, fileName);
        } else {
            std::cerr << "Failed to get root node for loading file" << std::endl;
        }
    } else {
        std::cerr << "Failed to get view for loading file" << std::endl;
    }
}

void Controller::onToggleLightingRequested(bool enabled)
{
    std::cout << "Toggle lighting requested, state: " << (enabled ? "ON" : "OFF") << std::endl;
    
    // 获取当前的视图和根节点
    osgViewer::View* view = m_viewWidget->getView();
    if (view) {
        osg::Group* rootNode = m_viewWidget->getRootNode();
        
        if (rootNode) {
            // 使用UIHandler切换光照
            m_uiHandler->toggleLighting(view, rootNode, enabled);
        } else {
            std::cerr << "Failed to get root node for toggling lighting" << std::endl;
        }
    } else {
        std::cerr << "Failed to get view for toggling lighting" << std::endl;
    }
}

void Controller::onCreateAtmosphereRequested()
{
    std::cout << "Create atmosphere requested" << std::endl;
    
    // 获取当前的相机和根节点
    osgViewer::View* view = m_viewWidget->getView();
    if (view) {
        osg::Camera* camera = view->getCamera();
        osg::Group* rootNode = m_viewWidget->getRootNode();
        
        if (camera && rootNode) {
            // 使用UIHandler创建大气效果
            m_uiHandler->createAtmosphere(camera, rootNode);
        } else {
            std::cerr << "Failed to get camera or root node for atmosphere effect" << std::endl;
        }
    } else {
        std::cerr << "Failed to get view for atmosphere effect" << std::endl;
    }
}

void Controller::onCreateFullscreenAtmosphereRequested()
{
    std::cout << "Create fullscreen atmosphere requested" << std::endl;
    
    // 获取当前的相机和根节点
    osgViewer::View* view = m_viewWidget->getView();
    if (view) {
        osg::Camera* camera = view->getCamera();
        osg::Group* rootNode = m_viewWidget->getRootNode();
        
        if (camera && rootNode) {
            m_uiHandler->createFullscreenAtmosphere(camera, rootNode);
        } else {
            std::cerr << "Failed to get camera or root node for fullscreen atmosphere effect" << std::endl;
        }
    } else {
        std::cerr << "Failed to get view for fullscreen atmosphere effect" << std::endl;
    }
}

void Controller::onFileLoadSuccess(const QString& fileName)
{
    // 文件加载成功，显示消息
    std::cout << "File loaded successfully: " << fileName.toStdString() << std::endl;
    
    // 注意：我们不再在每次加载单个文件时都移动场景到原点
    // 而是在所有文件加载完成后再移动整个场景
    // 这样可以避免场景堆叠在一起的问题
}

void Controller::onFileLoadError(const QString& fileName, const QString& error)
{
    // 文件加载失败，显示错误消息
    std::cerr << "Failed to load file: " << fileName.toStdString() 
              << ", Error: " << error.toStdString() << std::endl;
    
    // 显示错误消息框
    QMessageBox::warning(nullptr, tr("File Load Error"), 
                         tr("Failed to load file: %1\nError: %2").arg(fileName).arg(error));
}

// 注意：我们已经将moveSceneToOrigin功能移到UIHandler中，所以这里不再需要这个函数

void Controller::onUpdateCloudParameters(float shapescale, float detailScale, float windSpeed,
                                float weatherScale, float curlStrength, float curlScale,
                                float erosionStrength, float windDirX, float windDirY, float windDirZ,
                                float weatherWindX, float weatherWindY)
{
    std::cout << "Update cloud parameters requested" << std::endl;
    
    // 使用UIHandler更新云参数
    m_uiHandler->updateCloudParameters(shapescale, detailScale, windSpeed,
                                   weatherScale, curlStrength, curlScale,
                                   erosionStrength, windDirX, windDirY, windDirZ,
                                   weatherWindX, weatherWindY);
}

void Controller::onResetCloudParameters()
{
    std::cout << "Reset cloud parameters requested" << std::endl;
    
    // 使用UIHandler重置云参数
    m_uiHandler->resetCloudParameters();
}

void Controller::onUpdateMousePosition(float x, float y)
{

    // 使用UIHandler更新鼠标位置
    m_uiHandler->updateMousePosition(x, y);
}

void Controller::onCreateFullscreenTriangleRequested()
{
    std::cout << "Create fullscreen red triangle requested" << std::endl;
    
    // 使用UIHandler创建全屏红色三角形
    osgViewer::View* view = m_viewWidget->getView();
    if (view) {
        osg::Camera* camera = view->getCamera();
        osg::Group* rootNode = m_viewWidget->getRootNode();
        
        if (camera && rootNode) {
            // 清空当前场景
            rootNode->removeChildren(0, rootNode->getNumChildren());
            
            osg::Node* triangleNode = ueAtmosphereDemo->createAtmosphere(nullptr, camera);
            if (triangleNode) {
                rootNode->addChild(triangleNode);
            }
        } else {
            std::cerr << "Failed to get camera or root node for fullscreen triangle" << std::endl;
        }
    } else {
        std::cerr << "Failed to get view for fullscreen triangle" << std::endl;
    }
}