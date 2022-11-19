#include "node.h"

#include <QtCore/private/qmetaobjectbuilder_p.h>

#include "jsonconfig.h"
#include "JsonQObject.h"

Node::NamedValueGroup::NamedValueGroup(const QString &key, QVariant value)
    : key(key)
{
    values[0] = value;
}

const QVariant &Node::NamedValueGroup::value() const
{
    static const QVariant invalid;
    if (values.empty()) {
        return invalid;
    }

    return values.last();
}

int Node::NamedValueGroup::setValue(const QVariant &value)
{
    if (values.isEmpty()) {
        return -1;
    }
    auto it = values.end()--;
    it.value() = value;
    return it.key();
}

void Node::NamedValueGroup::writeValue(const QVariant &value, int level)
{
    values[level] = value;
}

void Node::NamedValueGroup::changePriority(int oldPrio, int newPrio)
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
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        QByteArray type;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        switch (it->values[0].type()) {
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
            qWarning() << "Unsupported property type" << it->key << it->values[0].type();
            continue;
        }
#else
        switch (it->values[0].typeId()) {
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
            qWarning() << "Unsupported property type" << it->key << it->values[0].typeName();
            continue;
        }
#endif
        QByteArray pname = it->key.toLatin1();
        QMetaPropertyBuilder pb = b.addProperty(pname, type);
        pb.setStdCppSet(false);
        pb.setWritable(!m_config->readonly());
        QByteArray sig(pname + "Changed()");
        QMetaMethodBuilder mb = b.addSignal(sig);
        mb.setReturnType("void");
        pb.setNotifySignal(mb);
    }

    for (auto it = m_childNodes.begin(); it != m_childNodes.end(); ++it) {
        QByteArray pname = (*it)->m_name.toLatin1();
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

    m_object = new JsonQObject(mo, this, m_parent ? m_parent->m_object : nullptr);
}

const QString &Node::name() const
{
    return m_name;
}

bool Node::moveLayer(int oldPriority, int newPriority)
{
    bool changed = false;
    for (int i = 0; i < properties.size(); ++i) {
        NamedValueGroup &p = properties[i];
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
    // split primitive propertties and Object properties
    QMap<QString, QJsonObject> objects;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            properties.append(NamedValueGroup(it.key(), it.value().toVariant()));
        } else {
            objects[it.key()] = it.value().toObject();
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        Node *n = new Node();
        n->m_config = m_config;
        n->m_name = it.key();
        m_childNodes.append(n);
        n->setJsonObject(it.value());
        n->m_parent = this;
    }
    createObject();
}

QJsonObject Node::toJsonObject(int level) const
{
    QJsonObject ret;
    for (const auto &g : properties) {
        if (level == -1) {
            ret[g.key] = QJsonValue::fromVariant(g.value());
        } else if (g.values.contains(level)) {
            QVariant v = g.values.value(level);
            if (v.isValid()) {
                ret[g.key] = QJsonValue::fromVariant(v);
            }
        }
    }
    for (const Node *n : m_childNodes) {
        QJsonObject obj = n->toJsonObject(level);
        if (!obj.isEmpty()) {
            ret[n->m_name] = obj;
        }
    }
    return ret;
}

// swap JSON object for nodes when layer file is changed
void Node::swapJsonObject(QJsonObject oldObject, QJsonObject newObject, int level)
{
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
    QMap<QString, QJsonObject> objects;

    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            auto ii = std::find_if(properties.begin(), properties.end(), [&](const NamedValueGroup &v) { return v.key == it.key();});
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
        (*ii)->updateJsonObject(it.value(), level);
    }
}

bool Node::updateProperty(int index, int level, QVariant value)
{
    QVariant oldValue = valueAt(index);
    properties[index].values[level] = value;

    if (oldValue != valueAt(index)) {
        propertyChangedHelper(index);
        return true;
    }
    return false;
}

void Node::removeProperty(int index, int level)
{
    QVariant value = valueAt(index);
    properties[index].values.remove(level);
    if (value != valueAt(index)) {
        propertyChangedHelper(index);
    }
}

void Node::clear()
{
    if (m_object) {
        m_object->deleteLater();
        m_object = nullptr;
    }

    properties.clear();
    m_name.clear();

    for (auto &child : m_childNodes) {
        child->clear();
    }
    qDeleteAll(m_childNodes);
    m_childNodes.clear();
}

void Node::unload(int level)
{
    for (int i = 0; i < properties.size(); ++i) {
        removeProperty(i, level);
    }
    for (auto &n : m_childNodes) {
        n->unload(level);
    }
}

void Node::emitDeferredSignals()
{
    for (qsizetype i = 0; i < properties.size(); ++i) {
        if (properties[i].emitPending) {
            m_object->notifyPropertyUpdate(i);
        }
    }
    for (Node *n : m_childNodes) {
        n->emitDeferredSignals();
    }
}

int Node::indexOfProperty(const QString &name) const
{
    auto it = std::find_if(properties.begin(), properties.end(), [&](const NamedValueGroup &grp) { return grp.key == name; });
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
        m_object->notifyPropertyUpdate(index);
    }
}

JsonQObject *Node::object() const
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
