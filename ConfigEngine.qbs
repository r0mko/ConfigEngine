import qbs

Project {
    property string installImportsDir
    property string installContentsPath: 'usr/local/'

    references: [
        'example/example.qbs',
        'src/plugin.qbs'
    ]
}
