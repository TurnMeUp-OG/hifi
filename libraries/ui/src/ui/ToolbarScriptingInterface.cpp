//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ToolbarScriptingInterface.h"

#include <QtCore/QThread>
#include <QtQuick/QQuickItem>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>
#include "../OffscreenUi.h"

QScriptValue toolbarToScriptValue(QScriptEngine* engine, ToolbarProxy* const &in) {
    if (!in) {
        return engine->undefinedValue();
    }
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects);
}

void toolbarFromScriptValue(const QScriptValue& value, ToolbarProxy* &out) {
    out = qobject_cast<ToolbarProxy*>(value.toQObject());
}

QScriptValue toolbarButtonToScriptValue(QScriptEngine* engine, ToolbarButtonProxy* const &in) {
    if (!in) {
        return engine->undefinedValue();
    }
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects);
}

void toolbarButtonFromScriptValue(const QScriptValue& value, ToolbarButtonProxy* &out) {
    out = qobject_cast<ToolbarButtonProxy*>(value.toQObject());
}


ToolbarButtonProxy::ToolbarButtonProxy(QObject* qmlObject, QObject* parent) : QmlWrapper(qmlObject, parent) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    _qmlButton = qobject_cast<QQuickItem*>(qmlObject);
    connect(qmlObject, SIGNAL(clicked()), this, SIGNAL(clicked()));
}

void ToolbarButtonProxy::editProperties(const QVariantMap& properties) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "editProperties", Q_ARG(QVariantMap, properties));
        return;
    }

    QVariantMap::const_iterator iter = properties.constBegin();
    while (iter != properties.constEnd()) {
        _properties[iter.key()] = iter.value();
        if (_qmlButton) {
            // [01/25 14:26:20] [WARNING] [default] QMetaObject::invokeMethod: No such method ToolbarButton_QMLTYPE_195::changeProperty(QVariant,QVariant)
            QMetaObject::invokeMethod(_qmlButton, "changeProperty", Qt::AutoConnection,
                                        Q_ARG(QVariant, QVariant(iter.key())), Q_ARG(QVariant, iter.value()));
        }
        ++iter;
    }
}

ToolbarProxy::ToolbarProxy(QObject* qmlObject, QObject* parent) : QmlWrapper(qmlObject, parent) { 
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

ToolbarButtonProxy* ToolbarProxy::addButton(const QVariant& properties) {
    if (QThread::currentThread() != thread()) {
        ToolbarButtonProxy* result = nullptr;
        QMetaObject::invokeMethod(this, "addButton", Qt::BlockingQueuedConnection, Q_RETURN_ARG(ToolbarButtonProxy*, result), Q_ARG(QVariant, properties));
        return result;
    }

    QVariant resultVar;
    bool invokeResult = QMetaObject::invokeMethod(_qmlObject, "addButton", Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, properties));
    if (!invokeResult) {
        return nullptr;
    }

    QObject* rawButton = qvariant_cast<QObject *>(resultVar);
    if (!rawButton) {
        return nullptr;
    }

    return new ToolbarButtonProxy(rawButton, this);
}

void ToolbarProxy::removeButton(const QVariant& name) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "removeButton", Q_ARG(QVariant, name));
        return;
    }

    QMetaObject::invokeMethod(_qmlObject, "removeButton", Q_ARG(QVariant, name));
}


ToolbarProxy* ToolbarScriptingInterface::getToolbar(const QString& toolbarId) {
    if (QThread::currentThread() != thread()) {
        ToolbarProxy* result = nullptr;
        QMetaObject::invokeMethod(this, "getToolbar", Qt::BlockingQueuedConnection, Q_RETURN_ARG(ToolbarProxy*, result), Q_ARG(QString, toolbarId));
        return result;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto desktop = offscreenUi->getDesktop();
    Qt::ConnectionType connectionType = Qt::AutoConnection;
    if (QThread::currentThread() != desktop->thread()) {
        connectionType = Qt::BlockingQueuedConnection;
    }
    QVariant resultVar;
    bool invokeResult = QMetaObject::invokeMethod(desktop, "getToolbar", connectionType, Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, toolbarId));
    if (!invokeResult) {
        return nullptr;
    }

    QObject* rawToolbar = qvariant_cast<QObject *>(resultVar);
    if (!rawToolbar) {
        return nullptr;
    }

    return new ToolbarProxy(rawToolbar);
}
