#include "viewmanager.h"
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osg/Camera>
#include <osgGA/TrackballManipulator>
#include <QDebug>

ViewManager::ViewManager()
    : m_eye(0.0, -10.0, 5.0)  // 调整初始相机位置，提高Z坐标避免贴近地面
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
        // 主视图使用稍微偏下的角度观察，提高相机高度避免贴近地面
        eye.set(center.x(), center.y() - viewDistance, center.z() + viewDistance * 0.5); // 增加Z坐标偏移
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
        manipulator->setVerticalAxisFixed(false);  // 修改这里，不固定垂直轴
        
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
    
    // 设置透视投影，调整裁剪面以适应大气渲染场景
    camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 0.1, 500000.0);
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
    
    // 设置透视投影，调整裁剪面以适应大气渲染场景
    camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 0.1, 500000.0);
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
    
    // 设置透视投影，调整裁剪面以适应大气渲染场景
    camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 0.1, 500000.0);
}

void ViewManager::setupMainView(osgViewer::Viewer* viewer, osg::Group* rootNode, int width, int height)
{
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // 计算宽高比
    double aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    // 主视图设置，调整相机位置避免贴近地面
    osg::Vec3 eye(0.0f, -15.0f, 8.0f);   // 进一步调整相机位置，提高Z坐标避免贴近地面
    osg::Vec3 center(0.0f, 0.0f, 0.0f);  // 看向原点
    osg::Vec3 up(0.0f, 0.0f, 1.0f);      // Z轴向上
    camera->setViewMatrixAsLookAt(eye, center, up);
    
    // 更新视图参数
    setViewParameters(eye, center, up);
    
    // 设置透视投影，调整裁剪面以适应大气渲染场景
    camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 0.1, 500000.0);
    
    // 确保深度测试正确配置
    osg::StateSet* stateset = camera->getOrCreateStateSet();
    stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
}

// 实现重置到主视角的功能
void ViewManager::resetToHomeView(osgViewer::Viewer* viewer)
{
    if (viewer && viewer->getCameraManipulator()) {
        viewer->getCameraManipulator()->home(0.0);
        qDebug() << "View reset to home position";
    }
}

// 实现适应视图功能
void ViewManager::fitToView(osgViewer::Viewer* viewer, osg::Group* rootNode)
{
    if (!viewer || !rootNode) return;
    
    // 计算整个场景的包围盒
    osg::ComputeBoundsVisitor boundsVisitor;
    rootNode->accept(boundsVisitor);
    osg::BoundingBox bb = boundsVisitor.getBoundingBox();
    
    // 如果包围盒有效，则调整相机位置以适应整个场景
    if (bb.valid())
    {
        // 获取当前相机操作器
        osgGA::CameraManipulator* manipulator = viewer->getCameraManipulator();
        if (manipulator)
        {
            // 设置新的home位置，使整个场景完整显示在视图中
            osg::Vec3 center = bb.center();
            osg::BoundingSphere bs(bb);
            float radius = bs.radius();
            
            // 如果半径为0（可能只有一个点），设置一个默认值
            if (radius == 0.0f) {
                radius = 1.0f;
            }
            
            // 设置合适的相机距离，提高相机高度避免贴近地面
            osg::Vec3 eye = center + osg::Vec3(0, -radius * 2.5f, radius * 1.5f); // 增加Z坐标偏移
            osg::Vec3 up(0.0f, 0.0f, 1.0f);
            
            manipulator->setHomePosition(eye, center, up);
            manipulator->home(0.0);
            
            qDebug() << "View fitted to scene bounds";
        }
    }
    else
    {
        // 如果没有有效的包围盒，重置到默认视角
        resetToHomeView(viewer);
    }
}

// 添加获取相机位置的方法
osg::Vec3d ViewManager::getCameraPosition(osgViewer::Viewer* viewer) const
{
    if (!viewer) return osg::Vec3d(0, 0, 0);
    
    // 从相机操纵器获取当前视角位置
    osgGA::CameraManipulator* manipulator = viewer->getCameraManipulator();
    if (manipulator) {
        osg::Matrixd matrix = manipulator->getMatrix();
        osg::Vec3d eye, center, up;
        matrix.getLookAt(eye, center, up);
        return eye;
    }
    
    return osg::Vec3d(0, 0, 0);
}