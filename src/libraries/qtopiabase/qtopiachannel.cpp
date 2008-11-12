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

#include <qtopiachannel.h>
#include <QObject>

#if !defined(QTOPIA_HOST) && !defined(QTOPIA_DBUS_IPC)
#if defined(Q_WS_QWS)
#include <qcopchannel_qws.h>
#define QTOPIA_REGULAR_QCOP
#elif defined(Q_WS_X11)
#include <qcopchannel_x11.h>
#define QTOPIA_REGULAR_QCOP
#endif
#endif

#if defined(QTOPIA_DBUS_IPC)
#include <qtdbus/qdbusconnection.h>
#include <qtdbus/qdbusmessage.h>
#elif defined(QTOPIA_REGULAR_QCOP)
#include <quuid.h>

// Maximum size of a QCop message before it must be broken up.
#define MAX_FRAGMENT_SIZE       4096
#endif

#include <qdebug.h>

#include <QString>
#include <QTimer>

#if defined(QTOPIA_DBUS_IPC)
#include "dbusipccommon_p.h"
#endif

class QtopiaChannel_Private
#if !defined(QTOPIA_DBUS_IPC) && defined(QTOPIA_REGULAR_QCOP)
    : public QCopChannel
#else
    : public QObject
#endif
{
    Q_OBJECT

public:
    QtopiaChannel_Private(const QString &channel, QtopiaChannel *parent);
    ~QtopiaChannel_Private();
#if defined(QTOPIA_DBUS_IPC)
    QString m_channelName;

    static bool dbusSend(const QString &channel,
                         const QString &msg,
                         const QByteArray &data);
#elif defined(QTOPIA_REGULAR_QCOP)
    QTimer *m_cleanupTimer;

    void receive(const QString& msg, const QByteArray &data);

    struct Fragment
    {
        QString uuid;
        QByteArray data;
        Fragment *next;
    };

    Fragment *m_fragments;
#endif
    QtopiaChannel *m_parent;

#if defined(QTOPIA_DBUS_IPC)
private slots:
    void handleSignalReceived(const QByteArray &arr, const QDBusMessage &msg);
#elif defined(QTOPIA_REGULAR_QCOP)
private slots:
    void cleanup();
#endif

};

QtopiaChannel_Private::QtopiaChannel_Private(const QString &channel, QtopiaChannel *parent) :
#if !defined(QTOPIA_DBUS_IPC) && defined(QTOPIA_REGULAR_QCOP)
        QCopChannel(channel, parent), m_fragments(0),
#endif
        m_parent(parent)
{
#if defined(QTOPIA_DBUS_IPC)
    m_channelName = channel;

    QDBusConnection dbc = QDBus::sessionBus();

    if (!dbc.isConnected())
        qFatal("Application: %s Connection to DBUS Daemon lost!!",
               qApp->applicationName().toLatin1().constData());

    QString dbusPath;
    convert_qcop_channel_to_dbus_path(channel, dbusPath);

    bool r = dbc.connect(QString(),                          // Service
                         dbusPath,                           // Path
                         dbusInterface,                      // Interface
                         QString(),                          // Name
                         this,
                         SLOT(handleSignalReceived(QByteArray,QDBusMessage)));

    if (!r)
        qFatal("dbc.connect failed!!");

#elif defined(QTOPIA_REGULAR_QCOP)
    m_cleanupTimer = new QTimer();
    m_cleanupTimer->setSingleShot(true);
    connect(m_cleanupTimer, SIGNAL(timeout()), this, SLOT(cleanup()) );
#else
    Q_UNUSED(channel);
    Q_UNUSED(parent);
#endif
}

QtopiaChannel_Private::~QtopiaChannel_Private()
{
#if !defined(QTOPIA_DBUS_IPC) && defined(QTOPIA_REGULAR_QCOP)
    if (m_cleanupTimer)
        delete m_cleanupTimer;
    cleanup();
#endif
}

#if defined(QTOPIA_DBUS_IPC)
void QtopiaChannel_Private::handleSignalReceived(const QByteArray &arr, const QDBusMessage &msg)
{
    QString message;
    convert_dbus_to_qcop_message_name(msg.member(), message);
    emit m_parent->received(message, arr);
}

bool QtopiaChannel_Private::dbusSend(const QString &channel,
                                     const QString &msg,
                                     const QByteArray &data)
{
    QDBusConnection dbc = QDBus::sessionBus();
    QString dbusPath;

    QDBusMessage message;

    // Handle this specially
    if (channel.startsWith("QPE/Application/")) {
        const int pref=16;
        QString app = channel.mid(pref);

        dbusPath = QString("/com/trolltech/qtopia/QPE/Application/");
        dbusPath += app;

        QString dbusService("com.trolltech.qtopia.QPE.Application.");
        dbusService += app;

        message = QDBusMessage::methodCall(dbusService, dbusPath, dbusInterface,
                                           "appMessage", dbc);
        message << msg;
    }
    else {
        QString dbusMsg;
        convert_qcop_message_name_to_dbus(msg, dbusMsg);
        convert_qcop_channel_to_dbus_path(channel, dbusPath);
        message = QDBusMessage::signal(dbusPath, dbusInterface, dbusMsg, dbc);
    }

    if (!data.isNull()) {
        message << data;
    } else {
        QByteArray arr("");
        message << arr;
    }

    bool ret = dbc.send(message);
    if (!ret)
        qWarning("Unable to send message: %s", dbc.lastError().message().toAscii().constData());

    return ret;
}
#endif

/*!
    \class QtopiaChannel
    \mainclass
    \ingroup ipc

    \brief The QtopiaChannel class provides communication capabilities
    between clients.

    QtopiaChannel provides a many-to-many (broadcast) inter-process
    communication protocol.  The messages transferred are identified
    by channel, message and message signature.  A channel is
    identified by a name, and anyone who wants to can listen to it,
    restricted by an active security framework, if any.
    The QtopiaChannel protocol allows clients to communicate both
    within the same address space and between different processes.

    There are currently two implementations for QtopiaChannel,
    one layered over the Qt QCop mechanism, and another one for DBus.
    QCop mechanism is currently only available on \l {Qtopia Core}.

    Typically, QtopiaChannel is either used to send messages to a
    channel using the provided static functions, or to listen to the
    traffic on a channel by deriving from the class to take advantage
    of the provided functionality for receiving messages.

    QtopiaChannel provides a couple of static functions which are usable
    without an object: The send() function, which sends the given
    message and data on the specified channel, and the isRegistered()
    function which queries the server for the existence of the given
    channel.

    In addition, the QtopiaChannel class provides the channel() function
    which returns the name of the object's channel, and the received()
    signal which is emitted with the given message and data when a
    QtopiaChannel receives a message from its channel.

    \sa QtopiaAbstractService
*/

/*!
    Constructs a QtopiaChannel with the given \a parent, and starts
    to listen on \a channel name.

    \sa isRegistered(), channel()
 */
QtopiaChannel::QtopiaChannel(const QString &channel, QObject *parent) :
        QObject(parent)
{
#ifndef QTOPIA_TEST     // Allow all channels to be hijacked for testing.
    if (channel.startsWith("QPE/Application")) {
        qWarning("QtopiaChannel does not support receiving on QPE/Application channels.");
        return;
    }
#endif
    m_data = new QtopiaChannel_Private(channel, this);
}

/*!
    Destroys this QtopiaChannel instance.
*/
QtopiaChannel::~QtopiaChannel()
{
    if (m_data) {
        delete m_data;
        m_data = 0;
    }
}

/*!
    Returns the name of the channel this object is listening on.
*/
QString QtopiaChannel::channel() const
{
#if defined(QTOPIA_DBUS_IPC)
    return m_data->m_channelName;
#elif defined(QTOPIA_REGULAR_QCOP)
    return m_data->channel();
#else
    return QString();
#endif
}

/*!
    Returns true if the \a channel is registered.  Note that this function
    always returns true in the DBus implementation.  In the QCop implementation,
    this function requires a round-trip to the QWS server, which may impact
    system performance.
*/
bool QtopiaChannel::isRegistered(const QString &channel)
{
#if defined(QTOPIA_DBUS_IPC)
    //TODO: There's no registration framework in DBUS for signals
    //      will need to add this later on
    //      For now just return whether we're connected
    QDBusConnection dbc = QDBus::sessionBus();
    return dbc.isConnected();
#elif defined(QTOPIA_REGULAR_QCOP)
    return QCopChannel::isRegistered(channel);
#else
    Q_UNUSED(channel);
    return false;
#endif
}

/*!
    Sends a message \a msg with no parameters on \a channel.
    Returns true if able to send the message; otherwise returns false.

    \sa received()
*/
bool QtopiaChannel::send(const QString &channel, const QString &msg)
{
#if defined(QTOPIA_DBUS_IPC)
    return QtopiaChannel_Private::dbusSend(channel, msg, QByteArray(""));
#elif defined(QTOPIA_REGULAR_QCOP)
    return QCopChannel::send(channel, msg);
#else
    Q_UNUSED(channel);
    Q_UNUSED(msg);
    return false;
#endif
}

/*!
    Sends a message \a msg on channel \a channel with the parameter
    data given by \a data.  Returns true if able to send the message;
    otherwise returns false.

    \sa received()
*/
bool QtopiaChannel::send(const QString &channel, const QString &msg,
                         const QByteArray &data)
{
#if defined(QTOPIA_DBUS_IPC)
    return QtopiaChannel_Private::dbusSend(channel, msg, data);
#elif defined(QTOPIA_REGULAR_QCOP)

    // Send the message as-is if it is smaller than the fragment size.
    if ( data.size() <= MAX_FRAGMENT_SIZE )
        return QCopChannel::send(channel, msg, data);

    // Compose the individual fragments and send them.
    QString uuid = QUuid::createUuid().toString();
    for ( int posn = 0; posn < data.size(); posn += MAX_FRAGMENT_SIZE ) {
        QByteArray fragment;
        if ( ( posn + MAX_FRAGMENT_SIZE ) <= data.size() ) {
            fragment = data.mid( posn, MAX_FRAGMENT_SIZE );
        } else {
            fragment = data.mid( posn );
        }
        QByteArray newData;
        {
            QDataStream stream
                ( &newData, QIODevice::WriteOnly | QIODevice::Append );
            stream << uuid;
            stream << posn;
            stream << data.size();
            stream << fragment;
        }
        if ( !QCopChannel::send( channel, msg + "_fragment_", newData ) ) {
            return false;
        }
    }
    return true;

#else
    Q_UNUSED(channel);
    Q_UNUSED(msg);
    Q_UNUSED(data);
    return false;
#endif
}

/*!
    Flushes all pending messages destined from this channel, causing them to be
    sent immediately.  Returns true if the flush succeeded; otherwise returns false.
*/
bool QtopiaChannel::flush()
{
#if defined(QTOPIA_DBUS_IPC)
    qWarning("QtopiaChannel::flush() - Not implemented for DBus");
    return true;
#elif defined(QTOPIA_REGULAR_QCOP)
    return QCopChannel::flush();
#else
    return false;
#endif
}

#if !defined(QTOPIA_DBUS_IPC) && defined(QTOPIA_REGULAR_QCOP)

void QtopiaChannel_Private::receive(const QString& msg, const QByteArray &data)
{
    // If this is not a fragmented message, then pass it on as-is.
    if ( !msg.endsWith( "_fragment_" ) ) {
        emit m_parent->received( msg, data );
        return;
    }

    // Pull apart the fragment into its components.
    QDataStream stream( data );
    QString uuid;
    int posn, size;
    QByteArray fragData;
    stream >> uuid;
    stream >> posn;
    stream >> size;
    stream >> fragData;

    // Find the existing fragment information.  For now, we assume that
    // the fragments arrive in the same order in which they were sent.
    Fragment *frag;
    Fragment *prev;
    if ( !posn ) {
        // First fragment in a new message.
        frag = new Fragment();
        frag->uuid = uuid;
        frag->data = fragData;
        frag->next = m_fragments;
        m_fragments = frag;
        prev = 0;
    } else {
        frag = m_fragments;
        prev = 0;
        while ( frag != 0 && frag->uuid != uuid ) {
            prev = frag;
            frag = frag->next;
        }
        if ( !frag || frag->data.size() != posn ) {
            return;
        }
        frag->data += fragData;
    }

    // Determine if we have collected up everything.
    if ( frag->data.size() >= size ) {
        if ( prev ) {
            prev->next = frag->next;
        } else {
            m_fragments = frag->next;
            if ( !m_fragments )
                m_cleanupTimer->stop();
        }
        emit m_parent->received( msg.left( msg.length() - 10 ), frag->data );
        delete frag;
    } else {
        // Delete fragments that are still hanging around after 20 seconds.
        m_cleanupTimer->start( 20000 );
    }
}

/*!
    \fn void QtopiaChannel::received(const QString& message, const QByteArray &data)

    This signal is emitted with the given \a message and \a data whenever the
    channel receives a message on the channel being listened on.

    \sa send()
 */

void QtopiaChannel_Private::cleanup()
{
    Fragment *frag = m_fragments;
    Fragment *next;
    while ( frag != 0 ) {
        next = frag->next;
        delete frag;
        frag = next;
    }
    m_fragments = 0;
}

#endif

#include "qtopiachannel.moc"
