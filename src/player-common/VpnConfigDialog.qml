import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

Rectangle {
    id: vpnRoot
    anchors.fill: parent
    color: "#000000"

    property var backendConfig: vpnConfig

    // Status Mapping (Unified Enum-based access)
    readonly property int vpnStatus: vpnRoot.backendConfig ? vpnRoot.backendConfig.status : 0
    readonly property bool isRegistered: !!(vpnRoot.backendConfig && vpnRoot.backendConfig.virtualIp !== "")
    property string statusMessage: vpnRoot.backendConfig ? vpnRoot.backendConfig.errorMessage : ""

    signal cancel()

    readonly property bool isMobile: Screen.primaryOrientation === Qt.PortraitOrientation || vpnRoot.width < 600
    readonly property real cardWidth:  isMobile ? vpnRoot.width  : Math.min(vpnRoot.width  * 0.9, 460)
    readonly property real cardHeight: isMobile ? vpnRoot.height : Math.min(vpnRoot.height * 0.95, 760)

    Component.onCompleted: if (vpnRoot.backendConfig) vpnRoot.backendConfig.load()

    // ── Proper State Management ───────────────────────────────────────────────
    state: {
        if (!backendConfig) return "UNREGISTERED";
        if (vpnStatus === backendConfig.Registering) return "REGISTERING";
        if (vpnStatus === backendConfig.Error)       return "ERROR";
        if (!isRegistered)                           return "UNREGISTERED";
        if (vpnStatus === backendConfig.Connecting)  return "CONNECTING";
        if (vpnStatus === backendConfig.Connected)   return "CONNECTED";
        return "DISCONNECTED";
    }

    states: [
        State {
            name: "UNREGISTERED"
            PropertyChanges { target: stage2; visible: true }
            PropertyChanges { target: stage3; visible: false }
            PropertyChanges { target: statusBanner; color: "#1a1a1a"; border.color: "#333" }
            PropertyChanges { target: statusText; text: "READY TO ENROLL"; color: "white" }
        },
        State {
            name: "REGISTERING"
            PropertyChanges { target: stage2; visible: true }
            PropertyChanges { target: stage3; visible: false }
            PropertyChanges { target: statusBanner; color: "#1a1a1a"; border.color: "#ffff00" }
            PropertyChanges { target: statusText; text: "NEGOTIATING…"; color: "#ffff00" }
        },
        State {
            name: "DISCONNECTED"
            PropertyChanges { target: stage2; visible: false }
            PropertyChanges { target: stage3; visible: true }
            PropertyChanges { target: connectBtn; visible: true }
            PropertyChanges { target: disconnectBtn; visible: false }
            PropertyChanges { target: statusBanner; color: "#1a1a1a"; border.color: "#333" }
            PropertyChanges { target: statusText; text: "TUNNEL IDLE"; color: "white" }
        },
        State {
            name: "CONNECTING"
            PropertyChanges { target: stage2; visible: false }
            PropertyChanges { target: stage3; visible: true }
            PropertyChanges { target: connectBtn; visible: true; text: "CONNECTING…" }
            PropertyChanges { target: disconnectBtn; visible: false }
            PropertyChanges { target: statusBanner; color: "#1a1a3d"; border.color: "#00e5ff" }
            PropertyChanges { target: statusText; text: "ESTABLISHING TUNNEL…"; color: "#00e5ff" }
        },
        State {
            name: "CONNECTED"
            PropertyChanges { target: stage2; visible: false }
            PropertyChanges { target: stage3; visible: true }
            PropertyChanges { target: connectBtn; visible: false }
            PropertyChanges { target: disconnectBtn; visible: true }
            PropertyChanges { target: statusBanner; color: "#1a3d1a"; border.color: "#00ff00" }
            PropertyChanges { target: statusText; text: "✓ TUNNEL ENCRYPTED AND ACTIVE"; color: "#00ff00" }
        },
        State {
            name: "ERROR"
            PropertyChanges { target: stage2; visible: !isRegistered }
            PropertyChanges { target: stage3; visible: isRegistered }
            PropertyChanges { target: statusBanner; color: "#3d1a1a"; border.color: "#ff4444" }
            PropertyChanges { target: statusText; text: "✕  " + vpnRoot.statusMessage; color: "#ff4444" }
        }
    ]

    transitions: [
        Transition {
            from: "*"; to: "*"
            ColorAnimation { duration: 250 }
        }
    ]

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

        // ── Status Banner (pinned footer) ──
        Rectangle {
            id: statusBanner
            height: 65
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                      leftMargin: 20; rightMargin: 20; bottomMargin: 20 }
            radius: 8

            RowLayout {
                anchors { fill: parent; margins: 14 }
                spacing: 12

                BusyIndicator {
                    running: vpnRoot.state === "REGISTERING" || vpnRoot.state === "CONNECTING"
                    visible: running
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                }

                Text {
                    id: statusText
                    font { pixelSize: 12; weight: Font.Bold }
                    Layout.fillWidth: true
                    wrapMode: Text.NoWrap
                    elide:    Text.ElideRight
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
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Item {
                width: contentScroll.availableWidth
                height: vpnLayout.implicitHeight

                ColumnLayout {
                    id: vpnLayout
                    width: parent.width
                    spacing: 22

                    // ── STAGE 1: DEVICE IDENTITY (Always Visible) ─────────────
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
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Text {
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
                            }
                        }
                    }

                    // ── STAGE 2: ACTION (Registration) ────────────────────────
                    ColumnLayout {
                        id: stage2
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        spacing: 12

                        Button {
                            id: registerBtn
                            text: "REGISTER & SECURE DEVICE"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 55
                            enabled: vpnRoot.state === "UNREGISTERED" || vpnRoot.state === "ERROR"

                            contentItem: Text {
                                text: registerBtn.text
                                color: registerBtn.enabled ? "black" : "#555"
                                font { weight: Font.Bold; pixelSize: 14 }
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
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
                        id: stage3
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        spacing: 10

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

                        // ── ACTION BUTTONS ────────────────────────────────────
                        Button {
                            id: connectBtn
                            text: "⬆  CONNECT TUNNEL"
                            enabled: vpnRoot.state === "DISCONNECTED" || vpnRoot.state === "ERROR"
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
                                SequentialAnimation on opacity {
                                    running:  vpnRoot.state === "CONNECTING"
                                    loops:    Animation.Infinite
                                    NumberAnimation { to: 0.4; duration: 700; easing.type: Easing.InOutSine }
                                    NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
                                }
                            }
                            onClicked: if (vpnRoot.backendConfig) vpnRoot.backendConfig.startVpn()
                        }

                        Button {
                            id: disconnectBtn
                            text: "⬇  DISCONNECT TUNNEL"
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

                        Item { Layout.preferredHeight: 8 }
                    }
                }
            }
        }
    }
}
