#include "demoshader.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
// 添加OpenGL头文件
#include <osg/GL>
#include <osg/Texture>
#include <osg/Image>
// 添加形状相关的头文件
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include<Qdir>
// 常量定义
const float DemoShader::kSunAngularRadius = 0.00935f / 2.0f;
const float DemoShader::kLengthUnitInMeters = 1000.0f;

DemoShader::DemoShader()
    : _viewDistanceMeters(5000.0f)  // 初始观察距离5km，更接近地球表面
    , _viewZenithAngleRadians(0.0f)  // 初始视角天顶角，从地面向上看
    , _viewAzimuthAngleRadians(0.0f)  // 初始视角方位角
    , _sunZenithAngleRadians(0.5f)  // 初始太阳天顶角，接近地平线
    , _sunAzimuthAngleRadians(0.0f)  // 初始太阳方位角
    , _exposure(15.0f)  // 初始曝光值，增加对比度
    , _atmosphereDensity(1.0f)  // 初始大气密度
    , _sunIntensity(20.0f)  // 初始太阳强度
    , _mieScattering(1.0f)  // 初始米氏散射系数
    , _rayleighScattering(1.0f)  // 初始瑞利散射系数
    , _texturesInitialized(false)
{
}

DemoShader::~DemoShader()
{
}

// 创建全屏四边形
osg::Geometry* DemoShader::createFullScreenQuad()
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    
    // 顶点数据 - 创建一个覆盖整个屏幕的四边形
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(-1.0f, -1.0f, 0.0f)); // 左下角
    vertices->push_back(osg::Vec3(1.0f, -1.0f, 0.0f));  // 右下角
    vertices->push_back(osg::Vec3(-1.0f, 1.0f, 0.0f));  // 左上角
    vertices->push_back(osg::Vec3(1.0f, 1.0f, 0.0f));   // 右上角
    
    geom->setVertexArray(vertices);
    
    // 顶点索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLE_STRIP);
    indices->push_back(0);
    indices->push_back(1);
    indices->push_back(2);
    indices->push_back(3);
    
    geom->addPrimitiveSet(indices);
    
    // 禁用光照以确保颜色正确显示
    osg::StateSet* stateset = geom->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    
    return geom.release();
}

osg::Node* DemoShader::createAtmosphereScene()
{
    std::cout << "Creating atmosphere scene with full-screen quad..." << std::endl;
    
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 创建全屏四边形
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> quad = createFullScreenQuad();
    geode->addDrawable(quad);
    
    // 创建大气散射着色器程序
    osg::ref_ptr<osg::Program> atmosphereProgram = new osg::Program;
    
    // 使用相对路径定位着色器文件
    std::string resourcePath = QDir::currentPath().toStdString() + "/../../shader";
    
    // 加载顶点着色器
    std::string vertexShaderPath = resourcePath + "/sky_vertex.txt";
    osg::ref_ptr<osg::Shader> vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, vertexShaderPath);
    if (!vertexShader) {
        // 如果失败，尝试使用绝对路径
        vertexShaderPath = "E:/qt test/qml+osg/shader/sky_vertex.txt";
        vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, vertexShaderPath);
        if (!vertexShader) {
            std::cerr << "Failed to load atmosphere vertex shader from: " << vertexShaderPath << std::endl;
            return nullptr;
        }
    }
    atmosphereProgram->addShader(vertexShader);
    
    // 加载片段着色器
    std::string fragmentShaderPath = resourcePath + "/sky_fragment.txt";
    osg::ref_ptr<osg::Shader> fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, fragmentShaderPath);
    if (!fragmentShader) {
        // 如果失败，尝试使用绝对路径
        fragmentShaderPath = "E:/qt test/qml+osg/shader/sky_fragment.txt";
        fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, fragmentShaderPath);
        if (!fragmentShader) {
            std::cerr << "Failed to load atmosphere fragment shader from: " << fragmentShaderPath << std::endl;
            return nullptr;
        }
    }
    atmosphereProgram->addShader(fragmentShader);
    
    // 设置着色器程序
    osg::StateSet* stateset = geode->getOrCreateStateSet();
    stateset->setAttributeAndModes(atmosphereProgram, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 保存状态集引用，以便后续更新
    _atmosphereStateSet = stateset;
    
    // 设置初始uniform变量
    updateAtmosphereUniforms(stateset);
    
    // 禁用深度测试以确保全屏四边形正确渲染
    stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    
    // 禁用光照
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    
    root->addChild(geode);
    
    std::cout << "Atmosphere scene created successfully" << std::endl;
    
    return root.release();
}

void DemoShader::updateAtmosphereUniforms(osg::StateSet* stateset)
{
    if (!stateset) return;
    
    // 根据参数计算太阳方向
    // 使用标准的天球坐标系
    // 天顶角：0度表示天顶，90度表示地平线，180度表示天底
    // 方位角：0度表示北方，90度表示东方
    // 在OpenGL坐标系中：X向右，Y向上，Z指向观察者
    osg::Vec3 sunDirection(
        sin(_sunZenithAngleRadians) * sin(_sunAzimuthAngleRadians),   // X分量
        cos(_sunZenithAngleRadians),                                   // Y分量（天顶角的余弦）
        sin(_sunZenithAngleRadians) * cos(_sunAzimuthAngleRadians)   // Z分量（负号用于调整方向）
    );
    sunDirection.normalize();
    
    // 更新太阳位置uniform
    osg::Uniform* sunPositionUniform = stateset->getUniform("sunPosition");
    if (sunPositionUniform) {
        sunPositionUniform->set(sunDirection);
    } else {
        stateset->addUniform(new osg::Uniform("sunPosition", sunDirection));
    }
    
    // 添加更多大气散射相关的uniform变量
    // 瑞利散射系数
    osg::Uniform* rayleighScatteringUniform = stateset->getUniform("rayleighScattering");
    if (rayleighScatteringUniform) {
        rayleighScatteringUniform->set(_rayleighScattering);
    } else {
        stateset->addUniform(new osg::Uniform("rayleighScattering", _rayleighScattering));
    }
    
    // 大气密度
    osg::Uniform* atmosphereDensityUniform = stateset->getUniform("atmosphereDensity");
    if (atmosphereDensityUniform) {
        atmosphereDensityUniform->set(_atmosphereDensity);
    } else {
        stateset->addUniform(new osg::Uniform("atmosphereDensity", _atmosphereDensity));
    }
    
    // 米氏散射系数
    osg::Uniform* mieScatteringUniform = stateset->getUniform("mieScattering");
    if (mieScatteringUniform) {
        mieScatteringUniform->set(_mieScattering);
    } else {
        stateset->addUniform(new osg::Uniform("mieScattering", _mieScattering));
    }
    
    // 太阳强度
    osg::Uniform* sunIntensityUniform = stateset->getUniform("sunIntensity");
    if (sunIntensityUniform) {
        sunIntensityUniform->set(_sunIntensity);
    } else {
        stateset->addUniform(new osg::Uniform("sunIntensity", _sunIntensity));
    }
    
    // 米氏散射方向性参数
    osg::Uniform* mieDirectionalGUniform = stateset->getUniform("mieDirectionalG");
    if (mieDirectionalGUniform) {
        mieDirectionalGUniform->set(0.8f);  // 固定值
    } else {
        stateset->addUniform(new osg::Uniform("mieDirectionalG", 0.8f));
    }
    
    // 上方向
    osg::Uniform* upUniform = stateset->getUniform("up");
    if (upUniform) {
        upUniform->set(osg::Vec3(0.0f, 1.0f, 0.0f));
    } else {
        stateset->addUniform(new osg::Uniform("up", osg::Vec3(0.0f, 1.0f, 0.0f)));
    }
    
    std::cout << "Updated atmosphere uniforms:" << std::endl;
    std::cout << "  Sun zenith angle: " << _sunZenithAngleRadians << " (" << _sunZenithAngleRadians * 180.0 / M_PI << " degrees)" << std::endl;
    std::cout << "  Sun azimuth angle: " << _sunAzimuthAngleRadians << " (" << _sunAzimuthAngleRadians * 180.0 / M_PI << " degrees)" << std::endl;
    std::cout << "  Sun direction: " << sunDirection.x() << ", " << sunDirection.y() << ", " << sunDirection.z() << std::endl;
    std::cout << "  Rayleigh scattering: " << _rayleighScattering << std::endl;
    std::cout << "  Atmosphere density: " << _atmosphereDensity << std::endl;
    std::cout << "  Mie scattering: " << _mieScattering << std::endl;
    std::cout << "  Sun intensity: " << _sunIntensity << std::endl;
    
    // 添加额外的调试信息
    std::cout << "  Sun direction length: " << sunDirection.length() << std::endl;
}
