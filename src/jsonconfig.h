#pragma once

#include <QObject>
#include <QJsonObject>
#include <QQmlParserStatus>
#include <QQmlListProperty>
#include <QPointer>
#include "private/node.h"

class ConfigLayer;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define qsizetype int
#endif

class JsonConfig : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QObject* configData READ configData NOTIFY configDataChanged)
    Q_PROPERTY(bool readonly READ readonly WRITE setReadonly NOTIFY readonlyChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QQmlListProperty<QObject> children READ qmlChildren NOTIFY childrenChanged)
    Q_PROPERTY(bool deferUpdate READ deferUpdate WRITE setDeferUpdate NOTIFY deferUpdateChanged)
    Q_PROPERTY(QStringList layers READ layers NOTIFY layersChanged)
    Q_PROPERTY(QStringList activeLayers READ activeLayers NOTIFY activeLayersChanged)

    Q_CLASSINFO("DefaultProperty", "children");

public:
    explicit JsonConfig(QObject *parent = nullptr);

    enum Status
    {
        Null,
        Error,
        ConfigLoaded,
        ConfigModified
    };
    Q_ENUM(Status);

    const QString &filePath() const;
    void setFilePath(const QString &newFilePath);

    QObject *configData() const;

    bool readonly() const;
    void setReadonly(bool newReadonly);

    Status status() const;

    bool deferChangeSignals() const;

    void classBegin() override;
    void componentComplete() override;

    bool event(QEvent *event) override;

    bool deferUpdate() const;
    void setDeferUpdate(bool newDeferUpdate);


    QStringList layers() const;
    QStringList activeLayers() const;

public slots:
    void changeLayerName(const QString &oldName, const QString &newName);
    void changeLayerPriority(const QString &name, int priority);
    QString loadLayer(const QString &path, QString name, int desiredIndex = -1);
    void writeConfig(const QString &path, const QString &layer);
    void unloadLayer(const QString &layer);
    void activateLayer(const QString &layer);
    void deactivateLayer(const QString &layer);
    void clear();
    void setProperty(const QString &layer, const QString &key, const QVariant &value);
    QVariant getProperty(const QString &layer, const QString &key);

    void beginUpdate();
    void endUpdate();

private slots:
    void handleAddedChild(int, QObject *object);
    void handleRemovedChild(int, QObject *object);
    void onUserObjectPropertyChanged();

signals:
    void filePathChanged();
    void configDataChanged();
    void readonlyChanged();
    void statusChanged();
    void childrenChanged();
    void deferUpdateChanged();
    void layersChanged();
    void activeLayersChanged();

protected:
    virtual void userObjectCreated(Node *node, QObject *object);
    QMap<QObject*, Node*> m_userObjects;

private:
    friend class Node;
    friend class ConfigLayer;

    static const int listenerSlotIndex;
    struct ConfigLayerData {
        int index = -1;
        bool active = false;
        bool modified = false;
        ConfigLayer *qmlLayer;
        QString name;
        QJsonObject object;
        static ConfigLayerData fromFile(const QString &path);
        static ConfigLayerData fromData(const QByteArray &json);

        enum { Null, FileError, ParseError, None, Active, Object } flag = Null;
    };

    QString m_filePath;
    Node m_root;
    QMap<QString, ConfigLayerData> m_layers;
    bool m_readonly = false;
    Status m_status = Null;
    QList<QObject*> m_children;
    QList<ConfigLayer*> m_qmlLayers;
    bool m_deferChangeSignals = false;
    bool m_updatePending = false;
    bool m_deferUpdate = true;
    bool m_updating = false;

    QQmlListProperty<QObject> qmlChildren();
    static void qmlChildrenAppend(QQmlListProperty<QObject> *list, QObject *object);
    static qsizetype qmlChildrenCount(QQmlListProperty<QObject> *list);
    static QObject *qmlChildrenAt(QQmlListProperty<QObject> *list, qsizetype index);
    void addQmlLayer(ConfigLayer *layer);
    ConfigLayerData *doLoadLayer(const QString &path, QString name, int desiredIndex);
    void updateLayerPath(const QString& layer, const QString &filePath);

    void doActivateLayer(ConfigLayerData *layer);
    void doDeactivateLayer(ConfigLayerData *layer);
    void scheduleUpdate();
    void update();

    void setStatus(Status newStatus);
    void checkModified();
    ConfigLayerData *getLayer(const QString &name);
};
