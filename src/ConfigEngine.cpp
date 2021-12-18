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























