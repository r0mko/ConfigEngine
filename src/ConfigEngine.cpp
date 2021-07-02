#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <QtCore/private/qmetaobjectbuilder_p.h>

#include "ConfigEngine.hpp"
#include "JsonQObject.hpp"

ConfigEngine::ConfigEngine(QObject *parent)
    : QObject(parent)
{
    m_data.resize(LevelsCount);
}

void ConfigEngine::loadData(const QByteArray &data, ConfigEngine::ConfigLevel level)
{
    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Parse error" << err.errorString();
        return;
    }
    if (!json.isObject()) {
        qWarning() << "Loaded JSON must be an object, actual data type" << data;
        return; // TODO: set invalid state
    }
    m_data[level] = json.object();
    updateTree(level);
}

void ConfigEngine::unloadConfig(ConfigEngine::ConfigLevel level)
{
    if (level == Global) {
        qWarning() << "Can't unload global config. Use clear() method instead.";
        return;
    }

    m_data[level] = QJsonObject();
    m_root.unload(level);

}

void ConfigEngine::clear()
{
    m_root.clear();
    resetContextProperty();
    emit rootChanged();
}

void ConfigEngine::setProperty(const QString &key, QVariant value, ConfigEngine::ConfigLevel level)
{
    QStringList parts = key.split(".");
    Node *n = &m_root;
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
                n = n->childNodes[childIdx];
            }
        }
    }
    if (propIdx != -1) {
        qDebug() << "Setting property" << key << "to" << value;
        n->updateProperty(propIdx, level, value);
    }
}

void ConfigEngine::setQmlEngine(QQmlEngine *qmlEngine)
{
    m_qmlEngine = qmlEngine;
    resetContextProperty();
}

QObject *ConfigEngine::root() const
{
    return m_root.object;
}

void ConfigEngine::loadConfig(const QString &path, ConfigEngine::ConfigLevel level)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "File" << path << "not found!";
        return;
    }
    QByteArray data = f.readAll();
    f.close();
    loadData(data, level);
}



void ConfigEngine::updateTree(ConfigEngine::ConfigLevel level)
{
    if (level == Global) {
        // initial config
        m_root.clear();
        m_root.setJsonObject(m_data[level]);
        emit rootChanged();
        resetContextProperty();
    } else {
        // update with new properties
        m_root.updateJsonObject(m_data[level], level);
    }
}

void ConfigEngine::resetContextProperty()
{
    if (m_qmlEngine) {
        m_qmlEngine->rootContext()->setContextProperty("$Config", m_root.object);
    }
}

const QVariant &ConfigEngine::Node::valueAt(int index) const
{
    return properties[index].value();
}

void ConfigEngine::Node::createObject()
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
        switch (it->values[Global].type()) {
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
            qWarning() << "Unsupported property type" << it->key << it->values[Global].type();
            continue;
        }
        QByteArray pname = it->key.toLatin1();
        QMetaPropertyBuilder pb = b.addProperty(pname, type);
        qDebug() << "Added property" << type << pname;
        pb.setStdCppSet(false);
        pb.setWritable(false);
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

void ConfigEngine::Node::setJsonObject(QJsonObject object)
{
    // split POD propertties from Object properties
    QMap<QString, QJsonObject> objects;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            qDebug() << "Adding JSON prop" << it.key() << "type" << it.value().type();
            properties.append(NamedValueGroup(it.key(), it.value().toVariant()));
        } else {
            objects[it.key()] = it.value().toObject();
        }
    }
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        Node *n = new Node();
        n->name = it.key();
        childNodes.append(n);
        n->setJsonObject(it.value());
        n->parent = this;
    }
    createObject();
}

void ConfigEngine::Node::updateJsonObject(QJsonObject object, ConfigEngine::ConfigLevel level)
{

    QMap<QString, QJsonObject> objects;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isObject()) {
            auto ii = std::find_if(properties.begin(), properties.end(), [&](const NamedValueGroup &v) { return v.key == it.key();});
            if (ii == properties.end()) {
                qWarning() << "Property" << fullPropertyName(it.key()) << "does not exist!";
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
            qWarning() << "Property" << fullPropertyName(it.key()) << "does not exist!";
            continue;
        }
        (*ii)->updateJsonObject(it.value(), level);
    }
}

void ConfigEngine::Node::updateProperty(int index, ConfigEngine::ConfigLevel level, QVariant value)
{
    QVariant oldValue = valueAt(index);
    properties[index].values[level] = value;

    bool notify = oldValue != value;

    if (notify) {
        object->notifyPropertyUpdate(index);
    }
}

void ConfigEngine::Node::clearProperty(int index, ConfigEngine::ConfigLevel level)
{
    QVariant value = valueAt(index);
    properties[index].values[level].clear();
    QVariant newValue = valueAt(index);
    bool notify = newValue != value;

    if (notify) {
        object->notifyPropertyUpdate(index);
    }
}

void ConfigEngine::Node::clear()
{
    if (object) {
        object->deleteLater();
        object = nullptr;
    }

    properties.clear();
    name.clear();

    for (auto child : childNodes) {
        child->clear();
    }
    qDeleteAll(childNodes);
    childNodes.clear();
}

void ConfigEngine::Node::unload(ConfigEngine::ConfigLevel level)
{
    for (int i = 0; i < properties.size(); ++i) {
        clearProperty(i, level);
    }
    for (auto n : childNodes) {
        n->unload(level);
    }
}

int ConfigEngine::Node::indexOfProperty(const QString &name) const
{
    auto it = std::find_if(properties.begin(), properties.end(), [&](const NamedValueGroup &grp) { return grp.key == name; });
    if (it == properties.end()) {
        return -1;
    }
    return std::distance(properties.begin(), it);
}

int ConfigEngine::Node::indexOfChild(const QString &name) const
{
    auto it = std::find_if(childNodes.begin(), childNodes.end(), [&](const Node *n) { return n->name == name; });
    if (it == childNodes.end()) {
        return -1;
    }
    return std::distance(childNodes.begin(), it);
}

QString ConfigEngine::Node::fullPropertyName(const QString &property) const
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

ConfigEngine::NamedValueGroup::NamedValueGroup(const QString &key, QVariant value)
    : key(key)
{
    values[Global] = value;
}

const QVariant &ConfigEngine::NamedValueGroup::value() const
{
    static const QVariant invalid;
    for(int i = LevelsCount - 1; i >= Global; --i) {
        if (values[i].isValid()) {
            return values[i];
        }
    }
    return invalid;
}
