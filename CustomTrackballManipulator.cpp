#include "CustomTrackballManipulator.h"
#include <osg/Notify>
#include <cmath>

CustomTrackballManipulator::CustomTrackballManipulator()
    : m_dragging(false)
{
    setVerticalAxisFixed(true);
    setAllowThrow(false);
}

bool CustomTrackballManipulator::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    switch (ea.getEventType())
    {
        case osgGA::GUIEventAdapter::PUSH:
        {
            m_dragging = true;
            break;
        }
        case osgGA::GUIEventAdapter::RELEASE:
        {
            m_dragging = false;
            break;
        }
        default:
            break;
    }
    
    // 调用父类的处理函数
    return osgGA::TrackballManipulator::handle(ea, us);
}

void CustomTrackballManipulator::setMouseWheelZoomFactor(double factor)
{
    // 不再调用父类方法，因为父类没有这个方法
    // 可以在这里添加自定义实现或者留空
}

bool CustomTrackballManipulator::performMovementRightMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
    // 右键：平移操作
    osg::Vec3d eye, center, up;
    getTransformation(eye, center, up);
    
    // 计算视口大小和模型距离来调整平移速度
    osg::Vec3d viewDirection = center - eye;
    double distance = viewDirection.length();
    
    // 根据距离调整平移速度，使其与旋转和缩放保持一致
    // 距离越大，平移速度越快，以确保在大场景中也有合适的移动速度
    double translateScale = distance * 0.05; // 5% of the distance，可根据需要调整
    
    // 计算平移向量
    osg::Vec3d translation;
    translation = (up * dy * translateScale) + (getSideVector(up, viewDirection) * dx * translateScale);
    
    // 应用平移
    eye += translation;
    center += translation;
    
    setTransformation(eye, center, up);
    
    return true;
}

bool CustomTrackballManipulator::performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
    // 左键：旋转操作（使用父类的实现）
    return osgGA::TrackballManipulator::performMovementLeftMouseButton(eventTimeDelta, dx, dy);
}

bool CustomTrackballManipulator::performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
    // 中键：缩放操作（使用父类的实现）
    return osgGA::TrackballManipulator::performMovementMiddleMouseButton(eventTimeDelta, dx, dy);
}

osg::Vec3d CustomTrackballManipulator::getSideVector(const osg::Vec3d& up, const osg::Vec3d& forward) const
{
    osg::Vec3d side = forward ^ up;
    side.normalize();
    return side;
}