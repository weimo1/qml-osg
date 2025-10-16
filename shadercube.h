#ifndef SHADERCUBE_H
#define SHADERCUBE_H

#include <osg/Node>
#include <osg/Program>
#include <osg/Matrix>
#include <string>

class ShaderCube
{
public:
    // 创建Shader程序
    static osg::Program* createShaderProgram();
    
    // 创建天空盒着色器程序
    static osg::Program* createSkyBoxShaderProgram();
    
    // 创建Shader立方体
    static osg::Node* createCube(float size = 1.0f);
    
    // 创建天空盒
    static osg::Node* createSkyBox(const std::string& resourcePath);
    
    // 设置uniform变量
    static void setupUniforms(osg::StateSet* stateset);
};

#endif // SHADERCUBE_H