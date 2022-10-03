#pragma once

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlEngineExtensionPlugin>

#include "jsonconfig.h"
#include "configlayer.h"


class ConfigPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

    void registerTypes(const char *uri) override
    {
        qmlRegisterType<JsonConfig>(uri, 1, 0, "JsonConfig");
        qmlRegisterType<ConfigLayer>(uri, 1, 0, "ConfigLayer");
    }
};
