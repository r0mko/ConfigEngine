#include "jsonconfig.h"
#include "JsonQObject.h"
#include "configlayer.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QJsonDocument>

namespace {

QString getNameFromPath(const QString &path)
{
    QUrl url = QUrl::fromLocalFile(path);
    QStringList ret = url.adjusted(QUrl::NormalizePathSegments).fileName().split('.');
    if (ret.size() > 1) {
        ret.removeLast();
        return ret.join(".");
    } else if (ret.size() == 1) {
        return ret.first(); // filename without extension
    }
    qWarning() << "Incorrect path format" << path;
    return QString();
}

QByteArray readFile(const QString &path, bool *ok)
{
    QFile f(path);

    if (!f.open(QIODevice::ReadOnly)) {
        QString msg = QString("File %1 not found").arg(path);
        qWarning().noquote() << msg;
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

QJsonObject parseData(const QByteArray &data, bool *ok)
{
    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        QString msg = QString("Parse error: %1").arg(err.errorString());
        qWarning().noquote() << msg;
        *ok = false;
        return QJsonObject();
    }
    if (!json.isObject()) {
        QString msg = QString("JSON must contain an object");
        qWarning().noquote() << msg;
        *ok = false;
        return QJsonObject(); // TODO: set invalid state
    }
    *ok = true;
    return json.object();
}

}

JsonConfig::JsonConfig(QObject *parent)
    : QObject{parent}
{
    m_root.setConfig(this);
}

const QString &JsonConfig::filePath() const
{
    return m_filePath;
}

void JsonConfig::setFilePath(const QString &newFilePath)
{
    if (m_filePath == newFilePath)
        return;
    m_filePath = newFilePath;
    emit filePathChanged();
    auto l = doLoadLayer(newFilePath, QString(), 0);
    if (!l || l->object.isEmpty()) {
        setStatus(Error);
        return;
    }
    scheduleUpdate();
}

QObject *JsonConfig::configData() const
{
    return m_root.object();
}

bool JsonConfig::readonly() const
{
    return m_readonly;
}

void JsonConfig::setReadonly(bool newReadonly)
{
    if (m_readonly == newReadonly)
        return;
    m_readonly = newReadonly;
    emit readonlyChanged();
}

JsonConfig::Status JsonConfig::status() const
{
    return m_status;
}

// loads config. If desired index is 0 or no other config is loaded, the loaded file will be treated as a root config
QString JsonConfig::loadLayer(const QString &path, QString name, int desiredIndex)
{
    auto it = doLoadLayer(path, name, desiredIndex);
    scheduleUpdate();
    return it->name;
}

void JsonConfig::writeConfig(const QString &path, const QString &layer)
{
    auto l = getLayer(layer);
    if (!l) {
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
    doc.setObject(m_root.toJsonObject(l->index));
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    l->modified = false;
    checkModified();
}

void JsonConfig::unloadLayer(const QString &layer)
{
    auto l = getLayer(layer);
    if (!l) {
        return;
    }
    m_root.unload(l->index);
    m_layers.remove(layer);
}

void JsonConfig::activateLayer(const QString &layer)
{
    auto l = getLayer(layer);
    if (!l || l->active) {
        return;
    }
    doActivateLayer(l);
}

void JsonConfig::deactivateLayer(const QString &layer)
{
    auto l = getLayer(layer);
    if (!l || !l->active) {
        return;
    }
    doDeactivateLayer(l);

}

void JsonConfig::clear()
{
    m_root.clear();
    emit configDataChanged();
}

void JsonConfig::setProperty(const QString &layer, const QString &key, QVariant value)
{
    int propIdx = -1;
    Node *n = m_root.getNode(key, &propIdx);
    auto l = getLayer(layer);
    if (!l) {
        return;
    }
    if (propIdx != -1) {
        n->updateProperty(propIdx, l->index, value);
    }
}

QVariant JsonConfig::getProperty(const QString &layer, const QString &key)
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

void JsonConfig::beginUpdate()
{
    m_deferChangeSignals = true;
}

void JsonConfig::endUpdate()
{
    m_deferChangeSignals = false;
    m_root.emitDeferredSignals();
}

void JsonConfig::handleAddedChild(int index, QObject *object)
{
    qDebug() << "Added child" << object;
    if (ConfigLayer *qmlLayer = qobject_cast<ConfigLayer*>(object)) {
        addQmlLayer(qmlLayer);
    }
}

void JsonConfig::handleRemovedChild(int index, QObject *object)
{
    qDebug() << "Removed child" << object;
    if (ConfigLayer *qmlLayer = qobject_cast<ConfigLayer*>(object)) {
        unloadLayer(qmlLayer->name());
        scheduleUpdate();
    }
}

bool JsonConfig::deferChangeSignals() const
{
    return m_deferChangeSignals;
}

void JsonConfig::classBegin()
{
}

void JsonConfig::componentComplete()
{
    for (ConfigLayer *l : qAsConst(m_qmlLayers)) {
        addQmlLayer(l);
    }
}

bool JsonConfig::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        update();
        return true;
    }
    return QObject::event(event);
}
QQmlListProperty<QObject> JsonConfig::qmlChildren() {

    return QQmlListProperty<QObject>(this, 0,
                                     &JsonConfig::qmlChildrenAppend,
                                     &JsonConfig::qmlChildrenCount,
                                     &JsonConfig::qmlChildrenAt,
                                     QQmlListProperty<QObject>::ClearFunction());
}
void JsonConfig::qmlChildrenAppend(QQmlListProperty<QObject> *list, QObject *object)
{
    JsonConfig *o = qobject_cast<JsonConfig*>(list->object);
    o->m_children.append(object);
    qDebug() << "Adding QML child" << object;
    if (object->inherits("QQmlInstantiator")) {
        connect(object, SIGNAL(objectAdded(int,QObject*)), o, SLOT(handleAddedChild(int,QObject*)));
        connect(object, SIGNAL(objectRemoved(int,QObject*)), o, SLOT(handleRemovedChild(int,QObject*)));
    }
    if (ConfigLayer *l = qobject_cast<ConfigLayer*>(object)) {
        o->m_qmlLayers.append(l);
    }
    emit o->childrenChanged();
}

qsizetype JsonConfig::qmlChildrenCount(QQmlListProperty<QObject> *list)
{
    JsonConfig *o = qobject_cast<JsonConfig*>(list->object);
    return o->m_children.size();
}

QObject *JsonConfig::qmlChildrenAt(QQmlListProperty<QObject> *list, qsizetype index)
{
    JsonConfig *o = qobject_cast<JsonConfig*>(list->object);
    return o->m_children.at(index);
}

void JsonConfig::addQmlLayer(ConfigLayer *layer)
{
    layer->setConfig(this);
    auto l = doLoadLayer(layer->filePath(), layer->name(), layer->priority());
    if (!l) {
        qWarning() << "Failed to load layer" << layer->filePath();
        return;
    }
    layer->doUpdateName(l->name);
    layer->doUpdatePriority(l->index);
    l->qmlLayer = layer;
    if (layer->active()) {
        l->active = true;
        l->flag = ConfigLayerData::Active;
        scheduleUpdate();
    }
}

JsonConfig::ConfigLayerData *JsonConfig::doLoadLayer(const QString &path, QString name, int desiredIndex)
{
    if (desiredIndex < 0) {
        if (m_layers.empty() && !m_root.object()) {
            qWarning() << "Loading layer" << path << "before root config";
            desiredIndex = m_layers.size() + 1;
        } else {
            desiredIndex = m_layers.size();
        }
    }
    if (name.isEmpty()) {
        name = getNameFromPath(path);
    }
    ConfigLayerData layer = ConfigLayerData::fromFile(path);
    if (layer.flag != ConfigLayerData::None) {
        qWarning() << "Layer loading error" << path;
        return nullptr;
    }
    layer.name = name;
    layer.index = desiredIndex;
    layer.flag = ConfigLayerData::Object;
    auto it = m_layers.insert(name, layer);
    emit layersChanged();
    return &it.value();
}

void JsonConfig::doActivateLayer(ConfigLayerData *layer)
{
    layer->active = true;
    layer->flag = ConfigLayerData::Active;
    scheduleUpdate();
}

void JsonConfig::doDeactivateLayer(ConfigLayerData *layer)
{
    layer->active = false;
    layer->flag = ConfigLayerData::Active;
    scheduleUpdate();
}

void JsonConfig::scheduleUpdate()
{
    if (!m_updatePending) {
        m_updatePending = true;
        if (m_deferUpdate) {
            qApp->postEvent(this, new QEvent(QEvent::UpdateRequest));
        } else {
            update();
        }
    }
}

void JsonConfig::update()
{
    beginUpdate();
    QMap<int, ConfigLayerData*> sortedLayers;
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        sortedLayers.insert(it->index, &it.value());
    }
    for (auto layer : sortedLayers) {
        if (layer->flag == ConfigLayerData::Object) {
            if (layer->index == 0) {
                m_root.setJsonObject(layer->object);
                emit configDataChanged();
                setStatus(ConfigLoaded);
            } else if (layer->active) {
                QJsonObject oldObj = m_root.toJsonObject(layer->index);
                m_root.swapJsonObject(oldObj, layer->object, layer->index);
            }
            layer->flag = ConfigLayerData::None;
        } else if (layer->flag == ConfigLayerData::Active) {
            if (layer->active) {
                m_root.updateJsonObject(layer->object, layer->index);
            } else {
                m_root.unload(layer->index);
            }
            layer->flag = ConfigLayerData::None;
            emit activeLayersChanged();
        }
    }
    endUpdate();
    m_updatePending = false;
}

void JsonConfig::setStatus(Status newStatus)
{
    if (m_status == newStatus)
        return;
    m_status = newStatus;
    emit statusChanged();
}

void JsonConfig::checkModified()
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

JsonConfig::ConfigLayerData *JsonConfig::getLayer(const QString &name)
{
    auto it = m_layers.find(name);
    if (it == m_layers.end()) {
        return nullptr;
    }
    return &it.value();
}

bool JsonConfig::deferUpdate() const
{
    return m_deferUpdate;
}

void JsonConfig::setDeferUpdate(bool newDeferUpdate)
{
    if (m_deferUpdate == newDeferUpdate)
        return;
    m_deferUpdate = newDeferUpdate;
    emit deferUpdateChanged();
}

void JsonConfig::updateLayer(const QString &layer, const QString &filePath)
{
    auto newLayer = ConfigLayerData::fromFile(filePath);
    if (newLayer.index != -1) {
        setStatus(Error);
        qWarning() << "Error updating layer" << layer << "file loading or parsing error" << filePath;
        return;
    }
    auto l = getLayer(layer);
    l->flag = ConfigLayerData::Object;
    l->object = newLayer.object;
    scheduleUpdate();
}

void JsonConfig::changeLayerName(const QString &oldName, const QString &newName)
{
    auto it = m_layers.find(oldName);
    if (it == m_layers.end()) {
        qWarning() << "Unknown layer" << oldName;
        return;
    }
    it->name = newName;
    ConfigLayerData layer = it.value();
    m_layers.erase(it);
    m_layers.insert(newName, layer);
}

void JsonConfig::changeLayerPriority(const QString &name, int priority)
{
    if (auto layer = getLayer(name)) {
        m_root.moveLayer(layer->index, priority);
        layer->index = priority;
    }
    m_root.emitDeferredSignals();
}

JsonConfig::ConfigLayerData JsonConfig::ConfigLayerData::fromFile(const QString &path)
{
    ConfigLayerData ret;
    bool ok;
    QByteArray data = readFile(path, &ok);
    if (!ok) {
        ret.flag = FileError;
        return ret;
    }
    QJsonObject obj = parseData(data, &ok);
    if (!ok) {
        ret.flag = ParseError;
        return ret;
    }
    ret.flag = None;
    ret.index = -1;
    ret.object = obj;
    return ret;
}

QStringList JsonConfig::layers() const
{
    QStringList ret;
    for (const auto &l : m_layers) {
        ret.append(l.name);
    }
    return ret;
}

QStringList JsonConfig::activeLayers() const
{
    QStringList ret;
    for (const auto &l : m_layers) {
        if (l.active && l.flag == ConfigLayerData::None) {
            ret.append(l.name);
        }
    }
    return ret;
}
