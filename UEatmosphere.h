#ifndef UE_ATMOSPHERE_H
#define UE_ATMOSPHERE_H

#include <osg/Geode>
#include <osg/Uniform>
#include <osg/Camera>
#include <osg/Texture3D>
#include <osg/Texture2D>
#include <osg/Image>

// UETimeCallback回调类声明
class UETimeCallback : public osg::UniformCallback
{
public:
    UETimeCallback() : mCurrentTime(0.0f) {}
    
    void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) override
    {
        // 更新当前时间（以秒为单位）
        mCurrentTime += 0.016f; // 假设60FPS
        if (mCurrentTime > 1000000.0f) // 防止数值过大
            mCurrentTime = 0.0f;
            
        uniform->set(mCurrentTime);
    }

private:
    float mCurrentTime;
};

// UECallback回调类声明
class UECallback : public osg::StateSet::Callback
{
public:
    explicit UECallback(osg::Camera* camera) : m_camera(camera), nFrame(0) {
        // 初始化鼠标位置
        m_mousePos.set(0.0f, 0.0f);
    }

    virtual void operator()(osg::StateSet* ss, osg::NodeVisitor* nv) override
    {    
        process(ss);
    }

    void process(osg::StateSet* ss);
    
    // 设置鼠标位置
    void setMousePosition(float x, float y) { m_mousePos.set(x, y); }
    osg::Vec2 getMousePosition() const { return m_mousePos; }
        
private:
    osg::Camera* m_camera;
    int nFrame;
    osg::Vec2 m_mousePos;  // 鼠标位置
};

class UEatmosphere : public osg::Geode
{
public:
    UEatmosphere();
    virtual ~UEatmosphere();


    osg::ref_ptr<osg::Geode> createFullScreenQuad( osg::Camera* camera);
    osg::Node* createAtmosphere(osg::Node* subgraph, osg::Camera* camera  );
    // 导出LUT纹理为图像文件
    void exportLUT(const std::string& filename);
    osg::Camera* createRTTCamera(osg::Texture2D*& tex, osg::Vec4 backColor);

    osg::Camera* createHUDCamera(double left, double right, double bottom, double top);
    
    // 更新云参数
    void updateCloudParameters(float shapescale, float detailScale, float windSpeed,
                              float weatherScale, float curlStrength, float curlScale,
                              float erosionStrength, float windDirX, float windDirY, float windDirZ,
                              float weatherWindX, float weatherWindY);
    
    // 重置云参数为默认值
    void resetCloudParameters();
    

    
    // 更新鼠标位置
    void updateMousePosition(float x, float y);
    private:
    // 初始化着色器
    void initShaders(osg::Camera* camera,osg::StateSet* ss);
    
    // 初始化uniform变量
    void initUniforms(osg::StateSet* ss);

    void initializeCloudTextures(osg::StateSet* ss);
    
    
    // 大气参数
    double m_sunZenithAngle;
    double m_sunAzimuthAngle;
    float m_exposure;
    float m_turbidity;
    float m_rayleigh;
    float m_mieCoefficient;
    float m_mieDirectionalG;
    
    // Uniform变量（用于着色器）
    osg::ref_ptr<osg::Uniform> _sun_direction;
    osg::ref_ptr<osg::Uniform> _exposure;
    osg::ref_ptr<osg::Uniform> _turbidity;
    osg::ref_ptr<osg::Uniform> _rayleigh;
    osg::ref_ptr<osg::Uniform> _mieCoefficient;
    osg::ref_ptr<osg::Uniform> _mie_phase_g;
    
    // 大气散射uniform变量
    osg::ref_ptr<osg::Uniform> _iResolution;
    osg::ref_ptr<osg::Uniform> _camera_pos;
    osg::ref_ptr<osg::Uniform> _camera_center;
    osg::ref_ptr<osg::Uniform> _camera_up;
    osg::ref_ptr<osg::Uniform> _camera_right;
    osg::ref_ptr<osg::Uniform> _camera_fov;
    osg::ref_ptr<osg::Uniform> _camera_aspect;
    osg::ref_ptr<osg::Uniform> _earth_center;
    osg::ref_ptr<osg::Uniform> _time;
    osg::ref_ptr<osg::Uniform> _sun_size;
    
    // 大气参数uniform变量 (AtmosphereParameter结构体)
    osg::ref_ptr<osg::Uniform> _seaLevel;
    osg::ref_ptr<osg::Uniform> _planetRadius;
    osg::ref_ptr<osg::Uniform> _atmosphereHeight;
    osg::ref_ptr<osg::Uniform> _sunLightIntensity;
    osg::ref_ptr<osg::Uniform> _sunLightColor;
    osg::ref_ptr<osg::Uniform> _sunDiskAngle;
    osg::ref_ptr<osg::Uniform> _rayleighScatteringScale;
    osg::ref_ptr<osg::Uniform> _rayleighScatteringScalarHeight;
    osg::ref_ptr<osg::Uniform> _mieScatteringScale;
    osg::ref_ptr<osg::Uniform> _mieAnisotropy;
    osg::ref_ptr<osg::Uniform> _mieScatteringScalarHeight;
    osg::ref_ptr<osg::Uniform> _ozoneAbsorptionScale;
    osg::ref_ptr<osg::Uniform> _ozoneLevelCenterHeight;
    osg::ref_ptr<osg::Uniform> _ozoneLevelWidth;
    
    // 云参数uniform变量
    osg::ref_ptr<osg::Uniform> _shapescale;
    osg::ref_ptr<osg::Uniform> _detailScale;
    osg::ref_ptr<osg::Uniform> _u_WindDir;
    osg::ref_ptr<osg::Uniform> _u_WindSpeed;
    osg::ref_ptr<osg::Uniform> _weatherScale;
    osg::ref_ptr<osg::Uniform> _weatherWind;
    osg::ref_ptr<osg::Uniform> _curlStrength;
    osg::ref_ptr<osg::Uniform> _curlScale;
    osg::ref_ptr<osg::Uniform> _erosionStrength;
    
    // 鼠标位置uniform变量
    osg::ref_ptr<osg::Uniform> _iMouse;
    
    // UECallback引用
    osg::ref_ptr<UECallback> _UECallback;
};

#endif // FAST_ATMOSPHERE_H