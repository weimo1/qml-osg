#ifndef ATMOSPHERE_DEMO_H
#define ATMOSPHERE_DEMO_H

#include <osg/Node>
#include <osg/Texture>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Matrix>
#include <osg/StateSet>
#include <osg/Geode>
#include <osg/Camera>
#include <osg/Group>
#include <osg/Uniform>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <osg/Texture2D>
#include <string>

// AtmoCallBackX回调类声明
class AtmoCallBackX : public osg::StateSet::Callback
{
public:

    explicit AtmoCallBackX(osg::Camera* camera, int userType = 0) : m_camera(camera), type(userType), nFrame(0) {}

    virtual void operator()(osg::StateSet* ss, osg::NodeVisitor* nv) override
    {    
        process(ss);
    }

    void process(osg::StateSet* ss);
        

private:
    osg::Camera* m_camera;
    int type;
    int nFrame;
    float exposure = 10.0f;
};

 class ResolutionCallback : public osg::UniformCallback
{
public:
    ResolutionCallback(osg::Camera* pData)
    {
        pView=pData;
    }
    void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv)
    {
        if(pView)
        {
            double dx, dy;
            dy= pView->getViewport()->height();
            dx=pView->getViewport()->width();
            uniform->set(osg::Vec2(dx, dy));
        }
    }
    osg::Camera* pView=nullptr;
};

class AtmosphereDemo : public osg::Referenced
{
public:
    AtmosphereDemo();
    virtual ~AtmosphereDemo();

    // 创建大气散射效果节点
    osg::Node* createAtmosphere(osg::Node* subgraph, osg::Camera* camera, const osg::Vec4& clearColour);
    
    // 设置太阳角度
    void setSunAngles(double sunZenithAngle, double sunAzimuthAngle);
    
    // 设置相机参数
    void setCameraParameters(double viewDistance, double viewZenithAngle, double viewAzimuthAngle);
    
    // 设置曝光值
    void setExposure(float exposure);
    static osg::Camera* createRTTCamera(osg::ref_ptr<osg::Texture2D>& tex);

    // 添加云纹理初始化函数声明
    void initializeCloudTextures(osg::StateSet* ss);

private:
    // 创建纹理
    osg::Texture* createTexture(int format, const std::string& fileName, int nW, int nH, int nT);
    
    // 创建RTT相机
    
    
    // 创建HUD相机
    osg::Camera* createHUDCamera(double left, double right, double bottom, double top);
    
    // 创建大气效果
    void createAtmosphereEffect(osg::StateSet* ss,osg::Camera* camera);
    
    // 私有成员变量
    double m_sunZenithAngle;
    double m_sunAzimuthAngle;
    double m_viewDistance;
    double m_viewZenithAngle;
    double m_viewAzimuthAngle;
    float m_exposure;
    bool m_initialized;  // 添加初始化标志
};

#endif // ATMOSPHERE_DEMO_H