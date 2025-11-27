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
};

#endif // UIHANDLER_H