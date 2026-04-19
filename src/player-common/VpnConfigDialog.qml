import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

Rectangle {
    id: vpnRoot
    anchors.fill: parent
    color: "#000000"

    property var backendConfig: vpnConfig

    // Status Mapping
    readonly property int vpnStatus: vpnRoot.backendConfig ? vpnRoot.backendConfig.status : 0
    readonly property bool isConnecting: vpnStatus === 1
    readonly property bool isConnected:  vpnStatus === 2
    readonly property bool hasError:     vpnStatus === 3
    readonly property bool isRegistering: vpnStatus === 4
    readonly property bool isRegistered: !!(vpnRoot.backendConfig && vpnRoot.backendConfig.virtualIp !== "")

    property string statusMessage: vpnRoot.backendConfig ? vpnRoot.backendConfig.errorMessage : ""

    signal cancel()

    readonly property bool isMobile: Screen.primaryOrientation === Qt.PortraitOrientation || vpnRoot.width < 600
    readonly property real cardWidth:  isMobile ? vpnRoot.width  : Math.min(vpnRoot.width  * 0.9, 460)
    readonly property real cardHeight: isMobile ? vpnRoot.height : Math.min(vpnRoot.height * 0.95, 760)

    Component.onCompleted: if (vpnRoot.backendConfig) vpnRoot.backendConfig.load()

    // ── Main Card ─────────────────────────────────────────────────────────────
    Rectangle {
        id: vpnCard
        width:  vpnRoot.cardWidth
        height: vpnRoot.cardHeight
        anchors.centerIn: parent
        color:  "#121212"
        radius: isMobile ? 0 : 16
        clip:   true

        // ── Header ────────────────────────────────────────────────────────────
        Rectangle {
            id: header
            width: parent.width
            height: 70
            color: "transparent"
            z: 10

            Button {
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 15 }
                width: 44; height: 44
                contentItem: Text {
                    text: "←"; color: "white"
                    font.pixelSize: 28
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment:   Text.AlignVCenter
                }
                background: Item {}
                onClicked: vpnRoot.cancel()
            }

            Text {
                text: "GATEWAY ACCESS"
                anchors.centerIn: parent
                color: "white"
                font { pixelSize: 20; weight: Font.Bold; letterSpacing: 2 }
            }
        }

        // ── Status Banner (pinned footer — defined before ScrollView so anchors resolve) ──
        Rectangle {
            id: statusBanner
            height: 65
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                      leftMargin: 20; rightMargin: 20; bottomMargin: 20 }
            radius: 8
            color:        vpnRoot.hasError    ? "#3d1a1a" : (vpnRoot.isConnected ? "#1a3d1a" : "#1a1a1a")
            border.color: vpnRoot.hasError    ? "#ff4444" : (vpnRoot.isConnected ? "#00ff00" : "#333")

            RowLayout {
                anchors { fill: parent; margins: 14 }
                spacing: 12

                BusyIndicator {
                    running: vpnRoot.isRegistering || vpnRoot.isConnecting
                    visible: running
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                }

                Text {
                    // FIX: wrapMode + maximumLineCount prevents vertical overflow
                    text: vpnRoot.hasError      ? ("✕  " + vpnRoot.statusMessage) :
                          vpnRoot.isConnected   ? "✓  TUNNEL ENCRYPTED" :
                          (vpnRoot.isConnecting || vpnRoot.isRegistering) ? "NEGOTIATING…" :
                          "READY TO ENROLL"

                    color: vpnRoot.hasError ? "#ff4444" : (vpnRoot.isConnected ? "#00ff00" : "white")
                    font { pixelSize: 12; weight: Font.Bold }
                    Layout.fillWidth: true
                    wrapMode: Text.NoWrap
                    elide:    Text.ElideRight      // single-line clamp — no vertical blowout
                }
            }
        }

        // ── Scrollable Content ─────────────────────────────────────────────────
        ScrollView {
            id: contentScroll
            anchors {
                top:    header.bottom
                left:   parent.left
                right:  parent.right
                bottom: statusBanner.top
                bottomMargin: 8
            }
            clip: true
            // FIX: fix contentWidth to the available width so ColumnLayout
            // never tries to expand horizontally, eliminating horizontal overflow.
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            // FIX: Use a plain Item wrapper instead of anchoring ColumnLayout
            // directly inside ScrollView — Qt does not support anchors on the
            // immediate child of ScrollView's internal Flickable contentItem.
            Item {
                width: contentScroll.availableWidth

                ColumnLayout {
                    id: vpnLayout
                    // FIX: bind width instead of using anchors
                    width: parent.width
                    x: 0; y: 0
                    spacing: 22

                    // ── STAGE 1: DEVICE IDENTITY ──────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        Layout.topMargin: 12
                        spacing: 8

                        Text {
                            text: "DEVICE IDENTITY"
                            color: "#666"
                            font { pixelSize: 11; weight: Font.Bold }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: identityCol.implicitHeight + 30
                            color: "#1a1a1a"; radius: 10; border.color: "#333"

                            ColumnLayout {
                                id: identityCol
                                anchors { fill: parent; margins: 15 }
                                spacing: 10

                                RowLayout {
                                    spacing: 8

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Text {
                                            text: vpnRoot.backendConfig ? vpnRoot.backendConfig.playerName : "UNKNOWN_DEVICE"
                                            color: "white"
                                            font { pixelSize: 17; weight: Font.Bold }
                                            // FIX: clamp long device names
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            // FIX: truncate pub-key display to prevent overflow
                                            text: "ID: " + (vpnRoot.backendConfig ? vpnRoot.backendConfig.publicKey.substring(0, 20) + "…" : "---")
                                            color: "#888"
                                            font { pixelSize: 11; family: "Monospace" }
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Button {
                                        text: "COPY"
                                        flat: true
                                        Layout.preferredWidth: 44
                                        contentItem: Text {
                                            text: parent.text
                                            color: "#ffff00"
                                            font { pixelSize: 10; weight: Font.Bold }
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment:   Text.AlignVCenter
                                        }
                                        onClicked: if (vpnRoot.backendConfig)
                                            vpnRoot.backendConfig.copyToClipboard(vpnRoot.backendConfig.publicKey)
                                    }
                                }

                                Rectangle { height: 1; Layout.fillWidth: true; color: "#333" }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    Text {
                                        text: "ENROLLMENT TOKEN (TENANT ASSET TAG)"
                                        color: "#888"
                                        font { pixelSize: 9; weight: Font.Bold }
                                    }

                                    TextField {
                                        id: tokenInput
                                        text: vpnRoot.backendConfig ? vpnRoot.backendConfig.enrollmentToken : ""
                                        placeholderText: "Enter token…"
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 45
                                        color: "#ffff00"
                                        font { pixelSize: 13; family: "Monospace" }
                                        leftPadding: 10
                                        background: Rectangle {
                                            color: "#121212"; radius: 6
                                            border.color: tokenInput.activeFocus ? "#ffff00" : "#222"
                                        }
                                        onTextChanged: if (vpnRoot.backendConfig)
                                            vpnRoot.backendConfig.enrollmentToken = text
                                    }
                                }
                            }
                        }
                    }

                    // ── STAGE 2: ACTION ───────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        spacing: 12
                        visible: !vpnRoot.isConnected && !vpnRoot.isRegistered

                        Button {
                            id: registerBtn
                            text: "REGISTER & SECURE DEVICE"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 55
                            enabled: !vpnRoot.isConnecting && !vpnRoot.isRegistering

                            contentItem: Text {
                                text: registerBtn.text
                                color: registerBtn.enabled ? "black" : "#555"
                                font { weight: Font.Bold; pixelSize: 14 }
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                // FIX: scale down on very narrow screens
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                color: registerBtn.enabled ? "#ffff00" : "#333"
                                radius: 8
                            }
                            onClicked: {
                                if (vpnRoot.backendConfig) {
                                    vpnRoot.backendConfig.save()
                                    vpnRoot.backendConfig.startVpn()
                                }
                            }
                        }
                    }

                    // ── STAGE 3: NETWORK RESULTS ──────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        spacing: 10
                        visible: vpnRoot.isRegistered || vpnRoot.isConnected

                        Text {
                            text: "ASSIGNED CONNECTION"
                            color: "#666"
                            font { pixelSize: 11; weight: Font.Bold }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: resultCol.implicitHeight + 30
                            color: "#0a2a0a"; radius: 10; border.color: "#1e4a1e"

                            ColumnLayout {
                                id: resultCol
                                anchors { fill: parent; margins: 15 }
                                spacing: 10

                                RowLayout {
                                    Text { text: "VIRTUAL IP:"; color: "#8b8"; font.pixelSize: 11 }
                                    Text {
                                        text: vpnRoot.backendConfig ? vpnRoot.backendConfig.virtualIp : "---"
                                        color: "white"; font.weight: Font.Bold
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                        elide: Text.ElideLeft
                                    }
                                }

                                Rectangle { height: 1; Layout.fillWidth: true; color: "#1e4a1e" }

                                RowLayout {
                                    Text { text: "GATEWAY:"; color: "#8b8"; font.pixelSize: 11 }
                                    Text {
                                        text: vpnRoot.backendConfig ? vpnRoot.backendConfig.serverEndpoint : "---"
                                        color: "white"; font.weight: Font.Bold
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignRight
                                        elide: Text.ElideLeft
                                    }
                                }
                            }
                        }

                        // ── MANUAL CONNECT BUTTON ─────────────────────────────
                        // Visible when registered but NOT yet tunnelling
                        Button {
                            id: connectBtn
                            text: vpnRoot.isConnecting ? "CONNECTING…" : "⬆  CONNECT TUNNEL"
                            visible: vpnRoot.isRegistered && !vpnRoot.isConnected
                            enabled: !vpnRoot.isConnecting && !vpnRoot.isRegistering
                            Layout.fillWidth: true
                            Layout.preferredHeight: 55

                            contentItem: Text {
                                text: connectBtn.text
                                color: connectBtn.enabled ? "black" : "#666"
                                font { weight: Font.Bold; pixelSize: 14 }
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                color: connectBtn.enabled ? "#00e5ff" : "#1a3a3a"
                                radius: 8
                                // Pulse animation while connecting
                                SequentialAnimation on opacity {
                                    running:  vpnRoot.isConnecting
                                    loops:    Animation.Infinite
                                    NumberAnimation { to: 0.4; duration: 700; easing.type: Easing.InOutSine }
                                    NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
                                }
                            }
                            onClicked: {
                                if (vpnRoot.backendConfig) {
                                    vpnRoot.backendConfig.startVpn()
                                }
                            }
                        }

                        // ── DISCONNECT BUTTON ─────────────────────────────────
                        Button {
                            id: disconnectBtn
                            text: "⬇  DISCONNECT TUNNEL"
                            visible: vpnRoot.isConnected
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            flat: true
                            contentItem: Text {
                                text: disconnectBtn.text
                                color: "#ff6b6b"
                                font { pixelSize: 13; weight: Font.Bold }
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: disconnectBtn.hovered ? "#3d1a1a" : "transparent"
                                radius: 8
                                border.color: "#ff4444"
                            }
                            onClicked: if (vpnRoot.backendConfig) vpnRoot.backendConfig.stopVpn()
                        }

                        // ── RESET REGISTRATION ────────────────────────────────
                        Button {
                            text: "RESET REGISTRATION"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            flat: true
                            contentItem: Text {
                                text: parent.text
                                color: "#ff4444"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                            }
                            onClicked: if (vpnRoot.backendConfig) vpnRoot.backendConfig.resetRegistration()
                        }

                        // Bottom spacing so content doesn't sit flush against the banner
                        Item { Layout.preferredHeight: 8 }
                    }
                }   // ColumnLayout
            }       // Item wrapper
        }           // ScrollView
    }               // vpnCard
}
