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

    property string deviceId: ""
    property string deviceIdPrefix: "BFD-"
    property string playlistUrl: ""
    property string errorMessage: ""

    signal accepted()
    signal rejected()

    // Improved Responsive Logic
    readonly property bool isMobile: Screen.primaryOrientation === Qt.PortraitOrientation || root.width < 600
    
    readonly property real cardWidth: isMobile ? root.width : Math.min(root.width * 0.9, 450)
    readonly property real cardHeight: isMobile ? root.height : Math.min(root.height * 0.95, 750)
    readonly property real cardRadius: isMobile ? 0 : 12
    
    readonly property real baseFontSize: isMobile ? 16 : 18
    readonly property real smallFontSize: baseFontSize * 0.8
    readonly property real fieldHeight: isMobile ? 60 : 65

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
                    
                    background: Item {}
                    onClicked: root.rejected()
                }

                ScrollView {
                    id: scrollView
                    anchors.fill: parent
                    anchors.topMargin: backButton.height + 20
                    contentWidth: availableWidth
                    // This ensures scrolling works when the keyboard appears
                    contentHeight: mainLayout.implicitHeight + 40
                    clip: true

                    // IMPORTANT: Override the default white background of ScrollView
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

                        
                         // Device ID 
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            
                            TextField {
                                id: deviceId
                                text:""// root.deviceIdPrefix + root.deviceId
                        
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.fieldHeight
                                color: "white"
                                font.pixelSize: root.baseFontSize
                                verticalAlignment: TextInput.AlignVCenter
                                leftPadding: 15
                                topPadding: 20
                                placeholderText: root.deviceIdPrefix + root.deviceId
                                placeholderTextColor: "#888888"
                                
                                background: Rectangle {
                                    color: "#252525"
                                    radius: 8
                                    border.color: deviceId.activeFocus ? "#ffff00" : "#333333"
                                    border.width: deviceId.activeFocus ? 2 : 1
                                    
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
                                onTextChanged: root.deviceId = text
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
                                placeholderText:"https://la-trading-api.onrender.com/api/v1/device-playlist/{deviceId}/xml"
                                placeholderTextColor:"#888888"
                                
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
                                onTextChanged: root.playlistUrl = text
                            }
                        }

                        // Action Button
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
                            }
                            onClicked: root.accepted()
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
                                var vpnDialog = stackView.push("VpnConfigDialog.qml")
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

                        // Bottom Padding
                        Item { Layout.preferredHeight: 20 }
                    }
                }
            }
        }
    }

}