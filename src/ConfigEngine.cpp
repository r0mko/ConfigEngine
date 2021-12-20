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
    m_root.engine = this;
}

ConfigEngine::~ConfigEngine() {}

void ConfigEngine::loadData(const QByteArray &data, ConfigEngine::ConfigLevel level)
{
    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        QString msg = QString("Parse error: %1").arg(err.errorString());
        setErrorString(msg);
        m_state.setFlag(Error);
        emit stateChanged();
        return;
    }
    if (!json.isObject()) {
        QString msg = QString("JSON must contain an object");
        setErrorString(msg);
        emit warning(msg);
        m_state.setFlag(Error);
        emit stateChanged();
        return; // TODO: set invalid state
    }
    m_state.setFlag(Error, false);

    updateTree(level, json.object());
    setConfigLoadedFlag(level, true);
}

void ConfigEngine::writeConfig(const QString &path, ConfigLevel level)
{
    QString cleanPath = path;
    if (cleanPath.startsWith("file:///")) {
        cleanPath = QUrl(cleanPath).toLocalFile();
    }
    QFile f(cleanPath);
    if (!f.open(QIODevice::WriteOnly)) {
        QString msg = QString("Failed to open file %1 for writing: %2").arg(path, f.errorString());
        setErrorString(msg);
        return;
    }
    QJsonDocument doc;
    doc.setObject(m_root.toJsonObject(level));
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    if (level >= 0) {
        setModifiedFlag(level, false);
    }
}

void ConfigEngine::unloadConfig(ConfigEngine::ConfigLevel level)
{
    if (m_state & GlobalConfigLoaded && level == Global) {
        QString msg = "Can't unload global config. Use clear() method instead.";
        setErrorString(msg);
        return;
    }

    m_root.unload(level);
    setConfigLoadedFlag(level, false);
}

void ConfigEngine::clear()
{
    m_root.clear();
    resetContextProperty();
    emit configChanged();
    setConfigLoadedFlag(Global, false);
    setConfigLoadedFlag(User, false);
    setConfigLoadedFlag(Project, false);
}

void ConfigEngine::setProperty(const QString &key, QVariant value, ConfigEngine::ConfigLevel level)
{
    if (level < 0 || level >= ConfigEngine::LevelsCount) {
        return;
    }
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
        n->updateProperty(propIdx, level, value);
        setModifiedFlag(level);
    }
}

void ConfigEngine::setQmlEngine(QQmlEngine *qmlEngine)
{
    m_qmlEngine = qmlEngine;
    resetContextProperty();
}

QObject *ConfigEngine::config() const
{
    return m_root.object;
}

void ConfigEngine::loadConfig(const QString &path, ConfigEngine::ConfigLevel level)
{
    if (level > Global && !m_state.testFlag(GlobalConfigLoaded)) {
        setErrorString("Global config must be loaded first");
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QString msg = QString("File %1 not found").arg(path);
        qWarning().noquote() << msg;
        emit warning(msg);
        return;
    }
    QByteArray data = f.readAll();
    f.close();
    loadData(data, level);
}

void ConfigEngine::updateTree(ConfigEngine::ConfigLevel level, QJsonObject data)
{
    if (level == Global) {
        // initial config
        m_root.clear();
        m_root.setJsonObject(data);
        emit configChanged();
        resetContextProperty();
    } else {
        // update with new properties
        m_root.updateJsonObject(data, level, this);
    }
}

void ConfigEngine::setModifiedFlag(ConfigLevel level, bool on)
{
    StateFlag f = StateFlag(GlobalConfigModified << level);
    if (on != m_state.testFlag(f)) {
        m_state.setFlag(f, on);
        emit stateChanged();
        switch (level) {
        case ConfigEngine::Global:
            emit globalConfigModifiedChanged();
            break;
        case ConfigEngine::User:
            emit userConfigModifiedChanged();
            break;
        case ConfigEngine::Project:
            emit projectConfigModifiedChanged();
            break;
        default:
            break;
        }
    }
}

void ConfigEngine::setConfigLoadedFlag(ConfigLevel level, bool on)
{
    StateFlag f = StateFlag(GlobalConfigLoaded << level);
    if (on != m_state.testFlag(f)) {
        m_state.setFlag(f, on);
        emit stateChanged();
    }
}

void ConfigEngine::resetContextProperty()
{
    if (m_qmlEngine) {
        m_qmlEngine->rootContext()->setContextProperty("$Config", m_root.object);
    }
}

bool ConfigEngine::globalConfigModified() const
{
    return m_state & (GlobalConfigModified << Global);
}

bool ConfigEngine::userConfigModified() const
{
    return m_state & (GlobalConfigModified << User);
}

bool ConfigEngine::projectConfigModified() const
{
    return m_state & (GlobalConfigModified << Project);
}

const ConfigEngine::StateFlags &ConfigEngine::state() const
{
    return m_state;
}

const QString &ConfigEngine::errorString() const
{
    return m_errorString;
}

void ConfigEngine::setErrorString(const QString &errorString)
{
    if (m_errorString != errorString) {
        m_errorString = errorString;
        emit errorStringChanged();
        emit warning(errorString);
        qWarning().noquote() << errorString;
    }
}
