import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3

Item {
    id: setupSuccessfulPage
    width: 400
    height: 400
    ColumnLayout {
        anchors.fill: parent

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.fill: parent
                color: "transparent"
            }
        }

        Text {
            id: text1
            width: 265
            height: 30
            text: "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\np, li { white-space: pre-wrap; }\n</style></head><body style=\" font-family:'Titillium Web'; font-size:13pt; font-weight:400; font-style:normal;\">\n<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Helvetica Neue'; font-size:24pt; font-weight:600;\">All Done</span></p></body></html>"
            font.pixelSize: 12
            textFormat: Text.RichText
            fontSizeMode: Text.FixedSize
            minimumPointSize: 32
            minimumPixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }


        Item {
            id: item1
            width: parent.parent.width
            height: 200

            AnimatedImage {
                id: checkMark
                width: 200
                height: 200
                source: "../../../resources/images/checkmark.gif"
                speed: 1.3
                anchors.horizontalCenter: parent.horizontalCenter
                fillMode: Image.PreserveAspectFit
                Component.onCompleted: this.currentFrame = 30
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.fill: parent
                color: "transparent"
            }
        }

        Button {
            id: nextJump
            text: qsTr("Let's Start!")
            Layout.alignment: Qt.AlignHCenter
            onHoveredChanged: opacity = hovered ? 0.8 : 1
            onClicked: setupComplete()
            Layout.preferredWidth: 200
            highlighted: true
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.fill: parent
                color: "transparent"
            }
        }
    }
}
