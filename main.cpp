#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include "configengine.h"
#include "jsonqobject.h"

//static const char* g_config = R"(
//  {
//      "myName": "Roman Novikov",
//      "editor": {
//          "backgroundColor": "#ffffff",
//          "fontSize": 18,
//          "fontFamily": "Helvetica",
//          "enableUndo": true,
//          "forceUpdate": false
//      },
//      "fonts": {
//          "preloadSystemFonts": true,
//          "fetchVariants": false,
//          "defaultSize": 14
//      },
//      "colors": {
//          "defaultBackground": "#ffffff",
//          "defaultLabel": "#000000",
//          "highligtText": "#ffffff",
//          "highligtBackground": "#000080",
//          "palette": {
//              "black": "#000000",
//              "blue": "#0000ff",
//              "red": "#ff0000",
//              "magenta": "#ff00ff",
//              "green": "#00ff00",
//              "cyan": "#00ffff",
//              "yellow": "#ffff00",
//              "white": "#ffffff"
//          }
//      }
//  }

//)";

static QObject *config_engine_provider(QQmlEngine *engine, QJSEngine *)
{
    auto ret = new ConfigEngine(engine);
    ret->setQmlEngine(engine);
    return ret;
}


int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

//    ConfigEngine conf;
//    conf.loadData(g_config);


//    qmlRegisterType<ConfigEngine>("Test", 1, 0, "ConfigEngine");

    qmlRegisterSingletonType<ConfigEngine>("Test", 1, 0, "ConfigEngine", config_engine_provider);
//    engine.rootContext()->setContextProperty("Config", conf.root());

//    const QMetaObject *mo = conf.root()->metaObject();
//    qDebug() << "Root object" << mo->className();
//    for (int i = 0; i < mo->propertyCount(); ++i) {
//        QMetaProperty mp = mo->property(i);
//        qDebug() << "Property" << mp.typeName() << mp.name();
//    }

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
