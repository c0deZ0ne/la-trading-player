import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: pinPadRoot
    anchors.fill: parent
    z: 99999

    property string currentInput: ""
    property string targetPin: (typeof LibFacade !== "undefined" && LibFacade.managementPin) ? LibFacade.managementPin.toString().trim() : "0000"

    signal success()
    signal cancel()

    Component.onCompleted: {
        console.log("[Security] PIN Pad Loaded. Target PIN is: '" + targetPin + "'")
    }

    // Dark overlay — blocks clicks reaching content behind
    Rectangle {
        anchors.fill: parent
        color: "#CC000000"
        MouseArea { anchors.fill: parent }
    }

    // Glassmorphism card
    Rectangle {
        id: container
        width: 320
        height: 500
        anchors.centerIn: parent
        radius: 28
        color: "#1AFFFFFF"
        border.color: "#33FFFFFF"
        border.width: 1

        // Subtle inner gradient
        Rectangle {
            anchors.fill: parent
            radius: 28
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#15FFFFFF" }
                GradientStop { position: 1.0; color: "#05000000" }
            }
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            spacing: 28
            width: 280

            // Title
            Text {
                text: "Security PIN"
                color: "white"
                font.pixelSize: 22
                font.weight: Font.Light
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.9
            }

            // PIN dots indicator
            Row {
                spacing: 18
                anchors.horizontalCenter: parent.horizontalCenter

                Repeater {
                    model: 4
                    delegate: Rectangle {
                        width: 14; height: 14; radius: 7
                        color: index < pinPadRoot.currentInput.length ? "#00E5FF" : "transparent"
                        border.color: index < pinPadRoot.currentInput.length ? "#00E5FF" : "#88FFFFFF"
                        border.width: 2
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }

            // Keypad grid
            Grid {
                columns: 3
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter

                Repeater {
                    model: ["1","2","3","4","5","6","7","8","9","C","0","⌫"]
                    delegate: Rectangle {
                        id: keyRect
                        width: 72; height: 72; radius: 36
                        color: keyArea.pressed ? "#44FFFFFF" : "#18FFFFFF"
                        border.color: "#22FFFFFF"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: "white"
                            font.pixelSize: modelData === "⌫" ? 20 : 24
                            font.weight: Font.Light
                        }

                        MouseArea {
                            id: keyArea
                            anchors.fill: parent
                            onClicked: {
                                if (modelData === "C") {
                                    pinPadRoot.currentInput = ""
                                } else if (modelData === "⌫") {
                                    pinPadRoot.currentInput = pinPadRoot.currentInput.slice(0, -1)
                                } else if (pinPadRoot.currentInput.length < 4) {
                                    pinPadRoot.currentInput += modelData
                                    if (pinPadRoot.currentInput.length === 4) {
                                        checkPin()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Cancel button
            Text {
                text: "Cancel"
                color: "#99FFFFFF"
                font.pixelSize: 15
                anchors.horizontalCenter: parent.horizontalCenter

                MouseArea {
                    anchors.fill: parent
                    onClicked: pinPadRoot.cancel()
                }
            }
        }
    }

    // Shake animation on wrong PIN
    SequentialAnimation {
        id: shakeAnimation
        PropertyAnimation { target: container; property: "anchors.horizontalCenterOffset"; to: -18; duration: 50 }
        PropertyAnimation { target: container; property: "anchors.horizontalCenterOffset"; to:  18; duration: 50 }
        PropertyAnimation { target: container; property: "anchors.horizontalCenterOffset"; to: -12; duration: 50 }
        PropertyAnimation { target: container; property: "anchors.horizontalCenterOffset"; to:  12; duration: 50 }
        PropertyAnimation { target: container; property: "anchors.horizontalCenterOffset"; to:   0; duration: 50 }
    }

    function checkPin() {
        console.log("[Security] Checking PIN: '" + currentInput + "' against target: '" + targetPin + "'")
        if (currentInput === targetPin) {
            console.log("[Security] PIN Correct — Access Granted")
            success()
        } else {
            console.warn("[Security] PIN Incorrect")
            shakeAnimation.start()
            currentInput = ""
        }
    }
}
