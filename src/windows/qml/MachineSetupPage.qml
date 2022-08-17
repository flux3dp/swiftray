import QtQuick 2.4
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3

import MachineSettings 1.0


Item {
    id: machineSetupPage
    width: 400
    height: 400

    MachineSettings {
        id: machineSettings
    }

    Component.onCompleted: function() {
        comboBoxModel.model = machineSettings.models("Lazervida");
    }

    function checkMachine() {
        if (comboBoxBrand.currentText == "Other" || comboBoxModel.currentText == "Other") {
            stackView.push(otherMachinePage);
        } else {
            createStandardProfile(comboBoxBrand.currentText, comboBoxModel.currentText);
            stackView.push(setupSuccessfulPage);
        }
    }

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
            text: "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\np, li { white-space: pre-wrap; }\n</style></head><body style=\" font-family:'Titillium Web'; font-size:13pt; font-weight:400; font-style:normal;\">\n<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Helvetica Neue'; font-size:20pt; font-weight:400;\">Select Your Machine</span></p></body></html>"
            font.pixelSize: 12
            font.hintingPreference: Font.PreferFullHinting
            renderType: Text.QtRendering
            textFormat: Text.RichText
            fontSizeMode: Text.FixedSize
            minimumPointSize: 32
            minimumPixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.fill: parent
                color: "transparent"
            }
        }

        ComboBox {
            id: comboBoxBrand
            model: machineSettings.brands()
            textRole: qsTr("")
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 200
            onCurrentTextChanged: {
                comboBoxModel.model = machineSettings.models(comboBoxBrand.currentText);
            }
        }

        ComboBox {
            id: comboBoxModel
            model: []
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 200
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
            text: qsTr("Next")
            checkable: true
            highlighted: true
            Layout.alignment: Qt.AlignHCenter
            onHoveredChanged: opacity = hovered ? 0.8 : 1
            onClicked: checkMachine()
            Layout.preferredWidth: 200
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

/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}
}
##^##*/
