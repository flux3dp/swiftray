import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import Vecty 1.0

Item {
    id: mainWindow
    visible: true
    anchors.fill: parent

    VCanvas {
        id: vcanvas
        anchors.fill: parent
        contentsScale: 1
    }


    /*Button {
        id: button
        x: 15
        width: 120
        height: 45
        anchors.top: parent.top
        anchors.topMargin: 15
        text: "Load"
        onClicked: openDialog.open()
    }

    Button {
        id: button1
        x: 150
        width: 120
        height: 45
        anchors.top: parent.top
        anchors.topMargin: 15
        text: "Save"
    }

    FileDialog {
        id: openDialog
        fileMode: FileDialog.OpenFile
        selectedNameFilter.index: 1
        nameFilters: ["SVG files (*.svg)"]
        folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: document.load(file)
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        defaultSuffix: document.fileType
        nameFilters: openDialog.nameFilters
        selectedNameFilter.index: document.fileType === "svg" ? 0 : 1
        folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: document.saveAs(file)
    }*/
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.9}
}
##^##*/

