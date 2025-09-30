#ifndef OSGRENDERER_H
#define OSGRENDERER_H

#include <QQuickFramebufferObject>
#include <osg/ref_ptr>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Camera>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Vec3>
#include <osg/Vec4>
#include <QDebug>
#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/Group>
class OSGViewport;

class OSGRenderer : public QQuickFramebufferObject::Renderer
{
public:
    OSGRenderer();
    ~OSGRenderer();

    void render() override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

private:
    void createScene();
    void initializeOSG(int width, int height);

    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<osg::Group> m_root;
};

#endif // OSGRENDERER_H