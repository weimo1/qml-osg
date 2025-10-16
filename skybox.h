#ifndef SKYBOX_H
#define SKYBOX_H

#include <osg/Transform>
#include <osg/TextureCubeMap>
#include <osg/Image>
#include <osg/Program>

class SkyBox : public osg::Transform
{
public:
    SkyBox();
    SkyBox(const SkyBox& copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY)
        : osg::Transform(copy, copyop) {}
    
    META_Node(osg, SkyBox);
    
    void setEnvironmentMap(unsigned int unit, osg::Image* posX,
                          osg::Image* negX, osg::Image* posY, osg::Image* negY,
                          osg::Image* posZ, osg::Image* negZ);
    
    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
    virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const;
    
protected:
    virtual ~SkyBox() {}
    
    // 创建球形天空盒着色器程序
    osg::Program* createSphereSkyBoxShaderProgram();
};

#endif // SKYBOX_H