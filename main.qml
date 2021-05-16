import QtQuick 2.12
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

        Button {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Load alternative config"
            onClicked: {
                ConfigEngine.loadConfig(":/otherConfig.json", ConfigEngine.Project)
            }
        }
    }

    Component.onCompleted: {
        ConfigEngine.loadConfig(":/testConfig.json", ConfigEngine.Global)
        console.warn("Config.myName", Config.myName)

    }

}


