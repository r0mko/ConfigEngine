#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include "ConfigEngine.hpp"
#include "private/node.h"
class JsonQObject : public QObject
{
public:
    JsonQObject(QObject *parent = nullptr);
    JsonQObject(QMetaObject *mo, Node *node, QObject *parent = nullptr);
    virtual ~JsonQObject();
    virtual int qt_metacall(QMetaObject::Call call, int id, void **arguments);
    virtual const QMetaObject *metaObject() const;

    static void staticMetaCallImpl(QObject *object, QMetaObject::Call call, int id, void **arguments);

    void notifyPropertyUpdate(int id);

    void emitSignalHelper(int signalIndex, QVariantList arguments);
    void emitSignal(int index, void **arguments);

private:
    void metacallImpl(QMetaObject::Call call, int id, void **arguments);

    QMetaObject *m_metaObject = nullptr;
    Node *m_node = nullptr;

    QVector<QVariant> m_propertyCache;
};

Q_DECLARE_METATYPE(JsonQObject*)
