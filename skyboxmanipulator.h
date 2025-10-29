#ifndef SKYBOXMANIPULATOR_H
#define SKYBOXMANIPULATOR_H

#include <osgGA/CameraManipulator>
#include <osg/Quat>
#include <osg/Matrixd>
#include <osg/Vec3d>

class SkyBoxManipulator : public osgGA::CameraManipulator
{
public:
    SkyBoxManipulator();
    virtual ~SkyBoxManipulator();

    // 继承自CameraManipulator的纯虚函数实现
    virtual void setByMatrix(const osg::Matrixd& matrix) override;
    virtual void setByInverseMatrix(const osg::Matrixd& matrix) override;
    virtual osg::Matrixd getMatrix() const override;
    virtual osg::Matrixd getInverseMatrix() const override;

    // 事件处理
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;

    // 初始化操作器
    virtual void init(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
    
    // 设置Home位置
    virtual void home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
    virtual void home(double currentTime) override;

    // 获取和设置中心点（不使用override，因为基类可能没有这些方法）
    virtual void setCenter(const osg::Vec3d& center) { _center = center; }
    virtual const osg::Vec3d& getCenter() const { return _center; }

    // 获取和设置旋转
    void setRotation(const osg::Quat& rotation) { _rotation = rotation; }
    const osg::Quat& getRotation() const { return _rotation; }

    // 获取和设置距离
    void setDistance(double distance) { _distance = distance; }
    double getDistance() const { return _distance; }

protected:
    osg::ref_ptr<osgGA::CameraManipulator> _delegate;
    osg::Vec3d _center;
    osg::Quat _rotation;
    double _distance;
    bool _mousePressed;
    float _lastX, _lastY;
    osg::Quat _lastRotation;
};

#endif // SKYBOXMANIPULATOR_H