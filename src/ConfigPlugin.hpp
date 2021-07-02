#pragma once

#include <QtQml/QQmlEngineExtensionPlugin>

#include "ConfigEngine.hpp"

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
			ret->setQmlEngine(engine);
			return ret;
		});
	}
};
