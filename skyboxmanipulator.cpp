#include "skyboxmanipulator.h"
#include <osgGA/GUIEventAdapter>
#include <osgGA/GUIActionAdapter>
#include <osg/Notify>
#include <cmath>

SkyBoxManipulator::SkyBoxManipulator()
    : _center(0.0, 0.0, 0.0)
    , _rotation()
    , _distance(5.0)  // 与ViewManager中的默认距离一致
    , _mousePressed(false)
    , _lastX(0.0f)
    , _lastY(0.0f)
{
}

SkyBoxManipulator::~SkyBoxManipulator()
{
}

void SkyBoxManipulator::setByMatrix(const osg::Matrixd& matrix)
{
    // 从矩阵中提取变换信息
    _center = matrix.getTrans();
    
    // 提取旋转信息
    _rotation = matrix.getRotate();
    
    // 计算距离 - 从矩阵的平移部分计算
    osg::Vec3d trans = matrix.getTrans();
    _distance = trans.length(); // 计算距离
    
    // 确保距离不为0
    if (_distance < 1.0) _distance = 5.0;
}

void SkyBoxManipulator::setByInverseMatrix(const osg::Matrixd& matrix)
{
    setByMatrix(osg::Matrixd::inverse(matrix));
}

osg::Matrixd SkyBoxManipulator::getMatrix() const
{
    // 使用ViewManager兼容的方式设置相机
    // 相机位置在Y轴负方向，看向原点，Z轴向上（与ViewManager的FrontView一致）
    osg::Vec3d eye(0.0, _distance, 0.0);   // 相机位置（Y轴负方向）
    osg::Vec3d center(0.0, 0.0, 0.0);       // 目标点（天空盒中心）
    osg::Vec3d up(0.0, 0.0, 1.0);           // 上方向向量（Z轴向上）
    
    // 创建lookAt矩阵
    osg::Matrixd viewMatrix;
    viewMatrix.makeLookAt(eye, center, up);
    
    // 应用旋转
    osg::Matrixd rotationMatrix;
    rotationMatrix.makeRotate(_rotation);
    
    // 返回组合矩阵
    return viewMatrix * rotationMatrix;
}

osg::Matrixd SkyBoxManipulator::getInverseMatrix() const
{
    return osg::Matrixd::inverse(getMatrix());
}

bool SkyBoxManipulator::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::PUSH:
        if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
        {
            _mousePressed = true;
            _lastX = ea.getX();
            _lastY = ea.getY();
            _lastRotation = _rotation;
            return true;
        }
        break;

    case osgGA::GUIEventAdapter::RELEASE:
        if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
        {
            _mousePressed = false;
            return true;
        }
        break;

    case osgGA::GUIEventAdapter::DRAG:
        if (_mousePressed)
        {
            float deltaX = ea.getX() - _lastX;
            float deltaY = ea.getY() - _lastY;

            // 根据鼠标移动计算旋转角度 - 降低灵敏度
            float sensitivity = 0.1f;  // 从0.5降低到0.1，使操作更平滑
            osg::Quat deltaRotation;
            
            // 绕Y轴旋转（左右拖动）- 注意符号以匹配期望的拖动方向
            deltaRotation.makeRotate(osg::DegreesToRadians(-deltaX * sensitivity), 0.0, 0.0, 1.0);
            _rotation = deltaRotation * _lastRotation;
            
            // 绕X轴旋转（上下拖动）- 注意符号以匹配期望的拖动方向
            osg::Quat deltaRotationX;
            deltaRotationX.makeRotate(osg::DegreesToRadians(-deltaY * sensitivity), 1.0, 0.0, 0.0);
            _rotation = _rotation * deltaRotationX;

            us.requestRedraw();
            return true;
        }
        break;
        


    case osgGA::GUIEventAdapter::FRAME:
        // 每帧更新
        return false;
    }

    return false;
}

void SkyBoxManipulator::init(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    home(ea, us);
}

void SkyBoxManipulator::home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    _center.set(0.0, 0.0, 0.0);
    _rotation = osg::Quat(); // 使用默认构造函数初始化为单位四元数
    _distance = 5.0;  // 设置默认距离为5
    us.requestRedraw();
}

void SkyBoxManipulator::home(double currentTime)
{
    _center.set(0.0, 0.0, 0.0);
    _rotation = osg::Quat(); // 使用默认构造函数初始化为单位四元数
    _distance = 5.0;  // 设置默认距离为5
}