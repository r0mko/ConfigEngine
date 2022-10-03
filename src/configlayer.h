#pragma once

#include <QObject>
#include <QQmlParserStatus>

class JsonConfig;

class ConfigLayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    explicit ConfigLayer(QObject *parent = nullptr);

    int priority() const;
    void setPriority(int newPriority);

    const QString &name() const;
    void setName(const QString &newName);

    const QString &filePath() const;
    void setFilePath(const QString &newFilePath);

    bool active() const;
    void setActive(bool newActive);

    void setConfig(JsonConfig *newConfig);

signals:
    void priorityChanged();
    void nameChanged();
    void filePathChanged();
    void activeChanged();
private:
    friend class JsonConfig;

    bool doUpdateName(const QString &newName);
    bool doUpdatePriority(int newPriority);

    QString m_filePath;
    QString m_name;
    int m_priority = -1;
    bool m_active = false;
    JsonConfig *m_config = nullptr;
};
