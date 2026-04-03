import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

Rectangle {
    id: vpnRoot
    anchors.fill: parent
    color: "transparent"

    // Background overlay for when loaded standalone
    Rectangle {
        anchors.fill: parent
        color: "#050505"
        opacity: 0.9
        visible: parent.width > vpnCard.width || parent.height > vpnCard.height
    }

    property string errorMessage: ""
    property bool isConnecting: false

    readonly property string publicKey: vpnConfig ? vpnConfig.publicKey : "N/A"

    signal saveConfig()
    signal cancel()
    signal generateIdentity()

    readonly property bool isMobile: Screen.primaryOrientation === Qt.PortraitOrientation || vpnRoot.width < 600
    readonly property real cardWidth: isMobile ? vpnRoot.width : Math.min(vpnRoot.width * 0.9, 450)
    readonly property real cardHeight: isMobile ? vpnRoot.height : Math.min(vpnRoot.height * 0.95, 750)
    readonly property real cardRadius: isMobile ? 0 : 12
    readonly property real baseFontSize: isMobile ? 16 : 18
    readonly property real smallFontSize: baseFontSize * 0.8
    readonly property real fieldHeight: isMobile ? 60 : 65

    Rectangle {
        id: vpnCard
        width: vpnRoot.cardWidth
        height: vpnRoot.cardHeight
        anchors.centerIn: parent
        color: "#1e1e1e"
        radius: vpnRoot.cardRadius
        clip: true

        // Header with Back Button
        Rectangle {
            id: header
            width: parent.width
            height: 60
            color: "transparent"
            z: 10

            Button {
                id: backBtn
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 10
                width: 44
                height: 44
                contentItem: Text {
                    text: "←"
                    color: "white"
                    font.pixelSize: 28
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Item {}
                onClicked: vpnRoot.cancel()
            }

            Text {
                text: "SECURE VPN SETUP"
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: vpnRoot.baseFontSize
                font.weight: Font.Bold
            }
        }

        ScrollView {
            id: vpnScrollView
            anchors.fill: parent
            anchors.topMargin: header.height
            contentWidth: availableWidth
            contentHeight: vpnLayout.implicitHeight + 40
            clip: true
            background: Rectangle { color: "transparent" }

            ColumnLayout {
                id: vpnLayout
                width: parent.width
                spacing: 15

                // Section: Device Identity
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    color: "#252525"
                    radius: 8
                    border.color: "#333333"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 8

                        Text {
                            text: "DEVICE PUBLIC KEY (SHARE WITH CMS)"
                            color: "#ffff00"
                            font.pixelSize: vpnRoot.smallFontSize * 0.8
                            font.weight: Font.Bold
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            color: "#1a1a1a"
                            radius: 4
                            
                            Text {
                                text: vpnRoot.publicKey
                                anchors.centerIn: parent
                                width: parent.width - 20
                                color: "white"
                                font.pixelSize: vpnRoot.baseFontSize * 0.9
                                font.family: "Monospace"
                                elide: Text.ElideMiddle
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Button {
                                id: copyBtn
                                text: "COPY KEY"
                                Layout.fillWidth: true
                                Layout.preferredHeight: 35
                                contentItem: Text {
                                    text: copyBtn.text
                                    font.pixelSize: vpnRoot.smallFontSize
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: "#ffff00"
                                    radius: 4
                                }
                                onClicked: {
                                    if (vpnConfig) {
                                        vpnConfig.copyToClipboard(vpnRoot.publicKey)
                                        copyBtn.text = "COPIED!"
                                        copyTimer.start()
                                    }
                                }
                            }

                            Timer {
                                id: copyTimer
                                interval: 2000
                                onTriggered: copyBtn.text = "COPY KEY"
                            }

                            Button {
                                id: refreshBtn
                                text: "REGENERATE"
                                Layout.fillWidth: true
                                Layout.preferredHeight: 35
                                contentItem: Text {
                                    text: refreshBtn.text
                                    font.pixelSize: vpnRoot.smallFontSize
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: "#444444"
                                    radius: 4
                                }
                                onClicked: {
                                    if (vpnConfig) vpnConfig.generateIdentity()
                                }
                            }
                        }
                    }
                }

                // Section: Server Configuration
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 12

                    // Server Public Key
                    TextField {
                        id: serverKeyInput
                        placeholderText: "Enter Server Public Key from CMS"
                        text: vpnConfig ? vpnConfig.serverPublicKey : ""
                        Layout.fillWidth: true
                        Layout.preferredHeight: vpnRoot.fieldHeight
                        color: "white"
                        font.pixelSize: vpnRoot.baseFontSize * 0.9
                        leftPadding: 15
                        topPadding: 20
                        placeholderTextColor: "#666666"
                        background: vpnFieldBg("SERVER PUBLIC KEY", serverKeyInput.activeFocus)
                        onTextChanged: if (vpnConfig && activeFocus) vpnConfig.serverPublicKey = text
                    }

                    // Server Endpoint
                    TextField {
                        id: serverIpInput
                        placeholderText: "vpn.example.com:51820"
                        text: vpnConfig ? vpnConfig.serverEndpoint : ""
                        Layout.fillWidth: true
                        Layout.preferredHeight: vpnRoot.fieldHeight
                        color: "white"
                        font.pixelSize: vpnRoot.baseFontSize * 0.9
                        leftPadding: 15
                        topPadding: 20
                        placeholderTextColor: "#666666"
                        background: vpnFieldBg("VPN SERVER ENDPOINT", serverIpInput.activeFocus)
                        onTextChanged: if (vpnConfig && activeFocus) vpnConfig.serverEndpoint = text
                    }

                    // Virtual IP
                    TextField {
                        id: virtualIpInput
                        placeholderText: "10.8.0.2/32"
                        text: vpnConfig ? vpnConfig.virtualIp : ""
                        Layout.fillWidth: true
                        Layout.preferredHeight: vpnRoot.fieldHeight
                        color: "white"
                        font.pixelSize: vpnRoot.baseFontSize * 0.9
                        leftPadding: 15
                        topPadding: 20
                        placeholderTextColor: "#666666"
                        background: vpnFieldBg("VIRTUAL IP (CLIENT)", virtualIpInput.activeFocus)
                        onTextChanged: if (vpnConfig && activeFocus) vpnConfig.virtualIp = text
                    }
                }

                // Action Buttons
                Button {
                    id: connectBtn
                    text: vpnRoot.isConnected ? "CONNECTED" : (vpnRoot.isConnecting ? "CONNECTING..." : "SAVE & CONNECT")
                    enabled: !vpnRoot.isConnecting && !vpnRoot.isConnected
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    contentItem: Text {
                        text: connectBtn.text
                        font.pixelSize: vpnRoot.baseFontSize
                        font.weight: Font.Bold
                        color: vpnRoot.isConnected ? "#00ff00" : "black"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        
                        SequentialAnimation on opacity {
                            running: vpnRoot.isConnecting
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.4; duration: 800; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 0.4; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                        }
                    }
                    background: Rectangle {
                        color: vpnRoot.isConnected ? "#1a3300" : (vpnRoot.isConnecting ? "#888800" : (connectBtn.pressed ? "#d4cc00" : "#ffff00"))
                        radius: 8
                        border.color: vpnRoot.isConnected ? "#00ff00" : "transparent"
                        border.width: vpnRoot.isConnected ? 2 : 0
                    }
                    onClicked: {
                        if (vpnConfig) {
                            vpnConfig.setIsEnabled(true)
                            vpnConfig.save()
                            LibFacade.saveVpnConfig()
                            vpnConfig.startVpn()
                        }
                    }
                }

                Text {
                    text: vpnRoot.errorMessage !== "" ? vpnRoot.errorMessage : 
                          (vpnRoot.isConnected ? "VPN Tunnel is active and secured." : "Generate an identity and register its Public Key in your CMS.")
                    color: vpnRoot.errorMessage !== "" ? "#ff4444" : (vpnRoot.isConnected ? "#00ff00" : "#888888")
                    font.pixelSize: vpnRoot.smallFontSize
                    Layout.fillWidth: true
                    Layout.leftMargin: 30
                    Layout.rightMargin: 30
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }

                Item { Layout.preferredHeight: 20 }
            }
        }
    }

    // Helper for background styling
    function vpnFieldBg(label, isFocused) {
        return Qt.createQmlObject('
            import QtQuick 2.12
            Rectangle {
                color: "#252525"
                radius: 8
                border.color: ' + (isFocused ? '"#ffff00"' : '"#333333"') + '
                border.width: ' + (isFocused ? '2' : '1') + '
                Text {
                    text: "' + label + '"
                    color: "#ffff00"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 15
                    anchors.topMargin: 6
                }
            }
        ', vpnRoot);
    }
}
