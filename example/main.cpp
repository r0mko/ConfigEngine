#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include "../src/jsonconfig.h"
#include "fontinfo.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    int mtid = qRegisterMetaType<FontInfo*>();
    qDebug() << QMetaType::typeName(mtid) << "registered as" << mtid;

    QQmlApplicationEngine engine;
    engine.addImportPath(app.applicationDirPath() + "/..");

    const QUrl url(QStringLiteral("qrc:/main_qt%1.qml").arg(QT_VERSION_MAJOR));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl)
        {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );
    engine.load(url);

    return app.exec();
}
