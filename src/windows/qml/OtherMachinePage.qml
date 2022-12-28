import QtQuick 2.4
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3

Item {
    id: machineSetupPage
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
            text: "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\np, li { white-space: pre-wrap; }\n</style></head><body style=\" font-family:'Titillium Web'; font-size:13pt; font-weight:400; font-style:normal;\">\n<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Helvetica Neue'; font-size:24pt; font-weight:600;\">Machine Info</span></p></body></html>"
            font.pixelSize: 12
            font.hintingPreference: Font.PreferFullHinting
            textFormat: Text.RichText
            fontSizeMode: Text.FixedSize
            minimumPointSize: 32
            minimumPixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ColumnLayout {

            Layout.alignment: Qt.AlignHCenter
            width: parent.width / 2

            RowLayout {

                Text {
                    text: qsTr("Name")
                    font.pixelSize: 12
                    Layout.preferredWidth: 60
                }

                TextField {
                    id: textBoxName
                    text: "My Machine"
                    Layout.preferredWidth: 150
                }
            }

            RowLayout {

                Text {
                    text: qsTr("Width")
                    font.pixelSize: 12
                    Layout.preferredWidth: 60
                }

                SpinBox {
                    id: spinBoxWidth
                    value: 100
                    Layout.preferredWidth: 150
                    to: 10000
                    from: 5
                    editable: true
                    textFromValue: function(value, locale) {
                        return "%1 mm".arg(value);
                    }
                    valueFromText: function(text, locale) {
                        return parseInt(text.split(' ')[0]);
                    }
                }
            }

            RowLayout {
                Text {
                    text: qsTr("Height")
                    font.pixelSize: 12
                    Layout.preferredWidth: 60
                    Layout.fillWidth: false
                }

                SpinBox {
                    id: spinBoxHeight
                    value: 100
                    Layout.preferredWidth: 150
                    from: 0
                    to: 10000
                    enabled: true
                    editable: true
                    textFromValue: function(value, locale) {
                        return "%1 mm".arg(value);
                    }
                    valueFromText: function(text, locale) {
                        return parseInt(text.split(' ')[0]);
                    }
                }
            }

            RowLayout {
                Layout.topMargin: 15
                Text {
                    text: qsTr("Origin")
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
                    Layout.preferredWidth: 60
                    Layout.fillWidth: false
                }

                ButtonGroup { id: radioGroup }
                ColumnLayout {
                    RowLayout {
                        Layout.alignment: Qt.AlignLeft
                        RadioButton {
                            id: topLeft
                            font.hintingPreference: Font.PreferFullHinting
                            LayoutMirroring.enabled: true
                            ButtonGroup.group: radioGroup
                        }

                        RadioButton {
                            id: topRight
                            font.hintingPreference: Font.PreferFullHinting
                            ButtonGroup.group: radioGroup
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignLeft
                        RadioButton {
                            id: bottomLeft
                            font.hintingPreference: Font.PreferFullHinting
                            checked: true
                            LayoutMirroring.enabled: true
                            ButtonGroup.group: radioGroup
                        }

                        RadioButton {
                            id: bottomRight
                            font.hintingPreference: Font.PreferFullHinting
                            ButtonGroup.group: radioGroup
                        }
                    }
                }
            }

        }


        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.fill: parent
                anchors.rightMargin: 0
                anchors.bottomMargin: 0
                anchors.leftMargin: 0
                anchors.topMargin: 0
                color: "transparent"
            }
        }

        Button {
            id: nextJump
            text: qsTr("Create Machine Profile")
            highlighted: true
            Layout.alignment: Qt.AlignHCenter
            onHoveredChanged: opacity = hovered ? 0.8 : 1
            onClicked: {
                createOtherProfile(textBoxName.text, spinBoxWidth.value, spinBoxHeight.value,
                                   topLeft.checked ? 0 : (topRight.checked ? 1 : (bottomLeft.checked ? 2 : 3)));
                stackView.push(setupSuccessfulPage)
            }
            Layout.preferredWidth: 220
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


