cmake_minimum_required(VERSION 3.16)
project(SimpleQMLApp)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置OSG路径（请根据实际安装路径修改）
set(OSG_ROOT "D:/OpenSceneGraph" CACHE PATH "Path to OpenSceneGraph root directory")
set(OSG_BUILD_PATH "${OSG_ROOT}/build")

# 检查OSG路径是否存在
if(NOT EXISTS ${OSG_ROOT})
    message(WARNING "OSG root directory not found: ${OSG_ROOT}")
else()
    message(STATUS "OSG root directory: ${OSG_ROOT}")
endif()

# 添加OSG包含目录
include_directories("${OSG_ROOT}/include")
include_directories("${OSG_BUILD_PATH}/include")

# 添加OSG库目录
link_directories("${OSG_BUILD_PATH}/lib")

# 添加OSG插件目录
set(OSG_PLUGIN_PATH "${OSG_BUILD_PATH}/bin/osgPlugins-3.6.5")

# 设置Qt 6路径
set(CMAKE_PREFIX_PATH "D:/Qt6/6.9.2/msvc2022_64")

# 查找Qt 6包
find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets QuickControls2 Qml OpenGL Gui)

# 启用自动moc、rcc和uic处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    simpleosgviewer.cpp
    simpleosgviewer.h
    simpleosgrenderer.cpp
    simpleosgrenderer.h
    mousehandler.cpp
    mousehandler.h
    SkyNode.cpp
    SkyNode.h
    viewmanager.cpp
    viewmanager.h
    uihandler.cpp
    uihandler.h
    AtmosphereDemo.cpp
    AtmosphereDemo.h
    qml.qrc
)

# 定义OSG库列表（不包含osgQt）
set(OSG_LIBRARIES
    optimized osg debug osgd
    optimized osgViewer debug osgViewerd
    optimized osgGA debug osgGAd
    optimized osgManipulator debug osgManipulatord
    optimized osgDB debug osgDBd
    optimized osgUtil debug osgUtild
    optimized osgText debug osgTextd
    optimized OpenThreads debug OpenThreadsd
)

# 链接Qt6库
target_link_libraries(${PROJECT_NAME} 
    Qt6::Core 
    Qt6::Quick 
    Qt6::Widgets
    Qt6::QuickControls2 
    Qt6::Qml
    Qt6::OpenGL
    Qt6::Gui
    ${OSG_LIBRARIES}
    opengl32
)