#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include "ConfigEngine.hpp"
#include "JsonQObject.hpp"

ConfigEngine::ConfigEngine(QObject *parent)
    : QObject(parent)
{
    m_root.setEngine(this);
}

ConfigEngine::~ConfigEngine() {}

QString getNameFromPath(const QString &path)
{
    QUrl url = QUrl::fromLocalFile(path);
    QStringList ret = url.adjusted(QUrl::NormalizePathSegments).fileName().split('.');
    if (!ret.isEmpty()) {
        return ret.first();
    }
    qWarning() << "Incorrect path format" << path;
    return QString();
}

void ConfigEngine::writeConfig(const QString &path, const QString &layer)
{
    auto it = m_layers.find(layer);
    if (it->index == -1) {
        qWarning() << "Layer" << layer << "not found";
        return;
    }
    QString cleanPath = path;
    if (cleanPath.startsWith("file:///")) {
        cleanPath = QUrl(cleanPath).toLocalFile();
    }
    QFile f(cleanPath);
    if (!f.open(QIODevice::WriteOnly)) {
        QString msg = QString("Failed to open file %1 for writing: %2").arg(path, f.errorString());
        qWarning().noquote() << msg;
        return;
    }
    QJsonDocument doc;
    doc.setObject(m_root.toJsonObject(it->index));
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    it->modified = false;
    checkModified();
}

void ConfigEngine::unloadLayer(const QString &layer)
{
    auto it = m_layers.find(layer);
    if (it == m_layers.end()) {
        qWarning() << "Layer" << layer << "not registered";
        return;
    }
    m_root.unload(it->index);
    m_layers.erase(it);
}

void ConfigEngine::activateLayer(const QString &layer)
{
    auto it = m_layers.find(layer);
    if (it == m_layers.end()) {
        qWarning() << "Layer" << layer << "not registered";
        return;
    }
    if (it->active) {
        return;
    }
    it->active = true;
    qDebug() << "Activating layer" << layer << "index" << it->index;
    m_root.updateJsonObject(it->object, it->index);
}

void ConfigEngine::deactivateLayer(const QString &layer)
{
    auto it = m_layers.find(layer);
    if (it == m_layers.end()) {
        qWarning() << "Layer" << layer << "not registered";
        return;
    }
    if (!it->active) {
        return;
    }
    it->active = false;
    qDebug() << "Deactivating layer" << layer << "index" << it->index;
    m_root.unload(it->index);
}

void ConfigEngine::clear()
{
    m_root.clear();
    emit configChanged();
}

void ConfigEngine::setProperty(const QString &layer, const QString &key, QVariant value)
{
    int propIdx = -1;
    Node *n = m_root.getNode(key, &propIdx);
    auto it = m_layers.find(layer);
    if (it == m_layers.end()) {
        qWarning() << "Layer" << layer << "not registered";
        return;
    }

    if (propIdx != -1) {
        n->updateProperty(propIdx, it->index, value);
    }
}

QVariant ConfigEngine::getProperty(const QString &layer, const QString &key)
{
    int index = 0;
    if (!layer.isEmpty()) {
        auto it = m_layers.find(layer);
        if (it != m_layers.end()) {
            qWarning() << "Layer" << layer << "not registered";
            return QVariant();
        }
        index = it.value().index;
    }
    int propIdx = -1;

    Node *n = m_root.getNode(key, &propIdx);
    if (propIdx != -1) {
        auto &vals = n->properties[propIdx].values;
        auto it = vals.find(index);
        if (it != vals.end()) {
            return it.value();
        }
    }
    return QVariant();
}

QJsonObject ConfigEngine::parseData(const QByteArray &data, bool *ok)
{
    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        QString msg = QString("Parse error: %1").arg(err.errorString());
        qWarning().noquote() << msg;
        setStatus(Error);
        *ok = false;
        return QJsonObject();
    }
    if (!json.isObject()) {
        QString msg = QString("JSON must contain an object");
        qWarning().noquote() << msg;
        setStatus(Error);
        *ok = false;
        return QJsonObject(); // TODO: set invalid state
    }
    *ok = true;
    return json.object();
}

ConfigEngine::ConfigLayer &ConfigEngine::getLayer(const QString &name)
{
    static ConfigLayer invalid;
    auto it = m_layers.find(name);
    if (it == m_layers.end()) {
        qWarning() << "Layer" << name << "not found";
        return invalid;
    }
    return it.operator*();
}

void ConfigEngine::setQmlEngine(QQmlEngine *qmlEngine)
{
    m_qmlEngine = qmlEngine;
}

QObject *ConfigEngine::config() const
{
    return m_root.object();
}


void ConfigEngine::updateTree(int level, QJsonObject data)
{
    if (level == 0) {
        // initial config
        m_root.clear();
        m_root.setJsonObject(data);
        emit configChanged();
    } else {
        // update with new properties
        m_root.updateJsonObject(data, level);
    }
}

void ConfigEngine::checkModified()
{
    int c = 0;
    for (const auto &layer : qAsConst(m_layers)) {
        ++c;
        if (layer.modified) {
            setStatus(ConfigModified);
            return;
        }
    }
    if (c > 0) {
        setStatus(ConfigLoaded);
    } else {
        setStatus(Null);
    }
}

QByteArray ConfigEngine::readFile(const QString &path, bool *ok)
{
    QFile f(path);

    if (!f.open(QIODevice::ReadOnly)) {
        QString msg = QString("File %1 not found").arg(path);
        qWarning().noquote() << msg;
        setStatus(Error);
        if (ok) {
            *ok = false;
        }
        return QByteArray();
    }
    QByteArray data = f.readAll();
    f.close();
    if (ok) {
        *ok = true;
    }
    return data;
}



bool ConfigEngine::deferChangeSignals() const
{
    return m_deferChangeSignals;
}

QString ConfigEngine::loadLayer(const QString &path, int desiredIndex)
{
    qDebug() << "Loading config" << path << "for level" << desiredIndex;
    if (desiredIndex == 0) {
        m_layers.clear();
    } else if (desiredIndex < 0 && !m_layers.isEmpty()) {
        desiredIndex = m_layers.size();
    } else if (m_layers.isEmpty()) {
        desiredIndex = 0; // implicitly load the global config
    }
    if (desiredIndex > 0 && m_status < ConfigLoaded) {
        qWarning() << "Can't load layers until the base config is loaded";
        return QString();
    }
    bool ok;
    QByteArray data = readFile(path, &ok);
    if (!ok) {
        return QString();
    }
    QJsonObject obj = parseData(data, &ok);
    if (!ok && desiredIndex == 0) {
        setStatus(Error);
        return QString();
    }
    QString name = getNameFromPath(path);
    ConfigLayer layer;
    layer.index = desiredIndex;
    layer.object = obj;
    auto it = m_layers.insert(name, layer);
    if (desiredIndex == 0 && m_layers.size() == 1) {
        it->active = true;
        qDebug() << "Loaded root config";
        m_root.setJsonObject(obj);
        setStatus(ConfigLoaded);
        qDebug() << m_root.object();
    }
    return name;
}


ConfigEngine::Status ConfigEngine::status() const
{
    return m_status;
}

void ConfigEngine::beginUpdate()
{
    m_deferChangeSignals = true;
}

void ConfigEngine::endUpdate()
{
    m_deferChangeSignals = false;
    m_root.emitDeferredSignals();
}

void ConfigEngine::setStatus(Status newStatus)
{
    if (m_status == newStatus)
        return;
    m_status = newStatus;
    emit statusChanged();
}

bool ConfigEngine::readonly() const
{
    return m_readonly;
}

void ConfigEngine::setReadonly(bool newReadonly)
{
    if (m_readonly == newReadonly)
        return;
    m_readonly = newReadonly;
    emit readonlyChanged();
}
