import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import Vecty 1.0
import Qt.labs.platform 1.1

Window {
    id: mainWindow
    width: 1280
    height: 720
    visible: true
    title: qsTr("Vecty")

    MenuBar {
        Menu {
            id: fileMenu
            title: "File"
            MenuItem {
                text: "Open"
                shortcut: StandardKey.Open
                onTriggered: openDialog.open()
            }
            MenuItem {
                text: "Save"
                shortcut: StandardKey.Save
            }
            MenuItem {
                separator: true
            }
            MenuItem {
                text: "Close"
                shortcut: StandardKey.Close
                onTriggered: mainWindow.close()
            }
        }
        Menu {
            id: editMenu
            title: "Edit"
            MenuItem {
                text: "Undo"
                shortcut: StandardKey.Undo
            }
            MenuItem {
                text: "Redo"
                shortcut: StandardKey.Redo
            }
            MenuItem {
                separator: true
            }
            MenuItem {
                text: "Cut"
                shortcut: StandardKey.Cut
                onTriggered: vcanvas.editCut()
            }
            MenuItem {
                text: "Copy"
                shortcut: StandardKey.Copy
                onTriggered: vcanvas.editCopy()
            }
            MenuItem {
                text: "Paste"
                shortcut: StandardKey.Paste
                onTriggered: vcanvas.editPaste()
            }
            MenuItem {
                text: "Delete"
                shortcut: StandardKey.Delete
                onTriggered: vcanvas.editDelete()
            }
        }
    }

    VCanvas {
        id: vcanvas
        anchors.fill: parent
        anchors.margins: 20
        anchors.topMargin: 75
        contentsScale: 1
    }

    Button {
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
    }

    VDoc {
        id: document
        canvas: vcanvas
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66}D{i:4}
}
##^##*/

