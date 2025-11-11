#include "SkyCloud.h"
#include "osg/Depth"
#include <osgUtil/CullVisitor>
#include "osgDB/ReadFile"
#include <QDir>
#include "osg/Texture2D"
#include <chrono>
#include <osg/Geometry>
#include <osg/Geode>


class SkyCloudCB : public osg::StateSet::Callback
{
private:
    osg::Camera* pCamera_ = nullptr;
    std::chrono::high_resolution_clock::time_point _startTime;

public:
    explicit SkyCloudCB(osg::Camera* camera) : pCamera_(camera), _startTime(std::chrono::high_resolution_clock::now())
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
        }
    }
};


SkyCloud::SkyCloud(osg::Camera* camera)
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
    ss->setRenderBinDetails(-2, "RenderBin");
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    // 初始化uniforms
    initUniforms();
    
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;
    std::string resourcePath = QDir::currentPath().toStdString() + "/../../shader/";
    osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX, resourcePath + "SkyAtmosphere.vert");
    pV->setName("SkyAtmosphere.vert");
    osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, resourcePath + "SkyAtmosphere.frag");
    pF->setName("SkyAtmosphere.frag");
    program->addShader(pV);
    program->addShader(pF);
    ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);

    // 添加uniforms
    ss->addUniform(_cloudDensity.get());
    ss->addUniform(_rayleigh.get());
    ss->addUniform(_turbidity.get());
    ss->addUniform(_mieCoefficient.get());
    ss->addUniform(_mieDirectionalG.get());
    ss->addUniform(_up.get());
    ss->addUniform(_sunPosition.get());
    ss->addUniform(new osg::Uniform("iTime", 0.0f));

    // 加载2D噪声贴图 - 定义云的分布区域
    osg::ref_ptr<osg::Image> cloudMapImage = osgDB::readImageFile("E:/C.png");
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

    // 加载细节噪声贴图
    osg::ref_ptr<osg::Image> detailMapImage = osgDB::readImageFile("E:/PN.png");
    if (detailMapImage.valid()) {
        osg::ref_ptr<osg::Texture2D> detailMapTexture = new osg::Texture2D();
        detailMapTexture->setImage(detailMapImage.get());
        detailMapTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        detailMapTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        detailMapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        detailMapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(1, detailMapTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("detailMap", 1));
    } else {
        // 如果无法加载细节贴图，创建一个默认的灰色纹理
        osg::ref_ptr<osg::Image> defaultDetailImage = new osg::Image();
        unsigned char* data = new unsigned char[4];
        data[0] = data[1] = data[2] = 128; data[3] = 255;
        defaultDetailImage->setImage(1, 1, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
        osg::ref_ptr<osg::Texture2D> defaultDetailTexture = new osg::Texture2D();
        defaultDetailTexture->setImage(defaultDetailImage.get());
        defaultDetailTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        defaultDetailTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        defaultDetailTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        defaultDetailTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(1, defaultDetailTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("detailMap", 1));
    }

    // 加载低频噪声贴图（覆盖遮罩）
    osg::ref_ptr<osg::Image> coverageMapImage = osgDB::readImageFile("E:/s.png");
    if (coverageMapImage.valid()) {
        osg::ref_ptr<osg::Texture2D> coverageMapTexture = new osg::Texture2D();
        coverageMapTexture->setImage(coverageMapImage.get());
        coverageMapTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        coverageMapTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        coverageMapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        coverageMapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(2, coverageMapTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("coverageMap", 2));
    } else {
        // 如果无法加载低频噪声贴图，创建一个默认的白色纹理
        osg::ref_ptr<osg::Image> defaultCoverageImage = new osg::Image();
        unsigned char* data = new unsigned char[4];
        data[0] = data[1] = data[2] = 255; data[3] = 255;
        defaultCoverageImage->setImage(1, 1, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
        osg::ref_ptr<osg::Texture2D> defaultCoverageTexture = new osg::Texture2D();
        defaultCoverageTexture->setImage(defaultCoverageImage.get());
        defaultCoverageTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        defaultCoverageTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        defaultCoverageTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        defaultCoverageTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(2, defaultCoverageTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("coverageMap", 2));
    }

    // 不在这里创建几何体，而是在demoshader.cpp中创建球体并添加为子节点

    SkyCloudCB* pCB = new SkyCloudCB(camera);
    ss->setUpdateCallback(pCB);
}

SkyCloud::SkyCloud() : osg::Transform()
{
}

void SkyCloud::initUniforms()
{

    _cloudDensity = new osg::Uniform("cloudDensity", 100.0f);  // 云密度
    
    // 添加大气散射所需的uniform变量
    _rayleigh  = new osg::Uniform("rayleigh", 3.0f);
	_mieCoefficient  = new osg::Uniform("mieCoefficient", 0.005f);
	_mieDirectionalG = new osg::Uniform("mieDirectionalG", 0.7f);
	_sunPosition     = new osg::Uniform("sunPosition", osg::Vec3(0.0f, -0.9994f, 0.035f));
	_up = new osg::Uniform("up", osg::Vec3(0.0f, 0.0f, 1.0f));	
    
}

bool SkyCloud::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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

bool SkyCloud::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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


// 设置云密度
void SkyCloud::setCloudDensity(float density)
{
    if (_cloudDensity.valid()) {
        _cloudDensity->set(density);
    }
}