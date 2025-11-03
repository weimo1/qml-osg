#ifndef DEMOSAHDER_H
#define DEMOSAHDER_H

#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osg/MatrixTransform>
#include <osg/Camera>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <QVector3D>
#include <vector>
#include <cmath>
#include "CloudSeaAtmosphere.h"

// 前向声明
class SkyBoxThree;

class DemoShader : public osg::Referenced
{
public:
    DemoShader();
    ~DemoShader();

    // 更新参数
    void updateParameters(float viewDistance, float viewZenithAngle, 
                         float viewAzimuthAngle, float sunZenithAngle, 
                         float sunAzimuthAngle, float exposure);
    
    // 获取着色器程序
    osg::Program* getProgram() { return _program.get(); }
    
    // 获取大气场景状态集
    osg::StateSet* getAtmosphereStateSet() { return _atmosphereStateSet.get(); }
    
    // 获取纹理
    osg::Texture2D* getTransmittanceTexture() { return _transmittanceTexture.get(); }
    osg::Texture3D* getScatteringTexture() { return _scatteringTexture.get(); }
    osg::Texture2D* getIrradianceTexture() { return _irradianceTexture.get(); }
    
    // 获取和设置参数的方法
    float getViewDistance() const { return _viewDistanceMeters; }
    float getViewZenithAngle() const { return _viewZenithAngleRadians; }
    float getViewAzimuthAngle() const { return _viewAzimuthAngleRadians; }
    float getSunZenithAngle() const { return _sunZenithAngleRadians; }
    float getSunAzimuthAngle() const { return _sunAzimuthAngleRadians; }
    float getExposure() const { return _exposure; }
    float getAtmosphereDensity() const { return _atmosphereDensity; }
    float getSunIntensity() const { return _sunIntensity; }
    float getMieScattering() const { return _mieScattering; }
    float getRayleighScattering() const { return _rayleighScattering; }
    
    void setViewDistance(float distance) { _viewDistanceMeters = distance; }
    void setViewZenithAngle(float angle) { _viewZenithAngleRadians = angle; }
    void setViewAzimuthAngle(float angle) { _viewAzimuthAngleRadians = angle; }
    void setSunZenithAngle(float angle) { _sunZenithAngleRadians = angle; }
    void setSunAzimuthAngle(float angle) { _sunAzimuthAngleRadians = angle; }
    void setExposure(float exposure) { _exposure = exposure; }
    void setAtmosphereDensity(float density) { _atmosphereDensity = density; }
    void setSunIntensity(float intensity) { _sunIntensity = intensity; }
    void setMieScattering(float mie) { _mieScattering = mie; }
    void setRayleighScattering(float rayleigh) { _rayleighScattering = rayleigh; }
    
    // 创建用于大气渲染的全屏四边形场景
    osg::Node* createAtmosphereScene();
    osg::Geometry* createFullScreenQuad();
    
    // 更新大气场景中的uniform变量
    void updateAtmosphereUniforms(osg::StateSet* stateset);

    // 更新场景中的uniform变量
    void updateSceneUniforms(osg::StateSet* stateset);
    
    // 新增：创建结合天空盒纹理和大气渲染的场景
    osg::Node* createTexturedAtmosphereScene();
    
    
    // 新增：更新大气场景uniform变量
    void updateAtmosphereSceneUniforms(osg::StateSet* stateset);
    
    // 新增：创建结合天空盒和大气渲染的场景
    osg::Node* createSkyboxAtmosphereScene(osgViewer::Viewer* viewer);
    
    // 新增：创建PBR立方体
    osg::Node* createPBRCube();
    
    // 新增：创建调试用的简单立方体
    osg::Node* createDebugCube();
    
    // 新增：创建使用改进大气着色器的场景
    osg::Node* createImprovedAtmosphereScene(osgViewer::Viewer* viewer);
    
    // 新增：创建结合天空盒大气和PBR立方体的场景
    osg::Node* createSkyboxAtmosphereWithPBRScene(osgViewer::Viewer* viewer);
    
    // 新增：创建云海大气效果场景
    osg::Node* createCloudSeaAtmosphereScene(osgViewer::Viewer* viewer);
    
    // 新增：更新云海大气参数
    void updateCloudSeaAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle,
                                         float cloudDensity, float cloudHeight,
                                         float cloudBaseHeight, float cloudRangeMin, float cloudRangeMax);

    // 获取CloudSeaAtmosphere引用
    osg::ref_ptr<CloudSeaAtmosphere> getCloudSeaAtmosphere() { return _cloudSeaAtmosphere; }
    
    // 新增：更新SkyNode大气参数的方法
    void updateSkyNodeAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                        float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG,
                                        float sunZenithAngle, float sunAzimuthAngle);
    
    // 新增：更新SkyNode云海大气参数的方法
    void updateSkyNodeCloudParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                   float sunZenithAngle, float sunAzimuthAngle,
                                   float cloudDensity, float cloudHeight,
                                   float cloudBaseHeight, float cloudRangeMin, float cloudRangeMax);
    
    // 辅助函数：递归查找SkyBoxThree节点
    osg::Node* findSkyBoxThreeNode(osg::Node* node);

private:
    // 着色器程序
    osg::ref_ptr<osg::Program> _program;
    
    // 纹理
    osg::ref_ptr<osg::Texture2D> _transmittanceTexture;
    osg::ref_ptr<osg::Texture3D> _scatteringTexture;
    osg::ref_ptr<osg::Texture2D> _irradianceTexture;
    
    // 纹理数据
    std::vector<float> _transmittanceData;
    std::vector<float> _scatteringData;
    std::vector<float> _irradianceData;
    
    // 几何体
    osg::ref_ptr<osg::Geometry> _fullScreenQuad;
    
    // 参数
    float _viewDistanceMeters;
    float _viewZenithAngleRadians;
    float _viewAzimuthAngleRadians;
    float _sunZenithAngleRadians;
    float _sunAzimuthAngleRadians;
    float _exposure;
    float _atmosphereDensity;
    float _sunIntensity;
    float _mieScattering;
    float _rayleighScattering;
    
    // 常量
    static const int TRANSMITTANCE_TEXTURE_WIDTH = 256;
    static const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;
    static const int SCATTERING_TEXTURE_WIDTH = 256;
    static const int SCATTERING_TEXTURE_HEIGHT = 128;
    static const int SCATTERING_TEXTURE_DEPTH = 32;
    static const int IRRADIANCE_TEXTURE_WIDTH = 64;
    static const int IRRADIANCE_TEXTURE_HEIGHT = 16;
    
    // 标志位，表示纹理是否已初始化
    bool _texturesInitialized;
    
    // 常量定义
    static const float kSunAngularRadius;
    static const float kLengthUnitInMeters;
    
    // 大气场景的状态集（用于动态更新）
    osg::ref_ptr<osg::StateSet> _atmosphereStateSet;
    
    // 云海大气对象引用
    osg::ref_ptr<CloudSeaAtmosphere> _cloudSeaAtmosphere;
    
    
    // 云海大气参数
    float _cloudSeaDensity;
    float _cloudSeaHeight;

};

#endif // DEMOSAHDER_H