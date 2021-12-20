#pragma once

#include <QString>
#include <QList>
#include <QVariant>

class JsonQObject;
class ConfigEngine;

struct Node
{
    struct NamedValueGroup
    {
        NamedValueGroup(const QString &key, QVariant value);
        QString key;
        QVarLengthArray<QVariant> values;
        const QVariant &value() const;
        int setValue(const QVariant &value);
        void writeValue(const QVariant &value, int level);

        bool modified = false;
    };

    QString name;
    Node *parent = nullptr;
    ConfigEngine *engine = nullptr;
    QList<Node*> childNodes;
    QList<NamedValueGroup> properties;

    JsonQObject *object = nullptr;

    inline const QVariant &valueAt(int index) const { return properties[index].value(); }

    void createObject();
    void setJsonObject(QJsonObject object);
    QJsonObject toJsonObject(int level) const;

    void updateJsonObject(QJsonObject object, int level, ConfigEngine *engine);
    void updateProperty(int index, int level, QVariant value);
    void clearProperty(int index, int level);

    void clear();
    void unload(int level);

    int indexOfProperty(const QString &name) const;
    int indexOfChild(const QString &name) const;

    QString fullPropertyName(const QString &property) const;
    void notifyChange(int level);
};

