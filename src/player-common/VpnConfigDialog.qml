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

    // Map Backend Status to UI
    // Status codes: 0: Disconnected, 1: Connecting, 2: Connected, 3: Error
    readonly property int vpnStatus: vpnConfig ? vpnConfig.status : 0
    readonly property bool isConnecting: vpnStatus === 1
    readonly property bool isConnected: vpnStatus === 2
    readonly property bool hasError: vpnStatus === 3
    property string statusMessage: vpnConfig ? vpnConfig.errorMessage : "Backend Not Initialized"

    signal saveConfig()
    signal cancel()

    readonly property bool isMobile: Screen.primaryOrientation === Qt.PortraitOrientation || vpnRoot.width < 600
    readonly property real cardWidth: isMobile ? vpnRoot.width : Math.min(vpnRoot.width * 0.9, 450)
    readonly property real cardHeight: isMobile ? vpnRoot.height : Math.min(vpnRoot.height * 0.95, 750)
    readonly property real cardRadius: isMobile ? 0 : 12
    readonly property real baseFontSize: isMobile ? 16 : 18
    readonly property real smallFontSize: baseFontSize * 0.8
    readonly property real fieldHeight: isMobile ? 60 : 65

    property bool showPrivateKey: false

    Component.onCompleted: {
        console.log("[VpnConfig] Loaded. Backend vpnConfig valid:", !!vpnConfig)
        if (vpnConfig) {
            vpnConfig.load()
        }
    }

    Connections {
        target: vpnConfig
        onStatusChanged: console.log("[VpnConfig] Status changed to: " + (vpnConfig ? vpnConfig.status : "null"))
        onErrorMessageChanged: console.log("[VpnConfig] Error message: " + (vpnConfig ? vpnConfig.errorMessage : "null"))
    }

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

                // Section: Device Identity (Editable Private/Public Keys)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 12

                    // Private Key Field
                    TextField {
                        id: privateKeyInput
                        text: vpnConfig ? vpnConfig.privateKey : ""
                        echoMode: vpnRoot.showPrivateKey ? TextInput.Normal : TextInput.Password
                        Layout.fillWidth: true
                        Layout.preferredHeight: vpnRoot.fieldHeight
                        color: "white"
                        font.pixelSize: vpnRoot.baseFontSize * 0.85
                        font.family: "Monospace"
                        leftPadding: 15
                        topPadding: 20
                        placeholderText: "Pasted Private Key here..."
                        background: vpnFieldBg("DEVICE PRIVATE KEY", privateKeyInput.activeFocus)
                        onTextEdited: {
                            if (vpnConfig) {
                                vpnConfig.privateKey = text
                                if (vpnRoot.hasError) vpnConfig.setStatus(0)
                            }
                        }
                        onEditingFinished: {
                             if (vpnConfig) {
                                 vpnConfig.privateKey = text
                                 vpnConfig.save()
                             }
                        }

                        // Action Buttons Row
                        Row {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 10
                            spacing: 12

                            // Clear Button
                            Button {
                                width: 24; height: 24
                                visible: privateKeyInput.text !== ""
                                contentItem: Text { text: "✕"; color: "#888888"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                background: Item {}
                                onClicked: {
                                    privateKeyInput.text = ""
                                    if (vpnConfig) {
                                        vpnConfig.privateKey = ""
                                        vpnConfig.save()
                                    }
                                }
                            }

                            // Reveal Toggle
                            Button {
                                width: 24; height: 24
                                contentItem: Text { text: vpnRoot.showPrivateKey ? "👁️" : "🙈"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                background: Item {}
                                onClicked: vpnRoot.showPrivateKey = !vpnRoot.showPrivateKey
                            }
                        }
                    }

                    // Public Key Field
                    TextField {
                        id: publicKeyInput
                        text: vpnConfig ? vpnConfig.publicKey : ""
                        Layout.fillWidth: true
                        Layout.preferredHeight: vpnRoot.fieldHeight
                        color: "#ffff00"
                        font.pixelSize: vpnRoot.baseFontSize * 0.85
                        font.family: "Monospace"
                        leftPadding: 15
                        topPadding: 20
                        placeholderText: "Corresponding Public Key..."
                        background: vpnFieldBg("DEVICE PUBLIC KEY (SHARE WITH CMS)", publicKeyInput.activeFocus)
                        onEditingFinished: {
                             if (vpnConfig) {
                                 vpnConfig.publicKey = text
                                 vpnConfig.save()
                             }
                        }

                        // Clear Button
                        Button {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 10
                            width: 24; height: 24
                            visible: publicKeyInput.text !== ""
                            contentItem: Text { text: "✕"; color: "#888888"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Item {}
                            onClicked: {
                                publicKeyInput.text = ""
                                if (vpnConfig) {
                                    vpnConfig.publicKey = ""
                                    vpnConfig.save()
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            id: copyBtn
                            text: "COPY PUBLIC KEY"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 35
                            contentItem: Text {
                                text: copyBtn.text
                                font.pixelSize: vpnRoot.smallFontSize
                                color: "black"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { color: "#ffff00"; radius: 4 }
                            onClicked: {
                                if (vpnConfig) {
                                    vpnConfig.copyToClipboard(vpnConfig.publicKey)
                                    copyBtn.text = "COPIED!"
                                    copyTimer.start()
                                }
                            }
                        }

                        Timer { id: copyTimer; interval: 2000; onTriggered: copyBtn.text = "COPY PUBLIC KEY" }

                        Button {
                            id: refreshBtn
                            text: "GENERATE NEW"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 35
                            contentItem: Text {
                                text: refreshBtn.text
                                font.pixelSize: vpnRoot.smallFontSize
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { color: "#444444"; radius: 4 }
                            onClicked: if (vpnConfig) vpnConfig.generateIdentity()
                        }
                    }
                }

                // Section: Server Configuration
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 12

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
                        background: vpnFieldBg("SERVER PUBLIC KEY", serverKeyInput.activeFocus)
                        onEditingFinished: {
                             if (vpnConfig) {
                                 vpnConfig.serverPublicKey = text
                                 vpnConfig.save()
                             }
                        }

                        // Clear Button
                        Button {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 10
                            width: 24; height: 24
                            visible: serverKeyInput.text !== ""
                            contentItem: Text { text: "✕"; color: "#888888"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Item {}
                            onClicked: {
                                serverKeyInput.text = ""
                                if (vpnConfig) {
                                    vpnConfig.serverPublicKey = ""
                                    vpnConfig.save()
                                }
                            }
                        }
                    }

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
                        background: vpnFieldBg("VPN SERVER ENDPOINT", serverIpInput.activeFocus)
                        onEditingFinished: {
                             if (vpnConfig) {
                                 vpnConfig.serverEndpoint = text
                                 vpnConfig.save()
                             }
                        }

                        // Clear Button
                        Button {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 10
                            width: 24; height: 24
                            visible: serverIpInput.text !== ""
                            contentItem: Text { text: "✕"; color: "#888888"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Item {}
                            onClicked: {
                                serverIpInput.text = ""
                                if (vpnConfig) {
                                    vpnConfig.serverEndpoint = ""
                                    vpnConfig.save()
                                }
                            }
                        }
                    }

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
                        background: vpnFieldBg("VIRTUAL IP (CLIENT)", virtualIpInput.activeFocus)
                        onEditingFinished: if (vpnConfig) vpnConfig.save()

                        // Clear Button
                        Button {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 10
                            width: 24; height: 24
                            visible: virtualIpInput.text !== ""
                            contentItem: Text { text: "✕"; color: "#888888"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Item {}
                            onClicked: {
                                virtualIpInput.text = ""
                                if (vpnConfig) {
                                    vpnConfig.virtualIp = ""
                                    vpnConfig.save()
                                }
                            }
                        }
                    }

                    TextField {
                        id: allowedIpsInput
                        placeholderText: "0.0.0.0/0"
                        text: vpnConfig ? vpnConfig.allowedIps : "0.0.0.0/0"
                        Layout.fillWidth: true
                        Layout.preferredHeight: vpnRoot.fieldHeight
                        color: "white"
                        font.pixelSize: vpnRoot.baseFontSize * 0.9
                        leftPadding: 15
                        topPadding: 20
                        background: vpnFieldBg("ALLOWED IPS (TARGET RANGE)", allowedIpsInput.activeFocus)
                        onEditingFinished: {
                             if (vpnConfig) {
                                 vpnConfig.allowedIps = text
                                 vpnConfig.save()
                             }
                        }

                        // Clear Button
                        Button {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 10
                            width: 24; height: 24
                            visible: allowedIpsInput.text !== ""
                            contentItem: Text { text: "✕"; color: "#888888"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Item {}
                            onClicked: {
                                allowedIpsInput.text = "0.0.0.0/0"
                                if (vpnConfig) {
                                    vpnConfig.allowedIps = "0.0.0.0/0"
                                    vpnConfig.save()
                                }
                            }
                        }
                    }
                }

                // Status Message Area
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    color: vpnRoot.hasError ? "#331111" : (vpnRoot.isConnected ? "#113311" : "transparent")
                    radius: 8
                    visible: vpnRoot.hasError || vpnRoot.isConnected || vpnRoot.isConnecting

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        BusyIndicator {
                            visible: vpnRoot.isConnecting
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 30
                        }

                        Text {
                            text: vpnRoot.hasError ? "ERROR: " + vpnRoot.statusMessage : 
                                  (vpnRoot.isConnected ? "✓ VPN TUNNEL SECURED" : 
                                  (vpnRoot.isConnecting ? "ESTABLISHING HANDSHAKE..." : ""))
                            color: vpnRoot.hasError ? "#ff4444" : (vpnRoot.isConnected ? "#00ff00" : "white")
                            font.pixelSize: vpnRoot.smallFontSize
                            font.weight: Font.Bold
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // Action Buttons
                Button {
                    id: connectBtn
                    text: vpnRoot.isConnected ? "CONNECTED" : (vpnRoot.isConnecting ? "CONNECTING..." : "SAVE & CONNECT")
                    enabled: !vpnRoot.isConnecting
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    contentItem: Text {
                        text: connectBtn.text
                        font.pixelSize: vpnRoot.baseFontSize
                        font.weight: Font.Bold
                        color: (vpnRoot.isConnected || vpnRoot.hasError) ? "white" : "black"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: vpnRoot.isConnected ? "#1a3300" : (vpnRoot.hasError ? "#aa0000" : (vpnRoot.isConnecting ? "#888800" : "#ffff00"))
                        radius: 8
                    }
                    onClicked: {
                        console.log("[VpnConfig] Connect clicked. vpnConfig valid:", !!vpnConfig)
                        if (!vpnConfig) {
                             statusMessage = "Error: VPN Backend missing. Please restart app."
                             return;
                        }

                        console.log("[VpnConfig] Attempting start with: Endpoint=" + serverIpInput.text + " ClientIP=" + virtualIpInput.text + " Allowed=" + allowedIpsInput.text)

                        // Instant UI Validation
                        if (privateKeyInput.text === "" || serverKeyInput.text === "" || serverIpInput.text === "" || virtualIpInput.text === "") {
                            vpnConfig.setErrorMessage("Please fill all required keys and server fields.");
                            vpnConfig.setStatus(3); // Error
                            return;
                        }

                        // Character Length Validation (WireGuard keys are typically 44 chars)
                        if (privateKeyInput.text.length < 40 || serverKeyInput.text.length < 40) {
                            vpnConfig.setErrorMessage("Invalid Key Format. Keys must be standard Base64 (approx 44 chars).");
                            vpnConfig.setStatus(3); 
                            return;
                        }

                        vpnConfig.setIsEnabled(true);
                        vpnConfig.save();
                        vpnConfig.startVpn();
                        console.log("[VpnConfig] vpnConfig.startVpn() called. Current status: " + vpnConfig.status)
                    }
                }

                Text {
                    text: vpnRoot.isConnected ? "The secure tunnel is active. All traffic is now routed through your VPC." : "Verify your identity keys and server endpoint before connecting."
                    color: "#888888"
                    font.pixelSize: vpnRoot.smallFontSize * 0.9
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

    // Premium Status Overlay (similar to ConfigDialog)
    Rectangle {
        id: statusOverlay
        anchors.fill: vpnCard
        color: "#1e1e1e"
        visible: vpnRoot.isConnecting || vpnRoot.isConnected || (vpnRoot.hasError && vpnRoot.statusMessage !== "")
        z: 100

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width * 0.8
            spacing: 20

            Item {
                Layout.alignment: Qt.AlignHCenter
                width: 80
                height: 80

                BusyIndicator {
                    anchors.fill: parent
                    running: vpnRoot.isConnecting
                    visible: vpnRoot.isConnecting
                }

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: "#00ff00"
                    font.pixelSize: 64
                    visible: vpnRoot.isConnected
                }

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: "#ff4444"
                    font.pixelSize: 64
                    visible: vpnRoot.hasError
                }
            }

            Text {
                text: vpnRoot.isConnecting ? "ESTABLISHING VPN TUNNEL..." : 
                      (vpnRoot.isConnected ? "CONNECTION SECURED" : "CONNECTION FAILED")
                color: "white"
                font.pixelSize: vpnRoot.baseFontSize * 1.2
                font.weight: Font.Bold
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                text: vpnRoot.hasError ? vpnRoot.statusMessage : 
                      (vpnRoot.isConnected ? "All traffic is now routed through your VPC." : "Please wait while we perform the WireGuard handshake.")
                color: "#888888"
                font.pixelSize: vpnRoot.smallFontSize
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Button {
                id: retryBtn
                text: "TRY AGAIN"
                visible: vpnRoot.hasError
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 150
                Layout.preferredHeight: 45
                contentItem: Text {
                    text: "TRY AGAIN"
                    color: "black"
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: "#ffff00"; radius: 6 }
                onClicked: vpnConfig.setStatus(0) // Back to neutral
            }

            Button {
                id: closeBtn
                text: "CLOSE"
                visible: vpnRoot.isConnected
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 150
                Layout.preferredHeight: 45
                contentItem: Text {
                    text: "CLOSE"
                    color: "white"
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: "#333333"; radius: 6 }
                onClicked: vpnRoot.cancel()
            }
        }
    }

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
                    font.pixelSize: 10
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
