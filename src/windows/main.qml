import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import Vecty 1.0

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
