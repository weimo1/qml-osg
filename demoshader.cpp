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
#include"SkyNode.h"
DemoShader::DemoShader()
    : _viewDistanceMeters(5000.0f)  // 初始观察距离5km，更接近地球表面
    , _viewZenithAngleRadians(0.0f)  // 初始视角天顶角，从地面向上看
    , _viewAzimuthAngleRadians(0.0f)  // 初始视角方位角
    , _sunZenithAngleRadians(0.5f)  // 初始太阳天顶角，接近地平线
    , _sunAzimuthAngleRadians(0.0f)  // 初始太阳方位角
    , _exposure(15.0f)  // 初始曝光值，增加对比度
    , _atmosphereDensity(2.0f)  // 初始大气密度 (turbidity)
    , _sunIntensity(20.0f)  // 初始太阳强度
    , _mieScattering(0.005f)  // 初始米氏散射系数
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
    
    // 纹理坐标
    osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
    texcoords->push_back(osg::Vec2(0.0f, 0.0f)); // 左下角
    texcoords->push_back(osg::Vec2(1.0f, 0.0f)); // 右下角
    texcoords->push_back(osg::Vec2(0.0f, 1.0f)); // 左上角
    texcoords->push_back(osg::Vec2(1.0f, 1.0f)); // 右上角
    
    geom->setVertexArray(vertices);
    geom->setTexCoordArray(0, texcoords);
    
    // 顶点索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLE_STRIP);
    indices->push_back(0);
    indices->push_back(1);
    indices->push_back(2);
    indices->push_back(3);
    
    geom->addPrimitiveSet(indices);
    
    // 设置顶点属性
    geom->setVertexAttribArray(0, vertices);  // 顶点位置
    geom->setVertexAttribBinding(0, osg::Geometry::BIND_PER_VERTEX);
    geom->setVertexAttribArray(1, texcoords); // 纹理坐标
    geom->setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
    
    // 禁用光照以确保颜色正确显示
    osg::StateSet* stateset = geom->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    
    return geom.release();
}

// 新增：创建结合天空盒纹理和大气渲染的场景
osg::Node* DemoShader::createTexturedAtmosphereScene()
{
    std::cout << "Creating textured atmosphere scene with full-screen quad..." << std::endl;
    
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
    std::string vertexShaderPath = resourcePath + "/vertex_shader.txt";
    osg::ref_ptr<osg::Shader> vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, vertexShaderPath);
    if (!vertexShader) {
        // 如果失败，尝试使用绝对路径
        vertexShaderPath = "E:/qt test/qml+osg/shader/vertex_shader.txt";
        vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, vertexShaderPath);
        if (!vertexShader) {
            std::cerr << "Failed to load atmosphere vertex shader from: " << vertexShaderPath << std::endl;
            return nullptr;
        }
    }
    atmosphereProgram->addShader(vertexShader);
    
    // 加载片段着色器
    std::string fragmentShaderPath = resourcePath + "/fragment_shader.txt";
    osg::ref_ptr<osg::Shader> fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, fragmentShaderPath);
    if (!fragmentShader) {
        // 如果失败，尝试使用绝对路径
        fragmentShaderPath = "E:/qt test/qml+osg/shader/fragment_shader.txt";
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
    
    // 加载预计算的纹理数据
    loadAtmosphereTextures(stateset);
    
    // 保存状态集引用，以便后续更新
    _atmosphereStateSet = stateset;
    
    // 设置初始uniform变量
    updateAtmosphereSceneUniforms(stateset);
    
    // 禁用深度测试以确保全屏四边形正确渲染
    stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    
    // 禁用光照
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    
    root->addChild(geode);
    
    std::cout << "Textured atmosphere scene created successfully" << std::endl;
    
    return root.release();
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
    std::string vertexShaderPath = resourcePath + "/atmosphere_vertex.txt";
    osg::ref_ptr<osg::Shader> vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, vertexShaderPath);
    if (!vertexShader) {
        // 如果失败，尝试使用绝对路径
        vertexShaderPath = "E:/qt test/qml+osg/shader/skybox_vertex.txt";
        vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, vertexShaderPath);
        if (!vertexShader) {
            std::cerr << "Failed to load atmosphere vertex shader from: " << vertexShaderPath << std::endl;
            return nullptr;
        }
    }
    atmosphereProgram->addShader(vertexShader);
    
    // 加载片段着色器
    std::string fragmentShaderPath = resourcePath + "/atmosphere_fragment.txt";
    osg::ref_ptr<osg::Shader> fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, fragmentShaderPath);
    if (!fragmentShader) {
        // 如果失败，尝试使用绝对路径
        fragmentShaderPath = "E:/qt test/qml+osg/shader/skybox_fragment.txt";
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
    
    // 添加纹理uniform，即使不使用纹理也需要添加以避免着色器错误
    stateset->addUniform(new osg::Uniform("skyTexture", 0));
    
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

// 新增：创建结合天空盒和大气渲染的场景
osg::Node* DemoShader::createSkyboxAtmosphereScene()
{
    return nullptr;
}

void DemoShader::loadAtmosphereTextures(osg::StateSet* stateset)
{
    if (!stateset) return;
    
    // 获取资源路径
    std::string resourcePath = QDir::currentPath().toStdString() + "/../../shader";
    
    // 加载透射率纹理数据
    std::string transmittancePath = resourcePath + "/transmittance.dat";
    osg::ref_ptr<osg::Texture2D> transmittanceTexture = loadTexture2DFromFile(transmittancePath, 
        TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    
    // 加载散射纹理数据
    std::string scatteringPath = resourcePath + "/scattering.dat";
    osg::ref_ptr<osg::Texture3D> scatteringTexture = loadTexture3DFromFile(scatteringPath,
        SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
    
    // 加载辐照度纹理数据
    std::string irradiancePath = resourcePath + "/irradiance.dat";
    osg::ref_ptr<osg::Texture2D> irradianceTexture = loadTexture2DFromFile(irradiancePath,
        IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
    
    if (!transmittanceTexture || !scatteringTexture || !irradianceTexture) {
        std::cerr << "Failed to load atmosphere textures" << std::endl;
        return;
    }
    
    // 设置纹理单元
    stateset->setTextureAttributeAndModes(0, transmittanceTexture, osg::StateAttribute::ON);
    stateset->setTextureAttributeAndModes(1, scatteringTexture, osg::StateAttribute::ON);
    stateset->setTextureAttributeAndModes(2, irradianceTexture, osg::StateAttribute::ON);
    
    // 设置uniform变量
    stateset->addUniform(new osg::Uniform("transmittance_texture", 0));
    stateset->addUniform(new osg::Uniform("scattering_texture", 1));
    stateset->addUniform(new osg::Uniform("irradiance_texture", 2));
    
    // 保存纹理引用
    _transmittanceTexture = transmittanceTexture;
    _scatteringTexture = scatteringTexture;
    _irradianceTexture = irradianceTexture;
    
    std::cout << "Atmosphere textures loaded successfully" << std::endl;
    std::cout << "  Transmittance texture: " << transmittancePath << std::endl;
    std::cout << "  Scattering texture: " << scatteringPath << std::endl;
    std::cout << "  Irradiance texture: " << irradiancePath << std::endl;
}

osg::Texture2D* DemoShader::loadTexture2DFromFile(const std::string& filename, int width, int height)
{
    // 打开文件
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }
    
    // 计算数据大小 (RGBA格式，每个分量是float)
    size_t dataSize = width * height * 4 * sizeof(float);
    std::vector<float> data(width * height * 4);
    
    // 读取数据
    size_t readSize = fread(data.data(), 1, dataSize, file);
    fclose(file);
    
    if (readSize != dataSize) {
        std::cerr << "Failed to read complete texture data from: " << filename << std::endl;
        return nullptr;
    }
    
    // 创建图像
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(width, height, 1, GL_RGBA, GL_FLOAT);
    
    // 复制数据
    float* imageData = (float*)image->data();
    for (int i = 0; i < width * height * 4; ++i) {
        imageData[i] = data[i];
    }
    
    // 创建纹理
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setImage(image);
    texture->setInternalFormat(GL_RGBA32F_ARB);
    texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    
    return texture.release();
}

osg::Texture3D* DemoShader::loadTexture3DFromFile(const std::string& filename, int width, int height, int depth)
{
    // 打开文件
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }
    
    // 计算数据大小 (RGBA格式，每个分量是float)
    size_t dataSize = width * height * depth * 4 * sizeof(float);
    std::vector<float> data(width * height * depth * 4);
    
    // 读取数据
    size_t readSize = fread(data.data(), 1, dataSize, file);
    fclose(file);
    
    if (readSize != dataSize) {
        std::cerr << "Failed to read complete texture data from: " << filename << std::endl;
        return nullptr;
    }
    
    // 创建图像
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(width, height, depth, GL_RGBA, GL_FLOAT);
    
    // 复制数据
    float* imageData = (float*)image->data();
    for (int i = 0; i < width * height * depth * 4; ++i) {
        imageData[i] = data[i];
    }
    
    // 创建纹理
    osg::ref_ptr<osg::Texture3D> texture = new osg::Texture3D;
    texture->setImage(image);
    texture->setInternalFormat(GL_RGBA32F_ARB);
    texture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    texture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    
    return texture.release();
}

void DemoShader::updateAtmosphereSceneUniforms(osg::StateSet* stateset)
{
    if (!stateset) return;
    
    // 相机位置
    osg::Vec3 camera(0.0f, 0.0f, _viewDistanceMeters / kLengthUnitInMeters);
    osg::Uniform* cameraUniform = stateset->getUniform("camera");
    if (cameraUniform) {
        cameraUniform->set(camera);
    } else {
        stateset->addUniform(new osg::Uniform("camera", camera));
    }
    
    // 曝光值
    osg::Uniform* exposureUniform = stateset->getUniform("exposure");
    if (exposureUniform) {
        exposureUniform->set(_exposure);
    } else {
        stateset->addUniform(new osg::Uniform("exposure", _exposure));
    }
    
    // 白点
    osg::Vec3 whitePoint(1.0f, 1.0f, 1.0f);
    osg::Uniform* whitePointUniform = stateset->getUniform("white_point");
    if (whitePointUniform) {
        whitePointUniform->set(whitePoint);
    } else {
        stateset->addUniform(new osg::Uniform("white_point", whitePoint));
    }
    
    // 地球中心
    osg::Vec3 earthCenter(0.0f, 0.0f, -6360000.0f / kLengthUnitInMeters);
    osg::Uniform* earthCenterUniform = stateset->getUniform("earth_center");
    if (earthCenterUniform) {
        earthCenterUniform->set(earthCenter);
    } else {
        stateset->addUniform(new osg::Uniform("earth_center", earthCenter));
    }
    
    // 太阳方向
    osg::Vec3 sunDirection(
        cos(_sunAzimuthAngleRadians) * sin(_sunZenithAngleRadians),
        sin(_sunAzimuthAngleRadians) * sin(_sunZenithAngleRadians),
        cos(_sunZenithAngleRadians)
    );
    sunDirection.normalize();
    
    osg::Uniform* sunDirectionUniform = stateset->getUniform("sun_direction");
    if (sunDirectionUniform) {
        sunDirectionUniform->set(sunDirection);
    } else {
        stateset->addUniform(new osg::Uniform("sun_direction", sunDirection));
    }
    
    // 太阳大小
    float sunAngularRadius = kSunAngularRadius;
    float sunCosAngularRadius = cos(sunAngularRadius);
    osg::Vec2 sunSize(tan(sunAngularRadius), sunCosAngularRadius);
    
    osg::Uniform* sunSizeUniform = stateset->getUniform("sun_size");
    if (sunSizeUniform) {
        sunSizeUniform->set(sunSize);
    } else {
        stateset->addUniform(new osg::Uniform("sun_size", sunSize));
    }
    
    std::cout << "Updated atmosphere scene uniforms:" << std::endl;
    std::cout << "  Camera: " << camera.x() << ", " << camera.y() << ", " << camera.z() << std::endl;
    std::cout << "  Exposure: " << _exposure << std::endl;
    std::cout << "  Sun direction: " << sunDirection.x() << ", " << sunDirection.y() << ", " << sunDirection.z() << std::endl;
    std::cout << "  Sun size: " << sunSize.x() << ", " << sunSize.y() << std::endl;
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
    
    // 大气密度 (turbidity)
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
        mieDirectionalGUniform->set(0.8f);
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
    
    // 相机位置（使用默认值）
    osg::Uniform* cameraPositionUniform = stateset->getUniform("cameraPosition");
    if (cameraPositionUniform) {
        cameraPositionUniform->set(osg::Vec3(0.0f, 0.0f, 0.0f));
    } else {
        stateset->addUniform(new osg::Uniform("cameraPosition", osg::Vec3(0.0f, 0.0f, 0.0f)));
    }
    
    // 视图逆矩阵（单位矩阵作为默认值）
    osg::Uniform* viewInverseUniform = stateset->getUniform("viewInverse");
    if (viewInverseUniform) {
        viewInverseUniform->set(osg::Matrix::identity());
    } else {
        stateset->addUniform(new osg::Uniform("viewInverse", osg::Matrix::identity()));
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