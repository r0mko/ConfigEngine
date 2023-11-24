import qbs

Project {
    property string installImportsDir
    property string installContentsPath: 'usr/local/'

    property bool buildAsStatic: false

    references: [
        'example/example.qbs',
        'src/plugin.qbs'
    ]
}
