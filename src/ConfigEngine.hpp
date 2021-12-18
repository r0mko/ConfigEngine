#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include "private/node.h"

class JsonQObject;
class QQmlEngine;

class ConfigEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* root READ root NOTIFY rootChanged)

public:
    enum ConfigLevel {
        Global = 0,
        User,
        Project,
        LevelsCount
    };
    Q_ENUM(ConfigLevel)

    ConfigEngine(QObject *parent = nullptr);

    virtual ~ConfigEngine() {}

    QObject *root() const;

    void setQmlEngine(QQmlEngine *qmlEngine);

public slots:
    void loadConfig(const QString &path, ConfigLevel level = Global);
    void loadData(const QByteArray &data, ConfigLevel level = Global);
    void unloadConfig(ConfigLevel level);
    void clear();
    void setProperty(const QString &key, QVariant value, ConfigLevel level = Global);

signals:
    void dataChanged(QJsonObject data);
    void rootChanged();

private:
    friend class JsonQObject;

	void resetContextProperty();

    Node m_root;
    QVarLengthArray<QJsonObject, LevelsCount> m_data;
    QQmlEngine *m_qmlEngine = nullptr;

    void updateTree(ConfigLevel level);
};
