#pragma once
#include "osg/Transform"
#include <osg/Texture2D>
#include <osg/Uniform>

// 体积云天空盒类
class VolumeCloudSky : public osg::Transform
{
public:
    VolumeCloudSky();
    VolumeCloudSky(osg::Camera* camera);

    VolumeCloudSky(const VolumeCloudSky& copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY) : osg::Transform(copy, copyop) {}

    void initUniforms();
    
    // 设置太阳角度
    void setSunZenithAngle(float angle);
    void setSunAzimuthAngle(float angle);
    
    // 设置云参数
    void setCloudDensity(float density);
    
    // 新增的云层控制参数setter方法
    void setDensityThreshold(float threshold);
    void setContrast(float contrast);
    void setDensityFactor(float factor);
    void setStepSize(float size);
    void setMaxSteps(int steps);

    META_Node(osg, VolumeCloudSky);

    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
    virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;

protected:
    virtual ~VolumeCloudSky() {}

private:
    // uniforms
    osg::ref_ptr<osg::Uniform> _sunZenithAngle;
    osg::ref_ptr<osg::Uniform> _sunAzimuthAngle;
    osg::ref_ptr<osg::Uniform> _cloudDensity;
    osg::ref_ptr<osg::Uniform> _rayleigh;
    osg::ref_ptr<osg::Uniform> _turbidity;
    osg::ref_ptr<osg::Uniform> _mieCoefficient;
    osg::ref_ptr<osg::Uniform> _mieDirectionalG;
    osg::ref_ptr<osg::Uniform> _up;
    
    // 新增的云层控制参数uniforms
    osg::ref_ptr<osg::Uniform> _densityThreshold;
    osg::ref_ptr<osg::Uniform> _contrast;
    osg::ref_ptr<osg::Uniform> _densityFactor;
    osg::ref_ptr<osg::Uniform> _stepSize;
    osg::ref_ptr<osg::Uniform> _maxSteps;
};