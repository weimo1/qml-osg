#include "skybox.h"
#include <osg/Depth>
#include <osg/TexGen>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
#include <iostream>
#include <cmath>

#include<osg/Vec3>
#include<osg/Vec4>
SkyBox::SkyBox()
{
    setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    setCullingActive(false);
    
    osg::StateSet* ss = getOrCreateStateSet();
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0f, 1.0f));
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    ss->setRenderBinDetails(5, "RenderBin");
    
    // 添加球形天空盒着色器程序
    ss->setAttributeAndModes(createSphereSkyBoxShaderProgram(), osg::StateAttribute::ON);
}

// 创建球形天空盒着色器程序
osg::Program* SkyBox::createSphereSkyBoxShaderProgram()
{
    // 顶点着色器源码 - 用于球形天空盒
    const char* vertexShaderSource = R"(
        #version 330 core
        
        layout(location = 0) in vec3 position;
        
        out vec3 TexCoords;
        
        uniform mat4 osg_ViewMatrix;
        uniform mat4 osg_ProjectionMatrix;
        
        void main()
        {
            TexCoords = position;
            // 移除视图矩阵的平移部分，确保天空盒始终围绕相机
            mat4 view = mat4(mat3(osg_ViewMatrix));
            vec4 pos = osg_ProjectionMatrix * view * vec4(position, 1.0);
            gl_Position = pos.xyww; // 使用.xyww而不是.xyzw来确保深度测试正确
        }
    )";
    
    // 片元着色器源码 - 用于球形天空盒
    const char* fragmentShaderSource = R"(
        #version 330 core
        
        in vec3 TexCoords;
        
        out vec4 FragColor;
        
        uniform samplerCube skybox;
        
        void main()
        {    
            FragColor = texture(skybox, TexCoords);
        }
    )";
    
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;
    
    // 创建并添加顶点着色器
    osg::ref_ptr<osg::Shader> vertexShader = new osg::Shader(osg::Shader::VERTEX, vertexShaderSource);
    program->addShader(vertexShader);
    
    // 创建并添加片元着色器
    osg::ref_ptr<osg::Shader> fragmentShader = new osg::Shader(osg::Shader::FRAGMENT, fragmentShaderSource);
    program->addShader(fragmentShader);
    
    return program.release();
}

void SkyBox::setEnvironmentMap(unsigned int unit,
                              osg::Image* posX, osg::Image* negX,
                              osg::Image* posY, osg::Image* negY,
                              osg::Image* posZ, osg::Image* negZ)
{
    osg::ref_ptr<osg::TextureCubeMap> cubemap = new osg::TextureCubeMap;
    
    cubemap->setImage(osg::TextureCubeMap::POSITIVE_X, posX);
    cubemap->setImage(osg::TextureCubeMap::NEGATIVE_X, negX);
    cubemap->setImage(osg::TextureCubeMap::POSITIVE_Y, posY);
    cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Y, negY);
    cubemap->setImage(osg::TextureCubeMap::POSITIVE_Z, posZ);
    cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Z, negZ);
    
    cubemap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    
    cubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    cubemap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    
    cubemap->setResizeNonPowerOfTwoHint(false);
    
    getOrCreateStateSet()->setTextureAttributeAndModes(unit, cubemap.get());
    
    // 添加采样器uniform
    getOrCreateStateSet()->addUniform(new osg::Uniform("skybox", (int)unit));
}

bool SkyBox::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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

bool SkyBox::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
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