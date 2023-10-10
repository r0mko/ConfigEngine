#include "node.h"

#include <private/qmetaobjectbuilder_p.h>

#include "jsonconfig.h"
#include "JsonQObject.h"
#include <QScopeGuard>

static constexpr const int MAX_RECURSION_DEPTH = 10;

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


Node::NamedMultiValue::NamedMultiValue(QString key, QVariant value)
    : key(std::move(key))
{
    values[0] = std::move(value);
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
    if (it.value() != value && !refs.isEmpty()) {
        auto refIt = refs.end();
        --refIt;
        refs.remove(refIt.key());
    }
    it.value() = value;
    return it.key();
}

void Node::NamedMultiValue::writeValue(const QVariant &value, int level)
{
    values[level] = value;
}

void Node::NamedMultiValue::changePriority(int oldPrio, int newPrio)
{
    if (changeValuePriority(oldPrio, newPrio)) {
        changeRefPriority(oldPrio, newPrio);
    }
}

bool Node::NamedMultiValue::changeValuePriority(int oldPrio, int newPrio)
{
    if (oldPrio == newPrio) {
        return false;
    }
    emitPending = newPrio >= values.lastKey() || oldPrio == values.lastKey();
    auto it = values.find(oldPrio);
    if (it == values.end() ) {
        return false;
    }
    auto it2 = values.find(newPrio);

    QVariant val = it.value();
    if (it2 != values.end()) {
        it2.value() = val;
    } else {
        values.insert(newPrio, val);
    }
    values.erase(it);
    return true;
}

bool Node::NamedMultiValue::changeRefPriority(int oldPrio, int newPrio)
{
    auto it = refs.find(oldPrio);
    if (it == refs.end() ) {
        return false;
    }
    auto it2 = refs.find(newPrio);

    QString val = it.value();
    if (it2 != refs.end()) {
        it2.value() = val;
    } else {
        refs.insert(newPrio, val);
    }
    refs.erase(it);
    return true;
}

bool Node::NamedMultiValue::isRef(int level) const
{
    return refs.contains(level);
}

const QString &Node::NamedMultiValue::ref() const
{
    static const QString invalid;
    if (refs.empty()) {
        return invalid;
    }

    return refs.last();
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
        m_object->blockSignals(m_config->m_updating || m_config->deferChangeSignals());
        updateObjectProperties();
        m_object->blockSignals(false);
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

const QString &Node::name() const
{
    return m_name;
}

bool Node::moveLayer(int oldPriority, int newPriority) // NOLINT
{
    LIMIT_RECURSION_DEPTH_RET(MAX_RECURSION_DEPTH, false);
    bool changed = false;
    for (auto & p : properties) {
        p.changePriority(oldPriority, newPriority);
        changed |= p.emitPending;
    }
    for (auto n : qAsConst(m_childNodes)) {
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        changed |= n->moveLayer(oldPriority, newPriority); // NOLINT
    }
    return changed;
}

void Node::setJsonObject(QJsonObject object) // NOLINT
{
    LIMIT_RECURSION_DEPTH(MAX_RECURSION_DEPTH);
    // split primitive propertties and Object properties
    m_cachedJsonObject = &object;
    QMap<QString, QJsonObject> objects;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            if (it.key().startsWith('$')) {
                handleSpecialProperty(it.key(), it.value().toString());
            } else {
                properties.append(NamedMultiValue(it.key(), it.value().toVariant()));
            }
        } else if (isRefObject(it.value().toObject())) {
            auto ref = getRefValue(it.value().toObject());
            NamedMultiValue p{it.key(), resolvedRef(resolvedRefPath(ref))};
            p.refs[0] = ref;
            properties.append(p);
        } else {
            objects[it.key()] = it.value().toObject();
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        Node *n = new Node();
        n->m_config = m_config;
        n->m_name = it.key();
        n->m_root = m_root ? m_root : this;
        m_childNodes.append(NodePtr(n));
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        n->setJsonObject(it.value()); // NOLINT
        n->m_parent = this;
    }
    createObject();
    m_cachedJsonObject = nullptr;
}

QJsonObject Node::toJsonObject(int level) const // NOLINT
{
    LIMIT_RECURSION_DEPTH_RET(MAX_RECURSION_DEPTH, {});
    QJsonObject ret;
    for (const auto &g : properties) {
        if (level == -1) {
            if (g.isRef(level)) {
                ret[g.key] = refToJsonObject(g.ref());
            } else {
                ret[g.key] = QJsonValue::fromVariant(g.value());
            }
        } else if (g.values.contains(level)) {
            QVariant v = g.values.value(level);
            if (v.isValid()) {
                if (g.isRef(level)) {
                    ret[g.key] = refToJsonObject(g.refs.value(level));
                } else {
                    ret[g.key] = QJsonValue::fromVariant(v);
                }
            }
        }
    }
    for (const auto & n : m_childNodes) {
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        QJsonObject obj = n->toJsonObject(level); // NOLINT
        if (!obj.isEmpty()) {
            ret[n->m_name] = obj;
        }
    }
    return ret;
}

// swap JSON object for nodes when layer file is changed
void Node::swapJsonObject(QJsonObject oldObject, QJsonObject newObject, int level) // NOLINT
{
    LIMIT_RECURSION_DEPTH(MAX_RECURSION_DEPTH);

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

    for (auto n : qAsConst(m_childNodes)) {
        auto it = newObject.find(n->name());
        if (it != newObject.end()) {
            // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
            n->swapJsonObject(oldObject.value(n->name()).toObject(), it->toObject(), level); // NOLINT
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
void Node::updateJsonObject(QJsonObject object, int level) // NOLINT
{
    LIMIT_RECURSION_DEPTH(MAX_RECURSION_DEPTH);
    m_cachedJsonObject = &object;
    QMap<QString, QJsonObject> objects;

    auto getPropertyIndex = [this](const QString & key) -> int {
        auto ii = std::find_if(properties.begin(), properties.end(), [&](const NamedMultiValue &v) { return v.key == key;});
        if (ii == properties.end()) {
            qWarning().noquote() << "Property" << fullPropertyName(key) << "does not exist in base config";
            return -1;
        }
        return std::distance(properties.begin(), ii);
    };
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            if (!it.key().startsWith('$')) {
                if (int id = getPropertyIndex(it.key()); id > -1) {
                    updateProperty(id, level, it.value().toVariant());
                }
            }
        } else if (isRefObject(it.value().toObject())) {
            if (int id = getPropertyIndex(it.key()); id > -1) {
                auto ref = getRefValue(it.value().toObject());
                updateProperty(id, level, resolvedRef(resolvedRefPath(ref)));
                properties[id].refs[level] = ref;
            }
        } else {
            objects[it.key()] = it.value().toObject();
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (!it.key().startsWith('$')) {
            auto ii = std::find_if(m_childNodes.begin(), m_childNodes.end(), [&](const NodePtr &node) { return node->m_name == it.key();});
            if (ii == m_childNodes.end()) {
                qWarning().noquote() << "Property" << fullPropertyName(it.key()) << "does not exist in base config";
                continue;
            }
            // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
            (*ii)->updateJsonObject(it.value(), level); // NOLINT
        }
    }
    m_cachedJsonObject = nullptr;
}

bool Node::updateProperty(int index, int level, const QVariant &value)
{
    QVariant oldValue = valueAt(index);
    auto &p = properties[index];
    p.values[level] = value;

    if (oldValue != valueAt(index)) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (typeHint != QMetaType::UnknownType && p.userTypePropertyIndex != -1) {
#else
        if (typeHint.isValid() && p.userTypePropertyIndex != -1) {
#endif
            const auto *mo = m_object->metaObject();
            QMetaProperty mp = mo->property(p.userTypePropertyIndex);
            m_object->blockSignals(true);
            mp.write(m_object, value);
            m_object->blockSignals(false);
        }
        p.refs.remove(level);
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
    p.refs.remove(level);
    auto newValue = valueAt(index);
    if (oldValue != newValue) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (typeHint != QMetaType::UnknownType && p.userTypePropertyIndex != -1) {
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

void Node::clear() // NOLINT
{
    LIMIT_RECURSION_DEPTH(MAX_RECURSION_DEPTH);
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
    m_childNodes.clear();
}

void Node::unload(int level) // NOLINT
{
    LIMIT_RECURSION_DEPTH(MAX_RECURSION_DEPTH);
    for (int i = 0; i < properties.size(); ++i) {
        removeProperty(i, level);
    }
    for (auto &n : m_childNodes) {
        // unrolling recursion to iteration makes code less readable. Recursion depth is limited.
        n->unload(level); // NOLINT
    }
}

void Node::emitDeferredSignals() // NOLINT
{
    LIMIT_RECURSION_DEPTH(MAX_RECURSION_DEPTH);
    for (qsizetype i = 0; i < properties.size(); ++i) {
        if (properties[i].emitPending) {
            notifyPropertyUpdate(i);
        }
    }
    for (auto n : qAsConst(m_childNodes)) {
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
    return int(std::distance(properties.begin(), it));
}

int Node::indexOfChild(const QString &name) const
{
    auto it = std::find_if(m_childNodes.begin(), m_childNodes.end(), [&](const NodePtr &n) { return n->m_name == name; });
    if (it == m_childNodes.end()) {
        return -1;
    }
    return int(std::distance(m_childNodes.begin(), it));
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

void Node::handleSpecialProperty(const QString &name, const QString &value)
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
    }
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
                n = n->m_childNodes[childIdx].data();
            }
        }
    }
    *indexOfProperty = propIdx;
    return n;
}

Node *Node::childAt(qsizetype index) const
{
    return m_childNodes.at(index).data();
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
    auto &p = properties[propertyIndex];
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
    p.emitPending = false;
}

 bool Node::isRefObject(const QJsonObject &object) const
 {
    return object.contains("$ref");
 }

 QString Node::getRefValue(const QJsonObject &object) const
 {
    return object.value("$ref").toString();
 }

QString Node::resolvedRefPath(const QString &ref) const
{
    auto path = ref.mid(2);
    return path.replace("/", ".").replace("~0", "~").replace("~1", "/");
}

QVariant Node::resolvedRef(const QString &path) const
{
    static const QVariant invalid;

    if (!m_root || !m_root->m_cachedJsonObject) {
        return invalid;
    }
    QVariant result;
    auto parts = path.split('.');
    auto object = *m_root->m_cachedJsonObject;
    while (!parts.isEmpty() && object.contains(parts.first())) {
        auto key = parts.first();
        auto v = object.value(key);
        parts.removeFirst();
        if (parts.isEmpty()) {
            if (!v.isObject()) {
                result = v.toVariant();
            } else if (isRefObject(v.toObject())) {
                result = resolvedRef(resolvedRefPath(getRefValue(v.toObject())));
            } else {
                qWarning() << "Reference to Json object not supported.";
                result = invalid;
            }
        } else if (v.isObject()) {
            object = v.toObject();
        } else {
            qWarning() << key << "in " << path << "is not an object.";
            result = invalid;
        }       
    }

    return result;
}

QJsonObject Node::refToJsonObject(const QString &ref) const
{
    QJsonObject object;
    object.insert("$ref", ref);
    return object;
}
