#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QString>
#include "UIHandler.h"
#include "osgWindow.h"
#include <osg/MatrixTransform>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingBox>
#include <osg/ref_ptr>

class FullscreenAtmosphere;

class UEatmosphere;

class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(GraphicsWindowQt* viewWidget, QObject *parent = nullptr);
    ~Controller();

public slots:
    void onLoadFileRequested();
    void onToggleLightingRequested(bool enabled);
    void onCreateAtmosphereRequested();
    void onFileLoadSuccess(const QString& fileName);
    void onFileLoadError(const QString& fileName, const QString& error);
    void onCreateFullscreenAtmosphereRequested();
    void onUpdateCloudParameters(float shapescale, float detailScale, float windSpeed,
                                float weatherScale, float curlStrength, float curlScale,
                                float erosionStrength, float windDirX, float windDirY, float windDirZ,
                                float weatherWindX, float weatherWindY);
    void onResetCloudParameters();
    
    void onUpdateMousePosition(float x, float y);
    
    void onCreateFullscreenTriangleRequested();

private:
    // 注意：我们已经将moveSceneToOrigin功能移到UIHandler中，所以这里不再需要这个函数
    
    GraphicsWindowQt* m_viewWidget;
    UIHandler* m_uiHandler;
    
    osg::ref_ptr<FullscreenAtmosphere> m_fullscreenAtmosphereDemo;
    osg::ref_ptr<UEatmosphere> ueAtmosphereDemo;
};

#endif // CONTROLLER_H