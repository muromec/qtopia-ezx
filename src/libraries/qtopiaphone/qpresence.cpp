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

#include <qpresence.h>
#include <QMap>

Q_IMPLEMENT_USER_METATYPE_ENUM(QPresence::Status)

/*!
    \class QPresence
    \mainclass
    \preliminary
    \brief The QPresence class provides presence information for Voice-Over-IP style phone services.
    \ingroup telephony

    Clients can monitor the presence status of other users with startMonitoring().
    The monitoredPresence() signal will be emitted whenever the status of the other
    users change.  The client should call stopMonitoring() when it no longer wishes
    to monitor changes in a remote user's availability.

    Clients can also alter the local user's presence status with setLocalPresence().
    The presence change will be advertised to any remote user that is currently
    monitoring the local user.

    \sa QCommInterface
*/

/*!
    \enum QPresence::Status
    This enum defines whether a Voice-Over-IP user is present or not.

    \value Unavailable The user is not available.
    \value Available The user is available.
*/

class QPresencePrivate
{
public:
    QMap< QString, int > monitored;
};

// Fix a uri so it is safe to use as a valuespace item identifier.
static QString makeSafe( const QString& uri )
{
    QString newUri;
    newUri += QChar('p');
    for ( int posn = 0; posn < uri.length(); ++posn ) {
        uint ch = uri[posn].unicode();
        if ( ( ch >= 'A' && ch <= 'Z' ) ||
             ( ch >= 'a' && ch <= 'z' ) ||
             ( ch >= '0' && ch <= '9' ) ||
             ch == ':' || ch == '@' || ch == '.' ) {
            newUri += QChar(ch);
        } else {
            newUri += QChar('_');
            newUri += QString::number( ch );
            newUri += QChar('_');
        }
    }
    return newUri;
}

/*!
    Construct a new presence monitoring object for \a service and attach
    it to \a parent.  The object will be created in client mode if
    \a mode is Client, or server mode otherwise.
*/
QPresence::QPresence( const QString& service, QObject *parent,
                      QCommInterface::Mode mode )
    : QCommInterface( "QPresence", service, parent, mode )
{
    proxyAll( staticMetaObject );
    d = new QPresencePrivate();
}

/*!
    Destroy this presence monitoring object.  Any remaining monitor
    requests will be stopped.
*/
QPresence::~QPresence()
{
    if ( mode() == Client ) {
        // Stop monitoring on anything that was still in the list.
        QMap<QString,int>::ConstIterator it;
        for ( it = d->monitored.begin(); it != d->monitored.end(); ++it ) {
            invoke( SLOT(stopMonitoring(QString)), it.key() );
        }
    }
}

/*!
    Start monitoring \a uri for presence changes.  The monitoredPresence()
    signal will be emitted whenever the status changes.  Returns false if
    the \a uri was already being monitored by this object.

    Server implementations that inherit from QPresence should call this
    base implementation before performing any other action.

    \sa monitoredPresence(), stopMonitoring()
*/
bool QPresence::startMonitoring( const QString& uri )
{
    if ( d->monitored.contains( uri ) ) {
        ++( d->monitored[uri] );
        return false;
    } else {
        d->monitored.insert( uri, 1 );
        if ( mode() == Client )
            invoke( SLOT(startMonitoring(QString)), uri );
        else
            setValue( "allMonitoredUris", QStringList( d->monitored.keys() ) );
        return true;
    }
}

/*!
    Stop monitoring \a uri for presence changes.  Returns false if \a uri
    was not currently being monitored by this object, or there are still
    remaining references to \a uri in the system.  Monitoring will be
    implicitly stopped when this object is destroyed.

    Server implementations that inherit from QPresence should call this
    base implementation before performing any other action.

    \sa monitoredPresence(), startMonitoring()
*/
bool QPresence::stopMonitoring( const QString& uri )
{
    if ( d->monitored.contains( uri ) ) {
        if ( --( d->monitored[uri] ) == 0 ) {
            d->monitored.remove( uri );
            if ( mode() == Client ) {
                invoke( SLOT(stopMonitoring(QString)), uri );
            } else {
                setValue( "allMonitoredUris",
                          QStringList( d->monitored.keys() ), Delayed );
                removeValue( makeSafe( uri ) );
            }
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/*!
    \property QPresence::localPresence
    \brief the presence status for the local user.
*/

/*!
    Returns the presence status for the local user.

    \sa setLocalPresence(), localPresenceChanged()
*/
QPresence::Status QPresence::localPresence() const
{
    return (QPresence::Status)
        ( value( "localPresence", (int)Unavailable ).toInt() );
}

/*!
    Sets the presence \a status for the local user.

    \sa localPresence(), localPresenceChanged()
*/
void QPresence::setLocalPresence( QPresence::Status status )
{
    invoke( SLOT(setLocalPresence(QPresence::Status)),
            qVariantFromValue( status ) );
}

/*!
    \property QPresence::monitoredUris
    \brief the list of uri's that are being monitored by this client object.
*/

/*!
    Returns the list of uri's that are being monitored by this client object
    due to previous calls on startMonitoring() and stopMonitoring()

    \sa allMonitoredUris(), monitoredUriStatus()
*/
QStringList QPresence::monitoredUris() const
{
    return d->monitored.keys();
}

/*!
    \property QPresence::allMonitoredUris
    \brief the list of all uri's that are being monitored in the system, even by other applications.
*/

/*!
    Returns the list of all uri's that are being monitored in the system,
    even by other applications.

    \sa monitoredUris(), monitoredUriStatus()
*/
QStringList QPresence::allMonitoredUris() const
{
    return value( "allMonitoredUris" ).toStringList();
}

/*!
    Returns the current monitoring status associated with \a uri.

    \sa monitoredUris(), allMonitoredUris()
*/
QPresence::Status QPresence::monitoredUriStatus( const QString& uri ) const
{
    return (QPresence::Status)
        ( value( makeSafe( uri ), (int)Unavailable ).toInt() );
}

/*!
    \fn void QPresence::monitoredPresence( const QString& uri, QPresence::Status status )

    Signal that is emitted when the monitored presence \a status on \a uri
    changes.  This signal may be emitted for uri's that are being monitored
    by other applications.  The \a uri should be checked by the caller
    against the expected values from monitoredUris().

    \sa startMonitoring(), stopMonitoring()
*/

/*!
    \fn void QPresence::localPresenceChanged()

    Signal that is emitted when the localPresence() value changes.

    \sa localPresence(), setLocalPresence()
*/

/*!
    Called from server implementations to update the monitoring \a status
    of \a uri.  This function will update the global state as seen by
    monitoredUriStatus() and then emits monitoredPresence().

    \sa monitoredPresence(), startMonitoring(), stopMonitoring()
*/
void QPresence::updateMonitoredPresence
        ( const QString& uri, QPresence::Status status )
{
    if ( mode() != Server )
        return;
    if ( status == Unavailable )
        removeValue( makeSafe( uri ) );
    else
        setValue( makeSafe( uri ), (int)status );
    emit monitoredPresence( uri, status );
}
