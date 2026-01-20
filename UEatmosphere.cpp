#include "UEatmosphere.h"
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



UEatmosphere::UEatmosphere()
    : m_sunZenithAngle(1.3)   // 天顶角约75度
    , m_sunAzimuthAngle(2.9)  // 方位角约166度
    , m_exposure(1.0f)
    , m_turbidity(2.0f)
    , m_rayleigh(1.0f)
    , m_mieCoefficient(0.01f)
    , m_mieDirectionalG(0.8f)
{
    
}

osg::ref_ptr<osg::Geode> UEatmosphere::createFullScreenQuad(osg::Camera* camera){
    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry(
        osg::Vec3(-1.0f, -1.0f, 0.0f),
        osg::Vec3(2.0f, 0.0f, 0.0f),
        osg::Vec3(0.0f, 2.0f, 0.0f));

    geom->setVertexAttribArray(0, geom->getVertexArray(), osg::Array::BIND_PER_VERTEX);
    geom->setVertexAttribArray(1, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);
    
    
    osg::ref_ptr<osg::Program> shaderProgram = new osg::Program;

    // 使用OSG的标准方法从文件读取着色器
    osg::ref_ptr<osg::Shader> vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, "e:/qwidget1/shader/pass1.vert");
    osg::ref_ptr<osg::Shader> fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "e:/qwidget1/shader/pass1.frag");


    shaderProgram->addShader(vertexShader);
    shaderProgram->addShader(fragmentShader);

    osg::StateSet* ss = geom->getOrCreateStateSet();
    ss->setAttributeAndModes(shaderProgram);

    //ss->setUpdateCallback(new FastCallback(camera));
    //inituniforms

    
    quad->addDrawable(geom);

    return quad.release();
}


osg::Node* UEatmosphere::createAtmosphere(osg::Node* subgraph, osg::Camera* camera){
     
    osg::Texture2D* textures = new osg::Texture2D;

    osg::ref_ptr<osg::Camera> mrtCam = createRTTCamera(textures, osg::Vec4(0.0, 0.0, 0.0, 1.0));
    mrtCam->addChild(createFullScreenQuad(camera));

    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    
    // 创建全屏三角形顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(-1.0f, -1.0f, 0.0f));  // 左下角
    vertices->push_back(osg::Vec3(-1.0f,  3.0f, 0.0f));  // 左上角 (y > 1 以覆盖屏幕)
    vertices->push_back(osg::Vec3( 3.0f, -1.0f, 0.0f));  // 右下角 (x > 1 以覆盖屏幕)
    
    geom->setVertexArray(vertices);
    
    // 设置顶点索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    indices->push_back(0);
    indices->push_back(1);
    indices->push_back(2);
    
    geom->addPrimitiveSet(indices);
    
    
    osg::StateSet* ss = geom->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(2, textures);  
    ss->addUniform(new osg::Uniform("uTexture0", 2));    
    
    initShaders(camera,ss);
    initUniforms(ss);

    quad->addDrawable(geom);


    quad->setNodeMask(1 << 12);


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

    // 确保RTT相机先添加，保证渲染顺序
    root->addChild(mrtCam.get());

    root->addChild(hudCam);
    
   
    return root.release();

}
UEatmosphere::~UEatmosphere()
{
    
}





void UEatmosphere::initShaders(osg::Camera* camera,osg::StateSet* ss)
{
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;

    // 使用OSG的标准方法从文件读取着色器
    osg::ref_ptr<osg::Shader> vertexShader = osg::Shader::readShaderFile(osg::Shader::VERTEX, "e:/qwidget1/shader/fast.vert");
    osg::ref_ptr<osg::Shader> fragmentShader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "e:/qwidget1/shader/fast.frag");

    // 检查着色器是否成功加载
    if (!vertexShader || !fragmentShader) {
        osg::notify(osg::WARN) << "Failed to load shaders from files." << std::endl;
        return;
    }

    // 将着色器添加到程序
    program->addShader(vertexShader);
    program->addShader(fragmentShader);


    ss->setAttributeAndModes(program);

    _UECallback = new UECallback(camera);
    ss->setUpdateCallback(_UECallback.get());
    initializeCloudTextures(ss);
   
}

void UEatmosphere::initUniforms(osg::StateSet* ss)
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
    _camera_center = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "camera_center");
    _camera_up = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "camera_up");
    _camera_right = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "camera_right");
    _camera_fov = new osg::Uniform(osg::Uniform::FLOAT, "camera_fov");
    _camera_aspect = new osg::Uniform(osg::Uniform::FLOAT, "camera_aspect");
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
    
    // 云参数uniform变量
    _shapescale = new osg::Uniform(osg::Uniform::FLOAT, "shapescale");
    _detailScale = new osg::Uniform(osg::Uniform::FLOAT, "detailScale");
    _u_WindDir = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "u_WindDir");
    _u_WindSpeed = new osg::Uniform(osg::Uniform::FLOAT, "u_WindSpeed");
    _weatherScale = new osg::Uniform(osg::Uniform::FLOAT, "weatherScale");
    _weatherWind = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "weatherWind");
    _curlStrength = new osg::Uniform(osg::Uniform::FLOAT, "curlStrength");
    _curlScale = new osg::Uniform(osg::Uniform::FLOAT, "curlScale");
    _erosionStrength = new osg::Uniform(osg::Uniform::FLOAT, "erosionStrength");
    
    // 鼠标位置uniform变量
    _iMouse = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "iMouse");

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
    
    // 设置相机参数初始值
    _camera_pos->set(osg::Vec3(0.0f, 0.0f, 0.0f));
    _camera_center->set(osg::Vec3(0.0f, 0.0f, 0.0f));
    _camera_up->set(osg::Vec3(0.0f, 0.0f, 1.0f));
    _camera_right->set(osg::Vec3(1.0f, 0.0f, 0.0f));
    _camera_fov->set(0.785398f);  // 45度 (弧度)
    _camera_aspect->set(1.77778f);  // 16:9 宽高比
    
    // 云参数初始值
    _shapescale->set(0.0003f);
    _detailScale->set(0.001f);
    _u_WindDir->set(osg::Vec3(1.0f, 0.0f, 0.0f));
    _u_WindSpeed->set(10.0f);
    _weatherScale->set(0.00005f);
    _weatherWind->set(osg::Vec2(0.01f, 0.0f));
    _curlStrength->set(100.0f);
    _curlScale->set(0.01f);
    _erosionStrength->set(0.5f);
    
    // 鼠标位置初始值
    _iMouse->set(osg::Vec2(557.0f, 664.0f));


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
    
    // 添加相机参数uniform变量
    ss->addUniform(_camera_pos);
    ss->addUniform(_camera_center);
    ss->addUniform(_camera_up);
    ss->addUniform(_camera_right);
    ss->addUniform(_camera_fov);
    ss->addUniform(_camera_aspect);
    
    // 添加云参数uniform变量
    ss->addUniform(_shapescale);
    ss->addUniform(_detailScale);
    ss->addUniform(_u_WindDir);
    ss->addUniform(_u_WindSpeed);
    ss->addUniform(_weatherScale);
    ss->addUniform(_weatherWind);
    ss->addUniform(_curlStrength);
    ss->addUniform(_curlScale);
    ss->addUniform(_erosionStrength);
    
    // 添加鼠标位置uniform变量
    ss->addUniform(_iMouse);


    osg::ref_ptr<osg::Texture2D> transmittanceLUT = new osg::Texture2D;
    osg::ref_ptr<osg::Image> transmittanceImage = osgDB::readImageFile("e:/qwidget1/shader/LUT.png");

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
    osg::ref_ptr<osg::Image> mutlutImage = osgDB::readImageFile("e:/qwidget1/shader/mutlut.png");

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
    _time->setUpdateCallback(new UETimeCallback());
}




void UEatmosphere::initializeCloudTextures(osg::StateSet* ss)
{
    // 加载3D基础形状纹理
    osg::ref_ptr<osg::Texture3D> shapeNoiseTexture = new osg::Texture3D;
    osg::ref_ptr<osg::Image> shapeNoiseImage = osgDB::readImageFile("E:/cloud1/WL.png");//0.003
   // osg::ref_ptr<osg::Image> shapeNoiseImage = osgDB::readImageFile("E:/cloud1/perlin-16-8.png"); //0.001

  // osg::ref_ptr<osg::Image> shapeNoiseImage = osgDB::readImageFile("E:/a.png");
    if (shapeNoiseImage.valid()) {
        shapeNoiseTexture->setImage(shapeNoiseImage);
        shapeNoiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
        shapeNoiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
        shapeNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        shapeNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        shapeNoiseTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
        
        // 将基础形状纹理绑定到纹理单元
        ss->setTextureAttributeAndModes(3, shapeNoiseTexture);
        ss->addUniform(new osg::Uniform("_ShapeNoiceTex", 3));
        
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
        ss->setTextureAttributeAndModes(4, detailNoiseTexture);
        ss->addUniform(new osg::Uniform("_DetailNoiceTex", 4));
        
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
        ss->setTextureAttributeAndModes(5, weatherTexture);
        ss->addUniform(new osg::Uniform("_WeatherNoiceTex", 5));
        
        printf("Weather texture loaded and configured successfully\n");
    } else {
        printf("Failed to load weather texture\n");
    }
    
    // 加载蓝噪声纹理，用于消除云渲染分层
    osg::ref_ptr<osg::Texture2D> blueNoiseTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> blueNoiseImage = osgDB::readImageFile("E:/cloud1/WL.png");
    
    if (blueNoiseImage.valid()) {
        blueNoiseTexture->setImage(blueNoiseImage);
        blueNoiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        blueNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将蓝噪声纹理绑定到纹理单元10
        ss->setTextureAttributeAndModes(6, blueNoiseTexture);
        ss->addUniform(new osg::Uniform("noisetexture1",6));
        
        printf("Blue noise texture loaded and configured successfully\n");
    } else {
        printf("Failed to load blue noise texture\n");
    }


    osg::ref_ptr<osg::Texture2D> curlNoiseTexture = new osg::Texture2D;
    osg::ref_ptr<osg::Image> curlNoiseImage = osgDB::readImageFile("E:/cloud1/chanel.png");
    
    if (curlNoiseImage.valid()) {
        curlNoiseTexture->setImage(curlNoiseImage);
        curlNoiseTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        curlNoiseTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        curlNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        curlNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将蓝噪声纹理绑定到纹理单元10
        ss->setTextureAttributeAndModes(7, curlNoiseTexture);
        ss->addUniform(new osg::Uniform("noisetexture", 7));
        
        printf("_curlNoiseTextexture loaded and configured successfully\n");
    } else {
        printf("Failed to load _curlNoiseTex texture\n");
    }


    osg::ref_ptr<osg::Texture2D> blueNoiseTexture2 = new osg::Texture2D;
    osg::ref_ptr<osg::Image> blueNoiseImage2 = osgDB::readImageFile("E:/cloud1/WL.png");
    
    if (blueNoiseImage2.valid()) {
        blueNoiseTexture2->setImage(blueNoiseImage2);
        blueNoiseTexture2->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture2->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        blueNoiseTexture2->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        blueNoiseTexture2->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将蓝噪声纹理绑定到纹理单元10
        ss->setTextureAttributeAndModes(8, blueNoiseTexture2);
        ss->addUniform(new osg::Uniform("noisetexture1",8));
        
        printf("Blue noise texture loaded and configured successfully\n");
    } else {
        printf("Failed to load blue noise texture\n");
    }


    osg::ref_ptr<osg::Texture2D> curlNoiseTexture2 = new osg::Texture2D;
    osg::ref_ptr<osg::Image> curlNoiseImage2 = osgDB::readImageFile("E:/cloud1/chanel.png");
    
    if (curlNoiseImage2.valid()) {
        curlNoiseTexture2->setImage(curlNoiseImage2);
        curlNoiseTexture2->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        curlNoiseTexture2->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        curlNoiseTexture2->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        curlNoiseTexture2->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        
        // 将蓝噪声纹理绑定到纹理单元10
        ss->setTextureAttributeAndModes(9, curlNoiseTexture2);
        ss->addUniform(new osg::Uniform("noisetexture", 9));
        
        printf("_curlNoiseTextexture loaded and configured successfully\n");
    } else {
        printf("Failed to load _curlNoiseTex texture\n");
    }


}




// FastCallback类的实现
void UECallback::process(osg::StateSet* ss)
{
    nFrame++;
    if (nFrame > 6000)
        nFrame = 1;  // 重置为1而不是10
 ss->getOrCreateUniform("iFrame", osg::Uniform::INT)->set(nFrame);
    // 每100帧更新一次分辨率uniform
    if (nFrame == 1 || nFrame % 100 == 1)  // 修改条件以确保能触发
    {
        if (m_camera) {
            osg::Viewport* viewport = m_camera->getViewport();
            if (viewport) {
                osg::Vec2 viewPort(viewport->width(), viewport->height());
                ss->getOrCreateUniform("iResolution1", osg::Uniform::FLOAT_VEC2)->set(viewPort);               
            }
        }
    }
        
        
    // 不再输出调试信息，因为这会非常频繁
   // std::cout << "Update mouse position requested: (" << m_mousePos.x() << ", " << m_mousePos.y() << ")" << std::endl;
        
    // 更新鼠标位置uniform变量 - 使用已存在的iMouse uniform
    // osg::Uniform* iMouseUniform = ss->getUniform("iMouse");
    // if (iMouseUniform) {
    //     iMouseUniform->set(m_mousePos);
    // }
        
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

        // 设置相机参数uniform (使用米单位)
        osg::Vec3 cameraPos = eye;
        osg::Vec3 cameraCenter = center;
        osg::Vec3 cameraUp = up;
        
        // 计算相机右方向
        osg::Vec3 cameraForward = (cameraCenter - cameraPos);
        cameraForward.normalize();
        osg::Vec3 cameraRight = cameraForward ^ cameraUp; // 叉积
        cameraRight.normalize();
        
        // 从相机操作器获取FOV和宽高比
        double fovy, aspectRatio, zNear, zFar;
        m_camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
        float cameraFov = static_cast<float>(osg::DegreesToRadians(fovy));
        float cameraAspect = static_cast<float>(aspectRatio);
        
        // 设置相机参数uniform
        ss->getOrCreateUniform("camera_pos", osg::Uniform::FLOAT_VEC3)->set(cameraPos);
        ss->getOrCreateUniform("camera_center", osg::Uniform::FLOAT_VEC3)->set(cameraCenter);
        ss->getOrCreateUniform("camera_up", osg::Uniform::FLOAT_VEC3)->set(cameraUp);
        ss->getOrCreateUniform("camera_right", osg::Uniform::FLOAT_VEC3)->set(cameraRight);
        ss->getOrCreateUniform("camera_fov", osg::Uniform::FLOAT)->set(cameraFov);
        ss->getOrCreateUniform("camera_aspect", osg::Uniform::FLOAT)->set(cameraAspect);
        
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
        ss->getOrCreateUniform("earth_center", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(0.0f, 0.0f,-6372050.0f));
        
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
        ss->getOrCreateUniform("groundTexture", osg::Uniform::INT)->set(2);
        // 每100帧打印一次调试信息
        if (nFrame % 100 == 1) {  // 修改条件以确保能触发
            printf("FastCallback processing frame %d\n", nFrame);
            printf("Camera position: (%f, %f, %f)\n", cameraPos.x(), cameraPos.y(), cameraPos.z());
            printf("Camera position length: %f\n", cameraPos.length());
            printf("Planet radius: %f\n", 6371000.0f);
        }
    }
}

osg::Camera* UEatmosphere::createHUDCamera(double left, double right, double bottom, double top)
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

osg::Camera* UEatmosphere::createRTTCamera(osg::Texture2D*& tex,  osg::Vec4 backColor)
{
    int width = 1024;
    int height = 1024;

    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearColor(backColor);
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);

    camera->setViewport(0, 0, width, height);

    tex = new osg::Texture2D;
    tex->setTextureSize(width, height);
    tex->setInternalFormat(GL_RGBA);
    tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

    camera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + 0), tex);
   
    return camera.release();
}

void UEatmosphere::updateCloudParameters(float shapescale, float detailScale, float windSpeed,
                              float weatherScale, float curlStrength, float curlScale,
                              float erosionStrength, float windDirX, float windDirY, float windDirZ,
                              float weatherWindX, float weatherWindY)
{
    // 更新云参数uniform变量
    if (_shapescale.valid()) {
        _shapescale->set(shapescale);
        printf("Updated shapescale to: %f\n", shapescale);
    }
    
    if (_detailScale.valid()) {
        _detailScale->set(detailScale);
        printf("Updated detailScale to: %f\n", detailScale);
    }
    
    if (_u_WindSpeed.valid()) {
        _u_WindSpeed->set(windSpeed);
        printf("Updated windSpeed to: %f\n", windSpeed);
    }
    
    if (_weatherScale.valid()) {
        _weatherScale->set(weatherScale);
        printf("Updated weatherScale to: %f\n", weatherScale);
    }
    
    if (_curlStrength.valid()) {
        _curlStrength->set(curlStrength);
        printf("Updated curlStrength to: %f\n", curlStrength);
    }
    
    if (_curlScale.valid()) {
        _curlScale->set(curlScale);
        printf("Updated curlScale to: %f\n", curlScale);
    }
    
    if (_erosionStrength.valid()) {
        _erosionStrength->set(erosionStrength);
        printf("Updated erosionStrength to: %f\n", erosionStrength);
    }
    
    if (_u_WindDir.valid()) {
        _u_WindDir->set(osg::Vec3(windDirX, windDirY, windDirZ));
        printf("Updated windDir to: (%f, %f, %f)\n", windDirX, windDirY, windDirZ);
    }
    
    if (_weatherWind.valid()) {
        _weatherWind->set(osg::Vec2(weatherWindX, weatherWindY));
        printf("Updated weatherWind to: (%f, %f)\n", weatherWindX, weatherWindY);
    }
}

void UEatmosphere::resetCloudParameters()
{
    // 重置云参数为默认值
    if (_shapescale.valid()) {
        _shapescale->set(0.0003f);
        printf("Reset shapescale to: %f\n", 0.0003f);
    }
    
    if (_detailScale.valid()) {
        _detailScale->set(0.001f);
        printf("Reset detailScale to: %f\n", 0.001f);
    }
    
    if (_u_WindSpeed.valid()) {
        _u_WindSpeed->set(10.0f);
        printf("Reset windSpeed to: %f\n", 10.0f);
    }
    
    if (_weatherScale.valid()) {
        _weatherScale->set(0.00005f);
        printf("Reset weatherScale to: %f\n", 0.00005f);
    }
    
    if (_curlStrength.valid()) {
        _curlStrength->set(100.0f);
        printf("Reset curlStrength to: %f\n", 100.0f);
    }
    
    if (_curlScale.valid()) {
        _curlScale->set(0.01f);
        printf("Reset curlScale to: %f\n", 0.01f);
    }
    
    if (_erosionStrength.valid()) {
        _erosionStrength->set(0.5f);
        printf("Reset erosionStrength to: %f\n", 0.5f);
    }
    
    if (_u_WindDir.valid()) {
        _u_WindDir->set(osg::Vec3(1.0f, 0.0f, 0.0f));
        printf("Reset windDir to: (%f, %f, %f)\n", 1.0f, 0.0f, 0.0f);
    }
    
    if (_weatherWind.valid()) {
        _weatherWind->set(osg::Vec2(0.01f, 0.0f));
        printf("Reset weatherWind to: (%f, %f)\n", 0.01f, 0.0f);
    }
}

void UEatmosphere::updateMousePosition(float x, float y)
{
    // 直接更新FastCallback中的鼠标位置
    if (_UECallback.valid()) {
        _UECallback->setMousePosition(x, y);
    }
    
}

