#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "OSGViewport.h"
#include <QQuickWindow>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // 设置OpenGL属性
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    
    // 设置表面格式
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); // 抗锯齿
    QSurfaceFormat::setDefaultFormat(format);
    
    // 设置图形API为OpenGL
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    
    QGuiApplication app(argc, argv);

    // 注册OSGViewport类型到QML
    qmlRegisterType<OSGViewport>("OSGIntegration", 1, 0, "OSGViewport");

    QQmlApplicationEngine engine;
    
    // 添加导入路径
    engine.addImportPath("D:/Qt6/6.9.2/msvc2022_64/qml");
    
    // 手动注入QML模块
    // 这里可以添加自定义的上下文属性或其他模块注入
    // 示例：注入自定义上下文属性
    // engine.rootContext()->setContextProperty("myContextProperty", QVariant("Hello from C++"));
    
    // 设置QML源文件
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);

    return app.exec();
}