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
                        settingsPinPad.pendingAction = function() { MainApp.openConfigDialog(); }
                        settingsPinPad.active = true;
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
                        settingsPinPad.pendingAction = function() { MainApp.openNetworkSettings(); }
                        settingsPinPad.active = true;
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
                        settingsPinPad.pendingAction = function() { vpnLoader.source = "qrc:/VpnConfigDialog.qml"; }
                        settingsPinPad.active = true;
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
            
            // SECURITY: Only visible after gesture
            visible: floatingSettingsMenu.expanded
            
            Text {
                anchors.centerIn: parent
                text: floatingSettingsMenu.expanded ? "×" : "•••"
                color: "white"
                font.pixelSize: floatingSettingsMenu.expanded ? 28 : 16
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: floatingSettingsMenu.expanded = !floatingSettingsMenu.expanded
            }

            Behavior on color { ColorAnimation { duration: 200 } }
        }
    }

    // HIDDEN GESTURE TRIGGER (Bottom Left)
    HiddenTrigger {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        z: 20000
        onTriggered: {
            console.warn("[Gesture] Menu Authorized")
            floatingSettingsMenu.expanded = true
        }
    }

    // PIN gate — shown before any settings dialog opens
    SettingsPinPad {
        id: settingsPinPad
        z: 25000
        property var pendingAction: null
        onSuccess: {
            if (pendingAction) pendingAction()
            pendingAction = null
            active = false
        }
        onCancel: {
            pendingAction = null
            active = false
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

    // New Download Progress Overlay
    DownloadOverlay {
        id: downloadProgressIndicator
        z: 30000 // Ensure it's above content but below critical dialogs
    }
}
