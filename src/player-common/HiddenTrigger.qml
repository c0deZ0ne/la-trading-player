import QtQuick 2.12

Item {
    id: hiddenTrigger
    width: 120
    height: 120
    
    property int tapCount: 0
    signal triggered()
    
    // Diagnostic visual (invisible in production)
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        opacity: 0
        
        Text {
            anchors.centerIn: parent
            text: tapCount > 0 ? tapCount : "SECRET"
            color: "transparent"
            font.bold: true
        }
    }

    MouseArea {
        id: tapMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: {
            tapCount++
            resetTimer.restart()
            console.warn("[Gesture] Input Received. Count: " + tapCount)
            if (tapCount >= 5) {
                tapCount = 0
                hiddenTrigger.triggered()
            }
        }
    }
    
    Timer {
        id: resetTimer
        interval: 2000
        onTriggered: {
            if (tapCount > 0) {
                console.log("[Gesture] Tap count reset")
                tapCount = 0
            }
        }
    }
}
