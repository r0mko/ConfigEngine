import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import Qt.labs.platform
import QtQuick.Controls.Material 6.0

import r0mko.config

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
        filePath: ":/mb_globalConfig.mb.json"

        Instantiator {
            id: _layerMaker
            model: root.model
            delegate: ConfigLayer {
                filePath: ":/%1.json".arg(modelData.name)
            }
            function objectAdded(index) {
                root.model[index].layer = object
            }
        }
    }


    JsonConfig {
        id: _fontConfig
        filePath: ":/fontconfig.json"

        ConfigLayer {
            id: _fontLayer
            filePath: ":/fontlayer.json"
            active: _fontLayerCb.checked
            onActiveChanged: {
                console.log("Setting font layer active", active)
            }
        }
        onActiveLayersChanged: {
            console.log("Active layers", activeLayers, configData.sansRomanLight50.family)
        }
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

        RowLayout {
            Label {
                text: "sansRomanLight50 size: %1 family: %2".arg(_fontConfig.configData.sansRomanLight50.size).arg(_fontConfig.configData.sansRomanLight50.family)
            }
        }

        CheckBox {
            id: _fontLayerCb
            Component.onCompleted: {
                checked = _fontLayer.active
            }
        }
    }
}

