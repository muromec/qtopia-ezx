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

#ifndef QPRESENCE_H
#define QPRESENCE_H

#include <qtopiaglobal.h>
#include <qcomminterface.h>
#include <qtopiaipcmarshal.h>

class QPresencePrivate;

class QTOPIAPHONE_EXPORT QPresence : public QCommInterface
{
    Q_OBJECT
    Q_ENUMS(Status)
    Q_PROPERTY(QPresence::Status localPresence READ localPresence WRITE setLocalPresence)
    Q_PROPERTY(QStringList monitoredUris READ monitoredUris)
    Q_PROPERTY(QStringList allMonitoredUris READ allMonitoredUris)
public:
    explicit QPresence( const QString& service = QString(),
                        QObject *parent = 0, QCommInterface::Mode mode = Client );
    ~QPresence();

    enum Status
    {
        Unavailable,
        Available
    };

    QPresence::Status localPresence() const;
    QStringList monitoredUris() const;
    QStringList allMonitoredUris() const;
    Q_INVOKABLE QPresence::Status monitoredUriStatus( const QString& uri ) const;

public slots:
    virtual bool startMonitoring( const QString& uri );
    virtual bool stopMonitoring( const QString& uri );
    virtual void setLocalPresence( QPresence::Status status );

signals:
    void monitoredPresence( const QString& uri, QPresence::Status status );
    void localPresenceChanged();

protected:
    void updateMonitoredPresence
            ( const QString& uri, QPresence::Status status );

private:
    QPresencePrivate *d;
};

Q_DECLARE_USER_METATYPE_ENUM(QPresence::Status)

#endif /* QPRESENCE_H */
