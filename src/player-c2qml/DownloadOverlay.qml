import QtQuick 2.12
import QtQuick.Controls 2.12

Rectangle {
    id: downloadOverlay
    width: parent.width * 0.4
    height: 80
    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottomMargin: 30
    
    radius: 40
    color: "#DD111111" // Dark translucent
    border.color: "#44FFFFFF"
    border.width: 1
    
    // Smooth fade in/out
    opacity: LibFacade.isDownloading ? 1 : 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 500 } }

    Row {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        
        // Progress Circle or Icon
        Rectangle {
            width: 40; height: 40; radius: 20
            color: "#22FFFFFF"
            anchors.verticalCenter: parent.verticalCenter
            Text {
                anchors.centerIn: parent
                text: "⬇️"
                font.pixelSize: 20
            }
        }

        Column {
            width: parent.width - 70
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5

            Text {
                text: LibFacade.downloadLabel
                color: "white"
                font.pixelSize: 16
                font.bold: true
            }

            // Progress Bar
            Rectangle {
                width: parent.width
                height: 4
                radius: 2
                color: "#33FFFFFF"
                
                Rectangle {
                    width: parent.width * Math.min(Math.max(LibFacade.downloadProgress, 0.0), 1.0)
                    height: parent.height
                    radius: 2
                    color: "#00BAFF" // Tech Blue
                    
                    Behavior on width { NumberAnimation { duration: 300 } }
                }
            }
        }
    }
}
