#include "JsonQObject.h"

#include <QtCore/QMetaProperty>

JsonQObject::JsonQObject(QObject *parent)
    : QObject(parent)
{

}

JsonQObject::JsonQObject(QMetaObject *mo, Node *node, QObject *parent)
    : QObject(parent),
      m_metaObject(mo),
      m_node(node)
{
    m_metaObject->d.static_metacall = staticMetaCallImpl;
}

JsonQObject::~JsonQObject()
{
    free(m_metaObject); // metaobject is unique for each JsonQObject
}

int JsonQObject::qt_metacall(QMetaObject::Call call, int id, void **arguments)
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

const QMetaObject *JsonQObject::metaObject() const
{
    return m_metaObject;
}

void JsonQObject::staticMetaCallImpl(QObject *object, QMetaObject::Call call, int id, void **arguments)
{
    JsonQObject *obj = reinterpret_cast<JsonQObject*>(object);
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

void JsonQObject::notifyPropertyUpdate(int id)
{
    id += m_metaObject->propertyOffset();
    int sig_id = m_metaObject->property(id).notifySignalIndex();
    emitSignalHelper(sig_id, QVariantList());
}

void JsonQObject::emitSignalHelper(int signalIndex, QVariantList arguments)
{
    QVector<void*> args;
    args.append(0);
    for(int i = 0; i < arguments.size(); ++i) {
        args.append(arguments[i].data());
    }
    emitSignal(signalIndex, args.data());
}

void JsonQObject::emitSignal(int index, void **arguments)
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

void JsonQObject::metacallImpl(QMetaObject::Call call, int id, void **arguments)
{
    switch (call) {
    case QMetaObject::ResetProperty:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case QMetaObject::QueryPropertyDesignable:
    case QMetaObject::QueryPropertyScriptable:
    case QMetaObject::QueryPropertyStored:
    case QMetaObject::QueryPropertyEditable:
    case QMetaObject::QueryPropertyUser:
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            int ptype = m_metaObject->property(id).userType();
            const QVariant &prop = m_node->valueAt(index);//properties[index].second;
            if (ptype != prop.userType()) {
                if (ptype == qMetaTypeId<qlonglong>()) {
                    qlonglong val = prop.toLongLong();
                    QMetaType::construct(ptype, arguments[0], &val);
                } else if (ptype == qMetaTypeId<double>()) {
                    double val = prop.toDouble();
                    QMetaType::construct(ptype, arguments[0], &val);
                }
            } else {
                QMetaType::construct(ptype, arguments[0], prop.constData());
            }
#else
            QMetaType ptype = m_metaObject->property(id).metaType();
            const QVariant &prop = m_node->valueAt(index);
            if (ptype != prop.metaType()) {
                if (ptype == QMetaType::fromType<qlonglong>()) {
                    qlonglong val = prop.toLongLong();
                    ptype.construct(arguments[0], &val);
                } else if (ptype == QMetaType::fromType<double>()) {
                    double val = prop.toDouble();
                    ptype.construct(arguments[0], &val);
                }
            } else {
                ptype.construct(arguments[0], prop.constData());
            }
#endif
        } else {
            *reinterpret_cast<QObject**>(a) = m_node->childAt(index)->object();
        }
        break;
    }
    case QMetaObject::WriteProperty: {
        int index = id - m_metaObject->propertyOffset();
        if (index < 0) {
            m_metaObject->superClass()->metacall(this, call, id, arguments);
            return;
        }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QVariant value = QVariant(m_metaObject->property(id).userType(), arguments[0]);
#else
        QVariant value = QVariant(m_metaObject->property(id).metaType(), arguments[0]);
#endif
        qDebug() << "Write property" << index << "val" << value;
        if (m_node->properties.size() > index) {
            m_node->properties[index].setValue(value);
            notifyPropertyUpdate(index);
        }
        break;
    }
    default:
        qWarning() << "Unable to handle metacall" << call;
        break;
    }

}
