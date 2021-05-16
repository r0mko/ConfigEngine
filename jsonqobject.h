#ifndef JSONQOBJECTBUILDER_H
#define JSONQOBJECTBUILDER_H

#include <QObject>
#include <QJsonObject>
#include "configengine.h"

class JSONQObject : public QObject
{
public:
    JSONQObject(QObject *parent = nullptr);
    JSONQObject(QMetaObject *mo, ConfigEngine::Node *node, QObject *parent = nullptr);
    virtual ~JSONQObject();
    virtual int qt_metacall(QMetaObject::Call call, int id, void **arguments);
    virtual const QMetaObject *metaObject() const;

    static void staticMetaCallImpl(QObject *object, QMetaObject::Call call, int id, void **arguments);

    void notifyPropertyUpdate(int id);

    void emitSignalHelper(int signalIndex, QVariantList arguments);
    void emitSignal(int index, void **arguments);

private:
    void metacallImpl(QMetaObject::Call call, int id, void **arguments);

    QMetaObject *m_metaObject = nullptr;
    ConfigEngine::Node *m_node = nullptr;

    QVector<QVariant> m_propertyCache;
};

Q_DECLARE_METATYPE(JSONQObject*)

#endif // JSONQOBJECTBUILDER_H
