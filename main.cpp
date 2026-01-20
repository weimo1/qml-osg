#include <QApplication>
#include "osgWindow.h"
#include "Controller.h"
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>

// 创建全局变量以便在事件处理器中访问
QLabel* mouseLabelPtr = nullptr;
QLabel* cameraLabelPtr = nullptr;
QMainWindow* mainWindowPtr = nullptr;

// 重写QMainWindow的resizeEvent方法
class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {}
    
protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QMainWindow::resizeEvent(event);
        
        // 更新鼠标标签的位置到右下角
        if (mouseLabelPtr) {
            mouseLabelPtr->move(this->width() - 130, this->height() - 30);
        }
        
        // 更新相机标签的位置到右上角
        if (cameraLabelPtr) {
            cameraLabelPtr->move(this->width() - 510, 10);
        }
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindowPtr = &mainWindow;
    mainWindow.setWindowTitle("OSG QOpenGLWidget Integration");
    
    // 创建我们的OSG窗口部件
    GraphicsWindowQt* osgWidget = new GraphicsWindowQt();
    
    // 创建控制器
    Controller* controller = new Controller(osgWidget);
    
    // 设置为主窗口的中心部件
    QWidget* centralWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(osgWidget);
    centralWidget->setLayout(layout);
    mainWindow.setCentralWidget(centralWidget);
    
    // 创建相机位置显示标签（放在右上角）
    QLabel* cameraLabel = new QLabel("Cam: X:0.00 Y:0.00 Z:0.00", centralWidget);
    cameraLabelPtr = cameraLabel;  // 保存指针以便在resizeEvent中使用
    cameraLabel->setStyleSheet("QLabel { background-color : rgba(255, 255, 255, 180); color : black; font-size: 14px; padding: 3px; };");
    cameraLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    cameraLabel->setMaximumHeight(25);  // 限制标签高度
    cameraLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    cameraLabel->setMinimumWidth(500);  // 设置最小宽度以确保显示完整

    cameraLabel->move(mainWindow.width() - 510, 10);  // 设置位置到右上角
    cameraLabel->show();  // 确保标签显示
    
    // 创建鼠标位置显示标签（放在右下角）
    QLabel* mouseLabel = new QLabel("Mouse: X:0 Y:0", centralWidget);
    mouseLabelPtr = mouseLabel;  // 保存指针以便在resizeEvent中使用
    mouseLabel->setStyleSheet("QLabel { background-color : rgba(255, 255, 255, 180); color : black; font-size: 14px; padding: 3px; };");
    mouseLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    mouseLabel->setMaximumHeight(20);  // 限制标签高度
    mouseLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mouseLabel->setMinimumWidth(120);  // 设置最小宽度以确保显示完整
    mouseLabel->move(mainWindow.width() - 130, mainWindow.height() - 30);  // 设置位置
    mouseLabel->show();  // 确保标签显示
    
    // 连接信号和槽
    QObject::connect(osgWidget, &GraphicsWindowQt::cameraPositionChanged, cameraLabel, &QLabel::setText);
    QObject::connect(osgWidget, &GraphicsWindowQt::mousePositionChanged, mouseLabel, &QLabel::setText);
    
    mainWindow.resize(1200, 800);
    mainWindow.show();

    return app.exec();
}