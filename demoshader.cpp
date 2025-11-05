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
#include "SkyNode.h"
#include "shaderpbr.h"
#include "shadercube.h"
#include "skyboxmanipulator.h"
#include "CloudSeaAtmosphere.h"
#include "VolumeCloudSky.h"
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
    , _cloudSeaDensity(0.8f)  // 初始云密度
    , _cloudSeaHeight(1000.0f)  // 初始云高度
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
osg::Node* DemoShader::createSkyboxAtmosphereScene(osgViewer::Viewer* viewer)
{
    if (!viewer) return nullptr;
    
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 创建天空盒
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    // 调整球体参数，使用更合适的半径
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), 100.0);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere);
    
    // 设置球体的细节级别，使其渲染更平滑
    drawable->setUseDisplayList(false);
    drawable->setUseVertexBufferObjects(true);
    
    geode->addDrawable(drawable);
    geode->setCullingActive(false);
    
    osg::ref_ptr<SkyBoxThree> skybox = new SkyBoxThree(viewer->getCamera());
    skybox->setName("skybox");
    skybox->addChild(geode.get());
    
    // 将天空盒添加到根节点
    if (skybox.valid()) {
        root->addChild(skybox);
    } else {
        std::cerr << "Failed to create skybox" << std::endl;
        return nullptr;
    }
    
    // 只在大气渲染的天空盒上使用SkyBoxManipulator
    // 为viewer设置自定义的SkyBoxManipulator操纵器
    osg::ref_ptr<SkyBoxManipulator> manipulator = new SkyBoxManipulator();
    viewer->setCameraManipulator(manipulator.get());
    
    std::cout << "Skybox atmosphere scene created successfully" << std::endl;
    std::cout << "SkyBoxManipulator enabled for atmosphere skybox" << std::endl;
    
    return root.release();
}

// 新增：创建使用改进大气着色器的场景
osg::Node* DemoShader::createImprovedAtmosphereScene(osgViewer::Viewer* viewer)
{
    if (!viewer) return nullptr;
    
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 创建一个球体几何体作为天空盒
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    // 调整球体参数，使用更合适的半径
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), 1000.0);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere);
    
    // 设置球体的细节级别，使其渲染更平滑
    drawable->setUseDisplayList(false);
    drawable->setUseVertexBufferObjects(true);
    
    geode->addDrawable(drawable);
    geode->setCullingActive(false);
    
    // 创建SkyBoxThree对象
    osg::ref_ptr<SkyBoxThree> skybox = new SkyBoxThree(viewer->getCamera());
    skybox->setName("improved_skybox");
    skybox->addChild(geode.get());
    
    // 将天空盒添加到根节点
    if (skybox.valid()) {
        root->addChild(skybox);
    } else {
        std::cerr << "Failed to create improved skybox" << std::endl;
        return nullptr;
    }
    
    std::cout << "Improved atmosphere scene created successfully" << std::endl;
     
    return root.release();
}

// 新增：创建PBR立方体
osg::Node* DemoShader::createPBRCube()
{
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 使用ShaderCube创建立方体
    osg::ref_ptr<osg::Node> cube = ShaderCube::createCube(1.0f);
    
    if (cube.valid()) {
        // 获取立方体的状态集
        osg::StateSet* cubeStateSet = cube->getOrCreateStateSet();
        
        // 使用ShaderPBR创建PBR着色器程序
        osg::ref_ptr<osg::Program> program = ShaderPBR::createPBRShaderSimpleIBL();
        if (program.valid()) {
            cubeStateSet->setAttributeAndModes(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }
        
        // 移除原有的着色器程序和纹理
        cubeStateSet->removeAttribute(osg::StateAttribute::PROGRAM);
        
        // 设置PBR材质参数
        cubeStateSet->addUniform(new osg::Uniform("albedo", osg::Vec3(0.8f, 0.2f, 0.2f))); // 红色基础颜色
        cubeStateSet->addUniform(new osg::Uniform("metallic", 0.0f)); // 非金属
        cubeStateSet->addUniform(new osg::Uniform("roughness", 0.5f)); // 中等粗糙度
        cubeStateSet->addUniform(new osg::Uniform("ao", 1.0f)); // 完全环境光遮蔽
        
        // 设置光源位置
        osg::ref_ptr<osg::Uniform> lightPositions = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "lightPositions", 4);
        lightPositions->setElement(0, osg::Vec3(5.0f, 5.0f, 5.0f));
        lightPositions->setElement(1, osg::Vec3(-5.0f, 5.0f, 5.0f));
        lightPositions->setElement(2, osg::Vec3(5.0f, -5.0f, 5.0f));
        lightPositions->setElement(3, osg::Vec3(-5.0f, -5.0f, 5.0f));
        cubeStateSet->addUniform(lightPositions);
        
        // 设置光源颜色
        osg::ref_ptr<osg::Uniform> lightColors = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "lightColors", 4);
        for(int i = 0; i < 4; i++) {
            lightColors->setElement(i, osg::Vec3(500.0f, 500.0f, 500.0f)); // 光源强度
        }
        cubeStateSet->addUniform(lightColors);
        
        // 设置相机位置
        cubeStateSet->addUniform(new osg::Uniform("camPos", osg::Vec3(0.0f, -5.0f, 0.0f)));
        
        // 设置渲染状态
        cubeStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        cubeStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        // 设置渲染顺序，确保立方体在天空盒之前渲染
        cubeStateSet->setRenderBinDetails(0, "RenderBin");
        
        root->addChild(cube);
    }
    
    std::cout << "PBR cube created successfully" << std::endl;
    
    return root.release();
}

// 新增：创建调试用的简单立方体（用于验证基本渲染功能）
osg::Node* DemoShader::createDebugCube()
{
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 使用ShaderCube创建立方体
    osg::ref_ptr<osg::Node> cube = ShaderCube::createCube(1.0f);
    
    if (cube.valid()) {
        // 获取立方体的状态集
        osg::StateSet* cubeStateSet = cube->getOrCreateStateSet();
        
        // 移除原有的着色器程序和纹理
        cubeStateSet->removeAttribute(osg::StateAttribute::PROGRAM);
        
        // 创建简单的调试着色器
        static const char* vertCode = R"(#version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            
            uniform mat4 osg_ModelViewProjectionMatrix;
            uniform mat4 osg_ModelViewMatrix;
            uniform mat3 osg_NormalMatrix;
            
            out vec3 WorldPos;
            out vec3 Normal;
            
            void main()
            {
                WorldPos = vec3(osg_ModelViewMatrix * vec4(aPos, 1.0));
                Normal = normalize(osg_NormalMatrix * aNormal);
                gl_Position = osg_ModelViewProjectionMatrix * vec4(aPos, 1.0);
            }
        )";
        
        static const char* fragCode = R"(#version 330 core
            out vec4 FragColor;
            in vec3 WorldPos;
            in vec3 Normal;
            
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            uniform vec3 lightColor;
            uniform vec3 objectColor;
            
            void main()
            {
                // 环境光
                float ambientStrength = 0.1;
                vec3 ambient = ambientStrength * lightColor;
                
                // 漫反射
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - WorldPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * lightColor;
                
                // 镜面反射
                float specularStrength = 0.5;
                vec3 viewDir = normalize(viewPos - WorldPos);
                vec3 reflectDir = reflect(-lightDir, norm);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
                vec3 specular = specularStrength * spec * lightColor;
                
                vec3 result = (ambient + diffuse + specular) * objectColor;
                FragColor = vec4(result, 1.0);
            }
        )";
        
        // 编译着色器
        osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX, vertCode);
        osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT, fragCode);
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(vertShader);
        program->addShader(fragShader);
        
        cubeStateSet->setAttributeAndModes(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        
        // 设置调试uniform变量
        cubeStateSet->addUniform(new osg::Uniform("objectColor", osg::Vec3(1.0f, 0.5f, 0.31f))); // 铜色
        cubeStateSet->addUniform(new osg::Uniform("lightPos", osg::Vec3(5.0f, 5.0f, 5.0f)));
        cubeStateSet->addUniform(new osg::Uniform("viewPos", osg::Vec3(0.0f, -5.0f, 0.0f)));
        cubeStateSet->addUniform(new osg::Uniform("lightColor", osg::Vec3(1.0f, 1.0f, 1.0f)));
        
        // 设置渲染状态
        cubeStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        cubeStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        cubeStateSet->setRenderBinDetails(0, "RenderBin");
        
        root->addChild(cube);
    }
    
    std::cout << "Debug cube created successfully" << std::endl;
    
    return root.release();
}

// 新增：创建结合天空盒大气和PBR立方体的场景
osg::Node* DemoShader::createSkyboxAtmosphereWithPBRScene(osgViewer::Viewer* viewer)
{
    if (!viewer) return nullptr;
    
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 创建一个球体几何体作为天空盒
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    // 使用较大的球体以包围整个场景
    geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), 1000.0)));
    geode->setCullingActive(false);
    
    // 创建SkyBoxThree对象
    osg::ref_ptr<SkyBoxThree> skybox = new SkyBoxThree(viewer->getCamera());
    skybox->setName("improved_skybox");
    skybox->addChild(geode.get());
    
    // 将天空盒添加到根节点
    if (skybox.valid()) {
        root->addChild(skybox);
    } else {
        std::cerr << "Failed to create improved skybox" << std::endl;
        return nullptr;
    }
    
    // 创建PBR立方体
    osg::ref_ptr<osg::Node> pbrCube = createPBRCube();
    if (pbrCube.valid()) {
        root->addChild(pbrCube);
    }
    // osg::ref_ptr<SkyBoxManipulator> manipulator = new SkyBoxManipulator();
    // viewer->setCameraManipulator(manipulator.get());
    std::cout << "Skybox atmosphere with PBR scene created successfully" << std::endl;
    
    return root.release();
}

// 新增：更新SkyNode大气参数的方法
void DemoShader::updateSkyNodeAtmosphereParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                               float turbidity, float rayleigh, float mieCoefficient, float mieDirectionalG,
                                               float sunZenithAngle, float sunAzimuthAngle)
{
    if (!viewer || !rootNode) return;
    
    std::cout << "Updating SkyNode atmosphere parameters..." << std::endl;
    std::cout << "  Turbidity: " << turbidity << ", Rayleigh: " << rayleigh 
              << ", Mie Coefficient: " << mieCoefficient << ", Mie Directional G: " << mieDirectionalG 
              << ", Sun Zenith Angle: " << sunZenithAngle << ", Sun Azimuth Angle: " << sunAzimuthAngle << std::endl;
    
    // 查找场景中的SkyBoxThree节点
    osg::Node* skyboxNode = nullptr;
    
    // 遍历根节点的所有子节点
    for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
        osg::Node* child = rootNode->getChild(i);
        if (!child) continue;
        
        // 首先检查当前子节点是否为SkyBoxThree
        if (child->getName() == "skybox" || child->getName() == "improved_skybox" || dynamic_cast<SkyBoxThree*>(child)) {
            skyboxNode = child;
            std::cout << "Found SkyBoxThree node at root level: " << child->getName() << std::endl;
            break;
        }
        
        // 如果当前子节点是Group，继续在其子节点中查找
        osg::Group* group = child->asGroup();
        if (group) {
            for (unsigned int j = 0; j < group->getNumChildren(); ++j) {
                osg::Node* grandChild = group->getChild(j);
                if (!grandChild) continue;
                
                // 检查孙节点是否为SkyBoxThree
                if (grandChild->getName() == "skybox" || grandChild->getName() == "improved_skybox" || dynamic_cast<SkyBoxThree*>(grandChild)) {
                    skyboxNode = grandChild;
                    std::cout << "Found SkyBoxThree node nested in group: " << grandChild->getName() << std::endl;
                    break;
                }
            }
            
            // 如果找到了就退出外层循环
            if (skyboxNode) break;
        }
    }
    
    // 如果还是没有找到，尝试更广泛的搜索
    if (!skyboxNode) {
        std::cout << "Performing extensive search for SkyBoxThree nodes..." << std::endl;
        for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
            osg::Node* child = rootNode->getChild(i);
            if (!child) continue;
            
            // 递归搜索所有子节点
            skyboxNode = findSkyBoxThreeNode(child);
            if (skyboxNode) {
                std::cout << "Found SkyBoxThree node through recursive search: " << skyboxNode->getName() << std::endl;
                break;
            }
        }
    }
    
    if (skyboxNode) {
        // 获取SkyBoxThree节点的状态集
        osg::StateSet* stateset = skyboxNode->getOrCreateStateSet();
        if (stateset) {
            std::cout << "StateSet acquired successfully" << std::endl;
            
            // 更新uniform变量
            // 注意：这里我们直接通过stateset来更新uniform，而不是通过SkyBoxThree的成员变量
            osg::Uniform* turbidityUniform = stateset->getUniform("turbidity");
            if (turbidityUniform) {
                turbidityUniform->set(turbidity);
                std::cout << "Turbidity uniform updated to: " << turbidity << std::endl;
            } else {
                std::cout << "Turbidity uniform not found" << std::endl;
            }
            
            osg::Uniform* rayleighUniform = stateset->getUniform("rayleigh");
            if (rayleighUniform) {
                rayleighUniform->set(rayleigh);
                std::cout << "Rayleigh uniform updated to: " << rayleigh << std::endl;
            } else {
                std::cout << "Rayleigh uniform not found" << std::endl;
            }
            
            osg::Uniform* mieCoefficientUniform = stateset->getUniform("mieCoefficient");
            if (mieCoefficientUniform) {
                mieCoefficientUniform->set(mieCoefficient);
                std::cout << "Mie Coefficient uniform updated to: " << mieCoefficient << std::endl;
            } else {
                std::cout << "Mie Coefficient uniform not found" << std::endl;
            }
            
            osg::Uniform* mieDirectionalGUniform = stateset->getUniform("mieDirectionalG");
            if (mieDirectionalGUniform) {
                mieDirectionalGUniform->set(mieDirectionalG);
                std::cout << "Mie Directional G uniform updated to: " << mieDirectionalG << std::endl;
            } else {
                std::cout << "Mie Directional G uniform not found" << std::endl;
            }
            
            // 新增：更新太阳天顶角度uniform
            osg::Uniform* sunZenithAngleUniform = stateset->getUniform("sunZenithAngle");
            if (sunZenithAngleUniform) {
                sunZenithAngleUniform->set(sunZenithAngle);
                std::cout << "Sun Zenith Angle uniform updated to: " << sunZenithAngle << std::endl;
            } else {
                std::cout << "Sun Zenith Angle uniform not found" << std::endl;
            }
            
            // 新增：更新太阳方位角度uniform
            osg::Uniform* sunAzimuthAngleUniform = stateset->getUniform("sunAzimuthAngle");
            if (sunAzimuthAngleUniform) {
                sunAzimuthAngleUniform->set(sunAzimuthAngle);
                std::cout << "Sun Azimuth Angle uniform updated to: " << sunAzimuthAngle << std::endl;
            } else {
                std::cout << "Sun Azimuth Angle uniform not found" << std::endl;
            }
        }
    } else {
        std::cerr << "Failed to find SkyBoxThree node in the scene" << std::endl;
        // 打印所有子节点的信息以便调试
        std::cout << "Scene children count: " << rootNode->getNumChildren() << std::endl;
        for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
            osg::Node* child = rootNode->getChild(i);
            if (child) {
                std::cout << "  Child " << i << ": " << child->getName() << " (class: " << child->className() << ")" << std::endl;
                // 如果是Group，打印其子节点
                osg::Group* group = child->asGroup();
                if (group) {
                    std::cout << "    Group children count: " << group->getNumChildren() << std::endl;
                    for (unsigned int j = 0; j < group->getNumChildren(); ++j) {
                        osg::Node* grandChild = group->getChild(j);
                        if (grandChild) {
                            std::cout << "      Grandchild " << j << ": " << grandChild->getName() << " (class: " << grandChild->className() << ")" << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    // 强制更新视图
    if (viewer) {
        std::cout << "Requesting redraw..." << std::endl;
        viewer->advance();
        viewer->requestRedraw();
    }
}

// 新增：更新SkyNode云海大气参数的方法
void DemoShader::updateSkyNodeCloudParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                           float sunZenithAngle, float sunAzimuthAngle,
                                           float cloudDensity, float cloudHeight,
                                           float cloudBaseHeight, float cloudRangeMin, float cloudRangeMax)
{
    if (!viewer || !rootNode) return;
    
    std::cout << "Updating SkyNode cloud parameters..." << std::endl;
    std::cout << "  Sun Zenith Angle: " << sunZenithAngle << ", Sun Azimuth Angle: " << sunAzimuthAngle 
              << ", Cloud Density: " << cloudDensity << ", Cloud Height: " << cloudHeight
              << ", Cloud Base Height: " << cloudBaseHeight << ", Cloud Range Min: " << cloudRangeMin
              << ", Cloud Range Max: " << cloudRangeMax << std::endl;
    
    // 查找场景中的SkyBoxThree节点
    osg::Node* skyboxNode = nullptr;
    
    // 遍历根节点的所有子节点
    for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
        osg::Node* child = rootNode->getChild(i);
        if (!child) continue;
        
        // 首先检查当前子节点是否为SkyBoxThree
        if (child->getName() == "skybox" || child->getName() == "improved_skybox" || dynamic_cast<SkyBoxThree*>(child)) {
            skyboxNode = child;
            std::cout << "Found SkyBoxThree node at root level: " << child->getName() << std::endl;
            break;
        }
        
        // 如果当前子节点是Group，继续在其子节点中查找
        osg::Group* group = child->asGroup();
        if (group) {
            for (unsigned int j = 0; j < group->getNumChildren(); ++j) {
                osg::Node* grandChild = group->getChild(j);
                if (!grandChild) continue;
                
                // 检查孙节点是否为SkyBoxThree
                if (grandChild->getName() == "skybox" || grandChild->getName() == "improved_skybox" || dynamic_cast<SkyBoxThree*>(grandChild)) {
                    skyboxNode = grandChild;
                    std::cout << "Found SkyBoxThree node nested in group: " << grandChild->getName() << std::endl;
                    break;
                }
            }
            
            // 如果找到了就退出外层循环
            if (skyboxNode) break;
        }
    }
    
    // 如果还是没有找到，尝试更广泛的搜索
    if (!skyboxNode) {
        std::cout << "Performing extensive search for SkyBoxThree nodes..." << std::endl;
        for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
            osg::Node* child = rootNode->getChild(i);
            if (!child) continue;
            
            // 递归搜索所有子节点
            skyboxNode = findSkyBoxThreeNode(child);
            if (skyboxNode) {
                std::cout << "Found SkyBoxThree node through recursive search: " << skyboxNode->getName() << std::endl;
                break;
            }
        }
    }
    
    if (skyboxNode) {
        // 获取SkyBoxThree节点的状态集
        osg::StateSet* stateset = skyboxNode->getOrCreateStateSet();
        if (stateset) {
            std::cout << "StateSet acquired successfully" << std::endl;
            
            // 更新太阳角度uniform变量
            osg::Uniform* sunZenithAngleUniform = stateset->getUniform("sunZenithAngle");
            if (sunZenithAngleUniform) {
                sunZenithAngleUniform->set(sunZenithAngle);
                std::cout << "Sun Zenith Angle uniform updated to: " << sunZenithAngle << std::endl;
            } else {
                std::cout << "Sun Zenith Angle uniform not found" << std::endl;
            }
            
            osg::Uniform* sunAzimuthAngleUniform = stateset->getUniform("sunAzimuthAngle");
            if (sunAzimuthAngleUniform) {
                sunAzimuthAngleUniform->set(sunAzimuthAngle);
                std::cout << "Sun Azimuth Angle uniform updated to: " << sunAzimuthAngle << std::endl;
            } else {
                std::cout << "Sun Azimuth Angle uniform not found" << std::endl;
            }
            
            // 更新云海参数uniform变量
            osg::Uniform* cloudDensityUniform = stateset->getUniform("cloudDensity");
            if (cloudDensityUniform) {
                cloudDensityUniform->set(cloudDensity);
                std::cout << "Cloud Density uniform updated to: " << cloudDensity << std::endl;
            } else {
                std::cout << "Cloud Density uniform not found" << std::endl;
            }
            
            osg::Uniform* cloudHeightUniform = stateset->getUniform("cloudHeight");
            if (cloudHeightUniform) {
                cloudHeightUniform->set(cloudHeight);
                std::cout << "Cloud Height uniform updated to: " << cloudHeight << std::endl;
            } else {
                std::cout << "Cloud Height uniform not found" << std::endl;
            }
            
            osg::Uniform* cloudBaseHeightUniform = stateset->getUniform("cloudBaseHeight");
            if (cloudBaseHeightUniform) {
                cloudBaseHeightUniform->set(cloudBaseHeight);
                std::cout << "Cloud Base Height uniform updated to: " << cloudBaseHeight << std::endl;
            } else {
                std::cout << "Cloud Base Height uniform not found" << std::endl;
            }
            
            osg::Uniform* cloudRangeMinUniform = stateset->getUniform("cloudRangeMin");
            if (cloudRangeMinUniform) {
                cloudRangeMinUniform->set(cloudRangeMin);
                std::cout << "Cloud Range Min uniform updated to: " << cloudRangeMin << std::endl;
            } else {
                std::cout << "Cloud Range Min uniform not found" << std::endl;
            }
            
            osg::Uniform* cloudRangeMaxUniform = stateset->getUniform("cloudRangeMax");
            if (cloudRangeMaxUniform) {
                cloudRangeMaxUniform->set(cloudRangeMax);
                std::cout << "Cloud Range Max uniform updated to: " << cloudRangeMax << std::endl;
            } else {
                std::cout << "Cloud Range Max uniform not found" << std::endl;
            }
        }
    } else {
        std::cerr << "Failed to find SkyBoxThree node in the scene" << std::endl;
        // 打印所有子节点的信息以便调试
        std::cout << "Scene children count: " << rootNode->getNumChildren() << std::endl;
        for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
            osg::Node* child = rootNode->getChild(i);
            if (child) {
                std::cout << "  Child " << i << ": " << child->getName() << " (class: " << child->className() << ")" << std::endl;
                // 如果是Group，打印其子节点
                osg::Group* group = child->asGroup();
                if (group) {
                    std::cout << "    Group children count: " << group->getNumChildren() << std::endl;
                    for (unsigned int j = 0; j < group->getNumChildren(); ++j) {
                        osg::Node* grandChild = group->getChild(j);
                        if (grandChild) {
                            std::cout << "      Grandchild " << j << ": " << grandChild->getName() << " (class: " << grandChild->className() << ")" << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    // 强制更新视图
    if (viewer) {
        std::cout << "Requesting redraw..." << std::endl;
        viewer->advance();
        viewer->requestRedraw();
    }
}

// 辅助函数：递归查找SkyBoxThree节点
osg::Node* DemoShader::findSkyBoxThreeNode(osg::Node* node)
{
    if (!node) return nullptr;
    
    // 检查当前节点是否为SkyBoxThree
    if (node->getName() == "skybox" || node->getName() == "improved_skybox" || dynamic_cast<SkyBoxThree*>(node)) {
        return node;
    }
    
    // 如果是Group，递归检查子节点
    osg::Group* group = node->asGroup();
    if (group) {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
            osg::Node* found = findSkyBoxThreeNode(group->getChild(i));
            if (found) return found;
        }
    }
    
    return nullptr;
}

// 新增：创建体积云天空盒场景
osg::Node* DemoShader::createVolumeCloudSkyScene(osgViewer::Viewer* viewer)
{
    if (!viewer) return nullptr;
    
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 创建一个球体几何体作为天空盒
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), 5000.0);  // 调整球体半径为1000.0
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere);
    
    drawable->setUseDisplayList(false);
    drawable->setUseVertexBufferObjects(true);
    
    geode->addDrawable(drawable);
    geode->setCullingActive(false);
    
    // 创建体积云天空盒对象
    osg::ref_ptr<VolumeCloudSky> volumeCloudSky = new VolumeCloudSky(viewer->getCamera());
    volumeCloudSky->setName("volume_cloud_sky");
    volumeCloudSky->addChild(geode.get());
    
    // 将天空盒添加到根节点
    if (volumeCloudSky.valid()) {
        root->addChild(volumeCloudSky);
    } else {
        std::cerr << "Failed to create volume cloud sky" << std::endl;
        return nullptr;
    }
    
    std::cout << "Volume Cloud Sky scene created successfully" << std::endl;
    
    return root.release();
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
    
    // 设置up向量uniform - 使用Z轴向上以匹配OSG坐标系
    osg::ref_ptr<osg::Uniform> upUniform = stateset->getUniform("up");
    if (upUniform.valid()) {
        upUniform->set(osg::Vec3(0.0f, 0.0f, 1.0f));
    } else {
        stateset->addUniform(new osg::Uniform("up", osg::Vec3(0.0f, 0.0f, 1.0f)));
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

// 新增：创建云海大气效果场景
osg::Node* DemoShader::createCloudSeaAtmosphereScene(osgViewer::Viewer* viewer)
{
    if (!viewer) return nullptr;
    
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 创建一个球体几何体作为天空盒
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), 1000.0);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere);
    
    drawable->setUseDisplayList(false);
    drawable->setUseVertexBufferObjects(true);
    
    geode->addDrawable(drawable);
    geode->setCullingActive(false);
    
    // 创建云海大气效果对象并保存引用
    _cloudSeaAtmosphere = new CloudSeaAtmosphere(viewer->getCamera());
    _cloudSeaAtmosphere->setName("cloud_sea_skybox");
    _cloudSeaAtmosphere->addChild(geode.get());
    
    // 创建初始云海参数
    _cloudSeaAtmosphere->setCloudDensity(_cloudSeaDensity);
    _cloudSeaAtmosphere->setCloudHeight(_cloudSeaHeight);
    
    // 将天空盒添加到根节点
    if (_cloudSeaAtmosphere.valid()) {
        root->addChild(_cloudSeaAtmosphere.get());
    } else {
        std::cerr << "Failed to create cloud sea atmosphere" << std::endl;
        return nullptr;
    }
    
    // 仅在云海大气的天空盒上使用SkyBoxManipulator
    osg::ref_ptr<SkyBoxManipulator> manipulator = new SkyBoxManipulator();
    viewer->setCameraManipulator(manipulator.get());
    
    std::cout << "Cloud Sea Atmosphere scene created successfully" << std::endl;
    std::cout << "SkyBoxManipulator enabled for cloud sea atmosphere" << std::endl;
    
    return root.release();
}

void DemoShader::updateCloudSeaAtmosphereParameters(float sunZenithAngle, float sunAzimuthAngle,
                                                   float cloudDensity, float cloudHeight,
                                                   float cloudBaseHeight, float cloudRangeMin, float cloudRangeMax)
{
    if (!_cloudSeaAtmosphere.valid()) {
        std::cerr << "Cloud sea atmosphere is not initialized" << std::endl;
        return;
    }
    
    // 更新太阳角度
    _cloudSeaAtmosphere->setSunZenithAngle(sunZenithAngle);
    _cloudSeaAtmosphere->setSunAzimuthAngle(sunAzimuthAngle);
    
    // 更新云海参数
    _cloudSeaAtmosphere->setCloudDensity(cloudDensity);
    _cloudSeaAtmosphere->setCloudHeight(cloudHeight);
    // 注意：CloudSeaAtmosphere类目前不支持cloudBaseHeight, cloudRangeMin, cloudRangeMax参数
    // 这些参数将在SkyNode中使用
    
    // 保存到成员变量
    _cloudSeaDensity = cloudDensity;
    _cloudSeaHeight = cloudHeight;
    _sunZenithAngleRadians = sunZenithAngle;
    _sunAzimuthAngleRadians = sunAzimuthAngle;
}

// 新增：创建使用CloudVolume着色器的云海大气场景

// 新增：更新体积云参数
void DemoShader::updateVolumeCloudParameters(osgViewer::Viewer* viewer, osg::Group* rootNode,
                                          float sunZenithAngle, float sunAzimuthAngle,
                                          float cloudDensity, float cloudHeight,
                                          float densityThreshold, float contrast, float densityFactor,
                                          float stepSize, float maxSteps)
{
    if (!viewer || !rootNode) return;
    
    std::cout << "Updating Volume Cloud parameters..." << std::endl;
    std::cout << "  Sun Zenith Angle: " << sunZenithAngle << ", Sun Azimuth Angle: " << sunAzimuthAngle 
              << ", Cloud Density: " << cloudDensity << ", Cloud Height: " << cloudHeight
              << ", Density Threshold: " << densityThreshold << ", Contrast: " << contrast
              << ", Density Factor: " << densityFactor << ", Step Size: " << stepSize
              << ", Max Steps: " << maxSteps << std::endl;
    
    // 查找场景中的VolumeCloudSky节点
    osg::Node* volumeCloudNode = nullptr;
    
    // 遍历根节点的所有子节点
    for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
        osg::Node* child = rootNode->getChild(i);
        if (!child) continue;
        
        // 首先检查当前子节点是否为VolumeCloudSky
        if (child->getName() == "volume_cloud_sky" || dynamic_cast<VolumeCloudSky*>(child)) {
            volumeCloudNode = child;
            std::cout << "Found VolumeCloudSky node at root level: " << child->getName() << std::endl;
            break;
        }
        
        // 如果当前子节点是Group，继续在其子节点中查找
        osg::Group* group = child->asGroup();
        if (group) {
            for (unsigned int j = 0; j < group->getNumChildren(); ++j) {
                osg::Node* grandChild = group->getChild(j);
                if (!grandChild) continue;
                
                // 检查孙节点是否为VolumeCloudSky
                if (grandChild->getName() == "volume_cloud_sky" || dynamic_cast<VolumeCloudSky*>(grandChild)) {
                    volumeCloudNode = grandChild;
                    std::cout << "Found VolumeCloudSky node nested in group: " << grandChild->getName() << std::endl;
                    break;
                }
            }
            
            // 如果找到了就退出外层循环
            if (volumeCloudNode) break;
        }
    }
    
    // 如果还是没有找到，尝试更广泛的搜索
    if (!volumeCloudNode) {
        std::cout << "Performing extensive search for VolumeCloudSky nodes..." << std::endl;
        for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
            osg::Node* child = rootNode->getChild(i);
            if (!child) continue;
            
            // 递归搜索所有子节点
            volumeCloudNode = findVolumeCloudSkyNode(child);
            if (volumeCloudNode) {
                std::cout << "Found VolumeCloudSky node through recursive search: " << volumeCloudNode->getName() << std::endl;
                break;
            }
        }
    }
    
    if (volumeCloudNode) {
        // 将节点转换为VolumeCloudSky类型并更新参数
        VolumeCloudSky* volumeCloudSky = dynamic_cast<VolumeCloudSky*>(volumeCloudNode);
        if (volumeCloudSky) {
            volumeCloudSky->setSunZenithAngle(sunZenithAngle);
            volumeCloudSky->setSunAzimuthAngle(sunAzimuthAngle);
            volumeCloudSky->setCloudDensity(cloudDensity);
            volumeCloudSky->setDensityThreshold(densityThreshold);
            volumeCloudSky->setContrast(contrast);
            volumeCloudSky->setDensityFactor(densityFactor);
            volumeCloudSky->setStepSize(stepSize);
            volumeCloudSky->setMaxSteps((int)maxSteps);
            std::cout << "Volume Cloud parameters updated successfully" << std::endl;
        } else {
            std::cerr << "Failed to cast node to VolumeCloudSky" << std::endl;
        }
    } else {
        std::cerr << "Failed to find VolumeCloudSky node in the scene" << std::endl;
        // 打印所有子节点的信息以便调试
        std::cout << "Scene children count: " << rootNode->getNumChildren() << std::endl;
        for (unsigned int i = 0; i < rootNode->getNumChildren(); ++i) {
            osg::Node* child = rootNode->getChild(i);
            if (child) {
                std::cout << "  Child " << i << ": " << child->getName() << " (class: " << child->className() << ")" << std::endl;
                // 如果是Group，打印其子节点
                osg::Group* group = child->asGroup();
                if (group) {
                    std::cout << "    Group children count: " << group->getNumChildren() << std::endl;
                    for (unsigned int j = 0; j < group->getNumChildren(); ++j) {
                        osg::Node* grandChild = group->getChild(j);
                        if (grandChild) {
                            std::cout << "      Grandchild " << j << ": " << grandChild->getName() << " (class: " << grandChild->className() << ")" << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    // 强制更新视图
    if (viewer) {
        std::cout << "Requesting redraw..." << std::endl;
        viewer->advance();
        viewer->requestRedraw();
    }
}

// 辅助函数：递归查找VolumeCloudSky节点
osg::Node* DemoShader::findVolumeCloudSkyNode(osg::Node* node)
{
    if (!node) return nullptr;
    
    // 检查当前节点是否为VolumeCloudSky
    if (node->getName() == "volume_cloud_sky" || dynamic_cast<VolumeCloudSky*>(node)) {
        return node;
    }
    
    // 如果是Group，递归检查子节点
    osg::Group* group = node->asGroup();
    if (group) {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
            osg::Node* found = findVolumeCloudSkyNode(group->getChild(i));
            if (found) return found;
        }
    }
    
    return nullptr;
}
