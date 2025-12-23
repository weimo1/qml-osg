#include "FullscreenAtmosphere.h"
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/StateSet>
#include <osg/Depth>
#include <osg/Program>
#include <osg/Shader>
#include <osgDB/ReadFile>
#include <cmath>
#include <fstream>
#include <sstream>
#include <osg/Texture3D>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/Program>
#include <osg/Shader>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osg/Camera>



FullscreenAtmosphere::FullscreenAtmosphere()
    : m_camera(nullptr)
    , m_rootNode(nullptr)
    , m_sunZenithAngle(1.3)   // 天顶角约75度
    , m_sunAzimuthAngle(2.9)  // 方位角约166度
    , m_exposure(1.0f)
    , m_turbidity(2.0f)
    , m_rayleigh(1.0f)
    , m_mieCoefficient(0.01f)
    , m_mieDirectionalG(0.8f)
{
    // 初始化几何体、着色器和uniform变量
    osg::Node* rootNode = initGeometry();
    initShaders();
    initUniforms();
    
    // 如果成功创建了根节点，将其添加到当前Geode中
    if (rootNode) {
        addChild(rootNode);
    }
}

FullscreenAtmosphere::~FullscreenAtmosphere()
{
    // 析构函数可以为空，因为所有OSG对象都使用ref_ptr管理
}

void FullscreenAtmosphere::setCamera(osg::Camera* camera)
{
    m_camera = camera;
    
    // 当设置相机时，确保AtmosphereCallback被添加到状态集中
    if (camera) {
        osg::StateSet* ss = getOrCreateStateSet();
        // 检查是否已经存在AtmosphereCallback，如果不存在则添加
        if (!ss->getUpdateCallback()) {
            ss->setUpdateCallback(new AtmosphereCallback(camera));
        }
    }
}

osg::Node* FullscreenAtmosphere::initGeometry()
{
    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    // 创建全屏四边形几何体，使用更大的范围确保完全覆盖视野
    osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry(
        osg::Vec3(-1.0f, -1.0f, 0.0f),
        osg::Vec3(2.0f, 0.0f, 0.0f),
        osg::Vec3(0.0f, 2.0f, 0.0f));

    // 只设置顶点数组，不设置纹理坐标数组，因为我们会在着色器中生成纹理坐标
    geom->setVertexAttribArray(0, geom->getVertexArray(), osg::Array::BIND_PER_VERTEX);
    // 移除这一行，因为我们不需要传递纹理坐标数组
    // geom->setVertexAttribArray(1, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);

    // 将几何体添加到Geode
    quad->addDrawable(geom);
    // 设置状态集

    osg::Camera* hudCam = createHUDCamera(0.0, 1.0, 0.0, 1.0);
    if (!hudCam) {
        printf("Failed to create HUD camera\n");
        return nullptr;
    }

    
    hudCam->addChild(quad.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (!root) {
        printf("Failed to create root group\n");
        return nullptr;
    }

    root->addChild(hudCam);

    // 保存根节点引用，以便在getNode()中返回
    m_rootNode = root;

    return root.release();
}

void FullscreenAtmosphere::initShaders()
{
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;

    // 使用OSG的标准方法从文件读取着色器
    osg::ref_ptr<osg::Shader> vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, "e:/qt test/qml-osg/shader/cloud.vert");
    osg::ref_ptr<osg::Shader> fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "e:/qt test/qml-osg/shader/cloud.frag");

    // 检查着色器是否成功加载
    if (!vertexShader || !fragmentShader) {
        osg::notify(osg::WARN) << "Failed to load shaders from files." << std::endl;
        return;
    }

    // 将着色器添加到程序
    program->addShader(vertexShader);
    program->addShader(fragmentShader);

    // 将程序添加到状态集
    osg::StateSet* ss = getOrCreateStateSet();
    ss->setAttributeAndModes(program);
    
    initializeCloudTextures(ss);
    // 注意：不在这里添加AtmosphereCallback，而是在setCamera中添加
}

void FullscreenAtmosphere::initUniforms()
{
    // 初始化uniform变量
    _iResolution = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "iResolution");
    _sun_direction = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "sun_direction");
    _exposure = new osg::Uniform(osg::Uniform::FLOAT, "exposure");
    _turbidity = new osg::Uniform(osg::Uniform::FLOAT, "turbidity");
    _rayleigh = new osg::Uniform(osg::Uniform::FLOAT, "rayleigh");
    _mieCoefficient = new osg::Uniform(osg::Uniform::FLOAT, "mieCoefficient");
    _mie_phase_g = new osg::Uniform(osg::Uniform::FLOAT, "mie_phase_g");
    
    // 大气散射uniform变量
    _camera_pos = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "camera_pos");
    _earth_center = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "earth_center");
    _time = new osg::Uniform(osg::Uniform::FLOAT, "iTime");
    _sun_size = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "sun_size");
    
    // 大气参数uniform变量 (AtmosphereParameter结构体)
    _seaLevel = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.SeaLevel");
    _planetRadius = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.PlanetRadius");
    _atmosphereHeight = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.AtmosphereHeight");
    _sunLightIntensity = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.SunLightIntensity");
    _sunLightColor = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "atmosphereParams.SunLightColor");
    _sunDiskAngle = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.SunDiskAngle");
    _rayleighScatteringScale = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.RayleighScatteringScale");
    _rayleighScatteringScalarHeight = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.RayleighScatteringScalarHeight");
    _mieScatteringScale = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.MieScatteringScale");
    _mieAnisotropy = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.MieAnisotropy");
    _mieScatteringScalarHeight = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.MieScatteringScalarHeight");
    _ozoneAbsorptionScale = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.OzoneAbsorptionScale");
    _ozoneLevelCenterHeight = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.OzoneLevelCenterHeight");
    _ozoneLevelWidth = new osg::Uniform(osg::Uniform::FLOAT, "atmosphereParams.OzoneLevelWidth");

    // 设置初始值
    _iResolution->set(osg::Vec2(1920.0f, 1080.0f));
    _exposure->set(10.0f);
    _turbidity->set(2.0f);
    _rayleigh->set(1.0f);
    _mieCoefficient->set(0.01f);
    _mie_phase_g->set(0.8f);
    
    _sun_direction->set(osg::Vec3(0.0f, 0.707f, 0.707f));  // 45度仰角
    _earth_center->set(osg::Vec3(0.0f, 0.0f, 0.0f));
    _time->set(0.0f);
    _sun_size->set(osg::Vec2(0.004675f, 0.999989f));
    
    // ===== 大气参数：使用千米(km)单位 =====
    _seaLevel->set(0.0f);
    _planetRadius->set(6371.0f);           // 地球半径：6371千米
    _atmosphereHeight->set(100.0f);        // 大气层厚度：100千米
    _sunLightIntensity->set(31.0f);
    _sunLightColor->set(osg::Vec3(1.0f, 1.0f, 1.0f));
    _sunDiskAngle->set(0.545f);            // 太阳角直径约0.53度
    _rayleighScatteringScale->set(1.0f);
    _rayleighScatteringScalarHeight->set(8.0f);   // 8千米
    _mieScatteringScale->set(1.0f);
    _mieAnisotropy->set(0.8f);
    _mieScatteringScalarHeight->set(1.2f);        // 1.2千米
    _ozoneAbsorptionScale->set(1.0f);
    _ozoneLevelCenterHeight->set(25.0f);          // 25千米
    _ozoneLevelWidth->set(15.0f); 

    // 将uniform变量添加到状态集
    osg::StateSet* ss = getOrCreateStateSet();
    ss->addUniform(_iResolution);
    ss->addUniform(_sun_direction);
    ss->addUniform(_exposure);
    ss->addUniform(_turbidity);
    ss->addUniform(_rayleigh);
    ss->addUniform(_mieCoefficient);
    ss->addUniform(_mie_phase_g);
    ss->addUniform(_camera_pos);
    ss->addUniform(_earth_center);
    ss->addUniform(_time);
    ss->addUniform(_sun_size);
    
    // 添加大气参数uniform变量
    ss->addUniform(_seaLevel);
    ss->addUniform(_planetRadius);
    ss->addUniform(_atmosphereHeight);
    ss->addUniform(_sunLightIntensity);
    ss->addUniform(_sunLightColor);
    ss->addUniform(_sunDiskAngle);
    ss->addUniform(_rayleighScatteringScale);
    ss->addUniform(_rayleighScatteringScalarHeight);
    ss->addUniform(_mieScatteringScale);
    ss->addUniform(_mieAnisotropy);
    ss->addUniform(_mieScatteringScalarHeight);
    ss->addUniform(_ozoneAbsorptionScale);
    ss->addUniform(_ozoneLevelCenterHeight);
    ss->addUniform(_ozoneLevelWidth);



    osg::ref_ptr<osg::Texture2D> transmittanceLUT = new osg::Texture2D;
    osg::ref_ptr<osg::Image> transmittanceImage = osgDB::readImageFile("e:/qt test/qml-osg/shader/LUT.png");

    if (transmittanceImage.valid()) {
        transmittanceLUT->setImage(transmittanceImage);
        transmittanceLUT->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        transmittanceLUT->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        transmittanceLUT->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        transmittanceLUT->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);


    ss->setTextureAttributeAndModes(0, transmittanceLUT, osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("transmittanceLUT", 0));
        
        printf("Transmittance texture loaded and configured successfully\n");
    } else {
        printf("Failed to load transmittance texture\n");
    }


    osg::ref_ptr<osg::Texture2D> mutlutLUT = new osg::Texture2D;
    osg::ref_ptr<osg::Image> mutlutImage = osgDB::readImageFile("e:/qt test/qml-osg/shader/mutlut.png");

    if (mutlutImage.valid()) {
        mutlutLUT->setImage(mutlutImage);
        mutlutLUT->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        mutlutLUT->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        mutlutLUT->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        mutlutLUT->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);


    ss->setTextureAttributeAndModes(1, mutlutLUT, osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("mutlutLUT", 1));
        
        printf("Mutlut texture loaded and configured successfully\n");
    } else {
        printf("Failed to load mutlut texture\n");
    }

    
    // 为_time uniform变量设置更新回调
    _time->setUpdateCallback(new TimeCallback());
}







void FullscreenAtmosphere::initializeCloudTextures(osg::StateSet* ss)
{
    // 加载3D基础形状纹理
    osg::ref_ptr<osg::Texture3D> shapeNoiseTexture = new osg::Texture3D;
    osg::ref_ptr<osg::Image> shapeNoiseImage = osgDB::readImageFile("E:/cloud1/WL.png");//0.003
  //  osg::ref_ptr<osg::Image> shapeNoiseImage = osgDB::readImageFile("E:/cloud1/perlin-16-8.png"); //0.001

  // osg::ref_ptr<osg::Image> shapeNoiseImage = osgDB::readImageFile("E:/a.png");
    if (shapeNoiseImage.valid()) {
        shapeNoiseTexture->setImage(shapeNoiseImage);
        shapeNoiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
        shapeNoiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
        shapeNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        shapeNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        shapeNoiseTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
        
        // 将基础形状纹理绑定到纹理单元7
        ss->setTextureAttributeAndModes(2, shapeNoiseTexture);
        ss->addUniform(new osg::Uniform("_ShapeNoiceTex", 2));
        
        printf("3D Shape noise texture loaded and configured successfully\n");
    } else {
        printf("Failed to load 3D shape noise texture\n");
    }
    
    // 加载3D细节纹理
    osg::ref_ptr<osg::Texture3D> detailNoiseTexture = new osg::Texture3D;
    osg::ref_ptr<osg::Image> detailNoiseImage = osgDB::readImageFile("E:/cloud1/cloudnoise.png");
    
    if (detailNoiseImage.valid()) {
        detailNoiseTexture->setImage(detailNoiseImage);
        detailNoiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
        detailNoiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
        detailNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        detailNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        detailNoiseTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
        
        // 将细节纹理绑定到纹理单元8
        ss->setTextureAttributeAndModes(3, detailNoiseTexture);
        ss->addUniform(new osg::Uniform("_DetailNoiceTex", 3));
        
        printf("3D Detail noise texture loaded and configured successfully\n");
    } else {
        printf("Failed to load 3D detail noise texture\n");
    }
    
    // 加载2D天气纹理
    osg::ref_ptr<osg::Texture2D> weatherTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> weatherImage = osgDB::readImageFile("E:/cloud1/Weather.png");
    
    if (weatherImage.valid()) {
        weatherTexture->setImage(weatherImage);
        weatherTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        weatherTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        weatherTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        weatherTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将天气纹理绑定到纹理单元9
        ss->setTextureAttributeAndModes(4, weatherTexture);
        ss->addUniform(new osg::Uniform("_WeatherNoiceTex", 4));
        
        printf("Weather texture loaded and configured successfully\n");
    } else {
        printf("Failed to load weather texture\n");
    }
    
    // 加载蓝噪声纹理，用于消除云渲染分层
    osg::ref_ptr<osg::Texture2D> blueNoiseTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> blueNoiseImage = osgDB::readImageFile("E:/cloud1/BlueNoise.png");
    
    if (blueNoiseImage.valid()) {
        blueNoiseTexture->setImage(blueNoiseImage);
        blueNoiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将蓝噪声纹理绑定到纹理单元10
        ss->setTextureAttributeAndModes(5, blueNoiseTexture);
        ss->addUniform(new osg::Uniform("blueNoiseTexture", 5));
        
        printf("Blue noise texture loaded and configured successfully\n");
    } else {
        printf("Failed to load blue noise texture\n");
    }


    osg::ref_ptr<osg::Texture2D> curlNoiseTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> curlNoiseImage = osgDB::readImageFile("E:/cloud1/curlNoise.png");
    
    if (curlNoiseImage.valid()) {
        curlNoiseTexture->setImage(curlNoiseImage);
        curlNoiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        curlNoiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        curlNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        curlNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将蓝噪声纹理绑定到纹理单元10
        ss->setTextureAttributeAndModes(6, curlNoiseTexture);
        ss->addUniform(new osg::Uniform("_curlNoiseTex", 6));
        
        printf("_curlNoiseTextexture loaded and configured successfully\n");
    } else {
        printf("Failed to load _curlNoiseTex texture\n");
    }



}










// AtmosphereCallback类的实现
void AtmosphereCallback::process(osg::StateSet* ss)
{
    nFrame++;
    if (nFrame > 6000)
        nFrame = 1;  // 重置为1而不是10

    // 每100帧更新一次分辨率uniform
    if (nFrame == 1 || nFrame % 100 == 1)  // 修改条件以确保能触发
    {
        if (m_camera) {
            osg::Viewport* viewport = m_camera->getViewport();
            if (viewport) {
                osg::Vec2 viewPort(viewport->width(), viewport->height());
                ss->getOrCreateUniform("iResolution", osg::Uniform::FLOAT_VEC2)->set(viewPort);
            }
        }
    }
    
    // 更新时间uniform变量
    float currentTime = nFrame * 0.016f; // 假设60FPS
    ss->getOrCreateUniform("iTime", osg::Uniform::FLOAT)->set(currentTime);

    if (m_camera) {
        osg::Matrixf viewMat = m_camera->getViewMatrix();
        osg::Matrixf projectMat = m_camera->getProjectionMatrix();
    
        osg::Vec3f eye, center, up;
        up = osg::Vec3f(0.0f, 0.0f, 1.0f);
        m_camera->getViewMatrixAsLookAt(eye, center, up);
        
        // 计算视图逆矩阵和投影逆矩阵
        osg::Matrixf viewInverse = osg::Matrixf::inverse(viewMat);
        osg::Matrixf projectInverse = osg::Matrixf::inverse(projectMat);

        // 设置相机位置uniform (使用米单位)
        osg::Vec3 cameraPos = eye;
        // 如果相机太接近原点，将其设置在地球表面
      
        ss->getOrCreateUniform("camera_pos", osg::Uniform::FLOAT_VEC3)->set(cameraPos);
        
        // 设置矩阵uniform
        // 注意：model_from_view应该是视图到世界的变换矩阵（即viewInverse）
        // view_from_clip应该是投影到视图的变换矩阵（即projectInverse）
        ss->getOrCreateUniform("model_from_view", osg::Uniform::FLOAT_MAT4)->set(viewInverse);
        ss->getOrCreateUniform("view_from_clip", osg::Uniform::FLOAT_MAT4)->set(projectInverse);
        
        // 设置太阳方向
        // 修复：使用更合理的太阳方向计算
        double sunAngle = currentTime * 0.1f; // 太阳移动速度
        double zAngle = 0.5 * (1.0 - sin(2400 * 0.0001)) * 3.14159265358979323846;
        double AO = 0.5 * 3.14159265358979323846;
        double ZO = 0.5*(1.0 - sin(nFrame * 0.01)) * 3.14159265358979323846;
     
        double asin = sin(AO);
        double acos = cos(AO);
        double zsin = sin(ZO);
        double zcos = cos(ZO);
        osg::Vec3 sun_direction(acos * zsin, asin * zsin, zcos);

        ss->getOrCreateUniform("sun_direction", osg::Uniform::FLOAT_VEC3)->set(sun_direction);
        
        // 设置太阳大小
        double kSunAngularRadius = 0.004675;
        osg::Vec2 sun_size(tan(kSunAngularRadius), cos(kSunAngularRadius));
        ss->getOrCreateUniform("sun_size", osg::Uniform::FLOAT_VEC2)->set(sun_size);
        
        // 设置曝光
        ss->getOrCreateUniform("exposure", osg::Uniform::FLOAT)->set(0.5f);
        
        // 设置地球中心
        ss->getOrCreateUniform("earth_center", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(0.0f, 0.0f,-6371050.0f));
        
        // 设置大气参数结构体的各个字段
        // 注意：在GLSL中，结构体成员需要通过"结构体名.成员名"的方式访问
        ss->getOrCreateUniform("atmosphereParams.SeaLevel", osg::Uniform::FLOAT)->set(0.0f);
        ss->getOrCreateUniform("atmosphereParams.PlanetRadius", osg::Uniform::FLOAT)->set(6371000.0f);  // 地球半径6371公里
        ss->getOrCreateUniform("atmosphereParams.AtmosphereHeight", osg::Uniform::FLOAT)->set(60000.0f);  // 大气层厚度100公里
        ss->getOrCreateUniform("atmosphereParams.SunLightColor", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(1.0f, 1.0f, 1.0f));
        ss->getOrCreateUniform("atmosphereParams.SunDiskAngle", osg::Uniform::FLOAT)->set(15.4f);
        ss->getOrCreateUniform("atmosphereParams.RayleighScatteringScale", osg::Uniform::FLOAT)->set(1.0f);
        ss->getOrCreateUniform("atmosphereParams.RayleighScatteringScalarHeight", osg::Uniform::FLOAT)->set(8000.0f);
        ss->getOrCreateUniform("atmosphereParams.MieScatteringScale", osg::Uniform::FLOAT)->set(1.0f);
        ss->getOrCreateUniform("atmosphereParams.MieAnisotropy", osg::Uniform::FLOAT)->set(0.8f);
        ss->getOrCreateUniform("atmosphereParams.MieScatteringScalarHeight", osg::Uniform::FLOAT)->set(1200.0f);
        ss->getOrCreateUniform("atmosphereParams.OzoneAbsorptionScale", osg::Uniform::FLOAT)->set(1.0f);
        ss->getOrCreateUniform("atmosphereParams.OzoneLevelCenterHeight", osg::Uniform::FLOAT)->set(25000.0f);
        ss->getOrCreateUniform("atmosphereParams.OzoneLevelWidth", osg::Uniform::FLOAT)->set(15000.0f);
        
        // 每100帧打印一次调试信息
        if (nFrame % 100 == 1) {  // 修改条件以确保能触发
            printf("AtmosphereCallback processing frame %d\n", nFrame);
            printf("Camera position: (%f, %f, %f)\n", cameraPos.x(), cameraPos.y(), cameraPos.z());
            printf("Camera position length: %f\n", cameraPos.length());
            printf("Planet radius: %f\n", 6371000.0f);

        }
    }
}
osg::Camera* FullscreenAtmosphere::createHUDCamera(double left, double right, double bottom, double top)
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
