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
    Q_PROPERTY(QObject* config READ config NOTIFY configChanged)
    Q_PROPERTY(bool readonly READ readonly WRITE setReadonly NOTIFY readonlyChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    ConfigEngine(QObject *parent = nullptr);
    virtual ~ConfigEngine();

    enum Status
    {
        Null,
        Error,
        ConfigLoaded,
        ConfigModified
    };
    Q_ENUM(Status);

    QObject *config() const;

    void setQmlEngine(QQmlEngine *qmlEngine);

    Status status() const;
    bool deferChangeSignals() const;

    bool readonly() const;
    void setReadonly(bool newReadonly);

public slots:
    // loads config. If desired index is 0 or no other configs were loaded before, the loaded file will be treated as a root config
    QString loadLayer(const QString &path, int desiredIndex = -1);
    void writeConfig(const QString &path, const QString &layer);
    void unloadLayer(const QString &layer);
    void activateLayer(const QString &layer);
    void deactivateLayer(const QString &layer);
    void clear();
    void setProperty(const QString &layer, const QString &key, QVariant value);
    QVariant getProperty(const QString &layer, const QString &key);

    void beginUpdate();
    void endUpdate();

signals:
    void configChanged();
    void statusChanged();
    void readonlyChanged();

private:
    void setStatus(Status newStatus);

    friend class JsonQObject;
    friend class Node;

    QJsonObject parseData(const QByteArray &data, bool *ok);

    Node m_root;
    QQmlEngine *m_qmlEngine = nullptr;

    struct ConfigLayer {
        int index = -1;
        bool active = false;
        bool modified = false;
        QJsonObject object;
    };

    QMap<QString, ConfigLayer> m_layers;
    Status m_status;
    bool m_deferChangeSignals = false;

    ConfigLayer &getLayer(const QString &name);
    void updateTree(int level, QJsonObject data);
    void checkModified();
    QByteArray readFile(const QString &path, bool *ok);

    bool m_readonly = false;
};
