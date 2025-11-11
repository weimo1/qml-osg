#pragma once
#include "osg/Transform"
#include <osg/Texture2D>
#include <osg/Uniform>

// 基础天空云类
class SkyCloud : public osg::Transform
{
public:
    SkyCloud();
    SkyCloud(osg::Camera* camera);

    SkyCloud(const SkyCloud& copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY) : osg::Transform(copy, copyop) {}

    void initUniforms();
    
    
    // 设置云参数
    void setCloudDensity(float density);

    META_Node(osg, SkyCloud);

    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
    virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;

protected:
    virtual ~SkyCloud() {}

private:

    osg::ref_ptr<osg::Uniform> _rayleigh;
    osg::ref_ptr<osg::Uniform> _turbidity;
    osg::ref_ptr<osg::Uniform> _mieCoefficient;
    osg::ref_ptr<osg::Uniform> _mieDirectionalG;
    osg::ref_ptr<osg::Uniform> _up;
    osg::ref_ptr<osg::Uniform> _sunPosition;

    // 云朵相关uniforms
    osg::ref_ptr<osg::Uniform> _cloudDensity;
};