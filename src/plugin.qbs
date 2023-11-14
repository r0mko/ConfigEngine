import qbs.FileInfo

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
        '*.h',
    ]

    Group {
        files: 'qmldir'
        qbs.install: true
        qbs.installPrefix: project.installContentsPath
        qbs.installDir: FileInfo.joinPaths(project.installImportsDir, 'r0mko/config')
    }

    bundle.isBundle: false

    install: true
    installDir: FileInfo.joinPaths(project.installImportsDir, 'r0mko/config')
    qbs.installPrefix: project.installContentsPath

    Export {
        Depends { name: 'cpp' }
        cpp.includePaths: exportingProduct.sourceDirectory
    }
}
