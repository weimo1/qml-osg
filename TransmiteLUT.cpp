#include "TransmiteLUT.h"
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/StateSet>
#include <osg/Depth>
#include <osg/Program>
#include <osg/Shader>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <cmath>
#include <fstream>
#include <sstream>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/Program>
#include <osg/Shader>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osg/Camera>
#include <osg/GL>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <QFile>
#include<iostream>

TransmiteLUT::TransmiteLUT()
    : m_camera(nullptr)
    , m_rootNode(nullptr)
{
    
}

TransmiteLUT::~TransmiteLUT()
{
    // 析构函数可以为空，因为所有OSG对象都使用ref_ptr管理
}

void TransmiteLUT::setCamera(osg::Camera* camera)
{
    m_camera = camera;
}


void TransmiteLUT::generateLUT()
{
    // 初始化LUT纹理
    transmittanceLUT = new osg::Texture2D;
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(64, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE);  // 使用浮点格式
    transmittanceLUT->setImage(image);
    transmittanceLUT->setTextureSize(64, 64);
    transmittanceLUT->setInternalFormat(GL_RGBA);  // 使用32位浮点格式
    transmittanceLUT->setSourceFormat(GL_RGBA);
    transmittanceLUT->setSourceType(GL_FLOAT);
    transmittanceLUT->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    transmittanceLUT->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    transmittanceLUT->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    transmittanceLUT->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    // 确保纹理可以被读取
    transmittanceLUT->setUnRefImageDataAfterApply(false);

    if (!transmittanceLUT) {
        osg::notify(osg::WARN) << "LUT texture is not initialized." << std::endl;
        return;
    }

    osg::notify(osg::INFO) << "Starting LUT generation..." << std::endl;

    // 创建RTT相机
    osg::ref_ptr<osg::Camera> rttCamera = new osg::Camera;
    rttCamera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    rttCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rttCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    rttCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    
    // 设置视口和投影矩阵
    rttCamera->setViewport(0, 0, 64, 64);
    rttCamera->setProjectionMatrixAsOrtho2D(-1, 1, -1, 1);
    
    // 将纹理附加到相机
    rttCamera->attach(osg::Camera::COLOR_BUFFER, transmittanceLUT.get());
    rttCamera->attach(osg::Camera::COLOR_BUFFER, image.get());
    
    // 创建一个四边形几何体
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> geometry = osg::createTexturedQuadGeometry(
        osg::Vec3(-1.0f, -1.0f, 0.0f),
        osg::Vec3(2.0f, 0.0f, 0.0f),
        osg::Vec3(0.0f, 2.0f, 0.0f)
    );
    geode->addDrawable(geometry.get());
    
    // 获取状态集并应用着色器
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;
    
    // 从文件读取着色器
    osg::ref_ptr<osg::Shader> vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, "e:/qt test/qml-osg/shader/LUT.vert");
    osg::ref_ptr<osg::Shader> fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "e:/qt test/qml-osg/shader/LUT.frag");
    
    // 检查着色器是否加载成功
    if (!vertexShader.valid() || !fragmentShader.valid()) {
        osg::notify(osg::WARN) << "Failed to load shader files." << std::endl;
        return;
    }
    
    // 将着色器添加到程序
    program->addShader(vertexShader.get());
    program->addShader(fragmentShader.get());
    
    osg::ref_ptr<osg::Texture2D> transmittanceLUT = new osg::Texture2D;
    osg::ref_ptr<osg::Image> transmittanceImage = osgDB::readImageFile("e:/qt test/qml-osg/shader/LUT.png");

    if (transmittanceImage.valid()) {
        transmittanceLUT->setImage(transmittanceImage);
        transmittanceLUT->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        transmittanceLUT->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        transmittanceLUT->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        transmittanceLUT->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);


        stateSet->setTextureAttributeAndModes(0, transmittanceLUT, osg::StateAttribute::ON);
        stateSet->addUniform(new osg::Uniform("transmittanceLUT", 0));
        
        printf("Transmittance texture loaded and configured successfully\n");
    } else {
        printf("Failed to load transmittance texture\n");
    }



    // 将程序添加到状态集

    stateSet->setAttribute(program.get());
    
    // 将几何体添加到相机
    rttCamera->addChild(geode.get());
    
    // 创建viewer并设置场景
    osgViewer::Viewer viewer;
    viewer.setSceneData(rttCamera.get());
    viewer.realize();
    
    // 渲染足够的帧以确保纹理生成
    for (int i = 0; i < 5; ++i) {
        viewer.frame();
    }
    glFinish();

}

void TransmiteLUT::exportLUT(const std::string& filename)
{
    osg::Texture2D* texture = transmittanceLUT.get();
    if (texture) {
        osg::Image* image = texture->getImage();
        if (image) {
            // 设置像素存储模式
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            // 从帧缓冲区读取像素数据
            image->readPixels(0, 0, image->s(), image->t(), image->getPixelFormat(), image->getDataType());
            
            // 保存图像文件
            if (osgDB::writeImageFile(*image, filename)) {
                osg::notify(osg::INFO) << "LUT图像成功保存到: " << filename << std::endl;
            } else {
                osg::notify(osg::WARN) << "保存LUT图像失败: " << filename << std::endl;
            }
        }
    }
}

osg::Camera* TransmiteLUT::createHUDCamera(double left, double right, double bottom, double top)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setAllowEventFocus(false);
    camera->setProjectionMatrix(osg::Matrix::ortho2D(left, right, bottom, top));
    camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    return camera.release();
}