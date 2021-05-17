#pragma once

#include <QObject>
#include <QJsonObject>
#include <QHash>

class QQmlEngine;
class JSONQObject;

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
    friend class JSONQObject;

    struct NamedValueGroup
    {
        NamedValueGroup(const QString &key, QVariant value);
        QString key;
        QVariant values[LevelsCount];
        const QVariant &value() const;
    };

    struct Node
    {
        QString name;
        Node *parent = nullptr;
        QList<Node*> childNodes;
        QList<NamedValueGroup> properties;

        JSONQObject *object = nullptr;

        const QVariant &valueAt(int index) const;

        void createObject();
        void setJsonObject(QJsonObject object);
        void updateJsonObject(QJsonObject object, ConfigLevel level);
        void updateProperty(int index, ConfigLevel level, QVariant value);
        void clearProperty(int index, ConfigLevel level);

        void clear();
        void unload(ConfigLevel level);

        int indexOfProperty(const QString &name) const;
        int indexOfChild(const QString &name) const;

        QString fullPropertyName(const QString &property) const;
    };

    Node m_root;
    QVarLengthArray<QJsonObject, LevelsCount> m_data;
    QQmlEngine *m_qmlEngine = nullptr;

    void updateTree(ConfigLevel level);
};
