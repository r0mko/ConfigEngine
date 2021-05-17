import QtQuick 2.12
import QtQuick.Layouts 1.15
import QtQuick.Controls 1.2
import QtQuick.Window 2.12
import Test 1.0


Window {
    id: root
    width: 640
    height: 480
    visible: true
    title: Config.myName
    color: Config.colors.palette.yellow

    Item {
        anchors.fill: parent
        anchors.margins: 8

        Text {
            anchors.centerIn: parent
            font.pixelSize: Config.editor.fontSize
            text: Config.editor.fontFamily
            rotation: Config.editor.rotation
        }

        RowLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom

            Button {
                text: "Load alternative config"
                onClicked: {
                    ConfigEngine.loadConfig(":/otherConfig.json", ConfigEngine.Project)
                }
            }
            Button {
                text: "Unload alternative config"
                onClicked: {
                    ConfigEngine.unloadConfig(ConfigEngine.Project)
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
                    ConfigEngine.setProperty("editor.rotation", _slider.value, ConfigEngine.Project)
                }
            }
        }

    }

    Component.onCompleted: {
        ConfigEngine.loadConfig(":/testConfig.json", ConfigEngine.Global)
        console.warn("Config.myName", Config.myName)

    }

}


