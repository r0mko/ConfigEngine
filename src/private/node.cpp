#include "node.h"

#include <private/qmetaobjectbuilder_p.h>

#include "jsonconfig.h"
#include "JsonQObject.h"
#include <QScopeGuard>


#define LIMIT_RECURSION_DEPTH(maxDepth) /* NOLINT(cppcoreguidelines-macro-usage) */                              \
    static int _rec_counter;                                                                                     \
    auto _rec_counter_cleanup = qScopeGuard([&]() { --_rec_counter; });                                          \
    if (++_rec_counter > maxDepth) {                                                                             \
        qWarning() << "Recursion limit exceeded in" << Q_FUNC_INFO << "Limit is" << maxDepth;       \
        return;                                                                                                  \
    }                                                                                                            \

#define LIMIT_RECURSION_DEPTH_RET(maxDepth, returnValue) /* NOLINT(cppcoreguidelines-macro-usage) */             \
    static int _rec_counter;                                                                                     \
    auto _rec_counter_cleanup = qScopeGuard([&]() { --_rec_counter; });                                          \
    if (++_rec_counter > maxDepth) {                                                                             \
        qWarning() << "Recursion limit exceeded in" << Q_FUNC_INFO << "Limit is" << maxDepth;       \
        return returnValue;                                                                                      \
    }                                                                                                            \


Node::NamedMultiValue::NamedMultiValue(const QString &key, QVariant value)
    : key(key)
{
    values[0] = value;
}

const QVariant &Node::NamedMultiValue::value() const
{
    static const QVariant invalid;
    if (values.empty()) {
        return invalid;
    }

    return values.last();
}

int Node::NamedMultiValue::setValue(const QVariant &value)
{
    if (values.isEmpty()) {
        return -1;
    }
    auto it = values.end();
    --it;
    it.value() = value;
    return it.key();
}

void Node::NamedMultiValue::writeValue(const QVariant &value, int level)
{
    values[level] = value;
}

void Node::NamedMultiValue::changePriority(int oldPrio, int newPrio)
{
    if (oldPrio == newPrio) {
        return;
    }
    emitPending = newPrio >= values.lastKey() || oldPrio == values.lastKey();
    qDebug() << "Changinng prio from" << oldPrio << "to" << newPrio << emitPending;
    auto it = values.find(oldPrio);
    if (it == values.end() ) {
        return;
    }
    auto it2 = values.find(newPrio);

    QVariant val = it.value();
    if (it2 != values.end()) {
        qInfo() << "Overwriting value at level" << newPrio << it2.value() << "with" << val << values.keys();
        it2.value() = val;
    } else {
        values.insert(newPrio, val);
    }
    values.erase(it);
}

void Node::createObject()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (typeHint != QMetaType::UnknownType) {
        const QMetaObject *mo = QMetaType::metaObjectForType(typeHint);
#else
    if (typeHint.isValid()) {
        const QMetaObject *mo = typeHint.metaObject();
#endif
        Q_ASSERT(mo);
        if (m_object) {
            m_object->deleteLater();
        }
        m_object = mo->newInstance();
        updateObjectProperties();
        m_config->userObjectCreated(this, m_object);
    } else {
        QMetaObjectBuilder b;
        QStringList classNameParts;
        Node *p = m_parent;
        if (!m_name.isEmpty()) {
            classNameParts.append(m_name);
            classNameParts.last()[0] = classNameParts.last()[0].toUpper();
        }
        while (p) {
            classNameParts.prepend(m_parent->m_name);
            classNameParts.first()[0] = classNameParts.first()[0].toUpper();
            p = p->m_parent;
        }
        if (!classNameParts.isEmpty()) {
            b.setClassName(classNameParts.join("").toLatin1());
        } else {
            b.setClassName("RootObject");
        }
        b.setSuperClass(&QObject::staticMetaObject);

        // add POD properties first
        for (auto &p : properties) {
            QByteArray type;
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            switch (p.values[0].type()) {
            case QVariant::Bool:
                type = "bool";
                break;
            case QVariant::Double:
                type = "double";
                break;
            case QVariant::String:
                type = "QString";
                break;
            case QVariant::List:
                type = "QVariantList";
                break;
            case QVariant::LongLong:
                type = "qlonglong";
                break;
            default:
                qWarning() << "Unsupported property type" << p.key << p.values[0].type();
                continue;
            }
    #else
            switch (p.values[0].typeId()) {
            case QMetaType::Bool:
                type = "bool";
                break;
            case QMetaType::Double:
                type = "double";
                break;
            case QMetaType::QString:
                type = "QString";
                break;
            case QMetaType::QVariantList:
                type = "QVariantList";
                break;
            case QMetaType::LongLong:
                type = "qlonglong";
                break;
            default:
                qWarning() << "Unsupported property type" << p.key << p.values[0].typeName();
                continue;
            }
    #endif
            QByteArray pname = p.key.toLatin1();
            QMetaPropertyBuilder pb = b.addProperty(pname, type);
            pb.setStdCppSet(false);
            pb.setWritable(!m_config->readonly());
            QByteArray sig(pname + "Changed()");
            QMetaMethodBuilder mb = b.addSignal(sig);
            mb.setReturnType("void");
            pb.setNotifySignal(mb);
        }

        for (auto &cn : m_childNodes) {
            QByteArray pname = cn->m_name.toLatin1();
            QMetaPropertyBuilder pb = b.addProperty(pname, "QObject*");
            QByteArray sig(pname + "Changed()");
            QMetaMethodBuilder mb = b.addSignal(sig);
            mb.setReturnType("void");
            pb.setNotifySignal(mb);
        }
        QMetaObject *mo = b.toMetaObject();
        if (m_object) {
            m_object->deleteLater();
        }

        m_object = new JsonQObject(mo, this, m_parent ? static_cast<QObject*>(m_parent->m_object) : m_config);
    }
}

void Node::updateObjectProperties()
{
    const QMetaObject *mo = m_object->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty mp = mo->property(i);
        int p_idx = indexOfProperty(mp.name());
        if (p_idx != -1) {
            auto & p = properties[p_idx];
            p.userTypePropertyIndex = i;
            mp.write(m_object, p.value());
            int sig_idx = mp.notifySignalIndex();
            if (sig_idx != -1) {
                if (p.listenerConnection) {
                    QObject::disconnect(p.listenerConnection);
                }
                p.listenerConnection = QMetaObject::connect(m_object, sig_idx, m_config, JsonConfig::listenerSlotIndex);
            }
        }
    }
}

void Node::notifyPropertyUpdate(int propertyIndex)
{
    const QMetaObject *mo = m_object->metaObject();
    const auto &p = properties[propertyIndex];
    bool userType = false;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    userType = (typeHint != QMetaType::UnknownType && p.userTypePropertyIndex != -1);
#else
    userType = (typeHint.isValid() && p.userTypePropertyIndex != -1);
#endif
    if (userType) {
        propertyIndex = p.userTypePropertyIndex;
    } else {
        propertyIndex += mo->propertyOffset();
    }
    int sig_id = mo->property(propertyIndex).notifySignalIndex();
    int loc_id = sig_id - mo->methodOffset();
    while (loc_id < 0) {
        mo = mo->superClass();
        loc_id = sig_id - mo->methodOffset();
    }
    QVector<void*> args;
    args.append(nullptr);
    QMetaObject::activate(m_object, mo, loc_id, args.data());
}

void Node::handleSpecialProperty(QString name, QString value)
{
    if (name == QLatin1String("$type")) {
        QByteArray typeName = (value + "*").toLatin1();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        typeHint = QMetaType::type(typeName.data());
        if (!QMetaType::isRegistered(typeHint)) {
            qWarning() << "Can't resolve type" << typeName << "Make sure that type name is correct and the corresponding type is registered by using qRegisterMetatype";
        }
        if (!QMetaType(typeHint).flags().testFlag(QMetaType::PointerToQObject)) {
            qWarning() << "Type" << typeName << "must inherit QObject";
            typeHint = QMetaType::UnknownType;
        }
#else
        typeHint = QMetaType::fromName(typeName);
        if (!typeHint.isValid()) {
            qWarning() << "Can't resolve type" << typeName << "Make sure that type name is correct and the corresponding type is registered by using qRegisterMetatype";
        }
        if (!typeHint.flags().testFlag(QMetaType::PointerToQObject)) {
            qWarning() << "Type" << typeName << "must inherit QObject";
            typeHint = QMetaType(QMetaType::UnknownType);
        }
#endif
    } else if (name == QLatin1String("$ref")) {
        qInfo() << "JSON pointers are not implemented yet.";
    }
}


const QString &Node::name() const
{
    return m_name;
}

bool Node::moveLayer(int oldPriority, int newPriority)
{
    LIMIT_RECURSION_DEPTH_RET(10, false);
    bool changed = false;
    for (int i = 0; i < properties.size(); ++i) {
        NamedMultiValue &p = properties[i];
        p.changePriority(oldPriority, newPriority);
        changed |= p.emitPending;
    }
    for (auto n : qAsConst(m_childNodes)) {
        changed |= n->moveLayer(oldPriority, newPriority);
    }
    return changed;
}



void Node::setJsonObject(QJsonObject object)
{
    LIMIT_RECURSION_DEPTH(10);
    QMap<QString, QJsonObject> objects;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            if (it.key().startsWith('$')) {
                handleSpecialProperty(it.key(), it.value().toString());
            } else {
                properties.append(NamedMultiValue(it.key(), it.value().toVariant()));
            }
        } else {
            objects[it.key()] = it.value().toObject();
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        Node *n = new Node();
        n->m_config = m_config;
        n->m_name = it.key();
        m_childNodes.append(n);
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        n->setJsonObject(it.value()); // NOLINT
        n->m_parent = this;
    }
    createObject();
}

QJsonObject Node::toJsonObject(int level) const
{
    LIMIT_RECURSION_DEPTH_RET(10, {});
    QJsonObject ret;
    for (const auto &g : properties) {
        if (level == -1) {
            ret[g.key] = QJsonValue::fromVariant(g.value());
        } else if (g.values.size() > level) {
            QVariant v = g.values.value(level);
            if (v.isValid()) {
                ret[g.key] = QJsonValue::fromVariant(v);
            }
        }
    }
    for (const Node *n : m_childNodes) {
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        QJsonObject obj = n->toJsonObject(level); // NOLINT
        if (!obj.isEmpty()) {
            ret[n->m_name] = obj;
        }
    }
    return ret;
}

// swap JSON object for nodes when layer file is changed
void Node::swapJsonObject(QJsonObject oldObject, QJsonObject newObject, int level)
{
    LIMIT_RECURSION_DEPTH(10);

    for (int i = 0; i < properties.size(); ++i) {
        auto it_old = oldObject.find(properties[i].key);
        auto it_new = newObject.find(properties[i].key);
        if (it_new != newObject.end()) {
            if (it_new->isObject()) {
                qWarning() << "Property" << it_new.key() << "has different type in the layer (Object)";
                removeProperty(i, level);
            } else {
                updateProperty(i, level, it_new.value().toVariant());
            }
            newObject.erase(it_new);
        } else if (it_old != oldObject.end()) {
            qDebug() << "Property" << fullPropertyName(properties[i].key) << "is missing in new config";
            removeProperty(i, level);
        }
    }

    for (Node *n : qAsConst(m_childNodes)) {
        auto it = newObject.find(n->name());
        if (it != newObject.end()) {
            n->swapJsonObject(oldObject.value(n->name()).toObject(), it->toObject(), level);
            it = newObject.erase(it);
        }
    }

    if (!newObject.isEmpty()) {
        QStringList keys = newObject.keys();
        for (auto &key : keys) {
            key = fullPropertyName(key);
        }
        qDebug().noquote() << "Unknown keys:" << keys.join(", ");
    }
}

// update existing properties with a new JSON object for given level. The object must be created, i. e., the initial config loaded.
void Node::updateJsonObject(QJsonObject object, int level)
{
    LIMIT_RECURSION_DEPTH(10);
    QMap<QString, QJsonObject> objects;

    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            auto ii = std::find_if(properties.begin(), properties.end(), [&](const NamedMultiValue &v) { return v.key == it.key();});
            if (ii == properties.end()) {
                QString msg = QString("Property %1 does not exist in base config").arg(fullPropertyName(it.key()));
                qWarning().noquote() << msg;
                continue;
            }
            int id = std::distance(properties.begin(), ii);
            updateProperty(id, level, it.value().toVariant());
        } else {
            objects[it.key()] = it.value().toObject();
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        auto ii = std::find_if(m_childNodes.begin(), m_childNodes.end(), [&](const Node *node) { return node->m_name == it.key();});
        if (ii == m_childNodes.end()) {
            QString msg = QString("Property %1 does not exist in base config").arg(fullPropertyName(it.key()));
            qWarning().noquote() << msg;
            continue;
        }
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        (*ii)->updateJsonObject(it.value(), level); // NOLINT
    }
}

bool Node::updateProperty(int index, int level, QVariant value)
{
    QVariant oldValue = valueAt(index);
    auto &p = properties[index];
    p.values[level] = value;

    if (oldValue != valueAt(index)) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (typeHint != QMetaType::UnknownType && p.custonTypePropertyIndex != -1) {
#else
        if (typeHint.isValid() && p.userTypePropertyIndex != -1) {
#endif
            const auto *mo = m_object->metaObject();
            QMetaProperty mp = mo->property(p.userTypePropertyIndex);
            m_object->blockSignals(true);
            mp.write(m_object, value);
            m_object->blockSignals(false);
        }
        propertyChangedHelper(index);
        return true;
    }
    return false;
}

void Node::removeProperty(int index, int level)
{
    auto &p = properties[index];
    auto oldValue = valueAt(index);
    p.values.remove(level);
    auto newValue = valueAt(index);
    if (oldValue != newValue) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (typeHint != QMetaType::UnknownType && p.custonTypePropertyIndex != -1) {
#else
        if (typeHint.isValid() && p.userTypePropertyIndex != -1) {
#endif
            const auto *mo = m_object->metaObject();
            QMetaProperty mp = mo->property(p.userTypePropertyIndex);
            m_object->blockSignals(true);
            mp.write(m_object, newValue);
            m_object->blockSignals(false);

        }
        propertyChangedHelper(index);
    }
}

void Node::clear()
{
    LIMIT_RECURSION_DEPTH(10);
    if (m_object) {
        m_object->deleteLater();
        m_object = nullptr;
    }

    properties.clear();
    m_name.clear();

    for (auto &child : m_childNodes) {
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        child->clear(); // NOLINT
    }
    qDeleteAll(m_childNodes);
    m_childNodes.clear();
}

void Node::unload(int level)
{
    LIMIT_RECURSION_DEPTH(10);
    for (int i = 0; i < properties.size(); ++i) {
        removeProperty(i, level);
    }
    for (auto &n : m_childNodes) {
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        n->unload(level); // NOLINT
    }
}

void Node::emitDeferredSignals()
{
    LIMIT_RECURSION_DEPTH(10);
    for (qsizetype i = 0; i < properties.size(); ++i) {
        if (properties[i].emitPending) {
            notifyPropertyUpdate(i);
        }
    }
    for (auto *n : qAsConst(m_childNodes)) {
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        n->emitDeferredSignals(); // NOLINT
    }
}

int Node::indexOfProperty(const QString &name) const
{
    auto it = std::find_if(properties.begin(), properties.end(), [&](const NamedMultiValue &grp) { return grp.key == name; });
    if (it == properties.end()) {
        return -1;
    }
    return std::distance(properties.begin(), it);
}

int Node::indexOfChild(const QString &name) const
{
    auto it = std::find_if(m_childNodes.begin(), m_childNodes.end(), [&](const Node *n) { return n->m_name == name; });
    if (it == m_childNodes.end()) {
        return -1;
    }
    return std::distance(m_childNodes.begin(), it);
}

QString Node::fullPropertyName(const QString &property) const
{
    QStringList pl;
    const Node *n = this;
    while (n) {
        if (!n->m_name.isEmpty()) {
            pl.prepend(n->m_name);
        }
        n = n->m_parent;
    }
    pl.append(property);
    return pl.join(".");
}

void Node::setConfig(JsonConfig *newConfig)
{
    m_config = newConfig;
}

void Node::propertyChangedHelper(int index)
{
    if (m_config->deferChangeSignals()) {
        properties[index].emitPending = true;
    } else {
        notifyPropertyUpdate(index);
    }
}

QObject *Node::object() const
{
    return m_object;
}

Node *Node::getNode(const QString &key, int *indexOfProperty)
{
    QStringList parts = key.split(".");
    Node *n = this;
    int propIdx = -1;
    while (n) {
        if (parts.size() == 1) {
            propIdx = n->indexOfProperty(parts.last());
            break;
        } else {
            int childIdx = n->indexOfChild(parts.takeFirst());
            if (childIdx == -1) {
                n = nullptr;
            } else {
                n = n->m_childNodes[childIdx];
            }
        }
    }
    *indexOfProperty = propIdx;
    return n;
}

Node *Node::childAt(qsizetype index) const
{
    return m_childNodes.at(index);
}
