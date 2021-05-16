#include "jsonqobject.h"
#include <QMetaProperty>

JSONQObject::JSONQObject(QObject *parent)
    : QObject(parent)
{

}

JSONQObject::JSONQObject(QMetaObject *mo, ConfigEngine::Node *node, QObject *parent)
    : QObject(parent),
      m_metaObject(mo),
      m_node(node)
{
    m_metaObject->d.static_metacall = staticMetaCallImpl;
    for (int i = 0; i < m_metaObject->propertyCount(); ++i) {
        QMetaProperty mp = m_metaObject->property(i);
        qDebug() << "Property" << i << "name" << mp.name();
    }
}

JSONQObject::~JSONQObject()
{
    qDebug() << "Called destructor of" << m_metaObject->className();
    free(m_metaObject);

}

int JSONQObject::qt_metacall(QMetaObject::Call call, int id, void **arguments)
{
    switch (call) {
    case QMetaObject::ReadProperty:
    case QMetaObject::WriteProperty:
        if (id < m_metaObject->propertyOffset()) {
            return QObject::qt_metacall(call, id, arguments);
        }
    default:
        break;
    }
    metacallImpl(call, id, arguments);
    return -1;
}

const QMetaObject *JSONQObject::metaObject() const
{
    return m_metaObject;
}

void JSONQObject::staticMetaCallImpl(QObject *object, QMetaObject::Call call, int id, void **arguments)
{
    JSONQObject *obj = reinterpret_cast<JSONQObject*>(object);
    switch (call) {
    case QMetaObject::ReadProperty:
    case QMetaObject::WriteProperty:
        id += obj->metaObject()->propertyOffset();
        break;
    default:
        break;
    }
    obj->metacallImpl(call, id, arguments);
}

void JSONQObject::notifyPropertyUpdate(int id)
{
    id += m_metaObject->propertyOffset();
    int sig_id = m_metaObject->property(id).notifySignalIndex();
    emitSignalHelper(sig_id, QVariantList());
}

void JSONQObject::emitSignalHelper(int signalIndex, QVariantList arguments)
{
    QVector<void*> args;
    args.append(0);
    for(int i = 0; i < arguments.size(); ++i) {
        args.append(arguments[i].data());
    }
    emitSignal(signalIndex, args.data());
}

void JSONQObject::emitSignal(int index, void **arguments)
{
    //    qDebug() << "Emitting signal id" << index << mo.method(index).methodSignature() << "in thread" << QThread::currentThread();
    const QMetaObject *m = m_metaObject;
    int loc_id = index - m->methodOffset();
    while (loc_id < 0) {
        m = m->superClass();
        loc_id = index - m->methodOffset();
    }
    QMetaObject::activate(this, m, loc_id, arguments);
}

void JSONQObject::metacallImpl(QMetaObject::Call call, int id, void **arguments)
{
    switch (call) {
    case QMetaObject::ResetProperty:
    case QMetaObject::QueryPropertyDesignable:
    case QMetaObject::QueryPropertyScriptable:
    case QMetaObject::QueryPropertyStored:
    case QMetaObject::QueryPropertyEditable:
    case QMetaObject::QueryPropertyUser:
    case QMetaObject::CreateInstance:
    case QMetaObject::IndexOfMethod:
    case QMetaObject::RegisterPropertyMetaType:
    case QMetaObject::RegisterMethodArgumentMetaType:
    case QMetaObject::InvokeMetaMethod:
        qDebug() << "Unhandled metacall type" << call << "id" << id;
        break;
    case QMetaObject::ReadProperty: {
        int index = id - m_metaObject->propertyOffset();
        if (index < 0) {
            qWarning() << "Index < 0";
            m_metaObject->superClass()->metacall(this, call, id, arguments);
            return;
        }
        bool isPod = true;
        if (index >= m_node->properties.size()) {
            index -= m_node->properties.size();
            isPod = false;
        }

        void *a = arguments[0];

        if (isPod) {
            const QVariant &prop = m_node->valueAt(index);//properties[index].second;
            QMetaType::construct(prop.userType(), arguments[0], prop.constData());
        } else {
            *reinterpret_cast<QObject**>(a) = m_node->childNodes[index]->object;
        }
        break;
    }
    case QMetaObject::WriteProperty: {
        qWarning() << "Write properties not supported";
        break;
    }
    default:
        qWarning() << "Unable to handle metacall" << call;
        break;
    }

}
