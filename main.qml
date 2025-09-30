import QtQuick 2.15
import QtQuick.Window 2.15
import OSGIntegration 1.0

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("OSG and QML Integration")

    OSGViewport {
        id: osgViewport
        anchors.fill: parent
    }
}