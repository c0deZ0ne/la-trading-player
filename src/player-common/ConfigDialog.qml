import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

Rectangle {
    id: root
    // Initialize with Screen dimensions if parent is null. 
    // Uses fallbacks to ensure dimensions are defined even if Screen is unavailable.
    width: Screen.width > 0 ? Screen.width : 540
    height: Screen.height > 0 ? Screen.height : 960
    color: "#000000"

    // Background overlay
    Rectangle {
        anchors.fill: parent
        color: "#050505"
        opacity: 0.9
    }

    property string deviceId: MyConfig ? MyConfig.getUuid() : ""
    property string playerName: MyConfig ? MyConfig.getPlayerName() : ""
    property string deviceIdPrefix: "BFD-"
    property string playlistUrl: MyConfig ? MyConfig.getIndexUri() : ""
    property string errorMessage: ""
    property bool isConnecting: false
    property bool isSuccess: false
    property string statusMessage: "Connecting to CMS..."

    signal accepted()
    signal rejected()

    Component.onCompleted: {
        if (MyConfig) {
            root.playerName = MyConfig.getPlayerName()
            root.playlistUrl = MyConfig.getIndexUri()
            root.deviceId = MyConfig.getUuid()
        }
    }

    // Connect to LibFacade signals for SaaS enrollment feedback
    Connections {
        target: LibFacade
        onInitStarted: {
            root.isConnecting = true
            root.statusMessage = "Enrolling Device..."
        }
        onInitFailed: {
            root.isConnecting = false
            root.errorMessage = reason
        }
        onReadyForPlaying: {
            root.isSuccess = true
            root.isConnecting = false
            // After 2 seconds, accept the dialog to start playback
            closeTimer.start()
        }
    }

    Timer {
        id: closeTimer
        interval: 2000
        onTriggered: root.accepted()
    }

    // Improved Responsive Logic
    readonly property bool isMobile: Screen.primaryOrientation === Qt.PortraitOrientation || root.width < 600
    
    readonly property real cardWidth: isMobile ? root.width : Math.min(root.width * 0.9, 450)
    readonly property real cardHeight: isMobile ? root.height : Math.min(root.height * 0.95, 750)
    readonly property real cardRadius: isMobile ? 0 : 12
    
    readonly property real baseFontSize: isMobile ? 16 : 18
    readonly property real smallFontSize: baseFontSize * 0.8
    readonly property real fieldHeight: isMobile ? 60 : 65

    states: [
        State {
            name: "input"
            when: !root.isConnecting && !root.isSuccess && LibFacade.downloadProgress <= 0
            PropertyChanges { target: loadingOverlay; opacity: 0; visible: false }
        },
        State {
            name: "connecting"
            when: root.isConnecting && !root.isSuccess && LibFacade.downloadProgress <= 0
            PropertyChanges { target: loadingOverlay; opacity: 1; visible: true }
        },
        State {
            name: "ota"
            when: LibFacade.downloadProgress > 0 && !root.isSuccess
            PropertyChanges { target: loadingOverlay; opacity: 1; visible: true }
        },
        State {
            name: "success"
            when: root.isSuccess
            PropertyChanges { target: loadingOverlay; opacity: 1; visible: true }
        }
    ]

    transitions: [
        Transition {
            from: "*"; to: "*"
            NumberAnimation { properties: "opacity"; duration: 250; easing.type: Easing.InOutQuad }
        }
    ]

    Rectangle {
        id: card
        width: root.cardWidth
        height: root.cardHeight
        anchors.centerIn: parent
        color: "#1e1e1e"
        radius: root.cardRadius
        clip: true

        StackView {
            id: stackView
            anchors.fill: parent
            initialItem: mainView
            
            replaceEnter: Transition { PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 200 } }
            replaceExit: Transition { PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 200 } }
            pushEnter: Transition { PropertyAnimation { property: "x"; from: root.width; to: 0; duration: 300; easing.type: Easing.OutCubic } }
            pushExit: Transition { PropertyAnimation { property: "x"; from: 0; to: -root.width; duration: 300; easing.type: Easing.OutCubic } }
            popEnter: Transition { PropertyAnimation { property: "x"; from: -root.width; to: 0; duration: 300; easing.type: Easing.OutCubic } }
            popExit: Transition { PropertyAnimation { property: "x"; from: 0; to: root.width; duration: 300; easing.type: Easing.OutCubic } }
        }

        Component {
            id: mainView
            Item {
                id: mainViewItem
                // Keyboard height in logical pixels — drives the ScrollView bottom margin
                // so content above the keyboard stays fully visible and scrollable.
                property real keyboardHeight: Qt.inputMethod.visible
                    ? Qt.inputMethod.keyboardRectangle.height
                    : 0

                Behavior on keyboardHeight {
                    NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                }

                // Scrolls the ScrollView so that a given Item is centred in the visible area.
                function ensureVisible(field) {
                    var fieldPos = field.mapToItem(mainLayout, 0, 0)
                    var targetY  = fieldPos.y - (scrollView.height - field.height) / 2
                    scrollView.contentItem.contentY =
                        Math.max(0, Math.min(targetY,
                            scrollView.contentItem.contentHeight - scrollView.height))
                }

                // Back Navigation
                Button {
                    id: backButton
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 10
                    width: 44 // Minimum touch target size
                    height: 44
                    z: 10
                    
                    contentItem: Text {
                        text: "←"
                        color: "white"
                        font.pixelSize: 28
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle { color: "transparent" }
                    onClicked: root.rejected()
                }

                ScrollView {
                    id: scrollView
                    anchors.fill: parent
                    anchors.topMargin: backButton.height + 20
                    anchors.bottomMargin: parent.keyboardHeight
                    contentWidth: availableWidth
                    contentHeight: mainLayout.implicitHeight + 40
                    clip: true

                    background: Rectangle { color: "transparent" }

                    ColumnLayout {
                        id: mainLayout
                        width: parent.width
                        spacing: 20

                        // Logo
                        Image {
                            source: "qrc:/images/logo-full.png"
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: Math.min(card.width * 0.45, 180)
                            fillMode: Image.PreserveAspectFit
                        }

                        
                        // Device ID (reused as Device Name)
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            
                            TextField {
                                id: nameInput
                                text: root.playerName
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.fieldHeight
                                placeholderText: "Enter Device Name"
                                font.pixelSize: root.baseFontSize
                                color: "white"
                                verticalAlignment: TextInput.AlignVCenter
                                leftPadding: 15
                                topPadding: 20
                                onActiveFocusChanged: if (activeFocus) mainViewItem.ensureVisible(nameInput)
                                
                                background: Rectangle {
                                    color: "#252525"
                                    radius: 8
                                    border.color: nameInput.activeFocus ? "#ffff00" : "#333333"
                                    border.width: nameInput.activeFocus ? 2 : 1
                                    
                                    Text {
                                        text: "DEVICE NAME"
                                        color: "#ffff00"
                                        font.pixelSize: root.smallFontSize * 0.8
                                        font.weight: Font.Bold
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.leftMargin: 15
                                        anchors.topMargin: 6
                                    }
                                }
                                onTextChanged: root.playerName = text
                            }
                        }


                        // SaaS Enrollment Token (Manual Provisioning)
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            
                            TextField {
                                id: tokenInput
                                text: LibFacade.vpnConfig ? LibFacade.vpnConfig.enrollmentToken : ""
                                placeholderText: "Enter 6-char Code"
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.fieldHeight
                                color: "white"
                                font.pixelSize: root.baseFontSize
                                verticalAlignment: TextInput.AlignVCenter
                                leftPadding: 15
                                topPadding: 20
                                onActiveFocusChanged: if (activeFocus) mainViewItem.ensureVisible(tokenInput)
                                background: Rectangle {
                                    color: "#252525"
                                    radius: 8
                                    border.color: tokenInput.activeFocus ? "#ffff00" : "#333333"
                                    border.width: tokenInput.activeFocus ? 2 : 1
                                    
                                    Text {
                                        text: "PAIRING CODE"
                                        color: "#ffff00"
                                        font.pixelSize: root.smallFontSize * 0.8
                                        font.weight: Font.Bold
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.leftMargin: 15
                                        anchors.topMargin: 6
                                    }
                                }
                                onTextChanged: if (LibFacade.vpnConfig) LibFacade.vpnConfig.enrollmentToken = text
                            }
                        }

                        // Server Base URL
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20

                            TextField {
                                id: serverUrlInput
                                text: LibFacade.vpnConfig ? LibFacade.vpnConfig.managementBaseUrl : ""
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.fieldHeight
                                color: "white"
                                font.pixelSize: root.baseFontSize
                                verticalAlignment: TextInput.AlignVCenter
                                leftPadding: 15
                                topPadding: 20
                                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText

                                // _oldBase: the confirmed server URL when editing began
                                // _oldPath: the playlist path portion (/api/v1/...) captured
                                //           once on focus so every keystroke uses the same base
                                property string _oldBase: ""
                                property string _oldPath: ""

                                onActiveFocusChanged: {
                                    if (activeFocus) mainViewItem.ensureVisible(serverUrlInput)
                                    if (activeFocus && LibFacade.vpnConfig) {
                                        _oldBase = LibFacade.vpnConfig.managementBaseUrl
                                        var playlist = urlInput.text
                                        _oldPath = (playlist.indexOf(_oldBase) === 0)
                                                   ? playlist.substring(_oldBase.length)
                                                   : ""
                                    }
                                }

                                // Live update every keystroke: typed text + frozen path snapshot.
                                // Never re-reads urlInput.text so the check stays valid throughout.
                                onTextChanged: {
                                    if (_oldPath.length > 0) {
                                        urlInput.text = text + _oldPath
                                        root.playlistUrl = urlInput.text
                                    }
                                }

                                // On confirm: pass raw text to C++ for normalization, then
                                // re-anchor the playlist to the normalized base and save both.
                                onEditingFinished: {
                                    if (LibFacade.vpnConfig && text.length > 0) {
                                        LibFacade.vpnConfig.managementBaseUrl = text
                                        var normalizedBase = LibFacade.vpnConfig.managementBaseUrl
                                        if (_oldPath.length > 0) {
                                            var finalPlaylist = normalizedBase + _oldPath
                                            urlInput.text = finalPlaylist
                                            root.playlistUrl = finalPlaylist
                                            if (MyConfig) MyConfig.setIndexUri(finalPlaylist)
                                        }
                                        _oldBase = normalizedBase
                                        _oldPath = ""
                                    }
                                }

                                background: Rectangle {
                                    color: "#252525"
                                    radius: 8
                                    border.color: serverUrlInput.activeFocus ? "#ffff00" : "#333333"
                                    border.width: serverUrlInput.activeFocus ? 2 : 1
                                    Text {
                                        text: "SERVER URL"
                                        color: "#ffff00"
                                        font.pixelSize: root.smallFontSize * 0.8
                                        font.weight: Font.Bold
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.leftMargin: 15
                                        anchors.topMargin: 6
                                    }
                                }
                            }
                        }

                        // URL Input
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20

                            TextField {
                                id: urlInput
                                text: root.playlistUrl
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.fieldHeight
                                color: "white"
                                font.pixelSize: root.baseFontSize
                                verticalAlignment: TextInput.AlignVCenter
                                leftPadding: 15
                                topPadding: 20
                                background: Rectangle {
                                    color: "#252525"
                                    radius: 8
                                    border.color: urlInput.activeFocus ? "#ffff00" : "#333333"
                                    border.width: urlInput.activeFocus ? 2 : 1

                                    Text {
                                        text: "PLAYLIST URL"
                                        color: "#ffff00"
                                        font.pixelSize: root.smallFontSize * 0.8
                                        font.weight: Font.Bold
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.leftMargin: 15
                                        anchors.topMargin: 6
                                    }
                                }
                                onActiveFocusChanged: if (activeFocus) mainViewItem.ensureVisible(urlInput)
                                onTextChanged: root.playlistUrl = text
                                onEditingFinished: {
                                    if (MyConfig && text.length > 0)
                                        MyConfig.setIndexUri(text)
                                }
                            }
                        }

                        // Unified Action Button
                        Button {
                            id: actionBtn
                            text: "PAIR DEVICE"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            
                            contentItem: Text {
                                text: actionBtn.text
                                font.pixelSize: root.baseFontSize
                                font.weight: Font.Bold
                                color: "black"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: actionBtn.pressed ? "#d4cc00" : "#ffff00"
                                radius: 8
                                opacity: actionBtn.enabled ? 1.0 : 0.5
                            }
                            enabled: !root.isConnecting && !root.isSuccess && tokenInput.text.length >= 6
                            onClicked: {
                                root.isConnecting = true
                                root.errorMessage = ""
                                if (LibFacade) {
                                    LibFacade.enrollDevice(tokenInput.text, urlInput.text)
                                }
                            }
                        }


                        // VPN SETUP Button
                        Button {
                            id: vpnBtn
                            text: "VPN SETUP"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 45
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            
                            contentItem: Text {
                                text: vpnBtn.text
                                font.pixelSize: root.smallFontSize
                                font.weight: Font.Bold
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: vpnBtn.pressed ? "#333333" : "#252525"
                                radius: 8
                                border.color: "#444444"
                            }
                            onClicked: {
                                var vpnDialog = stackView.push("VpnConfigDialog.qml", {"backendConfig": vpnConfig})
                                vpnDialog.cancel.connect(function() { stackView.pop() })
                            }
                        }

                        // Error/Info Messages
                        Text {
                            text: root.errorMessage !== "" ? root.errorMessage : "Enter credentials provided by your CMS administrator"
                            color: root.errorMessage !== "" ? "#ff4444" : "#888888"
                            font.pixelSize: root.smallFontSize
                            Layout.fillWidth: true
                            Layout.leftMargin: 30
                            Layout.rightMargin: 30
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }

                        // Version Label
                        Text {
                            text: "Version: " + (LibFacade ? LibFacade.appVersion : "---")
                            color: "#444444"
                            font.pixelSize: root.smallFontSize * 0.7
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            Layout.topMargin: -10
                        }

                        // Bottom Padding
                        Item { Layout.preferredHeight: 20 }
                    }
                }
            }
        }
    }

    // --- PREMIUM LOADING OVERLAY ---
    Rectangle {
        id: loadingOverlay
        anchors.fill: parent
        color: "#aa000000"
        z: 1000

        MouseArea { anchors.fill: parent } // Block interactions

        Rectangle {
            width: Math.min(parent.width * 0.8, 300)
            height: 200
            color: "#252525"
            radius: 16
            anchors.centerIn: parent
            border.color: root.isSuccess ? "#00ff00" : "#444444"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20

                Item {
                    Layout.alignment: Qt.AlignHCenter
                    width: 60
                    height: 60

                    BusyIndicator {
                        anchors.fill: parent
                        running: root.isConnecting
                        visible: root.isConnecting
                        palette.dark: "white"
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        color: "#00ff00"
                        font.pixelSize: 48
                        visible: root.isSuccess
                    }
                }

                Text {
                    text: root.isSuccess ? "PAIRING SUCCESSFUL!" : root.statusMessage
                    color: "white"
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: root.isSuccess ? "Starting Garlic Player..." : (LibFacade.isDownloading ? LibFacade.downloadLabel : "Please wait while we sync with CMS")
                    color: "#888888"
                    font.pixelSize: 14
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                ProgressBar {
                    id: otaProgress
                    visible: LibFacade.downloadProgress > 0 && LibFacade.downloadProgress < 1.0
                    value: LibFacade.downloadProgress
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
                    background: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 6
                        color: "#333333"
                        radius: 3
                    }
                    contentItem: Item {
                        implicitWidth: 200
                        implicitHeight: 6

                        Rectangle {
                            width: otaProgress.visualPosition * parent.width
                            height: parent.height
                            radius: 3
                            color: "#ffff00"
                        }
                    }
                }
            }
        }
    }
}