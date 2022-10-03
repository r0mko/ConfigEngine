#include "configlayer.h"
#include "jsonconfig.h"

ConfigLayer::ConfigLayer(QObject *parent)
    : QObject{parent}
{

}

int ConfigLayer::priority() const
{
    return m_priority;
}

void ConfigLayer::setPriority(int newPriority)
{
    if (!doUpdatePriority(newPriority)) {
        return;
    }
    if (!m_config) {
        return;
    }
    m_config->changeLayerPriority(m_name, newPriority);
}

const QString &ConfigLayer::name() const
{
    return m_name;
}

void ConfigLayer::setName(const QString &newName)
{
    QString oldName = m_name;
    if (!doUpdateName(newName)) {
        return;
    }
    if (!m_config) {
        return;
    }
    m_config->changeLayerName(oldName, m_name);
}

const QString &ConfigLayer::filePath() const
{
    return m_filePath;
}

void ConfigLayer::setFilePath(const QString &newFilePath)
{
    if (m_filePath == newFilePath)
        return;
    m_filePath = newFilePath;
    emit filePathChanged();
    if (!m_config) {
        return;
    }
    m_config->updateLayer(m_name, m_filePath);
}

bool ConfigLayer::active() const
{
    return m_active;
}

void ConfigLayer::setActive(bool newActive)
{
    if (m_active == newActive)
        return;
    m_active = newActive;
    emit activeChanged();
    if (!m_config) {
        return;
    }
    if (m_active) {
        m_config->activateLayer(m_name);
    } else {
        m_config->deactivateLayer(m_name);
    }
}


void ConfigLayer::setConfig(JsonConfig *newConfig)
{
    m_config = newConfig;
}

bool ConfigLayer::doUpdateName(const QString &newName)
{
    if (m_name == newName)
        return false;
    m_name = newName;
    emit nameChanged();
    return true;
}

bool ConfigLayer::doUpdatePriority(int newPriority)
{
    if (m_priority == newPriority)
        return false;
    if (newPriority == 0) {
        qWarning() << "Priority 0 is not allowed in layers! Priority value must be either -1 or >= 1";
    }
    m_priority = newPriority;
    emit priorityChanged();
    return true;
}
