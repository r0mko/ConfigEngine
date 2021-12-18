import qbs

DynamicLibrary {
    Depends { name: 'bundle' }
    Depends {
        name: 'Qt'
        submodules: ['core', 'core-private', 'gui', 'qml']
    }

    name: 'configplugin'

    Qt.qml.importName: 'r0mko.config'
    Qt.qml.importVersion: '1.0'

    cpp.includePaths: '.'

    files: [
        "private/node.cpp",
        "private/node.h",
        '*.cpp',
        '*.hpp',
    ]

    Group {
        files: 'qmldir'
        qbs.install: true
        qbs.installDir: 'r0mko/config'
    }

    bundle.isBundle: false

    install: true
    installDir: 'r0mko/config'
}
