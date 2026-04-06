import QtQuick 2.12
import QtMultimedia 5.12
import QtWebView 1.1

Item
{
    id: root
    anchors.fill: parent

    // Floating Settings Menu
    Item {
        id: floatingSettingsMenu
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        width: 60
        height: menuColumn.height + 80
        z: 10000

        property bool expanded: false

        // Background for expanded menu
        Rectangle {
            id: menuBg
            anchors.fill: parent
            radius: 30
            color: "#CC000000"
            border.color: "#33FFFFFF"
            border.width: 1
            opacity: floatingSettingsMenu.expanded ? 1 : 0
            visible: opacity > 0
            
            Behavior on opacity { NumberAnimation { duration: 250 } }
        }

        Column {
            id: menuColumn
            anchors.bottom: fabButton.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 15
            spacing: 15
            visible: floatingSettingsMenu.expanded
            opacity: floatingSettingsMenu.expanded ? 1 : 0
            
            Behavior on opacity { NumberAnimation { duration: 200 } }

            // Option 1: Player Configuration
            Rectangle {
                width: 44; height: 44; radius: 22
                color: "#22FFFFFF"
                Text { anchors.centerIn: parent; text: "⚙️"; font.pixelSize: 22 }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        floatingSettingsMenu.expanded = false;
                        MainApp.openConfigDialog();
                    }
                }
            }

            // Option 2: Network Configuration
            Rectangle {
                id: networkBtn
                width: 44; height: 44; radius: 22
                color: "#22FFFFFF"
                Text { anchors.centerIn: parent; text: "🌐"; font.pixelSize: 22 }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        floatingSettingsMenu.expanded = false;
                        MainApp.openNetworkSettings();
                    }
                }
            }

            // Option 3: VPN Configuration
            Rectangle {
                width: 44; height: 44; radius: 22
                color: vpnConfig && vpnConfig.isEnabled ? "#4400FF00" : "#22FFFFFF"
                Text { anchors.centerIn: parent; text: "🛡️"; font.pixelSize: 22 }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        floatingSettingsMenu.expanded = false;
                        vpnLoader.source = "qrc:/VpnConfigDialog.qml";
                    }
                }
            }
        }

        // Main FAB Button
        Rectangle {
            id: fabButton
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: 56
            height: 56
            radius: 28
            color: floatingSettingsMenu.expanded ? "#FF4444" : "#88000000"
            border.color: "white"
            border.width: 2
            
            Text {
                anchors.centerIn: parent
                text: floatingSettingsMenu.expanded ? "✕" : "⋮"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: floatingSettingsMenu.expanded = !floatingSettingsMenu.expanded
            }

            Behavior on color { ColorAnimation { duration: 200 } }
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
