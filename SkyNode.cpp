// TH3DGraphLibTestView.cpp : CTHGW3DGraphView 类的实现

#include "osg/Depth"
#include <osgUtil/CullVisitor>

#include "skyNode.h"
#include "osgDB/ReadFile"
#include<Qdir>

class SkyCB : public osg::StateSet::Callback
{
public:
	explicit SkyCB(osg::Camera * camera) : pCamera_(camera)
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
		}
	}
private:
	osg::Camera * pCamera_ = nullptr;
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
	osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX, resourcePath + "X1.vert");
	pV->setName("X1.vert");
	osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, resourcePath + "X1.frag");
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
    _sunZenithAngle = new osg::Uniform("sunZenithAngle", 80.0f * 3.14159f / 180.0f);  // 初始化太阳天顶角度为68度
    _sunAzimuthAngle = new osg::Uniform("sunAzimuthAngle", 270.0f * 3.14159f / 180.0f);  // 初始化太阳方位角度为90度
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
