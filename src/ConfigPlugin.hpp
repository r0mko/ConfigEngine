#pragma once

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlEngineExtensionPlugin>

#include "ConfigEngine.hpp"
#include <QQmlContext>

class ConfigPlugin
    : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

    void registerTypes(const char *uri) override
    {
        qmlRegisterSingletonType<ConfigEngine>(uri, 1, 0, "ConfigEngine", [](QQmlEngine *engine, QJSEngine *) -> QObject*
        {
            auto ret = new ConfigEngine{engine};
            engine->rootContext()->setContextProperty("$Config", ret->config());
            ret->setQmlEngine(engine);
            return ret;
        });
    }
};
