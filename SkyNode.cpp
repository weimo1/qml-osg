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

			osg::Matrixf vieMat = pCamera_->getViewMatrix();
			osg::Matrixf viewInverse = osg::Matrixf::inverse(vieMat);
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

	SkyCB* pCB = new SkyCB(pCamera);
	ss->setUpdateCallback(pCB);

}

void SkyBoxThree::initUniforms()
{
	_turbidity = new osg::Uniform("turbidity", 2.0f);
	_rayleigh = new osg::Uniform("rayleigh", 1.0f);
	_mieCoefficient = new osg::Uniform("mieCoefficient", 0.005f);
	_mieDirectionalG = new osg::Uniform("mieDirectionalG", 0.8f);
	_sunPosition = new osg::Uniform("sunPosition", osg::Vec3(0.0f, 0.1f, 0.3f));
	_up = new osg::Uniform("up", osg::Vec3(0.0f, 1.0f,0.0f));
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