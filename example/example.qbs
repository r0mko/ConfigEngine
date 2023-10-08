import qbs

CppApplication {
    Depends { name: 'bundle' }
    Depends { name: 'Qt.core' }
    Depends { name: 'Qt.gui' }
    Depends { name: 'Qt.qml' }

    Depends { name: 'configplugin'; cpp.link: true }

    name: 'example'

    files: [
        "fontinfo.cpp",
        "fontinfo.h",
        'main.cpp',
        'qml.qrc',
    ]

    bundle.isBundle: false

    install: true

    builtByDefault: false
}
