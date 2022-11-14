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
    // metaobject is unique for each JsonQObject and is created using malloc/memset by QMetaObjectBuilder, hence NOLINT
    free(m_metaObject); // NOLINT
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
    // staticMetaCall can only be called on an JsonQObject instance, so no need to use expensive cast, hence NOLINT
    JsonQObject *obj = reinterpret_cast<JsonQObject*>(object); // NOLINT
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
            QMetaObject::metacall(this, call, id, arguments);
            return;
        }
        bool isPod = true;
        if (index >= m_node->properties.size()) {
            index -= m_node->properties.size();
            isPod = false;
        }
        void *a = arguments[0]; // NOLINT

        if (isPod) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            int ptype = m_metaObject->property(id).userType();
            const QVariant &prop = m_node->valueAt(index);//properties[index].second;
            if (ptype != prop.userType()) {
                if (ptype == qMetaTypeId<qlonglong>()) {
                    qlonglong val = prop.toLongLong();
                    QMetaType::construct(ptype, arguments[0], &val); // NOLINT
                } else if (ptype == qMetaTypeId<double>()) {
                    double val = prop.toDouble();
                    QMetaType::construct(ptype, arguments[0], &val); // NOLINT
                }
            } else {
                QMetaType::construct(ptype, arguments[0], prop.constData()); // NOLINT
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
            *reinterpret_cast<QObject**>(a) = m_node->childAt(index)->object(); // NOLINT
        }
        break;
    }
    case QMetaObject::WriteProperty: {
        int index = id - m_metaObject->propertyOffset();
        if (index < 0) {
            QMetaObject::metacall(this, call, id, arguments);
            return;
        }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QVariant value = QVariant(m_metaObject->property(id).userType(), arguments[0]); // NOLINT
#else
        QVariant value = QVariant(m_metaObject->property(id).metaType(), arguments[0]); // NOLINT
#endif
        qDebug() << "Write property" << index << "val" << value;
        if (m_node->properties.size() > index) {
            m_node->properties[index].setValue(value);
            m_node->notifyPropertyUpdate(index);
        }
        break;
    }
    default:
        qWarning() << "Unable to handle metacall" << call;
        break;
    }

}
