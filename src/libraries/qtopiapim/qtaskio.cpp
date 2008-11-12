/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#include "qtaskio_p.h"
#include <QSettings>
#include <QDebug>
#include <qglobal.h>

#include <qtranslatablesettings.h>
#include <qtopianamespace.h>

#ifdef Q_WS_QWS
#include <qtopiaipcenvelope.h>
#endif
#include <qfile.h>

#ifdef Q_OS_WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

void QTaskIO::checkAdded(const QUniqueId &) { emit recordsUpdated(); }
void QTaskIO::checkRemoved(const QUniqueId &) { emit recordsUpdated(); }
void QTaskIO::checkRemoved(const QList<QUniqueId> &) { emit recordsUpdated(); }
void QTaskIO::checkUpdated(const QUniqueId &) { emit recordsUpdated(); }

QList<QTaskIO *> QTaskIO::activeTasks;

QTaskIO::QTaskIO(QObject *parent)
    : QRecordIO(parent)
{
    // default format.
    // should make a qstringlist with tr's?
    //setFormat("firstname , _ lastname | lastname | firstname | company");
#ifdef Q_WS_QWS
    QtopiaChannel *channel = new QtopiaChannel( "QPE/PIM",  this );

    connect( channel, SIGNAL(received(QString,QByteArray)),
            this, SLOT(pimMessage(QString,QByteArray)) );
#endif
    activeTasks.append(this);
}

QTaskIO::~QTaskIO()
{
    activeTasks.removeAll(this);
}
void QTaskIO::pimMessage(const QString &message, const QByteArray &data)
{
    QDataStream ds(data);
    if (message == "addedTask(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId task;
            ds >> task;
            checkAdded(task);
        }
    } else if (message == "removedTask(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId id;
            ds >> id;
            checkRemoved(id);
        }
    } else if (message == "removedTasks(int,QUuid,QList<QUniqueId>)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QList<QUniqueId> ids;
            ds >> ids;
            checkRemoved(ids);
        }
    } else if (message == "updatedTask(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId task;
            ds >> task;
            checkUpdated(task);
        }
    } else if (message == "reloadTasks()") {
        ensureDataCurrent();
    } else if (message == "reload(int)") {
        int force;
        ds >> force;
        ensureDataCurrent(force != 0);
    }
}
// assumes storage is row based.
// override if more efficient to grap partial task.
QVariant QTaskIO::taskField(int row, QTaskModel::Field k) const
{
    QTask t = task(row);
    switch(k) {
        default:
        case QTaskModel::Invalid:
            break;
        case QTaskModel::Identifier:
            return t.uid().toByteArray();
        case QTaskModel::Categories:
            return QVariant(t.categories());
        case QTaskModel::Description:
            return t.description();
        case QTaskModel::Priority:
            return t.priority();
        case QTaskModel::Completed:
            return t.isCompleted();
        case QTaskModel::PercentCompleted:
            return t.percentCompleted();
        case QTaskModel::Status:
            return t.status();
        case QTaskModel::DueDate:
            return t.dueDate();
        case QTaskModel::StartedDate:
            return t.startedDate();
        case QTaskModel::CompletedDate:
            return t.completedDate();
        case QTaskModel::Notes:
            return t.notes();
    }
    return QVariant();
}

bool QTaskIO::setTaskField(int row, QTaskModel::Field k,  const QVariant &v)
{
    QTask t = task(row);
    switch(k) {
        default:
        case QTaskModel::Invalid:
            break;
        case QTaskModel::Categories:
            if (v.canConvert(QVariant::StringList)) {
                t.setCategories(v.toStringList());
                return updateTask(t);
            }
            return false;
        case QTaskModel::Description:
            if (v.canConvert(QVariant::String)) {
                t.setDescription(v.toString());
                return updateTask(t);
            }
            return false;
        case QTaskModel::Priority:
            if (v.canConvert(QVariant::Int)) {
                t.setPriority(v.toInt());
                return updateTask(t);
            }
            return false;
        case QTaskModel::Completed:
            if (v.canConvert(QVariant::Bool)) {
                t.setCompleted(v.toBool());
                return updateTask(t);
            }
            return false;
        case QTaskModel::PercentCompleted:
            if (v.canConvert(QVariant::Int)) {
                t.setPercentCompleted(v.toInt());
                return updateTask(t);
            }
            return false;
        case QTaskModel::Status:
            if (v.canConvert(QVariant::Int)) {
                t.setStatus(v.toInt());
                return updateTask(t);
            }
            return false;
        case QTaskModel::DueDate:
            if (v.canConvert(QVariant::Date)) {
                t.setDueDate(v.toDate());
                return updateTask(t);
            }
            return false;
        case QTaskModel::StartedDate:
            if (v.canConvert(QVariant::Date)) {
                t.setStartedDate(v.toDate());
                return updateTask(t);
            }
            return false;
        case QTaskModel::CompletedDate:
            if (v.canConvert(QVariant::Date)) {
                t.setCompletedDate(v.toDate());
                return updateTask(t);
            }
            return false;
        case QTaskModel::Notes:
            if (v.canConvert(QVariant::String)) {
                t.setNotes(v.toString());
                return updateTask(t);
            }
            return false;
    }
    return false;
}

/*!
  \internal

  Ensures state of store is consistent with what is on permanent storage.  if \a force is true
  forces a reload of cache even if checks indicates is up to date.
*/
void QTaskIO::ensureDataCurrent(bool force)
{
    Q_UNUSED(force);
    emit recordsUpdated();
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has added the
  task with identifier \a id to the data store.
*/
void QTaskIO::notifyAdded(const QUniqueId &id)
{
#ifndef QT_NO_COP
    {
        QtopiaIpcEnvelope e("QPE/PIM", "addedTask(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }
#endif
    foreach(QTaskIO *c, activeTasks) {
        if (c != this && c->contextId() == contextId())
            c->checkAdded(id);
    }
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has updated the
  task with identifier \a id in the data store.
*/
void QTaskIO::notifyUpdated(const QUniqueId &id)
{
#ifndef QT_NO_COP
    {
        QtopiaIpcEnvelope e("QPE/PIM", "updatedTask(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }
#endif
    foreach(QTaskIO *c, activeTasks) {
        if (c != this && c->contextId() == contextId())
            c->checkUpdated(id);
    }
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has removed the
  task for \a id from the data store.
*/
void QTaskIO::notifyRemoved(const QUniqueId & id)
{
#ifndef QT_NO_COP
    {
        QtopiaIpcEnvelope e("QPE/PIM", "removedTask(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }
#endif
    foreach(QTaskIO *c, activeTasks) {
        if (c != this && c->contextId() == contextId())
            c->checkRemoved(id);
    }
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has removed the
  tasks for \a ids from the data store.
*/
void QTaskIO::notifyRemoved(const QList<QUniqueId> &ids)
{
#ifndef QT_NO_COP
    {
        QtopiaIpcEnvelope e("QPE/PIM", "removedTasks(int,QUuid,QList<QUniqueId>)");
        e << getpid();
        e << contextId();
        e << ids;
    }
#endif
    foreach(QTaskIO *c, activeTasks) {
        if (c != this && c->contextId() == contextId())
            c->checkRemoved(ids);
    }
}
