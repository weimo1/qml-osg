#include "Controller.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfoList>
#include <iostream>
#include <osg/MatrixTransform>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>

Controller::Controller(GraphicsWindowQt* viewWidget, QObject *parent)
    : QObject(parent)
    , m_viewWidget(viewWidget)
{
    // 创建UI处理器
    m_uiHandler = new UIHandler(this);
    
    // 连接GraphicsWindowQt的信号到控制器的槽
    connect(m_viewWidget, &GraphicsWindowQt::loadFileRequested, this, &Controller::onLoadFileRequested);
    connect(m_viewWidget, &GraphicsWindowQt::toggleLightingRequested, this, &Controller::onToggleLightingRequested);
    connect(m_viewWidget, &GraphicsWindowQt::createAtmosphereRequested, this, &Controller::onCreateAtmosphereRequested);  // 添加大气效果信号连接
    
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
    
    // 询问用户是加载单个文件还是目录
    QMessageBox msgBox;
    msgBox.setWindowTitle("Load Option");
    msgBox.setText("What would you like to load?");
    msgBox.setIcon(QMessageBox::Question);
    
    QPushButton *fileButton = msgBox.addButton(tr("Single File"), QMessageBox::ActionRole);
    QPushButton *dirButton = msgBox.addButton(tr("Directory"), QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == fileButton) {
        // 打开文件对话框让用户选择OSG文件
        QString fileName = QFileDialog::getOpenFileName(
            nullptr, 
            tr("Open OSG File"), 
            "", 
            tr("OSG Files (*.osg *.osgt *.osgb);;All Files (*)")
        );
        
        std::cout << "Selected file: " << fileName.toStdString() << std::endl;
        
        if (!fileName.isEmpty()) {
            // 使用UIHandler加载文件
            m_uiHandler->loadOSGFile(m_viewWidget->getView(), m_viewWidget->getRootNode(), fileName);
        }
    }
    else if (msgBox.clickedButton() == dirButton) {
        // 打开目录对话框让用户选择目录
        QString dirName = QFileDialog::getExistingDirectory(
            nullptr,
            tr("Select Directory"),
            "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
        
        std::cout << "Selected directory: " << dirName.toStdString() << std::endl;
        
        if (!dirName.isEmpty()) {
            // 使用UIHandler加载目录
            m_uiHandler->loadOSGFile(m_viewWidget->getView(), m_viewWidget->getRootNode(), dirName);
        }
    }
    // 如果点击取消按钮，则不执行任何操作
}

void Controller::onToggleLightingRequested(bool enabled)
{
    std::cout << "Toggle lighting requested: " << (enabled ? "ON" : "OFF") << std::endl;
    
    // 使用UIHandler切换光照
    m_uiHandler->toggleLighting(m_viewWidget->getView(), m_viewWidget->getRootNode(), enabled);
}

void Controller::onCreateAtmosphereRequested()
{
    std::cout << "Create atmosphere requested" << std::endl;
    
    // 使用UIHandler创建大气效果
    osgViewer::View* view = m_viewWidget->getView();
    if (view) {
        osg::Camera* camera = view->getCamera();
        osg::Group* rootNode = m_viewWidget->getRootNode();
        
        if (camera && rootNode) {
            m_uiHandler->createAtmosphere(camera, rootNode);
        } else {
            std::cerr << "Failed to get camera or root node for atmosphere effect" << std::endl;
        }
    } else {
        std::cerr << "Failed to get view for atmosphere effect" << std::endl;
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
