#include "AtmosphereDemo.h"
#include <osg/Geometry>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/Image>
#include <osgDB/ReadFile>
#include <osg/StateSet>
#include <osg/Geode>
#include <osg/Camera>
#include <osg/Group>
#include <osg/Matrix>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Uniform>
#include <cstdio>
#include <vector>
#include <cmath>

AtmosphereDemo::AtmosphereDemo()
    : m_sunZenithAngle(1.3)  // 天顶角约75度
    , m_sunAzimuthAngle(2.9)  // 方位角约166度
    , m_viewDistance(9000.0)
    , m_viewZenithAngle(1.47)
    , m_viewAzimuthAngle(-0.1)
    , m_exposure(1.0f)  // 更合理的曝光值
    , m_initialized(true)  // 初始化标志，直接设置为true
{
    // 构造函数体为空，所有初始化都在初始化列表中完成
}

AtmosphereDemo::~AtmosphereDemo()
{
    // 析构函数可以为空，因为所有OSG对象都使用ref_ptr管理
}

osg::Node* AtmosphereDemo::createAtmosphere(osg::Node* subgraph, osg::Camera* camera, const osg::Vec4& clearColour)
{
  
 
    osg::ref_ptr<osg::Texture2D> textures = new osg::Texture2D;
    osg::ref_ptr<osg::Camera> mrtCam = createRTTCamera(textures);
    mrtCam->addChild(subgraph);

    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    osg::Geometry* geom = osg::createTexturedQuadGeometry(
        osg::Vec3(-1.0f, -1.0f, 0.0f),
        osg::Vec3(2.0f, 0.0f, 0.0f),
        osg::Vec3(0.0f, 2.0f, 0.0f));

    geom->setVertexAttribArray(0, geom->getVertexArray(), osg::Array::BIND_PER_VERTEX);
    geom->setVertexAttribArray(1, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);

    osg::StateSet* ss = geom->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(3, textures.get());

    // 加载预计算的纹理 - 使用正确的路径
    osg::Texture* pT1 = createTexture(4, "shader/transmittance.dat", 256, 64, 0);
    if (pT1) ss->setTextureAttributeAndModes(0, pT1);
    
    osg::Texture* pT2 = createTexture(4, "shader/scattering.dat", 256, 128, 32);
    if (pT2) ss->setTextureAttributeAndModes(1, pT2);
    
   osg::Texture* pT3 = createTexture(4, "shader/irradiance.dat", 64, 16, 0);
    if (pT3) ss->setTextureAttributeAndModes(2, pT3);

    createAtmosphereEffect(ss, camera);
    
    // 初始化云纹理
    initializeCloudTextures(ss);

    quad->addDrawable(geom);
    // // 设置节点掩码以避免计算包围盒
    //quad->setNodeMask(0x0);

    osg::Camera* hudCam = createHUDCamera(0.0, 1.0, 0.0, 1.0);
    if (!hudCam) {
        printf("Failed to create HUD camera\n");
        return nullptr;
    }
    
    hudCam->addChild(quad.get());

    // 构建场景图 - 添加MRT相机和HUD相机
    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (!root) {
        printf("Failed to create root group\n");
        return nullptr;
    }
    
    // 添加MRT相机来渲染场景到纹理
    //root->addChild(mrtCam.get());
    // 添加HUD相机来显示纹理
    root->addChild(hudCam);

    return root.release();
}

osg::Texture* AtmosphereDemo::createTexture(int format, const std::string& fileName, int nW, int nH, int nT)
{
    FILE* fp = nullptr;
    long long fileSize = 0;

    // 打开文件 - 首先尝试直接路径
    errno_t err = fopen_s(&fp, fileName.c_str(), "rb");
    if (err != 0) {
        // 文件打开失败，尝试在项目目录中查找
        std::string projectPath = "E:/qt test/qml-osg/";
        std::string fullPath = projectPath + fileName;
        err = fopen_s(&fp, fullPath.c_str(), "rb");
        if (err != 0) {
            // 再次尝试在shader子目录中查找
            std::string shaderPath = projectPath + "shader/" + fileName;
            err = fopen_s(&fp, shaderPath.c_str(), "rb");
            if (err != 0) {
                printf("Failed to open texture file: %s\n", fileName.c_str());
                // 返回一个默认的纹理而不是nullptr
                osg::Texture2D* defaultTex = new osg::Texture2D;
                defaultTex->setTextureSize(256, 256);
                defaultTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
                defaultTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
                return defaultTex;
            }
        }
    }

    // 获取文件大小
    _fseeki64(fp, 0, SEEK_END);
    fileSize = _ftelli64(fp);
    _fseeki64(fp, 0, SEEK_SET);

    int eleSize = sizeof(float);
    int eleCount = static_cast<int>(fileSize / eleSize);

    // 检查文件大小是否合理
    if (eleCount <= 0) {
        printf("Invalid texture file size: %s\n", fileName.c_str());
        fclose(fp);
        // 返回一个默认的纹理而不是nullptr
        osg::Texture2D* defaultTex = new osg::Texture2D;
        defaultTex->setTextureSize(256, 256);
        defaultTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        defaultTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        return defaultTex;
    }

    std::vector<float> datas;
    datas.resize(eleCount);

    size_t result = fread(datas.data(), eleSize, eleCount, fp);
    fclose(fp);

    if (result != eleCount) {
        printf("Failed to read texture data from file: %s\n", fileName.c_str());
        // 返回一个默认的纹理而不是nullptr
        osg::Texture2D* defaultTex = new osg::Texture2D;
        defaultTex->setTextureSize(256, 256);
        defaultTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        defaultTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        return defaultTex;
    }

    if (format == 4)
    {
        if (nT == 0)
        {
            osg::Image* iImage = new osg::Image;
            iImage->allocateImage(nW, nH, 1, GL_RGBA, GL_FLOAT);
            osg::Vec4f* ptr = (osg::Vec4f*)iImage->data();
            for (int i = 0; i < eleCount / 4; i++)
            {
                ptr[0].set(datas[4 * i + 0], datas[4 * i + 1], datas[4 * i + 2], datas[4 * i + 3]);
                ptr++;
            }

            osg::Texture2D* pTex = new osg::Texture2D;
            pTex->setImage(iImage);
            pTex->setInternalFormat(GL_RGBA32F_ARB);
            pTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            pTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            pTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
            pTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

            printf("Texture loaded successfully: %s\n", fileName.c_str());  // 添加成功加载纹理的消息
            return pTex;
        }
        else
        {
            osg::Image* iImage = new osg::Image;
            iImage->allocateImage(nW, nH, nT, GL_RGBA, GL_FLOAT);
            osg::Vec4f* ptr = (osg::Vec4f*)iImage->data();
            for (int i = 0; i < eleCount / 4; i++)
            {
                ptr[0].set(datas[4 * i + 0], datas[4 * i + 1], datas[4 * i + 2], datas[4 * i + 3]);
                ptr++;
            }
            osg::Texture3D* pTex = new osg::Texture3D;
            pTex->setImage(iImage);
            pTex->setInternalFormat(GL_RGBA32F_ARB);
            pTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            pTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            pTex->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
            pTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
            pTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

            printf("3D Texture loaded successfully: %s\n", fileName.c_str());  // 添加成功加载3D纹理的消息
            return pTex;
        }
    }
    if (format == 3)
    {
        if (nT == 0)
        {
            osg::Image* iImage = new osg::Image;
            iImage->allocateImage(nW, nH, 1, GL_RGB, GL_FLOAT);
            osg::Vec3f* ptr = (osg::Vec3f*)iImage->data();
            for (int i = 0; i < eleCount / 3; i++)
            {
                ptr[0].set(datas[3 * i + 0], datas[3 * i + 1], datas[3 * i + 2]);
                ptr++;
            }

            osg::Texture2D* pTex = new osg::Texture2D;
            pTex->setImage(iImage);
            iImage->flipVertical();
            pTex->setInternalFormat(GL_RGB32F_ARB);
            pTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            pTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            pTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
            pTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

            printf("RGB Texture loaded successfully: %s\n", fileName.c_str());  // 添加成功加载RGB纹理的消息
            return pTex;
        }
    }

    // 如果所有格式都不匹配，返回默认纹理
    osg::Texture2D* defaultTex = new osg::Texture2D;
    defaultTex->setTextureSize(256, 256);
    defaultTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    defaultTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    return defaultTex;
}

osg::Camera* AtmosphereDemo::createRTTCamera(osg::ref_ptr<osg::Texture2D>& tex)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);

    tex = new osg::Texture2D;
    tex->setTextureSize(1024, 1024);
    tex->setInternalFormat(GL_RGBA);
    tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

    camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    camera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0), tex.get());
    return camera.release();
}

osg::Camera* AtmosphereDemo::createHUDCamera(double left, double right, double bottom, double top)
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

void AtmosphereDemo::createAtmosphereEffect(osg::StateSet* ss, osg::Camera* camera)
{
    // 读取着色器文件
    std::string strDir = "E:/qt test/qml-osg/shader/";
    
    osg::Program* atmosphereProgram = new osg::Program;
    // 使用MRT测试着色器
    osg::Shader* pV = osgDB::readShaderFile(osg::Shader::VERTEX, strDir + "demo.vert");
    osg::Shader* pF = osgDB::readShaderFile(osg::Shader::FRAGMENT, strDir + "demo.frag");
    
   
    if (pV) {
            pV->setName("atmosphere.vert");
            atmosphereProgram->addShader(pV);
            printf("Vertex shader loaded successfully\n");
        }
        
        if (pF) {
            pF->setName("atmosphere.frag");
            atmosphereProgram->addShader(pF);
            printf("Fragment shader loaded successfully\n");
        }
    
    ss->setAttributeAndModes(atmosphereProgram);
 
    // 设置纹理单元 - groundTexture使用纹理单元3
    ss->setUpdateCallback(new AtmoCallBackX(camera));
    ss->addUniform(new osg::Uniform("groundTexture", 3));

    printf("Simple atmosphere shaders loaded and configured successfully\n");
}

// 添加云纹理初始化函数
void AtmosphereDemo::initializeCloudTextures(osg::StateSet* ss)
{
    // 加载云纹理图（天气图）
    osg::ref_ptr<osg::Texture2D> cloudTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> cloudImage = osgDB::readImageFile("E:/W.png");
    
    if (cloudImage.valid()) {
        cloudTexture->setImage(cloudImage);
        cloudTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        cloudTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        cloudTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        cloudTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将云纹理绑定到纹理单元4
        ss->setTextureAttributeAndModes(4, cloudTexture);
        ss->addUniform(new osg::Uniform("cloudTexture", 4));
        
        printf("Cloud texture loaded and configured successfully\n");
    } else {
        printf("Failed to load cloud texture\n");
    }
    
    // 加载云形状纹理
    osg::ref_ptr<osg::Texture2D> cloudShapeTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> cloudShapeImage = osgDB::readImageFile("E:/wea.png");
    
    if (cloudShapeImage.valid()) {
        cloudShapeTexture->setImage(cloudShapeImage);
        cloudShapeTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        cloudShapeTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        cloudShapeTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        cloudShapeTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将云形状纹理绑定到纹理单元5
        ss->setTextureAttributeAndModes(5, cloudShapeTexture);
        ss->addUniform(new osg::Uniform("cloudShapeTexture", 5));
        
        printf("Cloud shape texture loaded and configured successfully\n");
    } else {
        printf("Failed to load cloud shape texture\n");
    }
    
    // 加载蓝噪声纹理
    osg::ref_ptr<osg::Texture2D> blueNoiseTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> blueNoiseImage = osgDB::readImageFile("E:/b.png");
    
    if (blueNoiseImage.valid()) {
        blueNoiseTexture->setImage(blueNoiseImage);
        blueNoiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将蓝噪声纹理绑定到纹理单元6
        ss->setTextureAttributeAndModes(6, blueNoiseTexture);
        ss->addUniform(new osg::Uniform("blueNoiseTexture", 6));
        
        printf("Blue noise texture loaded and configured successfully\n");
    } else {
        printf("Failed to load blue noise texture\n");
    }

}

// AtmoCallBackX类的实现
void AtmoCallBackX::process(osg::StateSet* ss)
{
    nFrame++;
    if (nFrame > 6000)
        nFrame = 10;

    // 每60帧更新一次分辨率uniform
    if (nFrame ==1 )
        {
            osg::Vec2 viewPort(1280, 1024);
            osg::Uniform* iResolution = new osg::Uniform("iResolution", viewPort);
            iResolution->setUpdateCallback(new ResolutionCallback(m_camera));
            ss->addUniform(iResolution);
        }
     
        // 更新时间uniform变量
        float currentTime = nFrame * 0.016f; // 假设60FPS
        ss->getOrCreateUniform("time", osg::Uniform::FLOAT)->set(currentTime);

        osg::Matrixf vieMat     = m_camera->getViewMatrix();
        osg::Matrixf projectMat = m_camera->getProjectionMatrix();
    
        osg::Vec3f eye, center, up;
        osg::Camera* pCamera_ = m_camera;
        if (pCamera_)
            pCamera_->getViewMatrixAsLookAt(eye, center, up);
        //
        osg::Matrixf viewInverse    = vieMat.inverse(vieMat);
        osg::Matrixf projectInverse = projectMat.inverse(projectMat);

        ss->getOrCreateUniform("model_from_view", osg::Uniform::FLOAT_MAT4)->set(viewInverse);
        ss->getOrCreateUniform("view_from_clip",  osg::Uniform::FLOAT_MAT4)->set(projectInverse);

        ss->getOrCreateUniform("camera",      osg::Uniform::FLOAT_VEC3)->set(eye*0.01);

      
        ss->getOrCreateUniform("earth_center", osg::Uniform::FLOAT_VEC3)->set((eye - osg::Vec3(0, 0, 6360000))*0.001);

    // 设置太阳方向
    float acos, asin, zcos, zsin;
    const double PI = 3.14159265358979323846;
    double AO = 0.5 * PI;
    double ZO = 0.5*(1.0 - sin(1200 * 0.0001)) * PI;
     
    asin = sin(AO);
    acos = cos(AO);
    zsin = sin(ZO);
    zcos = cos(ZO);
    osg::Vec3 sun_direction(acos * zsin, asin * zsin, zcos);
    ss->getOrCreateUniform("sun_direction", osg::Uniform::FLOAT_VEC3)->set(sun_direction);

    double kSunAngularRadius = 0.004675;
    osg::Vec2 sun_size(tan(kSunAngularRadius), cos(kSunAngularRadius));
    ss->getOrCreateUniform("sun_size", osg::Uniform::FLOAT_VEC2)->set(sun_size);

    // 根据type设置不同的uniform
    if (type == 0) {
        ss->getOrCreateUniform("transmittance_texture", osg::Uniform::INT)->set(0);
        ss->getOrCreateUniform("scattering_texture", osg::Uniform::INT)->set(1);
        ss->getOrCreateUniform("single_mie_scattering_texture", osg::Uniform::INT)->set(1);
        ss->getOrCreateUniform("irradiance_texture", osg::Uniform::INT)->set(2);
        ss->getOrCreateUniform("groundTexture", osg::Uniform::INT)->set(3);

        osg::Vec3 white_point(1.0, 1.0, 1.0);
        ss->getOrCreateUniform("white_point", osg::Uniform::FLOAT_VEC3)->set(white_point);
        ss->getOrCreateUniform("exposure", osg::Uniform::FLOAT)->set(10.0f);
    }
    // 每100帧打印一次调试信息
    if (nFrame % 100 == 0) {
        printf("AtmoCallBackX processing frame %d\n", nFrame);
    }
}
