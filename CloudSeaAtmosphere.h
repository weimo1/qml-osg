#pragma once

#include <osg/Transform>
#include <osg/Uniform>
#include <osg/Camera>

/**
 * 云海大气效果
 * 实现云海与天空的融合渲染效果
 */
class CloudSeaAtmosphere : public osg::Transform
{
public:
    CloudSeaAtmosphere();
    CloudSeaAtmosphere(osg::Camera* pCamera);

    CloudSeaAtmosphere(const CloudSeaAtmosphere& copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY) 
        : osg::Transform(copy, copyop) {}

    void initUniforms();
    void setSunZenithAngle(float angle);
    void setSunAzimuthAngle(float angle);
    void setCloudDensity(float density);
    void setCloudHeight(float height);
    void setAtmosphereColor(osg::Vec3 color);

    META_Node(osg, CloudSeaAtmosphere);

    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
    virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;

protected:
    virtual ~CloudSeaAtmosphere() {}

private:
    osg::ref_ptr<osg::Uniform> _turbidity;
    osg::ref_ptr<osg::Uniform> _rayleigh;
    osg::ref_ptr<osg::Uniform> _mieCoefficient;
    osg::ref_ptr<osg::Uniform> _mieDirectionalG;
    osg::ref_ptr<osg::Uniform> _sunPosition;
    osg::ref_ptr<osg::Uniform> _up;
    osg::ref_ptr<osg::Uniform> _sunZenithAngle;
    osg::ref_ptr<osg::Uniform> _sunAzimuthAngle;
    osg::ref_ptr<osg::Uniform> _cloudDensity;
    osg::ref_ptr<osg::Uniform> _cloudHeight;
    osg::ref_ptr<osg::Uniform> _atmosphereColor;
};
