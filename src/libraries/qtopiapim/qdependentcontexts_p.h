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

#ifndef APPOINTMENT_BIRTHDAY_PRIVATE_H
#define APPOINTMENT_BIRTHDAY_PRIVATE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QList>
#include <QDateTime>

#include <qtopia/pim/qappointment.h>
#include <qtopiasql.h>

#include "qappointmentsqlio_p.h"

class QDependentEventsContext : public QAppointmentContext
{
    Q_OBJECT
public:
    QDependentEventsContext(QObject *parent, QObject *access);

    using QAppointmentContext::title;
    QString title(const QPimSource &) const;

    using QAppointmentContext::editable;
    bool editable() const;

    void setVisibleSources(const QSet<QPimSource> &);
    QSet<QPimSource> visibleSources() const;
    QSet<QPimSource> sources() const;

    using QAppointmentContext::exists;
    bool exists(const QUniqueId &) const;
    QPimSource source(const QUniqueId &) const;

    bool updateAppointment(const QAppointment &);
    bool removeAppointment(const QUniqueId &);
    QUniqueId addAppointment(const QAppointment &, const QPimSource &);

    bool removeOccurrence(const QUniqueId &original, const QDate &);
    QUniqueId replaceOccurrence(const QUniqueId &original, const QOccurrence &, const QDate& = QDate());
    QUniqueId replaceRemaining(const QUniqueId &original, const QAppointment &, const QDate& = QDate());

    uint mappedContext();

protected:
    QAppointmentSqlIO *mAccess;
    int mMappedContext;
};

class QContactBirthdayContext : public QDependentEventsContext
{
    Q_OBJECT
public:
    QContactBirthdayContext(QObject *parent, QObject *access);

    QIcon icon() const;
    QString description() const;
    using QDependentEventsContext::title;
    QString title() const;

    QUuid id() const;

    QPimSource defaultSource() const;
private:
    QPimSource mSource;
};

class QContactAnniversaryContext : public QDependentEventsContext
{
    Q_OBJECT
public:
    QContactAnniversaryContext(QObject *parent, QObject *access);

    QIcon icon() const;
    QString description() const;
    using QDependentEventsContext::title;
    QString title() const;

    QUuid id() const;

    QPimSource defaultSource() const;
private:
    QPimSource mSource;
};

class QTaskEventContext : public QDependentEventsContext
{
    Q_OBJECT
public:
    QTaskEventContext(QObject *parent, QObject *access);

    QIcon icon() const;
    QString description() const;
    using QDependentEventsContext::title;
    QString title() const;

    QUuid id() const;

    QPimSource defaultSource() const;
private:
    QPimSource mSource;
};

#endif
