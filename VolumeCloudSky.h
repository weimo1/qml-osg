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
    
    // 光照参数setter方法
    void setLightColor(const osg::Vec3& color);
    void setSpecularStrength(float strength);
    void setShininess(float shininess);
    
    // 新增的云层控制参数setter方法
    void setDensityThreshold(float threshold);
    void setContrast(float contrast);
    void setDensityFactor(float factor);
    void setStepSize(float size);
    void setMaxSteps(int steps);
    
    // 新增：与SkyNode兼容的参数设置方法
    void setAtmosphereParameters(float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG, float sunZenithAngle, float sunAzimuthAngle);
    void setCloudParameters(float density, float densityThreshold, float contrast, float densityFactor, float stepSize, int maxSteps);
    
    // 新增：直接从SkyNode参数更新VolumeCloudSky
    void updateFromSkyNodeParameters(float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG, 
                                   float sunZenithAngle, float sunAzimuthAngle, float cloudDensity);
                                   
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