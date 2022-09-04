#pragma once

#include <QString>
#include <QList>
#include <QVariant>

class JsonQObject;
class ConfigEngine;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define qsizetype int
#endif

class Node
{
public:
    struct NamedValueGroup
    {
        NamedValueGroup(const QString &key, QVariant value);
        QString key;
        QMap<int, QVariant> values;
        bool emitPending = false;
        const QVariant &value() const;
        int setValue(const QVariant &value);
        void writeValue(const QVariant &value, int level);
    };

    QList<NamedValueGroup> properties;

    inline const QVariant &valueAt(int index) const { return properties[index].value(); }

    void setJsonObject(QJsonObject object);
    QJsonObject toJsonObject(int level) const;

    void updateJsonObject(QJsonObject object, int level);
    bool updateProperty(int index, int level, QVariant value);
    void removeProperty(int index, int level);

    void clear();
    void unload(int level);
    void emitDeferredSignals();

    int indexOfProperty(const QString &name) const;
    int indexOfChild(const QString &name) const;

    QString fullPropertyName(const QString &property) const;
    void setEngine(ConfigEngine *newEngine);
    Node *childAt(qsizetype index) const;

    JsonQObject *object() const;
    Node *getNode(const QString &key, int *indexOfProperty);

private:
    void propertyChangedHelper(int index);
    void createObject();

    JsonQObject *m_object = nullptr;
    Node *m_parent = nullptr;
    QString m_name;
    ConfigEngine *m_engine = nullptr;
    QList<Node*> m_childNodes;
};

