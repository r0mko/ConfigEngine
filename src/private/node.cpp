#include "node.h"

#include <QtCore/private/qmetaobjectbuilder_p.h>

#include "ConfigEngine.hpp"
#include "JsonQObject.hpp"

Node::NamedValueGroup::NamedValueGroup(const QString &key, QVariant value)
    : key(key)
{
    values.reserve(ConfigEngine::LevelsCount);
    values.resize(ConfigEngine::LevelsCount);
    values[ConfigEngine::Global] = value;
}

const QVariant &Node::NamedValueGroup::value() const
{
    static const QVariant invalid;
    for(int i = ConfigEngine::LevelsCount - 1; i >= ConfigEngine::Global; --i) {
        if (values[i].isValid()) {
            return values[i];
        }
    }
    return invalid;
}

int Node::NamedValueGroup::setValue(const QVariant &value)
{
    for(int i = ConfigEngine::LevelsCount - 1; i >= ConfigEngine::Global; --i) {
        if (values[i].isValid()) {
            values[i] = value;
            return i;
        }
    }
    return -1;
}

void Node::NamedValueGroup::writeValue(const QVariant &value, int level)
{
    if (level >= 0 && level < ConfigEngine::LevelsCount && values[level] != value) {
        values[level] = value;
        modified = true;
    }
}

void Node::createObject()
{
    QMetaObjectBuilder b;
    QStringList classNameParts;
    Node *p = parent;
    if (!name.isEmpty()) {
        classNameParts.append(name);
        classNameParts.last()[0] = classNameParts.last()[0].toUpper();
    }
    while (p) {
        classNameParts.prepend(parent->name);
        classNameParts.first()[0] = classNameParts.first()[0].toUpper();
        p = p->parent;
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
        switch (it->values[ConfigEngine::Global].type()) {
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
            qWarning() << "Unsupported property type" << it->key << it->values[ConfigEngine::Global].type();
            continue;
        }
        QByteArray pname = it->key.toLatin1();
        QMetaPropertyBuilder pb = b.addProperty(pname, type);
        pb.setStdCppSet(false);
        pb.setWritable(true);
        QByteArray sig(pname + "Changed()");
        QMetaMethodBuilder mb = b.addSignal(sig);
        mb.setReturnType("void");
        pb.setNotifySignal(mb);
    }

    for (auto it = childNodes.begin(); it != childNodes.end(); ++it) {
        QByteArray pname = (*it)->name.toLatin1();
        QMetaPropertyBuilder pb = b.addProperty(pname, "QObject*");
        QByteArray sig(pname + "Changed()");
        QMetaMethodBuilder mb = b.addSignal(sig);
        mb.setReturnType("void");
        pb.setNotifySignal(mb);
    }
    QMetaObject *mo = b.toMetaObject();
    if (object) {
        object->deleteLater();
    }

    object = new JsonQObject(mo, this, parent ? parent->object : nullptr);
}

void Node::setJsonObject(QJsonObject object)
{
    // split POD propertties from Object properties
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
        n->engine = engine;
        n->name = it.key();
        childNodes.append(n);
        n->setJsonObject(it.value());
        n->parent = this;
    }
    createObject();
}

QJsonObject Node::toJsonObject(int level) const
{
    QJsonObject ret;
    for (const auto &g : properties) {
        if (level == -1) {
            ret[g.key] = QJsonValue::fromVariant(g.value());
        } else if (g.values.size() > level) {
            QVariant v = g.values[level];
            if (v.isValid()) {
                ret[g.key] = QJsonValue::fromVariant(v);
            }
        }
    }
    for (const Node *n : childNodes) {
        QJsonObject obj = n->toJsonObject(level);
        if (!obj.isEmpty()) {
            ret[n->name] = obj;
        }
    }
    return ret;
}

void Node::updateJsonObject(QJsonObject object, int level, ConfigEngine *engine)
{
    QMap<QString, QJsonObject> objects;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            auto ii = std::find_if(properties.begin(), properties.end(), [&](const NamedValueGroup &v) { return v.key == it.key();});
            if (ii == properties.end()) {
                QString msg = QString("Property %1 does not exist in the global config").arg(fullPropertyName(it.key()));
                engine->setErrorString(msg);
                continue;
            }
            int id = std::distance(properties.begin(), ii);
            updateProperty(id, level, it.value().toVariant());
        } else {
            objects[it.key()] = it.value().toObject();
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        auto ii = std::find_if(childNodes.begin(), childNodes.end(), [&](const Node *node) { return node->name == it.key();});
        if (ii == childNodes.end()) {
            QString msg = QString("Property %1 does not exist in the global config").arg(fullPropertyName(it.key()));
            engine->setErrorString(msg);
            continue;
        }
        (*ii)->updateJsonObject(it.value(), level, engine);
    }
}

void Node::updateProperty(int index, int level, QVariant value)
{
    QVariant oldValue = valueAt(index);
    properties[index].values[level] = value;

    bool notify = oldValue != value;

    if (notify) {
        object->notifyPropertyUpdate(index);
    }
}

void Node::clearProperty(int index, int level)
{
    QVariant value = valueAt(index);
    properties[index].values[level].clear();
    QVariant newValue = valueAt(index);
    bool notify = newValue != value;

    if (notify) {
        object->notifyPropertyUpdate(index);
    }
}

void Node::clear()
{
    if (object) {
        object->deleteLater();
        object = nullptr;
    }

    properties.clear();
    name.clear();

    for (auto &child : childNodes) {
        child->clear();
    }
    qDeleteAll(childNodes);
    childNodes.clear();
}

void Node::unload(int level)
{
    for (int i = 0; i < properties.size(); ++i) {
        clearProperty(i, level);
    }
    for (auto &n : childNodes) {
        n->unload(level);
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
    auto it = std::find_if(childNodes.begin(), childNodes.end(), [&](const Node *n) { return n->name == name; });
    if (it == childNodes.end()) {
        return -1;
    }
    return std::distance(childNodes.begin(), it);
}

QString Node::fullPropertyName(const QString &property) const
{
    QStringList pl;
    const Node *n = this;
    while (n) {
        if (!n->name.isEmpty()) {
            pl.prepend(n->name);
        }
        n = n->parent;
    }
    pl.append(property);
    return pl.join(".");
}

void Node::notifyChange(int level)
{
    engine->setModifiedFlag(ConfigEngine::ConfigLevel(level));
}
