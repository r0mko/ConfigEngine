import QtQuick 2.12
import QtQuick.Layouts 1.15
import QtQuick.Controls 1.2
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.3

import r0mko.config 1.0

Window {
    id: _root
    width: 640
    height: 480
    visible: true
    title: $Config.myName
    color: $Config.editor.colors.backgroundColor

    Item {
        anchors.fill: parent
        anchors.margins: 8

        ColumnLayout {
            Row {
                spacing: 1
                Repeater {
                    model: 7

                    Rectangle {
                        width: 16
                        height: 16
                        border.width: 1
                        border.color: "#000"
                        color: ConfigEngine.state & (1 << index) ? "#ffdd33" : "#333333"
                    }
                }
            }
            Label {
                text: "Global config modified"
                color: ConfigEngine.globalConfigModified ? $Config.editor.colors.labelEnabledColor : $Config.editor.colors.labelDisabledColor
            }
            Label {
                text: "User config modified"
                color: ConfigEngine.userConfigModified ? $Config.editor.colors.labelEnabledColor : $Config.editor.colors.labelDisabledColor
            }
            Label {
                text: "Project config modified"
                color: ConfigEngine.projectConfigModified ? $Config.editor.colors.labelEnabledColor : $Config.editor.colors.labelDisabledColor
            }
        }

        Text {
            anchors.centerIn: parent
            font.pixelSize: $Config.editor.fontSize
            text: $Config.editor.fontFamily
            font.family: $Config.editor.fontFamily
            rotation: $Config.editor.rotation
            color: $Config.editor.colors.textDefaultColor
        }

        RowLayout {
            anchors.bottom: parent.bottom
            ColumnLayout {

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Load user config"
                    onClicked: {
                        ConfigEngine.loadConfig(":/userConfig.json", ConfigEngine.User)
                    }
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Load project config"
                    onClicked: {
                        ConfigEngine.loadConfig(":/projectConfig.json", ConfigEngine.Project)
                    }
                }

            }
            ColumnLayout {
                Button {
                    text: "Unload user config"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        ConfigEngine.unloadConfig(ConfigEngine.User)
                    }
                }
                Button {
                    text: "Unload project config"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        ConfigEngine.unloadConfig(ConfigEngine.Project)
                    }
                }
            }

            ColumnLayout {

                Button {
                    text: "Save main config as..."
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        _fileChooser.configLevel = ConfigEngine.Global
                        _fileChooser.open()
                    }
                }

                Button {
                    text: "Save user config as..."
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        _fileChooser.configLevel = ConfigEngine.User
                        _fileChooser.open()
                    }
                }

                Button {
                    text: "Save project config as..."
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        _fileChooser.configLevel = ConfigEngine.Project
                        _fileChooser.open()
                    }
                }

                Button {
                    text: "Save merged config as..."
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        _fileChooser.configLevel = ConfigEngine.MergeLevels
                        _fileChooser.open()
                    }
                }
            }


            Button {
                text: "Clear"
                onClicked: {
                    ConfigEngine.clear()
                }
            }

            Slider {
                id: _slider
                minimumValue: -90
                maximumValue: 90
                onValueChanged: {
//                    ConfigEngine.setProperty("editor.rotation", _slider.value, ConfigEngine.Global)
                    $Config.editor.rotation = _slider.value
                }
            }



        }

    }

    FileDialog {
        id: _fileChooser
        property int configLevel
        selectExisting: false
        title: "Save config as..."
        selectMultiple: false
        folder: shortcuts.documents
        defaultSuffix: "json"
        onAccepted: {
            ConfigEngine.writeConfig(fileUrl, configLevel)
            console.log("Saved %1".arg(Qt.resolvedUrl(fileUrl)))
        }
    }

    Component.onCompleted: {
        ConfigEngine.loadConfig(":/globalConfig.json", ConfigEngine.Global)
    }
}
