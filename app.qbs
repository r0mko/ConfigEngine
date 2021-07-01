import qbs

CppApplication {
	Depends { name: 'Qt'; submodules: ['core-private', 'gui', 'qml'] }
	files: [
		'configengine.cpp',
		'configengine.h',
		'jsonqobject.cpp',
		'jsonqobject.h',
		'main.cpp',
		'qml.qrc'
	]
}
