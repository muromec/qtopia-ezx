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

#include "voipmanager.h"
#include <qvaluespace.h>

#include <QSqlQuery>
#include <QTimer>
#include <QContact>

class VoIPManagerPrivate {
public:
    VoIPManagerPrivate()
    : netReg(0),
    presence(0),
    userPreferredPresence(QPresence::Available),
    serviceManager(0),
    status(0),
    voipHideMsgTimer(0),
    voipHideMsg(false)
    {}

    QNetworkRegistration *netReg;
    QPresence *presence;
    QPresence::Status userPreferredPresence;
    QCommServiceManager *serviceManager;
    QValueSpaceObject *status;
    QTimer *voipHideMsgTimer;
    bool voipHideMsg;
};

/*!
    \class VoIPManager
    \ingroup QtopiaServer
    \brief The VoIPManager class maintains information about the active VoIP telephony service.

    This class provides access to some of the common facilities of the \c{voip} telephony
    service, if it is running within the system, to ease the implementation of VoIP-specific
    user interfaces in the server.  The telephony service itself is started by PhoneServer
    at system start up.

    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
    \sa PhoneServer
*/

/*!
    Returns the VoIPManager instance.
*/
VoIPManager * VoIPManager::instance()
{
    // TODO: replace calls to VoIPManager::instance() with
    // direct calls to qtopiaTask<VoIPManager>() and then
    // remove this function.
    return qtopiaTask<VoIPManager>();
}

/*!
    \reimp
*/
QString VoIPManager::callType() const
{
    return "VoIP";      // No tr
}

/*!
    \reimp
*/
QString VoIPManager::trCallType() const
{
    return tr("VoIP");
}

/*!
    \reimp
*/
QString VoIPManager::callTypeIcon() const
{
    return "sipsettings/SIP";   // No tr
}

/*!
    \reimp
*/
QAbstractCallPolicyManager::CallHandling VoIPManager::handling
        (const QString& number)
{
    // If no network registration, then cannot use this to dial.
    if ( registrationState() != QTelephony::RegistrationHome )
        return CannotHandle;

    // If at least one '@' sign, then assume that it is a VoIP URI.
    if ( number.contains(QChar('@')) )
        return CanHandle;

    // Probably an ordinary number, which may be gatewayable via VoIP.
    // TODO: may want to user to be able to choose to turn this on/off.
    return CanHandle;
}

/*!
    \reimp
*/
QString VoIPManager::registrationMessage() const
{
    // Don't display anything if we do not have a VoIP service active yet.
    if ( !d->netReg )
        return QString();

    QString voipMsg;
    QTelephony::RegistrationState v = registrationState();

    switch (v) {
    case QTelephony::RegistrationNone:
        if(!d->voipHideMsg)
            voipMsg = tr("No VoIP network");
        break;
    case QTelephony::RegistrationHome:
    case QTelephony::RegistrationUnknown:
    case QTelephony::RegistrationRoaming:
        ((VoIPManager *)this)->startMonitoring();
        break;
    case QTelephony::RegistrationSearching:
        voipMsg = tr("Searching VoIP network");
        break;
    case QTelephony::RegistrationDenied:
        voipMsg += tr("VoIP Authentication Failed");
        break;
    }

    // If no registration, we want to hide the message after
    // some time, because the VoIP service may not be configured.
    if(v == QTelephony::RegistrationNone) {
        if(!d->voipHideMsg && !d->voipHideMsgTimer->isActive())
            d->voipHideMsgTimer->start(7000);
    } else {
        d->voipHideMsgTimer->stop();
        d->voipHideMsg = false;
    }

    return voipMsg;
}

/*!
    \reimp
*/
QString VoIPManager::registrationIcon() const
{
    // No specific icon used for VoIP registration messages.
    return QString();
}

/*!
    \reimp
*/
QTelephony::RegistrationState VoIPManager::registrationState() const
{
    if ( d->netReg )
        return d->netReg->registrationState();
    else
        return QTelephony::RegistrationNone;
}

/*!
    Returns the presence status of the local user.

    \sa localPresenceChanged()
*/
QPresence::Status VoIPManager::localPresence() const
{
    if ( d->presence )
        return d->presence->localPresence();
    else
        return QPresence::Unavailable;
}

/*!
    \fn void VoIPManager::localPresenceChanged(QPresence::Status status)

    Signal that is emitted when localPresence() changes to \a status.

    \sa localPresence()
*/

/*!
    Returns true if the VoIP user associated with \a uri is available for calling;
    false otherwise.

    \sa monitoredPresenceChanged(), startMonitoring()
*/
bool VoIPManager::isAvailable( const QString &uri )
{
    if ( d->presence ) {
        // If we were monitoring this uri because it was in the user's
        // contacts, then check to see if they are available.  If the
        // uri is not in the user's contacts, then assume that it is
        // available and let the dialing sequence fail later if not.
        if ( d->presence->monitoredUris().contains( uri ) )
            return d->presence->monitoredUriStatus( uri ) == QPresence::Available;
        else
            return true;
    } else {
        // The VoIP stack does not support presence, so just assume it is available.
        return true;
    }
}

/*!
    \fn void VoIPManager::monitoredPresenceChanged(const QString& uri, bool available)

    Signal that is emitted when the presence of the user identified by \a uri
    changes to \a available.

    \sa isAvailable(), startMonitoring()
*/

/*!
    Create a new VoIP telephony service manager and attach it to \a parent.
*/
VoIPManager::VoIPManager(QObject *parent)
    : QAbstractCallPolicyManager(parent)
{
    d = new VoIPManagerPrivate;
    d->status = new QValueSpaceObject("/Telephony/Status/VoIP", this);
    d->voipHideMsgTimer = new QTimer( this );
    d->voipHideMsgTimer->setSingleShot( true );
    connect( d->voipHideMsgTimer, SIGNAL(timeout()),
             this, SLOT(hideMessageTimeout()) );
#ifdef QTOPIA_VOIP
    // The "voip" telephony handler may not have started yet.
    // Hook onto QCommServiceManager to watch for it.
    d->serviceManager = new QCommServiceManager( this );
    connect( d->serviceManager, SIGNAL(servicesChanged()),
             this, SLOT(servicesChanged()) );

    // Just in case it has already started.
    servicesChanged();
#endif
}

void VoIPManager::registrationStateChanged()
{
    if ( d->netReg ) {
        emit registrationChanged( d->netReg->registrationState() );
        d->status->setAttribute("Registered", d->netReg->registrationState() == QTelephony::RegistrationHome);

        if ( d->presence ) {
            d->presence->setLocalPresence(
                    d->netReg->registrationState() == QTelephony::RegistrationHome ?
                    d->userPreferredPresence : QPresence::Unavailable );
        }
    } else {
        emit registrationChanged( QTelephony::RegistrationNone );
        d->status->setAttribute("Registered", false);
    }
}

void VoIPManager::localPresenceChanged()
{
    if ( d->presence ) {
        emit localPresenceChanged( d->presence->localPresence() );
        // cache the preferred presence and use this presence when registered to network again after unregistering.
        d->userPreferredPresence = d->presence->localPresence();
        d->status->setAttribute("Presence", (d->presence->localPresence() == QPresence::Available)?"Available":"Unavailable");
        d->status->setAttribute("Present", d->presence->localPresence() == QPresence::Available);
    } else {
        emit localPresenceChanged( QPresence::Unavailable );
        d->status->setAttribute("Presence", "Unavailable");
        d->status->setAttribute("Present", false);
    }
}

void VoIPManager::monitoredPresence( const QString &uri, QPresence::Status status )
{
    emit monitoredPresenceChanged( uri, status == QPresence::Available );
}

void VoIPManager::servicesChanged()
{
#ifdef QTOPIA_VOIP
    if ( !d->netReg ) {
        if ( d->serviceManager->interfaces( "voip" )       // No tr
                    .contains( "QNetworkRegistration" ) ) {
            serviceStarted();
        }
    } else {
        if ( !d->serviceManager->interfaces( "voip" )      // No tr
                    .contains( "QNetworkRegistration" ) ) {
            serviceStopped();
        }
    }
#endif
}

void VoIPManager::serviceStarted()
{
#ifdef QTOPIA_VOIP
    // The "voip" handler has started up, so attach to the service.
    d->netReg = new QNetworkRegistration( "voip", this ); // No tr
    if ( !d->netReg->available() ) {
        delete d->netReg;
        d->netReg = 0;
    } else {
        connect( d->netReg, SIGNAL(registrationStateChanged()),
                 this, SLOT(registrationStateChanged()) );
        registrationStateChanged();
    }
    d->presence = new QPresence( "voip", this );
    if ( !d->presence->available() ) {
        delete d->presence;
        d->presence = 0;
    } else {
        connect( d->presence, SIGNAL(localPresenceChanged()),
                this, SLOT(localPresenceChanged()) );
        connect( d->presence, SIGNAL(monitoredPresence(QString,QPresence::Status)),
                this, SLOT(monitoredPresence(QString,QPresence::Status)) );
    }
#endif
}

void VoIPManager::serviceStopped()
{
#ifdef QTOPIA_VOIP
    // The "voip" handler has shut down, so detach from the service.
    delete d->netReg;
    d->netReg = 0;
    registrationStateChanged();
    delete d->presence;
    d->presence = 0;
    localPresenceChanged();
#endif
}

/*!
    Start monitoring all contacts in the user's contact list that have
    associated VoIP identities.  The monitoredPresenceChanged() signal
    will be emitted whenever the presence information for a monitored
    contact changes.

    \sa isAvailable(), monitoredPresenceChanged()
*/
void VoIPManager::startMonitoring()
{
#ifdef QTOPIA_VOIP
    QSqlQuery q;
    QList<QContact::PhoneType> voipList;
    voipList << QContact::VOIP << QContact::HomeVOIP << QContact::BusinessVOIP; // XXX this needs the phone type api
    QString query(QLatin1String("SELECT phone_number from contactphonenumbers WHERE phone_type IN ("));
    int typecount = voipList.count();
    foreach(QContact::PhoneType p, voipList) {
        query.append(QString::number((int)p));
        if (--typecount > 0)
            query.append(',');
    }
    query.append(')');
    q.prepare(query);
    q.exec();
    while(q.next())
        d->presence->startMonitoring( q.value(0).toString() );
#endif
}

void VoIPManager::hideMessageTimeout()
{
    // Mark the message as hidden and then fake a registration state change.
    d->voipHideMsg = true;
    emit registrationChanged( registrationState() );
}

QTOPIA_TASK(VoIP, VoIPManager);
QTOPIA_TASK_PROVIDES(VoIP, VoIPManager);
QTOPIA_TASK_PROVIDES(VoIP, QAbstractCallPolicyManager);
