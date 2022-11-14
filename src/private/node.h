/*
 * Mercedes-Benz AG ("COMPANY") CONFIDENTIAL
 * Unpublished Copyright (c) 2022 Mercedes-Benz AG, All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains the property of COMPANY. The intellectual and
 * technical concepts contained herein are proprietary to COMPANY and may be covered by U.S. and Foreign
 * Patents, patents in process, and are protected by trade secret or copyright law. Dissemination of this
 * information or reproduction of this material is strictly forbidden unless prior written permission is obtained
 * from COMPANY.  Access to the source code contained herein is hereby forbidden to anyone except current
 * COMPANY employees, managers or contractors who have executed Confidentiality and Non-disclosure agreements
 * explicitly covering such access.
 *
 * The copyright notice above does not evidence any actual or intended publication or
 * disclosure  of  this source code, which includes information that is confidential and/or proprietary,
 * and is a trade secret, of  COMPANY. ANY REPRODUCTION, MODIFICATION, DISTRIBUTION, PUBLIC  PERFORMANCE,
 * OR PUBLIC DISPLAY OF OR THROUGH USE  OF THIS  SOURCE CODE  WITHOUT  THE EXPRESS WRITTEN CONSENT OF
 * COMPANY IS STRICTLY PROHIBITED, AND IN VIOLATION OF APPLICABLE LAWS AND INTERNATIONAL TREATIES.  THE
 * RECEIPT OR POSSESSION OF  THIS SOURCE CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY RIGHTS
 * TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE, USE, OR SELL ANYTHING THAT IT  MAY
 * DESCRIBE, IN WHOLE OR IN PART.
 */

#pragma once

#include <QString>
#include <QList>
#include <QVariant>

class JsonQObject;
class JsonConfig;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define qsizetype int
#endif

class Node
{
public:
    struct NamedMultiValue
    {
        NamedMultiValue(const QString &key, QVariant value);
        QString key;
        QMap<int, QVariant> values;
        bool emitPending = false;
        int userTypePropertyIndex = -1;
        QMetaObject::Connection listenerConnection;
        const QVariant &value() const;
        int setValue(const QVariant &value);
        void writeValue(const QVariant &value, int level);
        void changePriority(int oldPrio, int newPrio);
    };

    QList<NamedMultiValue> properties;

    inline const QVariant &valueAt(int index) const { return properties[index].value(); }

    void setJsonObject(QJsonObject object);
    QJsonObject toJsonObject(int level) const;

    void swapJsonObject(QJsonObject oldObject, QJsonObject object, int level);
    void updateJsonObject(QJsonObject object, int level);
    bool updateProperty(int index, int level, QVariant value);
    void removeProperty(int index, int level);
    void clear();
    void unload(int level);
    void emitDeferredSignals();
    int indexOfProperty(const QString &name) const;
    int indexOfChild(const QString &name) const;
    QString fullPropertyName(const QString &property) const;
    void setConfig(JsonConfig *newConfig);
    Node *childAt(qsizetype index) const;
    QObject *object() const;
    Node *getNode(const QString &key, int *indexOfProperty);
    const QString &name() const;
    bool moveLayer(int oldPriority, int newPriority);
    void notifyPropertyUpdate(int propertyIndex);

private:
    void propertyChangedHelper(int index);
    void createObject();
    void updateObjectProperties();
    static void emitSignalHelper(QObject *object, int signalIndex);

    QObject *m_object = nullptr;
    Node *m_parent = nullptr;
    QString m_name;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    int typeHint = QMetaType::UnknownType;
#else
    QMetaType typeHint { QMetaType::UnknownType };
#endif
    JsonConfig *m_config = nullptr;
    QList<Node*> m_childNodes;
    void handleSpecialProperty(QString name, QString value);
};

