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

#include "qappointmentio_p.h"
#include <qtopianamespace.h>

#include <qtopiaipcenvelope.h>

#ifndef Q_WS_WIN
#include <unistd.h>
#endif

QList<QAppointmentIO *> QAppointmentIO::activeAppointments;

QAppointmentIO::QAppointmentIO(QObject *parent)
    : QRecordIO(parent)
{
    QtopiaChannel *channel = new QtopiaChannel( "QPE/PIM",  this );

    connect( channel, SIGNAL(received(QString,QByteArray)),
             this, SLOT(pimMessage(QString,QByteArray)) );

    activeAppointments.append(this);
}

QAppointmentIO::~QAppointmentIO()
{
    activeAppointments.removeAll(this);
}

QVariant QAppointmentIO::appointmentField(int row, QAppointmentModel::Field k) const
{
    return QAppointmentModel::appointmentField(appointment(row), k);
}

bool QAppointmentIO::setAppointmentField(int row, QAppointmentModel::Field k,  const QVariant &v)
{
    QAppointment t = appointment(row);
    if (QAppointmentModel::setAppointmentField(t, k, v))
        return updateAppointment(t);
    return false;
}

void QAppointmentIO::checkAdded(const QUniqueId &id)
{
    Q_UNUSED(id);
    emit recordsUpdated();
}

void QAppointmentIO::checkRemoved(const QUniqueId &id)
{
    Q_UNUSED(id);
    emit recordsUpdated();
}

void QAppointmentIO::checkRemoved(const QList<QUniqueId> &ids)
{
    Q_UNUSED(ids);
    emit recordsUpdated();
}

void QAppointmentIO::checkUpdated(const QUniqueId &id)
{
    Q_UNUSED(id);
    emit recordsUpdated();
}


void QAppointmentIO::notifyAdded(const QUniqueId &id)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "addedAppointment(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }

    foreach(QAppointmentIO *c, activeAppointments) {
        if (c != this && c->contextId() == contextId())
            c->checkAdded(id);
    }
}

void QAppointmentIO::notifyUpdated(const QUniqueId &id)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "updatedAppointment(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }

    foreach(QAppointmentIO *c, activeAppointments) {
        if (c != this && c->contextId() == contextId())
            c->checkUpdated(id);
    }
}

void QAppointmentIO::notifyRemoved(const QUniqueId &id)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "removedAppointment(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }

    foreach(QAppointmentIO *c, activeAppointments) {
        if (c != this && c->contextId() == contextId())
            c->checkRemoved(id);
    }
}

void QAppointmentIO::notifyRemoved(const QList<QUniqueId> &ids)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "removedAppointments(int,QUuid,QList<QUniqueId>)");
        e << getpid();
        e << contextId();
        e << ids;
    }

    foreach(QAppointmentIO *c, activeAppointments) {
        if (c != this && c->contextId() == contextId())
            c->checkRemoved(ids);
    }
}

void QAppointmentIO::ensureDataCurrent(bool force)
{
    Q_UNUSED(force);
    emit recordsUpdated();
}

QDateTime QAppointmentIO::nextAlarm( const QAppointment &appointment)
{
    QDateTime now = QDateTime::currentDateTime();
    // -1 days to make room for timezone shift.
    QOccurrence o = appointment.nextOccurrence(now.date().addDays(-1));
    while (o.isValid()) {
        if (now <= o.alarmInCurrentTZ())
            return o.alarmInCurrentTZ();
        o = o.nextOccurrence();
    }
    return QDateTime();
}

// alarm functions
void QAppointmentIO::removeAlarm(const QAppointment &appointment)
{
#ifdef Q_WS_QWS
    // TODO Needs to be able to set up the return to be a service.
    QDateTime when = nextAlarm(appointment);
    if (!when.isNull())
        Qtopia::deleteAlarm(when, "Calendar", "alarm(QDateTime,int)", appointment.alarmDelay());
#else
    Q_UNUSED( appointment );
#endif
}

void QAppointmentIO::addAlarm(const QAppointment &appointment)
{
#ifdef Q_WS_QWS
    // TODO Needs to be able to set up the return to be a service.
    QDateTime when = nextAlarm(appointment);
    if (!when.isNull())
        Qtopia::addAlarm(when, "Calendar", "alarm(QDateTime,int)", appointment.alarmDelay());
#else
    Q_UNUSED( appointment );
#endif
}

void QAppointmentIO::pimMessage(const QString &message, const QByteArray &data)
{
    QDataStream ds(data);
    if (message == "addedAppointment(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId appointment;
            ds >> appointment;
            checkAdded(appointment);
        }
    } else if (message == "removedAppointment(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId id;
            ds >> id;
            checkRemoved(id);
        }
    } else if (message == "removedAppointments(int,QUuid,QList<QUniqueId>)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QList<QUniqueId> ids;
            ds >> ids;
            checkRemoved(ids);
        }
    } else if (message == "updatedAppointment(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId appointment;
            ds >> appointment;
            checkUpdated(appointment);
        }
    } else if (message == "reloadAppointments()") {
        ensureDataCurrent();
    } else if (message == "reload(int)") {
        int force;
        ds >> force;
        ensureDataCurrent(force != 0);
    }
}
