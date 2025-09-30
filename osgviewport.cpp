#include "OSGViewport.h"
#include "OSGRenderer.h"
#include <QQuickWindow>
#include <QTimer>
#include <QDebug>

OSGViewport::OSGViewport(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    QTimer * timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &OSGViewport::update);
    timer->start(16); 
    qDebug() << "OSGViewport created";
}



QQuickFramebufferObject::Renderer *OSGViewport::createRenderer() const
{
    return new OSGRenderer();
}