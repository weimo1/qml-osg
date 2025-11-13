// TH3DGraphLibTestView.cpp : CTHGW3DGraphView 类的实现

#include "osg/Depth"
#include <osgUtil/CullVisitor>

#include "skyNode.h"
#include "osgDB/ReadFile"


class SkyCB : public osg::StateSet::Callback
{
public:
	explicit SkyCB(osg::Camera * camera) : pCamera_(camera)
	{
		// 初始化时间
		startTime = osg::Timer::instance()->tick();
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

			osg::Matrixf vieMat = pCamera_->getViewMatrix();
			osg::Matrixf viewInverse = vieMat.inverse(vieMat);
			ss->getOrCreateUniform("viewInverse", osg::Uniform::FLOAT_MAT4)->set(viewInverse);
			
			// 更新时间uniform，使用更快的速度
			double currentTime = osg::Timer::instance()->delta_s(startTime, osg::Timer::instance()->tick());
			ss->getOrCreateUniform("time", osg::Uniform::FLOAT)->set((float)currentTime * 0.5f); // 加快云朵动画速度
		}
	}
private:
	osg::Camera * pCamera_ = nullptr;
	osg::Timer_t startTime;
};

SkyBoxThree::SkyBoxThree()
{

}

SkyBoxThree::SkyBoxThree(osg::Camera * pCamera)
{
	setReferenceFrame(osg::Transform::ABSOLUTE_RF);

	setCullingActive(false);

	osg::StateSet* ss = getOrCreateStateSet();
	ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0f, 1.0f));

	ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
	ss->setRenderBinDetails(-2, "RenderBin");

	//add
	initUniforms();

	osg::ref_ptr<osg::Program> program = new osg::Program;

	std::string strDir = "E:/qml/shader/";
	osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX, strDir + "X1.vert");
	pV->setName("X1.vert");
	osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, strDir + "X1.frag");
	pF->setName("X1.frag");
	program->addShader(pV);
	program->addShader(pF);
	ss->setAttributeAndModes(program.get(), osg::StateAttribute::ON);

	ss->addUniform(_turbidity.get());
	ss->addUniform(_rayleigh.get());
	ss->addUniform(_mieCoefficient.get());
	ss->addUniform(_mieDirectionalG.get());
	ss->addUniform(_exposure.get());
	ss->addUniform(_sunPosition.get());
	ss->addUniform(_up.get());
	
	// 添加云朵uniforms
	ss->addUniform(_cloudCoverage.get());
	ss->addUniform(_cloudSpeed.get());
	ss->addUniform(_cloudDensity.get());
	ss->addUniform(_time.get());

	SkyCB* pCB = new SkyCB(pCamera);
	ss->setUpdateCallback(pCB);

}

void SkyBoxThree::initUniforms()
{
	_turbidity = new osg::Uniform("turbidity",10.0f );
	_rayleigh  = new osg::Uniform("rayleigh", 3.0f);
	_mieCoefficient  = new osg::Uniform("mieCoefficient", 0.005f);
	_mieDirectionalG = new osg::Uniform("mieDirectionalG", 0.7f);
	_exposure        = new osg::Uniform("exposure", 0.5f);
	_sunPosition     = new osg::Uniform("sunPosition", osg::Vec3(0.0f, -0.9994f, 0.035f));
	_up = new osg::Uniform("up", osg::Vec3(0.0f, 0.0f, 1.0f));	
	
	// 初始化云朵uniforms，使用更明显的默认值以增加云的数量
	_cloudCoverage = new osg::Uniform("cloudCoverage", 0.8f);   // 80%云覆盖率，增加云的数量
	_cloudSpeed = new osg::Uniform("cloudSpeed", 0.5f);         // 更快的云速度
	_cloudDensity = new osg::Uniform("cloudDensity", 1.2f);     // 更高的云密度，使云更厚更明显
	_time = new osg::Uniform("time", 0.0f);                     // 时间
}


bool SkyBoxThree::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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

bool SkyBoxThree::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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