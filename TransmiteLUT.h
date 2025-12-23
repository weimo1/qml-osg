#ifndef TRANSMITELUT_H
#define TRANSMITELUT_H

#include <osg/Geode>
#include <osg/Uniform>
#include <osg/Camera>
#include <osg/Texture2D>
#include <osg/Program>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osg/Timer>

class TransmiteLUT; // 前向声明

class SaveImageCallback;

class TransmiteLUT : public osg::Geode
{
    friend class SaveImageCallback; // 声明友元类

public:
    TransmiteLUT();
    virtual ~TransmiteLUT();

    // 设置相机
    void setCamera(osg::Camera* camera);
    
    // 生成LUT纹理
    void generateLUT();
    
    // 导出LUT纹理为图像文件
    void exportLUT(const std::string& filename);
    
    // 获取LUT纹理
    osg::Texture2D* getTexture() { return transmittanceLUT.get(); }
    
    osg::Camera* createHUDCamera(double left, double right, double bottom, double top);
    
private:
    // 初始化几何体
    osg::Node* initGeometry();
    
    // 初始化着色器
    void initShaders();
    
    // 相机指针
    osg::Camera* m_camera;
    
    // 根节点引用
    osg::ref_ptr<osg::Node> m_rootNode;
    
    // LUT纹理
    osg::ref_ptr<osg::Texture2D> transmittanceLUT;
    
    // Uniform变量
    osg::ref_ptr<osg::Uniform> _atmosphereParams;
    
    // 添加时间变量用于回调
    osg::ref_ptr<osg::Uniform> _time;

    osg::ref_ptr<osg::Camera> rttCamera;
};

#endif // TRANSMITELUT_H