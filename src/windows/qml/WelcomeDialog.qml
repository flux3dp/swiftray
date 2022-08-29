import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3

Rectangle {
    id: startupDialog
    width: 400
    height: 400
    color: "#F8F8F8"

    signal setupComplete
    signal createStandardProfile(string brand, string model)
    signal createOtherProfile(string name, int width, int height, int origin)

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: ColumnLayout {
            id: column
            anchors.fill: parent

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
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

            Item {
                id: item1
                width: parent.parent.width
                height: 200

                Image {
                    id: image
                    width: 200
                    height: 200
                    source: "../../../resources/images/icon.png"
                    anchors.horizontalCenter: parent.horizontalCenter
                    fillMode: Image.PreserveAspectFit
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    height: 47
                    anchors.fill: parent
                    color: "transparent"
                }
            }

            Text {
                id: text1
                width: 265
                height: 75
                color: "#333333"
                text: qsTr("Welcome!
This is your first time using Swiftray,
what would you like to do first?")
                font.pixelSize: 12
                lineHeight: 1.2
                textFormat: Text.PlainText
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

            Button {
                id: setupBtn
                text: qsTr("Setup My Machine Profile")
                flat: false
                highlighted: true
                Layout.preferredWidth: 200
                Layout.alignment: Qt.AlignHCenter
                onHoveredChanged: opacity = hovered ? 0.8 : 1
                onClicked: stackView.push(machineSetupPage)
            }

            Button {
                id: playAroundBtn
                text: qsTr("Play Around!")
                Layout.preferredWidth: 200
                Layout.alignment: Qt.AlignHCenter
                onHoveredChanged: opacity = hovered ? 0.8 : 1
                onClicked: setupComplete()
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

    Component {
        id: machineSetupPage
        MachineSetupPage {}
    }

    Component {
        id: setupSuccessfulPage
        SetupSuccessfulPage {}
    }

    Component {
        id: otherMachinePage
        OtherMachinePage {}
    }

    states: [
        State {
            name: "setup"
        }
    ]
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}
}
##^##*/

