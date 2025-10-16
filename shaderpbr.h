#ifndef SHADERPBR_H
#define SHADERPBR_H

#include <osg/Node>
#include <osg/Program>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/MatrixTransform>
#include <osg/TextureCubeMap>
#include <string>

// 添加前向声明
class ShaderCube;

class ShaderPBR
{
public:
    // 创建带PBR效果的球体阵列
    static osg::Node* createPBRSphere(float radius = 1.0f);
    
    // 创建单个球体
    static osg::Node* createSingleSphere(float radius);
    
    // 创建PBR着色器程序
    static osg::Program* createPBRShaderProgram();
    
    // 创建改进版PBR着色器程序（带IBL）
    static osg::Program* createPBRShaderProgramWithIBL();
    
    // 创建简化版PBR着色器（带增强IBL）
    static osg::Program* createPBRShaderSimpleIBL();
    
    // 创建带天空盒的PBR场景
    static osg::Node* createPBRSceneWithSkybox(float sphereRadius = 1.0f);
    
    // 创建天空盒（使用ShaderCube实现）
    static osg::Node* createSkybox(const std::string& resourcePath);
};

#endif // SHADERPBR_H