#include "osgWindow.h"
#include "CustomTrackballManipulator.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QBoxLayout>
#include <QEnterEvent>
#include <QApplication>
#include <osg/Notify>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/StateSet>
#include <osg/MatrixTransform>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <iostream>
#include <iomanip>
#include <sstream>

GraphicsWindowQt::GraphicsWindowQt(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_mouseOverButton(false)
{
    // Set up timer for periodic updates
    connect(&m_timer, &QTimer::timeout, this, QOverload<>::of(&GraphicsWindowQt::update));
    m_timer.start(20); // 50 Hz update rate
    
    // Connect timer to update camera position label
    connect(&m_timer, &QTimer::timeout, this, &GraphicsWindowQt::updateCameraPosition);
    
    // Create UI components
    createUI();
    
    // 设置窗口属性以确保按钮能正确接收事件
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

GraphicsWindowQt::~GraphicsWindowQt()
{
    // Clean up OSG resources
    m_viewer = nullptr;
    m_graphicsWindow = nullptr;
    m_root = nullptr;
}

void GraphicsWindowQt::initializeGL()
{
    // Initialize OSG graphics context
    setupOSG(width(), height());
    createSimpleScene();
}

void GraphicsWindowQt::paintGL()
{
    // Render the scene
    if (m_graphicsWindow)
    {
        if (_firstFrame)
        {
            GLuint defaultFboId = this->defaultFramebufferObject();
            m_graphicsWindow->setDefaultFboId(defaultFboId);  // must set for internal FBO
            _firstFrame = false;
        }
        if (isVisibleTo(QApplication::activeWindow())) {
            m_viewer->frame();
        }
    }
}

void GraphicsWindowQt::resizeGL(int width, int height)
{
    // Handle resize events
    if (m_graphicsWindow.valid())
    {
        // Update the graphics window
        m_graphicsWindow->getEventQueue()->windowResize(this->x(), this->y(), width, height);
        m_graphicsWindow->resized(this->x(), this->y(), width, height);
        
        // Update all cameras
        osgViewer::ViewerBase::Cameras cameras;
        m_viewer->getCameras(cameras);
        
        for (auto camera : cameras)
        {
            camera->setViewport(0, 0, width, height);
        }
    }
    
    // 重新调整按钮容器的位置
    if (m_buttonContainer) {
        m_buttonContainer->setGeometry(10, 10, 200, 40);
        m_buttonContainer->raise(); // 确保按钮容器在最上层
    }
}

void GraphicsWindowQt::keyPressEvent(QKeyEvent* event)
{
    if (m_graphicsWindow.valid())
    {
        QString keyString = event->text();
        if (!keyString.isEmpty())
        {
            m_graphicsWindow->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KeySymbol(keyString.toStdString()[0]));
        }
    }
    QOpenGLWidget::keyPressEvent(event);
}

void GraphicsWindowQt::keyReleaseEvent(QKeyEvent* event)
{
    if (m_graphicsWindow.valid())
    {
        QString keyString = event->text();
        if (!keyString.isEmpty())
        {
            m_graphicsWindow->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KeySymbol(keyString.toStdString()[0]));
        }
    }
    QOpenGLWidget::keyReleaseEvent(event);
}

void GraphicsWindowQt::mousePressEvent(QMouseEvent* event)
{
    // 检查点击是否在按钮区域
    if (m_buttonContainer && m_buttonContainer->geometry().contains(event->pos())) {
        // 直接将事件转发给按钮容器
        QWidget::mousePressEvent(event);
        return;
    }
    
    if (m_graphicsWindow.valid())
    {
        int button = 0;
        switch (event->button())
        {
        case Qt::LeftButton:
            button = 1;
            break;
        case Qt::MiddleButton:
            button = 2;
            break;
        case Qt::RightButton:
            button = 3;
            break;
        default:
            break;
        }
        m_graphicsWindow->getEventQueue()->mouseButtonPress(event->x(), event->y(), button);
    }
    QOpenGLWidget::mousePressEvent(event);
}

void GraphicsWindowQt::mouseReleaseEvent(QMouseEvent* event)
{
    // 检查点击是否在按钮区域
    if (m_buttonContainer && m_buttonContainer->geometry().contains(event->pos())) {
        // 直接将事件转发给按钮容器
        QWidget::mouseReleaseEvent(event);
        return;
    }
    
    if (m_graphicsWindow.valid())
    {
        int button = 0;
        switch (event->button())
        {
        case Qt::LeftButton:
            button = 1;
            break;
        case Qt::MiddleButton:
            button = 2;
            break;
        case Qt::RightButton:
            button = 3;
            break;
        default:
            break;
        }
        m_graphicsWindow->getEventQueue()->mouseButtonRelease(event->x(), event->y(), button);
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GraphicsWindowQt::mouseMoveEvent(QMouseEvent* event)
{
    // 更新鼠标位置
    m_mousePos = event->pos();
    
    // 检查鼠标是否在按钮区域
    if (m_buttonContainer && m_buttonContainer->geometry().contains(event->pos())) {
        m_mouseOverButton = true;
        // 直接将事件转发给按钮容器
        QWidget::mouseMoveEvent(event);
    } else {
        m_mouseOverButton = false;
        // 发出鼠标位置信号
        QString mousePosStr = QString("Mouse: X:%1 Y:%2").arg(m_mousePos.x()).arg(m_mousePos.y());
        emit mousePositionChanged(mousePosStr);
        
        if (m_graphicsWindow.valid())
        {
            m_graphicsWindow->getEventQueue()->mouseMotion(event->x(), event->y());
        }
        QOpenGLWidget::mouseMoveEvent(event);
    }
}

void GraphicsWindowQt::enterEvent(QEnterEvent* event)
{
    m_mouseOverButton = false;
    QOpenGLWidget::enterEvent(event);
}

void GraphicsWindowQt::leaveEvent(QEvent* event)
{
    m_mouseOverButton = false;
    QOpenGLWidget::leaveEvent(event);
}

void GraphicsWindowQt::wheelEvent(QWheelEvent* event)
{
    if (m_graphicsWindow.valid())
    {
        QPoint delta = event->angleDelta();
        double deltaValue = delta.y() > 0 ? 1.0 : -1.0;
        m_graphicsWindow->getEventQueue()->mouseScroll(
            delta.y() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN);
    }
    QOpenGLWidget::wheelEvent(event);
}

void GraphicsWindowQt::setupOSG(int width, int height)
{
    // Create the composite viewer
    m_viewer = new osgViewer::CompositeViewer;
    m_viewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
    m_viewer->setRunFrameScheme(osgViewer::ViewerBase::ON_DEMAND);
    
    // Create graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = this->x();
    traits->y = this->y();
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = nullptr;
    traits->setInheritedWindowPixelFormat = true;
    
    // Create embedded graphics window
    m_graphicsWindow = new osgViewer::GraphicsWindowEmbedded(traits.get());
    
    // Create camera
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setViewport(0, 0, width, height);
    camera->setGraphicsContext(m_graphicsWindow.get());
    camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(width)/static_cast<double>(height), 1.0, 10000.0);
    // Set background color to light blue
    camera->setClearColor(osg::Vec4(0.7f, 0.8f, 1.0f, 1.0f));
    
    // Create view
    osgViewer::View* view = new osgViewer::View;
    view->setCamera(camera);
    
    // 使用自定义的TrackballManipulator
    osg::ref_ptr<CustomTrackballManipulator> manipulator = new CustomTrackballManipulator;
    
    view->setCameraManipulator(manipulator.get());
    
    // Add view to viewer
    m_viewer->addView(view);
}

void GraphicsWindowQt::createSimpleScene()
{
    // Create root node
    m_root = new osg::Group;
    
    osg::ref_ptr<osg::Node> model = osgDB::readNodeFile("D:\\modlefile\\modefile1\\Data\\Tile_-486_-169\\Tile_-486_-169.osgb");
    m_root->addChild(model);
    
    // 将场景移动到原点
    moveSceneToOrigin();
    
    // Set the scene data for all views
    for (unsigned int i = 0; i < m_viewer->getNumViews(); ++i)
    {
        m_viewer->getView(i)->setSceneData(m_root);
    }
}

// 新增函数实现
void GraphicsWindowQt::moveSceneToOrigin()
{
    if (!m_root) return;
    
    // 使用ComputeBoundsVisitor计算整个场景的边界
    osg::ComputeBoundsVisitor boundVisitor;
    m_root->accept(boundVisitor);
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
        for (unsigned int i = 0; i < m_root->getNumChildren(); ++i) {
            children.push_back(m_root->getChild(i));
        }
        
        // 清空根节点
        m_root->removeChildren(0, m_root->getNumChildren());
        
        // 将所有子节点添加到变换节点中
        for (unsigned int i = 0; i < children.size(); ++i) {
            transform->addChild(children[i]);
        }
        
        // 将变换节点添加回根节点
        m_root->addChild(transform);
        
        std::cout << "Scene moved to origin. Offset applied: (" 
                  << offset.x() << ", " << offset.y() << ", " << offset.z() << ")" << std::endl;
    }
}

osgGA::EventQueue* GraphicsWindowQt::getEventQueue() const
{
    if (m_graphicsWindow.valid())
    {
        return m_graphicsWindow->getEventQueue();
    }
    return nullptr;
}

osg::Vec3 GraphicsWindowQt::getCameraPosition() const
{
    if (m_viewer.valid() && m_viewer->getNumViews() > 0) {
        osgViewer::View* view = m_viewer->getView(0);
        if (view) {
            osg::Camera* camera = view->getCamera();
            if (camera) {
                // Get the view matrix
                osg::Vec3d eye, center, up;
                camera->getViewMatrixAsLookAt(eye, center, up);
                return eye;
            }
        }
    }
    return osg::Vec3(0.0f, 0.0f, 0.0f);
}

void GraphicsWindowQt::updateCameraPosition()
{
    osg::Vec3 camPos = getCameraPosition();
    QString camPosStr = QString("Cam: X:%1 Y:%2 Z:%3")
        .arg(camPos.x(), 0, 'f', 2)
        .arg(camPos.y(), 0, 'f', 2)
        .arg(camPos.z(), 0, 'f', 2);
    emit cameraPositionChanged(camPosStr);
}

void GraphicsWindowQt::createUI()
{
    // Create button container
    m_buttonContainer = new QWidget(this);
    m_buttonContainer->setStyleSheet("background-color: rgba(200, 200, 200, 150); border-radius: 5px;");
    m_buttonContainer->setAutoFillBackground(true);
    
    // Create layout for buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout(m_buttonContainer);
    buttonLayout->setContentsMargins(5, 5, 5, 5);
    buttonLayout->setSpacing(5);
    
    // Create load file button
    m_loadFileButton = new QPushButton("Load File", m_buttonContainer);
    m_loadFileButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; padding: 5px; border-radius: 3px; }"
                                   "QPushButton:hover { background-color: #45a049; }"
                                   "QPushButton:pressed { background-color: #3d8b40; }");
    connect(m_loadFileButton, &QPushButton::clicked, this, &GraphicsWindowQt::onLoadFileButtonClicked);
    
    // Create toggle lighting button
    m_toggleLightingButton = new QPushButton("Lighting", m_buttonContainer);
    m_toggleLightingButton->setCheckable(true);
    m_toggleLightingButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; border: none; padding: 5px; border-radius: 3px; }"
                                         "QPushButton:checked { background-color: #0b7dda; }"
                                         "QPushButton:hover { background-color: #1976D2; }"
                                         "QPushButton:pressed { background-color: #0d47a1; }");
    connect(m_toggleLightingButton, &QPushButton::clicked, this, &GraphicsWindowQt::onToggleLightingButtonClicked);
    
    // Create atmosphere button
    m_createAtmosphereButton = new QPushButton("Atmosphere", m_buttonContainer);
    m_createAtmosphereButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; border: none; padding: 5px; border-radius: 3px; }"
                                           "QPushButton:hover { background-color: #F57C00; }"
                                           "QPushButton:pressed { background-color: #EF6C00; }");
    connect(m_createAtmosphereButton, &QPushButton::clicked, this, &GraphicsWindowQt::onCreateAtmosphereButtonClicked);
    
    // Add buttons to layout
    buttonLayout->addWidget(m_loadFileButton);
    buttonLayout->addWidget(m_toggleLightingButton);
    buttonLayout->addWidget(m_createAtmosphereButton);
    buttonLayout->addStretch();
    
    // Position the button container at the top
    m_buttonContainer->setGeometry(10, 10, 220, 30);
    m_buttonContainer->show();
    m_buttonContainer->raise(); // 确保按钮容器在最上层
}

void GraphicsWindowQt::onLoadFileButtonClicked()
{
    std::cout << "Load file button clicked" << std::endl;
    emit loadFileRequested();
}

void GraphicsWindowQt::onToggleLightingButtonClicked()
{
    bool enabled = m_toggleLightingButton->isChecked();
    std::cout << "Toggle lighting button clicked, state: " << (enabled ? "ON" : "OFF") << std::endl;
    emit toggleLightingRequested(enabled);
}

void GraphicsWindowQt::onCreateAtmosphereButtonClicked()
{
    std::cout << "Create atmosphere button clicked" << std::endl;
    emit createAtmosphereRequested();
}

// Public methods to access OSG components
osgViewer::View* GraphicsWindowQt::getView() const
{
    if (m_viewer.valid() && m_viewer->getNumViews() > 0) {
        return m_viewer->getView(0);
    }
    return nullptr;
}

osg::Group* GraphicsWindowQt::getRootNode() const
{
    return m_root.get();
}