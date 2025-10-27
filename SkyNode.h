
#pragma once
#include "osg/Transform"
#include <osg/TextureCubeMap>


//构件对象
class  SkyBoxThree : public osg::Transform
{
public:
    SkyBoxThree();
    SkyBoxThree(osg::Camera * pCamera);

    SkyBoxThree(const SkyBoxThree& copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY) : osg::Transform(copy, copyop) {}

    void initUniforms();
    void setSunZenithAngle(float angle);  // 新增：设置太阳天顶角度的方法
    void setSunAzimuthAngle(float angle);  // 新增：设置太阳方位角度的方法

    META_Node(osg, SkyBoxThree);

 

    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
    virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
protected:
    virtual ~SkyBoxThree() {}
private:
    // uniforms
    osg::ref_ptr<osg::Uniform> _turbidity;
    osg::ref_ptr<osg::Uniform> _rayleigh;
    osg::ref_ptr<osg::Uniform> _mieCoefficient;
    osg::ref_ptr<osg::Uniform> _mieDirectionalG;
    osg::ref_ptr<osg::Uniform> _sunPosition;
    osg::ref_ptr<osg::Uniform> _up;
    osg::ref_ptr<osg::Uniform> _sunZenithAngle;  // 新增：太阳天顶角度uniform
    osg::ref_ptr<osg::Uniform> _sunAzimuthAngle;  // 新增：太阳方位角度uniform

};