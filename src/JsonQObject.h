#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include "private/node.h"

class JsonQObject : public QObject
{
public:
    JsonQObject(QObject *parent = nullptr);
    JsonQObject(QMetaObject *mo, Node *node, QObject *parent = nullptr);
    ~JsonQObject() override;
    int qt_metacall(QMetaObject::Call call, int id, void **arguments) override;
    const QMetaObject *metaObject() const override;

    static void staticMetaCallImpl(QObject *object, QMetaObject::Call call, int id, void **arguments);

private:
    void metacallImpl(QMetaObject::Call call, int id, void **arguments);

    QMetaObject *m_metaObject = nullptr;
    Node *m_node = nullptr;
};

Q_DECLARE_METATYPE(JsonQObject*)
