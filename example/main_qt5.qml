import QtQuick 2.12
import QtQml 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.0
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.3

import r0mko.config 1.0


Window {
    id: root
    width: 800
    height: 800
    visible: true
    title: config.myName
    color: config.editor.colors.backgroundColor
    property var model: [{"name": "config_1", "layer": null }, { "name": "config_2", "layer": null }, { "name": "config_3", "layer": null }]

    property QtObject config: _config.configData

    JsonConfig {
        id: _config
        filePath: ":/global.config.json"

        Instantiator {
            id: _layerMaker
            model: root.model
            delegate: ConfigLayer {
                filePath: ":/%1.json".arg(modelData.name)
            }
            onObjectAdded: {
                root.model[index].layer = object
            }
        }
    }

    JsonConfig {
        id: _fontConfig
        filePath: ":/fontconfig.json"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        ColumnLayout {
            Label {
                text: "Config status %1".arg(_config.status)
                color: _config.status >= JsonConfig.ConfigLoaded ? config.editor.colors.labelEnabledColor : config.editor.colors.labelDisabledColor
            }
            Label {
                text: "Config modified"
                color: _config.status == JsonConfig.ConfigModified? config.editor.colors.labelEnabledColor : config.editor.colors.labelDisabledColor
            }

            RowLayout {
                spacing: 0
                Label {
                    text: "Loaded layers:"
                    color: config.editor.colors.labelEnabledColor
                }

                Repeater {
                    model: _config.layers
                    delegate: Rectangle {
                        width: _layerName.width + 8
                        height: _layerName.height + 4
                        border.width: 1
                        border.color: "#000"
                        color: "#eee"
                        radius: 3
                        Label {
                            id: _layerName
                            anchors.centerIn: parent
                            text: modelData
                        }
                    }
                }
            }
            RowLayout {
                spacing: 0
                Label {
                    text: "Active layers:"
                    color: config.editor.colors.labelEnabledColor
                }

                Repeater {
                    model: _config.activeLayers
                    delegate: Rectangle {
                        width: _layerName2.width + 8
                        height: _layerName2.height + 4
                        border.width: 1
                        border.color: "#000"
                        color: "#eee"
                        radius: 3
                        Label {
                            id: _layerName2
                            anchors.centerIn: parent
                            text: modelData
                        }
                    }
                }
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
            Repeater {
                model: root.model
                delegate: RowLayout {
                    id: _delegate

                    property string config: modelData.name
                    property string layerName
                    property bool activated: false


                    Label {
                        text: "Layer %1 active".arg(config)
                        color: root.config.editor.colors.labelEnabledColor
                    }
                    CheckBox {
                        onClicked: {
                            if (checked) {
                                _config.activateLayer(config)
                            } else {
                                _config.deactivateLayer(config)
                            }
                        }
                    }

                    Label {
                        text: "Priority:"
                        color: root.config.editor.colors.labelEnabledColor
                    }

                    TextField {
                        background: Rectangle {
                            border.color: "#000"
                            border.width: 1
                            color: "#fff"
                            implicitWidth: 40
                            radius: 5

                        }

                        validator: IntValidator {}
                        onEditingFinished: {
                            _config.changeLayerPriority(config, Number(text))
                            focus = false
                        }
                    }
                }
            }



            Button {
                text: "Unload layers"
                onClicked: {
                    _layerMaker.active = false
                }
            }

            Button {
                text: "Export config"
                onClicked: {
                    var data =_config.exportConfig()
                    console.info(data)
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

        Flow {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: Object.keys(_fontConfig.configData)
                Button {
                    text: modelData
                    enabled: modelData.indexOf("Changed") == -1
                    onClicked: {
                        console.info(Object.keys(_fontConfig.configData[modelData]))
                        _fontConfig.configData[modelData].size = 111;
                    }
                }
            }
        }
    }
}
