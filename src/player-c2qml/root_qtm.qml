import QtQuick 2.12
import QtMultimedia 5.12
import QtWebView 1.1

Item
{
    id: root
    width: 800
    height: 600

    // VPN Status Indicator (Visual feedback for testing)
    Rectangle {
        id: vpnStatusIndicator
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        width: 44
        height: 44
        radius: 22
        color: vpnConfig && vpnConfig.isEnabled ? "#00ff00" : "#ff4444"
        opacity: vpnConfig && vpnConfig.isEnabled ? 0.8 : 0.4
        border.color: "white"
        border.width: 2
        z: 9999 // Ensure it stays on top of media zones

        Text {
            anchors.centerIn: parent
            text: "🛡️"
            font.pixelSize: 24
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                console.log("VPN Indicator clicked, opening config...")
                vpnLoader.source = "qrc:/VpnConfigDialog.qml"
            }
        }

        SequentialAnimation on opacity {
            running: vpnConfig && vpnConfig.isEnabled
            loops: Animation.Infinite
            NumberAnimation { from: 0.8; to: 0.4; duration: 1000; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 0.4; to: 0.8; duration: 1000; easing.type: Easing.InOutQuad }
        }
    }

    Loader {
        id: vpnLoader
        anchors.fill: parent
        z: 10000
        focus: true
        
        onLoaded: {
            if (item) {
                item.cancel.connect(function() { vpnLoader.source = "" })
                item.saveConfig.connect(function() { vpnLoader.source = "" })
            }
        }
    }
}
