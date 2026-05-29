import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: root

    signal success()
    signal cancel()

    // If true the pad is visible; callers toggle this
    property bool active: false

    visible: active
    anchors.fill: parent

    // Dim overlay
    Rectangle {
        anchors.fill: parent
        color: "#88000000"

        MouseArea { anchors.fill: parent } // block clicks to content behind
    }

    // Glassmorphism card
    Rectangle {
        id: card
        anchors.centerIn: parent
        width: 320
        height: 480
        radius: 20
        color: "#CC1a1a2e"
        border.color: "#44ffffff"
        border.width: 1

        layer.enabled: true

        // Title
        Text {
            id: title
            anchors {
                top: parent.top
                topMargin: 28
                horizontalCenter: parent.horizontalCenter
            }
            text: "Enter PIN"
            color: "white"
            font.pixelSize: 22
            font.bold: true
        }

        // Dot indicators
        Row {
            id: dots
            anchors {
                top: title.bottom
                topMargin: 20
                horizontalCenter: parent.horizontalCenter
            }
            spacing: 16

            Repeater {
                model: 4
                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    color: index < enteredPin.length ? "white" : "transparent"
                    border.color: "white"
                    border.width: 2
                }
            }
        }

        // Error label
        Text {
            id: errorLabel
            anchors {
                top: dots.bottom
                topMargin: 8
                horizontalCenter: parent.horizontalCenter
            }
            text: ""
            color: "#ff6b6b"
            font.pixelSize: 13
            visible: text !== ""
        }

        // Keypad grid
        Grid {
            id: keypad
            anchors {
                top: errorLabel.bottom
                topMargin: 16
                horizontalCenter: parent.horizontalCenter
            }
            columns: 3
            spacing: 12

            Repeater {
                model: ["1","2","3","4","5","6","7","8","9","","0","DEL"]

                Rectangle {
                    width: 72
                    height: 56
                    radius: 12
                    color: modelData === "" ? "transparent" : (keyArea.pressed ? "#66ffffff" : "#33ffffff")
                    border.color: modelData === "" ? "transparent" : "#44ffffff"
                    border.width: 1
                    visible: modelData !== "" || index === 9  // keep empty slot for layout

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: "white"
                        font.pixelSize: modelData === "DEL" ? 14 : 22
                        font.bold: true
                    }

                    MouseArea {
                        id: keyArea
                        anchors.fill: parent
                        enabled: modelData !== ""
                        onClicked: {
                            if (modelData === "DEL") {
                                if (enteredPin.length > 0)
                                    enteredPin = enteredPin.slice(0, -1)
                            } else if (enteredPin.length < 4) {
                                enteredPin += modelData
                                if (enteredPin.length === 4)
                                    checkPin()
                            }
                        }
                    }
                }
            }
        }

        // Cancel button
        Text {
            anchors {
                top: keypad.bottom
                topMargin: 24
                horizontalCenter: parent.horizontalCenter
            }
            text: "Cancel"
            color: "#aaaaaa"
            font.pixelSize: 15
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    reset()
                    root.cancel()
                }
            }
        }

        // Shake animation — use horizontalCenterOffset to avoid conflict with anchors.centerIn
        SequentialAnimation {
            id: shakeAnim
            PropertyAnimation { target: card; property: "anchors.horizontalCenterOffset"; to: -14; duration: 50 }
            PropertyAnimation { target: card; property: "anchors.horizontalCenterOffset"; to:  14; duration: 50 }
            PropertyAnimation { target: card; property: "anchors.horizontalCenterOffset"; to:  -9; duration: 40 }
            PropertyAnimation { target: card; property: "anchors.horizontalCenterOffset"; to:   9; duration: 40 }
            PropertyAnimation { target: card; property: "anchors.horizontalCenterOffset"; to:   0; duration: 30 }
            onStopped: {
                enteredPin = ""
                errorLabel.text = "Incorrect PIN. Try again."
            }
        }
    }

    property string enteredPin: ""

    function checkPin() {
        var expected = LibFacade.managementPin
        if (enteredPin === expected) {
            reset()
            root.success()
        } else {
            shakeAnim.start()
        }
    }

    function reset() {
        enteredPin = ""
        errorLabel.text = ""
    }

    onActiveChanged: {
        if (active) reset()
    }
}
