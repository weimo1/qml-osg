#ifndef UIHANDLER_H
#define UIHANDLER_H

#include <QObject>
#include <QString>
#include <osg/ref_ptr>
#include <osgViewer/View>
#include <osg/Group>
#include <osg/Camera>

// 前向声明
class AtmosphereDemo;

class FastAtmosphere;

class FullscreenAtmosphere;

class UEatmosphere;

class UIHandler : public QObject
{
    Q_OBJECT

public:
    explicit UIHandler(QObject *parent = nullptr);
    ~UIHandler();
    
    // 加载OSG文件的主函数
    void loadOSGFile(osgViewer::View* view, osg::Group* rootNode, const QString& fileName);
    
    // 切换光照功能
    void toggleLighting(osgViewer::View* view, osg::Group* rootNode, bool enabled);
    
    // 创建大气效果
    void createAtmosphere(osg::Camera* camera, osg::Group* rootNode);
    
    // 在所有文件加载完成后将场景移动到原点
    void moveSceneToOrigin(osgViewer::View* view, osg::Group* rootNode);
    
    // 创建全屏大气效果
    void createFullscreenAtmosphere(osg::Camera* camera, osg::Group* rootNode);
    
    // 更新云参数
    void updateCloudParameters(float shapescale, float detailScale, float windSpeed,
                              float weatherScale, float curlStrength, float curlScale,
                              float erosionStrength, float windDirX, float windDirY, float windDirZ,
                              float weatherWindX, float weatherWindY);
    
    // 重置云参数为默认值
    void resetCloudParameters();
    
    // 更新鼠标位置
    void updateMousePosition(float x, float y);

signals:
    void fileLoadSuccess(const QString& fileName);
    void fileLoadError(const QString& fileName, const QString& error);

private:
    // 加载单个OSG文件
    void loadSingleOSGFile(osgViewer::View* view, osg::Group* rootNode, const QString& fileName);
    
    // 从目录加载OSG文件
    void loadOSGFilesFromDirectory(osgViewer::View* view, osg::Group* rootNode, const QString& dirPath);
    
    // 适应视图以显示所有模型
    void fitToView(osgViewer::View* view, osg::Group* rootNode);
    
    // 大气效果实例
    osg::ref_ptr<AtmosphereDemo> m_atmosphereDemo;


    osg::ref_ptr<FullscreenAtmosphere> m_fullscreenAtmosphereDemo;

    osg::ref_ptr<FastAtmosphere> m_fastAtmosphereDemo;
    
};

#endif // UIHANDLER_H