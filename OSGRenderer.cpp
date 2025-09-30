#include "OSGRenderer.h"
#include "OSGViewport.h"
#include <QOpenGLFramebufferObject>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/MatrixTransform>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Camera>
#include <QDebug>
#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/Group>

OSGRenderer::OSGRenderer()
{

}

OSGRenderer::~OSGRenderer()
{
    // 清理资源
}

void OSGRenderer::initializeOSG(int width, int height){
    if (!QOpenGLContext::currentContext()) {
        qCritical() << "No OpenGL context available!";
        return;
    }
    

    m_viewer = new osgViewer::Viewer();

     osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow = 
        new osgViewer::GraphicsWindowEmbedded(0, 0, width, height);
    
    // 获取相机并设置渲染目标
    osg::Camera* camera = m_viewer->getCamera();
    camera->setGraphicsContext(graphicsWindow.get());
    camera->setViewport(new osg::Viewport(0, 0, width, height));
    camera->setProjectionMatrixAsPerspective(45.0f, static_cast<double>(width)/static_cast<double>(height), 1.0, 10000.0);
    camera->setClearColor(osg::Vec4(0.2f, 0.3f, 0.8f, 1.0f));
    // 创建场景根节点
    m_root = new osg::Group();
    
    // 创建一个简单的场景
    createScene();
    
    // 设置场景数据
    m_viewer->setSceneData(m_root.get());
    
    qDebug() << "OSGRenderer initialized with scene";
}

void OSGRenderer::render()
{
    if (!QOpenGLContext::currentContext()) {
        qWarning() << "No OpenGL context, skipping render";
        return;
    }
    //获取窗口的大小
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];

    if (!m_viewer) {
        qDebug() << "Initializing OSG...";
        initializeOSG(width, height);
    }

    if (m_viewer && m_viewer->isRealized()) {
        qDebug() << "Viewer is realized, rendering frame";
        //设置窗口的大小，获取OpenGL上下文
        m_viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
        m_viewer->getCamera()->setProjectionMatrixAsPerspective(30.0f, 
            static_cast<double>(width)/static_cast<double>(height), 1.0, 10000.0);
             if (QOpenGLContext::currentContext()) {
            GLuint fboId = QOpenGLContext::currentContext()->defaultFramebufferObject();
            if (m_viewer->getCamera()->getGraphicsContext()) {
                m_viewer->getCamera()->getGraphicsContext()->setDefaultFboId(fboId);
            }
        }
        //渲染
        m_viewer->frame();
        //恢复状态
        glPopClientAttrib();
        glPopAttrib();
        } else {
        qDebug() << "Viewer not realized, showing blue background";
        // 如果OSG未正确初始化，则显示蓝色背景
        glViewport(0, 0, width, height);
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
}

QOpenGLFramebufferObject *OSGRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4); // 抗锯齿
    return new QOpenGLFramebufferObject(size, format);
}

void OSGRenderer::createScene(){
    // 创建场景
    //看不到图形可能是视角的问题
}