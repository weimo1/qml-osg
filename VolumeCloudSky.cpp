#include "VolumeCloudSky.h"
#include "osg/Depth"
#include <osgUtil/CullVisitor>
#include "osgDB/ReadFile"
#include <QDir>
#include "osg/Texture2D"
#include <chrono>
#include <osg/Geometry>
#include <osg/Geode>

class VolumeCloudCB : public osg::StateSet::Callback
{
private:
    osg::Camera* pCamera_ = nullptr;
    std::chrono::high_resolution_clock::time_point _startTime;

public:
    explicit VolumeCloudCB(osg::Camera* camera) : pCamera_(camera), _startTime(std::chrono::high_resolution_clock::now())
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

            // 视图矩阵逆矩阵计算
            osg::Matrixf viewMat = pCamera_->getViewMatrix();
            osg::Matrixf viewInverse = osg::Matrixf::inverse(viewMat);
            ss->getOrCreateUniform("viewInverse", osg::Uniform::FLOAT_MAT4)->set(viewInverse);
            
            // 更新时间uniform
            auto now = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(now - _startTime).count();
            ss->getOrCreateUniform("iTime", osg::Uniform::FLOAT)->set(elapsed);
            
            // 计算太阳方向（基于天顶角和方位角）
            float sunZenithAngle = 75.0f * 3.14159f / 180.0f;  // 75度天顶角
            float sunAzimuthAngle = 40.0f * 3.14159f / 180.0f; // 40度方位角
            osg::Vec3 sunDirection(
                sin(sunZenithAngle) * cos(sunAzimuthAngle),
                sin(sunZenithAngle) * sin(sunAzimuthAngle),
                cos(sunZenithAngle)
            );
            sunDirection.normalize();
            ss->getOrCreateUniform("sunDirection", osg::Uniform::FLOAT_VEC3)->set(sunDirection);
        }
    }
};

VolumeCloudSky::VolumeCloudSky()
{
}

VolumeCloudSky::VolumeCloudSky(osg::Camera* camera)
{
    // 使用绝对参考框架，使天空盒不受场景变换影响
    setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    setCullingActive(false);

    osg::StateSet* ss = getOrCreateStateSet();
    // 启用混合以支持透明度
    ss->setMode(GL_BLEND, osg::StateAttribute::ON);
    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    // 深度测试设置
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0f, 1.0f));
    // 确保天空盒在最远深度渲染
    ss->setRenderBinDetails(1000, "RenderBin");
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    // 初始化uniforms
    initUniforms();
    
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;
    std::string resourcePath = QDir::currentPath().toStdString() + "/../../shader/";
    osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX, resourcePath + "CloudSky.vert");
    pV->setName("C.vert");
    osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, resourcePath + "CloudSky.frag");
    pF->setName("C.frag");
    program->addShader(pV);
    program->addShader(pF);
    ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);

    // 添加uniforms
    ss->addUniform(_sunZenithAngle.get());
    ss->addUniform(_sunAzimuthAngle.get());
    ss->addUniform(_cloudDensity.get());
    ss->addUniform(_rayleigh.get());
    ss->addUniform(_turbidity.get());
    ss->addUniform(_mieCoefficient.get());
    ss->addUniform(_mieDirectionalG.get());
    ss->addUniform(_up.get());
    ss->addUniform(new osg::Uniform("iTime", 0.0f));

    // 加载2D噪声贴图 - 定义云的分布区域
    osg::ref_ptr<osg::Image> cloudMapImage = osgDB::readImageFile("E:/a.png");
    if (cloudMapImage.valid()) {
        osg::ref_ptr<osg::Texture2D> cloudMapTexture = new osg::Texture2D();
        cloudMapTexture->setImage(cloudMapImage.get());
        cloudMapTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        cloudMapTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        cloudMapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        cloudMapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(0, cloudMapTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("cloudMap", 0));
    } else {
        // 如果无法加载噪声贴图，创建一个默认的白色纹理
        osg::ref_ptr<osg::Image> defaultImage = new osg::Image();
        unsigned char* data = new unsigned char[4];
        data[0] = data[1] = data[2] = 255; data[3] = 255;
        defaultImage->setImage(1, 1, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
        osg::ref_ptr<osg::Texture2D> defaultTexture = new osg::Texture2D();
        defaultTexture->setImage(defaultImage.get());
        defaultTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        defaultTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        defaultTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        defaultTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(0, defaultTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("cloudMap", 0));
    }

    // 加载蓝噪声贴图
    osg::ref_ptr<osg::Image> blueNoiseImage = osgDB::readImageFile("E:/b.png");
    if (blueNoiseImage.valid()) {
        osg::ref_ptr<osg::Texture2D> blueNoiseTexture = new osg::Texture2D();
        blueNoiseTexture->setImage(blueNoiseImage.get());
        blueNoiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(1, blueNoiseTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("blueNoise", 1));
    } else {
        // 如果无法加载蓝噪声贴图，使用默认的噪声纹理
        osg::ref_ptr<osg::Image> defaultBlueNoiseImage = new osg::Image();
        unsigned char* data = new unsigned char[4];
        data[0] = data[1] = data[2] = 128; data[3] = 255;
        defaultBlueNoiseImage->setImage(1, 1, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
        osg::ref_ptr<osg::Texture2D> defaultBlueNoiseTexture = new osg::Texture2D();
        defaultBlueNoiseTexture->setImage(defaultBlueNoiseImage.get());
        defaultBlueNoiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        defaultBlueNoiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        defaultBlueNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        defaultBlueNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(1, defaultBlueNoiseTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("blueNoise", 1));
    }

    // 不在这里创建几何体，而是在demoshader.cpp中创建球体并添加为子节点

    VolumeCloudCB* pCB = new VolumeCloudCB(camera);
    ss->setUpdateCallback(pCB);
}

    // 不再需要createCube函数

void VolumeCloudSky::initUniforms()
{
    _sunZenithAngle = new osg::Uniform("sunZenithAngle", 75.0f * 3.14159f / 180.0f);  // 调整太阳天顶角为75度（根据用户偏好）
    _sunAzimuthAngle = new osg::Uniform("sunAzimuthAngle", 40.0f * 3.14159f / 180.0f);  // 40度方位角（根据用户偏好）
    _cloudDensity = new osg::Uniform("cloudDensity", 100.0f);  // 增加云密度使效果更明显
    
    // 添加大气散射所需的uniform变量
    osg::ref_ptr<osg::Uniform> rayleigh = new osg::Uniform("rayleigh", 1.0f);
    osg::ref_ptr<osg::Uniform> turbidity = new osg::Uniform("turbidity", 2.0f);
    osg::ref_ptr<osg::Uniform> mieCoefficient = new osg::Uniform("mieCoefficient", 0.005f);
    osg::ref_ptr<osg::Uniform> mieDirectionalG = new osg::Uniform("mieDirectionalG", 0.8f);
    osg::ref_ptr<osg::Uniform> up = new osg::Uniform("up", osg::Vec3(0.0f, 0.0f, 1.0f));
    
    // 保存uniform变量引用以便后续更新
    _rayleigh = rayleigh;
    _turbidity = turbidity;
    _mieCoefficient = mieCoefficient;
    _mieDirectionalG = mieDirectionalG;
    _up = up;
}

bool VolumeCloudSky::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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

bool VolumeCloudSky::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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

// 设置太阳天顶角度
void VolumeCloudSky::setSunZenithAngle(float angle)
{
    if (_sunZenithAngle.valid()) {
        _sunZenithAngle->set(angle);
    }
}

// 设置太阳方位角度
void VolumeCloudSky::setSunAzimuthAngle(float angle)
{
    if (_sunAzimuthAngle.valid()) {
        _sunAzimuthAngle->set(angle);
    }
}

// 设置云密度
void VolumeCloudSky::setCloudDensity(float density)
{
    if (_cloudDensity.valid()) {
        _cloudDensity->set(density);
    }
}