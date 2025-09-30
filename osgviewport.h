#ifndef OSGVIEWPORT_H
#define OSGVIEWPORT_H

#include <QQuickFramebufferObject>

// 前向声明
class OSGRenderer;

/**
 * @brief OSGViewport 类
 * 
 * 这个类继承自 QQuickFramebufferObject，用于在 QML 中集成 OpenSceneGraph (OSG) 渲染窗口。
 * 它允许在 QML 界面中显示 3D 内容，并与 Qt Quick 的渲染循环集成。
 */
class OSGViewport : public QQuickFramebufferObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit OSGViewport(QQuickItem *parent = nullptr);
    
    /**
     * @brief 创建渲染器
     * @return 返回一个 Renderer 对象，用于执行实际的渲染操作
     */
    Renderer *createRenderer() const override;
    
};

#endif // OSGVIEWPORT_H