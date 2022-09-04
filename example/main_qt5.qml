import QtQuick 2.12
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.0
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.3

import r0mko.config 1.0


Window {
    id: _root
    width: 800
    height: 800
    visible: true
    title: config.myName
    color: config.editor.colors.backgroundColor

    property QtObject config

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        ColumnLayout {
            Label {
                text: "Config loaded"
                color: ConfigEngine.status >= ConfigEngine.ConfigLoaded ? config.editor.colors.labelEnabledColor : config.editor.colors.labelDisabledColor
            }
            Label {
                text: "Config modified"
                color: ConfigEngine.status == ConfigEngine.ConfigModified? config.editor.colors.labelEnabledColor : config.editor.colors.labelDisabledColor
            }
        }

        Item {
            id: _playfield
            Layout.fillWidth: true
            Layout.fillHeight: true
            Text {
                anchors.centerIn: parent
                font.pixelSize: config.editor.fontSize
                text: config.editor.fontFamily
                font.family: config.editor.fontFamily
                rotation: config.editor.rotation
                color: config.editor.colors.textDefaultColor
            }
        }


        ColumnLayout {
            id: _configs
            property var model: ["config_1", "config_2", "config_3"]

            Repeater {
                model: _configs.model
                delegate: RowLayout {
                    id: _delegate

                    property string config: modelData
                    property string layerName
                    property bool activated: false

                    Label {
                        text: config
                    }

                    Button {
                        text: "Load".arg(config)
                        enabled: layerName.length == 0
                        onClicked: {
                            layerName = ConfigEngine.loadLayer(":/%1.json".arg(config))
                        }
                    }

                    Button {
                        text: "Activate".arg(config)
                        enabled: layerName.length > 0 && !activated
                        onClicked: {
                            ConfigEngine.activateLayer(layerName)
                            activated = true
                        }
                    }

                    Button {
                        text: "Deactivate".arg(config)
                        enabled: layerName.length > 0 && activated
                        onClicked: {
                            ConfigEngine.deactivateLayer(layerName)
                            activated = false
                        }
                    }
                    Button {
                        text: "Unload".arg(config)
                        enabled: layerName.length > 0
                        onClicked: {
                            ConfigEngine.unloadLayer(layerName)
                            activated = false
                            layerName = ""
                        }
                    }
                }
            }

            Slider {
                id: _slider
                from: -90
                to: 90
                onValueChanged: {
                    config.editor.rotation = _slider.value
                    console.log("Setting value", _slider.value)
                }
            }
        }
    }

    Component.onCompleted: {
        ConfigEngine.loadLayer(":/globalConfig.json")
        _root.config = ConfigEngine.config
        console.log("default color", config)
    }
}
