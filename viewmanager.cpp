#include "viewmanager.h"
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osg/Camera>
#include <osgGA/TrackballManipulator>
#include <QDebug>

ViewManager::ViewManager()
    : m_eye(0.0, -5.0, 0.0)
    , m_center(0.0, 0.0, 0.0)
    , m_up(0.0, 0.0, 1.0)
{
}

ViewManager::~ViewManager()
{
}

// 设置视图参数
void ViewManager::setViewParameters(osg::Vec3d eye, osg::Vec3d center, osg::Vec3d up)
{
    m_eye = eye;
    m_center = center;
    m_up = up;
}

// 从操作器获取当前视图参数
void ViewManager::updateViewParametersFromManipulator(osgViewer::Viewer* viewer)
{
    if (!viewer) return;
    
    osgGA::TrackballManipulator* manipulator = dynamic_cast<osgGA::TrackballManipulator*>(viewer->getCameraManipulator());
    if (manipulator) {
        // 获取当前的相机矩阵
        osg::Matrixd matrix = manipulator->getMatrix();
        
        // 从矩阵中提取eye, center, up参数
        matrix.getLookAt(m_eye, m_center, m_up);
    }
    
    // 确保设置透视投影矩阵
    osg::Camera* camera = viewer->getCamera();
    if (camera) {
        // 获取当前视口大小
        osg::Viewport* viewport = camera->getViewport();
        if (viewport) {
            double aspectRatio = static_cast<double>(viewport->width()) / static_cast<double>(viewport->height());
            // 设置透视投影
            camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 1.0, 10000.0);
        }
    }
}

// 根据视图类型设置相机
void ViewManager::setupCameraForView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height, SimpleOSGViewer::ViewType viewType)
{
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // 设置视口
    camera->setViewport(new osg::Viewport(0, 0, width, height));
    
    // 计算宽高比
    double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    // 根据视图类型设置不同的视角
    switch (viewType) {
    case SimpleOSGViewer::FrontView:  // 前视图 (从前方看)
        setupFrontView(viewer, rootNode, width, height);
        break;
        
    case SimpleOSGViewer::SideView:  // 侧视图 (从右侧看)
        setupSideView(viewer, rootNode, width, height);
        break;
        
    case SimpleOSGViewer::TopView:  // 俯视图 (从上方看)
        setupTopView(viewer, rootNode, width, height);
        break;
        
    case SimpleOSGViewer::MainView:  // 主视图 (默认视角)
    default:
        setupMainView(viewer, rootNode, width, height);
        break;
    }
}

// 设置特定视图类型
void ViewManager::setViewType(osgViewer::Viewer* viewer, osg::Group* rootNode, SimpleOSGViewer::ViewType viewType)
{
    if (!viewer || !rootNode) return;
    
    // 获取场景的边界框，用于设置操作器的旋转中心
    osg::ComputeBoundsVisitor boundsVisitor;
    rootNode->accept(boundsVisitor);
    osg::BoundingBox bb = boundsVisitor.getBoundingBox();
    
    osg::Vec3 center;
    double modelSize = 1.0;
    if (bb.valid()) {
        // 使用场景的中心作为旋转中心
        center = bb.center();
        // 计算模型大小
        modelSize = bb.radius() > 0 ? bb.radius() : 1.0;
    } else {
        // 如果没有有效场景，使用默认中心
        center.set(0.0f, 0.0f, 0.0f);
    }
    
    // 根据视图类型设置操作器的home位置
    osg::Vec3 eye, up;
    double viewDistance = modelSize * 2.0; // 根据模型大小调整视距
    switch (viewType) {
    case SimpleOSGViewer::FrontView:  // 前视图 (从前方看)
        eye.set(center.x(), center.y() - viewDistance, center.z());
        up.set(0.0f, 0.0f, 1.0f);
        break;
    case SimpleOSGViewer::SideView:  // 侧视图 (从右侧看)
        eye.set(center.x() + viewDistance, center.y(), center.z());
        up.set(0.0f, 0.0f, 1.0f);
        break;
    case SimpleOSGViewer::TopView:  // 俯视图 (从上方看)
        eye.set(center.x(), center.y(), center.z() + viewDistance);
        up.set(0.0f, 1.0f, 0.0f);
        break;
    case SimpleOSGViewer::MainView:  // 主视图 (默认视角)
    default:
        // 主视图使用稍微偏下的角度观察
        eye.set(center.x(), center.y() - viewDistance, center.z() + viewDistance * 0.2);
        up.set(0.0f, 0.0f, 1.0f);
        break;
    }
    
    // 更新视图参数
    setViewParameters(eye, center, up);
    
    // 更新操作器设置
    osgGA::TrackballManipulator* manipulator = dynamic_cast<osgGA::TrackballManipulator*>(viewer->getCameraManipulator());
    if (manipulator) {
        // 重新设置操作器的home位置，确保旋转中心是模型的中心
        manipulator->setHomePosition(eye, center, up);
        
        // 对于所有视图类型，都不固定垂直轴以获得更好的旋转体验
        manipulator->setVerticalAxisFixed(false);
        
        // 禁止投掷效果，避免自动旋转
        manipulator->setAllowThrow(false);
        
        // 立即应用home位置
        manipulator->home(0.0);
    }
    
    // 获取当前视口大小并更新相机设置
    if (viewer->getCamera()) {
        // 获取当前视口大小
        osg::Viewport* viewport = viewer->getCamera()->getViewport();
        if (viewport) {
            int width = viewport->width();
            int height = viewport->height();
            setupCameraForView(viewer, rootNode, width, height, viewType);
        }
    }
}

// 视图相关的辅助函数实现
void ViewManager::setupFrontView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height)
{
    if (!viewer || !rootNode) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // 计算宽高比
    double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    // 获取场景的边界框，用于计算合适的视图范围
    osg::ComputeBoundsVisitor boundsVisitor;
    rootNode->accept(boundsVisitor);
    osg::BoundingBox bb = boundsVisitor.getBoundingBox();
    
    osg::Vec3 eye, center, up;
    double viewDistance = 5.0; // 默认距离
    double modelSize = 1.0;    // 默认模型大小
    
    if (bb.valid()) {
        // 使用场景的中心作为视图中心
        center = bb.center();
        
        // 计算模型大小
        modelSize = bb.radius() > 0 ? bb.radius() : 1.0;
        viewDistance = modelSize * 2.0; // 根据模型大小调整视距
        
        // 前视图设置 - 从Y轴负方向看
        eye.set(center.x(), center.y() - viewDistance, center.z());
        up.set(0.0f, 0.0f, 1.0f);  // Z轴向上
    } else {
        // 如果没有有效场景，使用默认设置
        eye.set(0.0f, -5.0f, 0.0f);
        center.set(0.0f, 0.0f, 0.0f);
        up.set(0.0f, 0.0f, 1.0f);
    }
    
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 更新视图参数
    setViewParameters(eye, center, up);
    
    // 设置透视投影（不再使用正交投影）
    camera->setProjectionMatrixAsPerspective(45.0f, static_cast<double>(width) / static_cast<double>(height), 1.0, 10000.0);
}

void ViewManager::setupSideView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height)
{
    if (!viewer || !rootNode) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // 计算宽高比
    double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    // 获取场景的边界框，用于计算合适的视图范围
    osg::ComputeBoundsVisitor boundsVisitor;
    rootNode->accept(boundsVisitor);
    osg::BoundingBox bb = boundsVisitor.getBoundingBox();
    
    osg::Vec3 eye, center, up;
    double viewDistance = 5.0; // 默认距离
    double modelSize = 1.0;    // 默认模型大小
    
    if (bb.valid()) {
        // 使用场景的中心作为视图中心
        center = bb.center();
        
        // 计算模型大小
        modelSize = bb.radius() > 0 ? bb.radius() : 1.0;
        viewDistance = modelSize * 2.0; // 根据模型大小调整视距
        
        // 侧视图设置 - 从X轴正方向看
        eye.set(center.x() + viewDistance, center.y(), center.z());
        up.set(0.0f, 0.0f, 1.0f);  // Z轴向上
    } else {
        // 如果没有有效场景，使用默认设置
        eye.set(5.0f, 0.0f, 0.0f);
        center.set(0.0f, 0.0f, 0.0f);
        up.set(0.0f, 0.0f, 1.0f);
    }
    
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 更新视图参数
    setViewParameters(eye, center, up);
    
    // 设置透视投影（不再使用正交投影）
    camera->setProjectionMatrixAsPerspective(45.0f, static_cast<double>(width) / static_cast<double>(height), 1.0, 10000.0);
}

void ViewManager::setupTopView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height)
{
    if (!viewer || !rootNode) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // 计算宽高比
    double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    // 获取场景的边界框，用于计算合适的视图范围
    osg::ComputeBoundsVisitor boundsVisitor;
    rootNode->accept(boundsVisitor);
    osg::BoundingBox bb = boundsVisitor.getBoundingBox();
    
    osg::Vec3 eye, center, up;
    double viewDistance = 5.0; // 默认距离
    double modelSize = 1.0;    // 默认模型大小
    
    if (bb.valid()) {
        // 使用场景的中心作为视图中心
        center = bb.center();
        
        // 计算模型大小
        modelSize = bb.radius() > 0 ? bb.radius() : 1.0;
        viewDistance = modelSize * 2.0; // 根据模型大小调整视距
        
        // 俯视图设置 - 从Z轴正方向看
        eye.set(center.x(), center.y(), center.z() + viewDistance);
        up.set(0.0f, 1.0f, 0.0f);  // Y轴向上
    } else {
        // 如果没有有效场景，使用默认设置
        eye.set(0.0f, 0.0f, 5.0f);
        center.set(0.0f, 0.0f, 0.0f);
        up.set(0.0f, 1.0f, 0.0f);
    }
    
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 更新视图参数
    setViewParameters(eye, center, up);
    
    // 设置透视投影（不再使用正交投影）
    camera->setProjectionMatrixAsPerspective(45.0f, static_cast<double>(width) / static_cast<double>(height), 1.0, 10000.0);
}

void ViewManager::setupMainView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height)
{
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // 计算宽高比
    double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    // 主视图设置
    osg::Vec3 eye(0.0f, -5.0f, 0.0f);   // 从前方偏下位置观察
    osg::Vec3 center(0.0f, 0.0f, 0.0f);  // 看向原点
    osg::Vec3 up(0.0f, 0.0f, 1.0f);      // Z轴向上
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 更新视图参数
    setViewParameters(eye, center, up);
    
    // 设置透视投影
    camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 1.0, 10000.0);
    
    // 确保深度测试正确配置
    osg::StateSet* stateset = camera->getOrCreateStateSet();
    stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
}
