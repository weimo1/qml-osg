// TH3DGraphLibTestView.cpp : CTHGW3DGraphView 类的实现

#include "osg/Depth"
#include <osgUtil/CullVisitor>

#include "skyNode.h"
#include "osgDB/ReadFile"
#include<Qdir>
#include "osg/Texture2D"  // 添加纹理头文件
#include <chrono>         // 添加时间头文件

class SkyCB : public osg::StateSet::Callback
{
private:
    osg::Camera * pCamera_ = nullptr;
    std::chrono::high_resolution_clock::time_point _startTime;  // 添加时间变量

public:
    explicit SkyCB(osg::Camera * camera) : pCamera_(camera), _startTime(std::chrono::high_resolution_clock::now())
    {
    }

    virtual void operator()(osg::StateSet* ss, osg::NodeVisitor* nv)
    {
        preocess(ss);
    }

    void preocess(osg::StateSet* ss)
    {
        if (pCamera_)
        {
            osg::Vec3f eye, center, up;
            pCamera_->getViewMatrixAsLookAt(eye, center, up);
            ss->getOrCreateUniform("cameraPosition", osg::Uniform::FLOAT_VEC3)->set(eye);

            // 修复视图矩阵逆矩阵计算
            osg::Matrixf viewMat = pCamera_->getViewMatrix();
            osg::Matrixf viewInverse = osg::Matrixf::inverse(viewMat);
            ss->getOrCreateUniform("viewInverse", osg::Uniform::FLOAT_MAT4)->set(viewInverse);
            
            // 更新时间uniform
            auto now = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(now - _startTime).count();
            ss->getOrCreateUniform("iTime", osg::Uniform::FLOAT)->set(elapsed);
        }
    }
};

SkyBoxThree::SkyBoxThree()
{

}

SkyBoxThree::SkyBoxThree(osg::Camera * pCamera)
{
    // 使用绝对参考框架，使天空盒不受场景变换影响
    setReferenceFrame(osg::Transform::ABSOLUTE_RF);

    setCullingActive(false);

    osg::StateSet* ss = getOrCreateStateSet();
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0f, 1.0f));
    ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);

    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    ss->setRenderBinDetails(0, "RenderBin");

    //add
    initUniforms();
    osg::ref_ptr<osg::Program> program = new osg::Program;
    std::string resourcePath = QDir::currentPath().toStdString() + "/../../shader/";
    osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX, resourcePath + "x1.vert");
    pV->setName("X1.vert");
    osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, resourcePath + "x1.frag");
    pF->setName("X1.frag");
    program->addShader(pV);
    program->addShader(pF);
    ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);

    ss->addUniform(_turbidity.get());
    ss->addUniform(_rayleigh.get());
    ss->addUniform(_mieCoefficient.get());
    ss->addUniform(_mieDirectionalG.get());
    ss->addUniform(_sunPosition.get());
    ss->addUniform(_up.get());
    ss->addUniform(_sunZenithAngle.get());  // 添加太阳天顶角度uniform
    ss->addUniform(_sunAzimuthAngle.get());  // 添加太阳方位角度uniform
    ss->addUniform(_cloudDensity.get());  // 添加云密度uniform
    ss->addUniform(_cloudHeight.get());  // 添加云厚度uniform
    ss->addUniform(_cloudBaseHeight.get());  // 添加云层底部高度uniform
    ss->addUniform(_cloudRangeMin.get());  // 添加云层近裁剪距离uniform
    ss->addUniform(_cloudRangeMax.get());  // 添加云层远裁剪距离uniform
    ss->addUniform(new osg::Uniform("iTime", 0.0f));  // 添加时间uniform

    
    osg::ref_ptr<osg::Image> noiseImage = osgDB::readImageFile("E:/a.png");
    if (noiseImage.valid()) {
        osg::ref_ptr<osg::Texture2D> noiseTexture = new osg::Texture2D();
        noiseTexture->setImage(noiseImage.get());
        noiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        noiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        noiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        noiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(0, noiseTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("iChannel0", 0));
    } else {
        // 如果无法加载噪声贴图，创建一个默认的白色纹理
        osg::ref_ptr<osg::Image> defaultImage = new osg::Image();
        unsigned char* data = new unsigned char[4];
        data[0] = data[1] = data[2] = 255; data[3] = 255; // 白色
        defaultImage->setImage(1, 1, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
        osg::ref_ptr<osg::Texture2D> defaultTexture = new osg::Texture2D();
        defaultTexture->setImage(defaultImage.get());
        defaultTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        defaultTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        defaultTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        defaultTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        ss->setTextureAttributeAndModes(0, defaultTexture, osg::StateAttribute::ON);
        ss->addUniform(new osg::Uniform("iChannel0", 0));
    }


    SkyCB* pCB = new SkyCB(pCamera);
    ss->setUpdateCallback(pCB);

}

void SkyBoxThree::initUniforms()
{
    _turbidity = new osg::Uniform("turbidity", 2.0f);
    _rayleigh = new osg::Uniform("rayleigh", 1.0f);
    _mieCoefficient = new osg::Uniform("mieCoefficient", 0.005f);
    _mieDirectionalG = new osg::Uniform("mieDirectionalG", 0.8f);
    _sunPosition = new osg::Uniform("sunPosition", osg::Vec3(0.0f, 0.7f, 0.8f));
    _up = new osg::Uniform("up", osg::Vec3(0.0f, 0.0f, 1.0f));  // 使用Z轴向上
    _sunZenithAngle = new osg::Uniform("sunZenithAngle", 45.0f * 3.14159f / 180.0f);  // 调整太阳天顶角度为45度（更高）
    _sunAzimuthAngle = new osg::Uniform("sunAzimuthAngle", 270.0f * 3.14159f / 180.0f);  // 初始化太阳方位角度为90度
    _cloudDensity = new osg::Uniform("cloudDensity", 3.0f);  // 降低初始云密度
    _cloudHeight = new osg::Uniform("cloudHeight", 800.0f);  // 初始化云厚度
    _cloudBaseHeight = new osg::Uniform("cloudBaseHeight", 1500.0f);  // 初始化云层底部高度
    _cloudRangeMin = new osg::Uniform("cloudRangeMin", 0.0f);  // 初始化云层近裁剪距离
    _cloudRangeMax = new osg::Uniform("cloudRangeMax", 50000.0f);  // 初始化云层远裁剪距离
}

bool SkyBoxThree::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
	if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
	{
		osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
		// 使天空盒始终中心在相机位置，不会因相机旋转而旋转
		matrix.preMult(osg::Matrix::translate(cv->getEyeLocal()));
		return true;
	}
	else
		return osg::Transform::computeLocalToWorldMatrix(matrix, nv);
}

bool SkyBoxThree::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
	if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
	{
		osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
		// 使天空盒始终中心在相机位置，不会因相机旋转而旋转
		matrix.postMult(osg::Matrix::translate(-cv->getEyeLocal()));
		return true;
	}
	else
		return osg::Transform::computeWorldToLocalMatrix(matrix, nv);
}

// 新增：设置太阳天顶角度的方法实现
void SkyBoxThree::setSunZenithAngle(float angle)
{
    if (_sunZenithAngle.valid()) {
        _sunZenithAngle->set(angle);
    }
}

// 新增：设置太阳方位角度的方法实现
void SkyBoxThree::setSunAzimuthAngle(float angle)
{
    if (_sunAzimuthAngle.valid()) {
        _sunAzimuthAngle->set(angle);
    }
}

// 新增：设置云密度的方法实现
void SkyBoxThree::setCloudDensity(float density)
{
    if (_cloudDensity.valid()) {
        _cloudDensity->set(density);
    }
}

// 新增：设置云厚度的方法实现
void SkyBoxThree::setCloudHeight(float height)
{
    if (_cloudHeight.valid()) {
        _cloudHeight->set(height);
    }
}

// 新增：设置云层底部高度的方法实现
void SkyBoxThree::setCloudBaseHeight(float baseHeight)
{
    if (_cloudBaseHeight.valid()) {
        _cloudBaseHeight->set(baseHeight);
    }
}

// 新增：设置云层近裁剪距离的方法实现
void SkyBoxThree::setCloudRangeMin(float rangeMin)
{
    if (_cloudRangeMin.valid()) {
        _cloudRangeMin->set(rangeMin);
    }
}

// 新增：设置云层远裁剪距离的方法实现
void SkyBoxThree::setCloudRangeMax(float rangeMax)
{
    if (_cloudRangeMax.valid()) {
        _cloudRangeMax->set(rangeMax);
    }
}
