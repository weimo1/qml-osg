#include "CloudSeaAtmosphere.h"
#include <osgUtil/CullVisitor>
#include <osgDB/ReadFile>
#include <QDir>
#include <osg/Depth>

class CloudSeaCB : public osg::StateSet::Callback
{
public:
    explicit CloudSeaCB(osg::Camera* camera) : pCamera_(camera)
    {
    }

    virtual void operator()(osg::StateSet* ss, osg::NodeVisitor* nv)
    {
        process(ss);
    }

    void process(osg::StateSet* ss)
    {
        if (pCamera_)
        {
            osg::Vec3f eye, center, up;
            pCamera_->getViewMatrixAsLookAt(eye, center, up);
            ss->getOrCreateUniform("cameraPosition", osg::Uniform::FLOAT_VEC3)->set(eye);

            osg::Matrixf viewMat = pCamera_->getViewMatrix();
            osg::Matrixf viewInverse = osg::Matrixf::inverse(viewMat);
            ss->getOrCreateUniform("viewInverse", osg::Uniform::FLOAT_MAT4)->set(viewInverse);
        }
    }

private:
    osg::Camera* pCamera_ = nullptr;
};

CloudSeaAtmosphere::CloudSeaAtmosphere()
{
}

CloudSeaAtmosphere::CloudSeaAtmosphere(osg::Camera* pCamera)
{
    setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    setCullingActive(false);

    osg::StateSet* ss = getOrCreateStateSet();
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0f, 1.0f));
    ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);

    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    ss->setRenderBinDetails(0, "RenderBin");

    initUniforms();
    
    osg::ref_ptr<osg::Program> program = new osg::Program;
    std::string resourcePath = QDir::currentPath().toStdString() + "/../../shader/";
    
    // 使用云海着色器
    osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX, resourcePath + "CloudSea.vert");
    if (!pV) {
        pV = osgDB::readShaderFile(osg::Shader::VERTEX, resourcePath + "X1.vert");
    }
    pV->setName("CloudSea.vert");
    
    osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, resourcePath + "CloudSea.frag");
    if (!pF) {
        pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, resourcePath + "X1.frag");
    }
    pF->setName("CloudSea.frag");
    
    program->addShader(pV);
    program->addShader(pF);
    ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);

    ss->addUniform(_turbidity.get());
    ss->addUniform(_rayleigh.get());
    ss->addUniform(_mieCoefficient.get());
    ss->addUniform(_mieDirectionalG.get());
    ss->addUniform(_sunPosition.get());
    ss->addUniform(_up.get());
    ss->addUniform(_sunZenithAngle.get());
    ss->addUniform(_sunAzimuthAngle.get());
    ss->addUniform(_cloudDensity.get());
    ss->addUniform(_cloudHeight.get());
    ss->addUniform(_atmosphereColor.get());

    CloudSeaCB* pCB = new CloudSeaCB(pCamera);
    ss->setUpdateCallback(pCB);
}

void CloudSeaAtmosphere::initUniforms()
{
    _turbidity = new osg::Uniform("turbidity", 2.0f);
    _rayleigh = new osg::Uniform("rayleigh", 1.0f);
    _mieCoefficient = new osg::Uniform("mieCoefficient", 0.005f);
    _mieDirectionalG = new osg::Uniform("mieDirectionalG", 0.8f);
    _sunPosition = new osg::Uniform("sunPosition", osg::Vec3(0.0f, 0.7f, 0.8f));
    _up = new osg::Uniform("up", osg::Vec3(0.0f, 0.0f, 1.0f));
    _sunZenithAngle = new osg::Uniform("sunZenithAngle", 80.0f * 3.14159f / 180.0f);
    _sunAzimuthAngle = new osg::Uniform("sunAzimuthAngle", 270.0f * 3.14159f / 180.0f);
    
    // 云海特有的参数
    _cloudDensity = new osg::Uniform("cloudDensity", 0.8f);
    _cloudHeight = new osg::Uniform("cloudHeight", 1000.0f);
    _atmosphereColor = new osg::Uniform("atmosphereColor", osg::Vec3(0.5f, 0.7f, 1.0f));
}

bool CloudSeaAtmosphere::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
        matrix.preMult(osg::Matrix::translate(cv->getEyeLocal()));
        return true;
    }
    else
        return osg::Transform::computeLocalToWorldMatrix(matrix, nv);
}

bool CloudSeaAtmosphere::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
        matrix.postMult(osg::Matrix::translate(-cv->getEyeLocal()));
        return true;
    }
    else
        return osg::Transform::computeWorldToLocalMatrix(matrix, nv);
}

void CloudSeaAtmosphere::setSunZenithAngle(float angle)
{
    if (_sunZenithAngle.valid()) {
        _sunZenithAngle->set(angle);
    }
}

void CloudSeaAtmosphere::setSunAzimuthAngle(float angle)
{
    if (_sunAzimuthAngle.valid()) {
        _sunAzimuthAngle->set(angle);
    }
}

void CloudSeaAtmosphere::setCloudDensity(float density)
{
    if (_cloudDensity.valid()) {
        _cloudDensity->set(density);
    }
}

void CloudSeaAtmosphere::setCloudHeight(float height)
{
    if (_cloudHeight.valid()) {
        _cloudHeight->set(height);
    }
}

void CloudSeaAtmosphere::setAtmosphereColor(osg::Vec3 color)
{
    if (_atmosphereColor.valid()) {
        _atmosphereColor->set(color);
    }
}
