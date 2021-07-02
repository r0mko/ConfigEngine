import qbs

CppApplication {
    Depends { name: 'bundle' }
    Depends { name: 'Qt.core' }
    Depends { name: 'Qt.gui' }
    Depends { name: 'Qt.qml' }

    Depends { name: 'configplugin'; cpp.link: false }

    name: 'example'

    files: [
        'main.cpp',
        'qml.qrc'
    ]

    bundle.isBundle: false

    install: true
}
