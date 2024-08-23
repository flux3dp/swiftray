import QtQuick
import QtQuick.Window
import QtQuick.Controls
import Swiftray 1.0

Item {
    id: mainWindow
    visible: true
    anchors.fill: parent

    Canvas {
        id: vcanvas
        anchors.fill: parent
        contentsScale: 1
    }
}
