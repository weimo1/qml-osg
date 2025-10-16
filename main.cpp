#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>
#include <QQuickWindow>
#include <QQmlContext>
#include <QDir>
#include <QQuickStyle>
#include "simpleosgviewer.h"

int main(int argc, char *argv[])
{
    // 设置OpenGL图形API - 这对于OSG集成至关重要
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    
    // 设置支持自定义的样式
    QQuickStyle::setStyle("Basic");
    
    // 设置OpenGL格式 - 使用兼容性更好的设置
    QSurfaceFormat format;
    format.setVersion(2, 1);  // 使用较低版本以提高兼容性
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);

    // 注册自定义QML类型
    qmlRegisterType<SimpleOSGViewer>("OSGViewer", 1, 0, "SimpleOSGViewer");

    QQmlApplicationEngine engine;
    
    // 添加Qt 6的QML路径
    engine.addImportPath("D:/Qt6/6.9.2/msvc2022_64/qml");
    
    const QUrl url(QStringLiteral("qrc:///qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);

    return app.exec();
}