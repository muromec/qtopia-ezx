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

#include "qbluetoothabstractservice.h"

#include <qbluetoothaddress.h>
#include <qbluetoothsdprecord.h>
#include <qbluetoothlocaldevice.h>
#include <qtopiacomm/private/qsdpxmlgenerator_p.h>
#include <qtopiaipcadaptor.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include <QTimer>
#include <QFile>
#include <QBuffer>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


/*
    This class passes messages (over IPC) between a QBluetoothAbstractService 
    and the BluetoothServiceManager (in the QPE server).
 */
class Messenger : public QtopiaIpcAdaptor
{
    friend class QBluetoothAbstractService;
    Q_OBJECT
public:
    Messenger(QBluetoothAbstractService *service);

    QBluetoothAbstractService *m_service;

public slots:
    void startService(const QString &name);
    void stopService(const QString &name);
    void setSecurityOptions(const QString &name, QBluetooth::SecurityOptions options);

private slots:
    void registerSelf();
    void started(bool error, const QString &);
    void stopped();

signals:
    void registerService(const QString &name, const QString &translatableName);
    void serviceStarted(const QString &name, bool error, const QString &errorDesc);
    void serviceStopped(const QString &name);
};

Messenger::Messenger(QBluetoothAbstractService *service)
    : QtopiaIpcAdaptor("QPE/BluetoothServiceProviders", service),
      m_service(service)
{
    publishAll(SignalsAndSlots);

    QObject::connect(service, SIGNAL(started(bool,QString)),
                     SLOT(started(bool,QString)));
    QObject::connect(service, SIGNAL(stopped()), SLOT(stopped()));

    // register the service at the end of this run loop
    QTimer::singleShot(0, this, SLOT(registerSelf()));
}

void Messenger::startService(const QString &name)
{
    if (name == m_service->name())
        m_service->start();
}

void Messenger::stopService(const QString &name)
{
    if (name == m_service->name())
        m_service->stop();
}

void Messenger::setSecurityOptions(const QString &name, QBluetooth::SecurityOptions options)
{
    if (name == m_service->name())
        m_service->setSecurityOptions(options);
}

void Messenger::registerSelf()
{
    emit registerService(m_service->name(), m_service->displayName());
}

void Messenger::started(bool error, const QString &desc)
{
    emit serviceStarted(m_service->name(), error, desc);
}

void Messenger::stopped()
{
    emit serviceStopped(m_service->name());
}


// ========================================

class QBluetoothAbstractServicePrivate : public QObject
{
    Q_OBJECT

public:
    QBluetoothAbstractServicePrivate(const QString &name,
                                     const QString &displayName,
                                     QBluetoothAbstractService *parent);

    quint32 registerRecord(const QString &record);
    bool unregisterRecord(quint32 handle);

    QString m_name;
    QString m_displayName;
};

QBluetoothAbstractServicePrivate::QBluetoothAbstractServicePrivate(const QString &name, const QString &displayName, QBluetoothAbstractService *parent)
    : QObject(parent),
      m_name(name),
      m_displayName(displayName)
{
}

quint32 QBluetoothAbstractServicePrivate::registerRecord(const QString &record)
{
    QDBusConnection dbc = QDBusConnection::systemBus();
    QDBusInterface iface("org.bluez", "/org/bluez",
                        "org.bluez.Database", dbc);
    if (!iface.isValid())
        return 0;

    QDBusReply<quint32> reply = iface.call("AddServiceRecordFromXML", record);
    if (!reply.isValid()) 
        return 0;

    return reply.value();
}

bool QBluetoothAbstractServicePrivate::unregisterRecord(quint32 handle)
{
    QDBusConnection dbc = QDBusConnection::systemBus();
    QDBusInterface iface("org.bluez", "/org/bluez",
                        "org.bluez.Database", dbc);
    if (!iface.isValid())
        return false;

    QDBusReply<void> reply = iface.call("RemoveServiceRecord",
                                        QVariant::fromValue(handle));
    if (!reply.isValid()) 
        return false;

    return true;
}



// ============================================================

/*!
    \class QBluetoothAbstractService
    \mainclass
    \brief  The QBluetoothAbstractService class provides a base interface class for Bluetooth services within Qtopia.

    To create a Qtopia Bluetooth service, subclass QBluetoothAbstractService
    and implement the start(), stop() and setSecurityOptions() methods. Your
    service will automatically be registered and accessible as a Bluetooth
    service within Qtopia. This means the service will be accessible to
    external parties through QBluetoothServiceController. It will also be
    shown in the list of local services in Qtopia's Bluetooth settings
    application, allowing end users to modify the service's settings.

    Naturally, it is possible to implement Bluetooth services outside of
    Qtopia by using the BlueZ libraries and standard Linux tools. However,
    such services will not be accessible through Qtopia.

    \sa {Tutorial: Creating a Bluetooth service}

    \ingroup qtopiabluetooth
*/


/*!
    Constructs a Bluetooth service. The \a name is a unique name that identifies
    the service, and \a displayName is a user-friendly, internationalized name
    for this service that can be displayed to the end user. The \a parent is
    the QObject parent for this service.
 */
QBluetoothAbstractService::QBluetoothAbstractService(const QString &name, const QString &displayName, QObject *parent)
    : QObject(parent),
      m_data(new QBluetoothAbstractServicePrivate(name, displayName, this))
{
    new Messenger(this);
}

/*!
    Destroys a Bluetooth service.
 */
QBluetoothAbstractService::~QBluetoothAbstractService()
{
}

/*!
    Registers the SDP service record \a record for this Bluetooth service
    and returns the service record handle of the newly registered service.
    Returns zero if the registration failed.

    \sa unregisterRecord()
 */
quint32 QBluetoothAbstractService::registerRecord(const QBluetoothSdpRecord &record)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QSdpXmlGenerator::generate(record, &buffer);

    buffer.seek(0);
    return m_data->registerRecord(QString::fromUtf8(buffer.readAll()));
}

/*!
    Uses the XML data from the file \a filename to register the SDP service
    record for this Bluetooth service and returns the service record handle of 
    the newly registered service. Returns zero if the registration failed.

    See \l {Tutorial: Creating a Bluetooth service#Using a XML-formatted SDP record}{Using a XML-formatted SDP record} for details on how to
    generate a XML-formatted SDP reord.

    \warning The given file must be UTF-8 encoded to be parsed correctly.

    \sa unregisterRecord()
 */
quint32 QBluetoothAbstractService::registerRecord(const QString &filename)
{
    int fd = ::open(QFile::encodeName(filename), O_RDONLY);
    if (fd == -1) {
        qWarning("QBluetoothAbstractService: error opening file %s, cannot register SDP record", filename.toLatin1().constData());
        return 0;
    }

    quint32 result = 0;

    // from QResource
    struct stat st;
    if (fstat(fd, &st) != -1) {
        uchar *ptr;
        ptr = reinterpret_cast<uchar *>(
                mmap(0, st.st_size,             // any address, whole file
                     PROT_READ,                 // read-only memory
                     MAP_FILE | MAP_PRIVATE,    // swap-backed map from file
                     fd, 0));                   // from offset 0 of fd
        if (ptr && ptr != reinterpret_cast<uchar *>(MAP_FAILED)) {
            // register the record
            result = m_data->registerRecord( QString::fromUtf8((const char*)ptr) );

            // unmap to clean up
            munmap(ptr, st.st_size);
        }
    }
    ::close(fd);

    return result;
}

/*!
    Unregisters the SDP service record with the service record handle
    \a handle.

    Returns whether the record was successfully unregistered.

    \sa registerRecord()
 */
bool QBluetoothAbstractService::unregisterRecord(quint32 handle)
{
    return m_data->unregisterRecord(handle);
}

/*!
    Returns the unique name that identifies this service.
 */
QString QBluetoothAbstractService::name() const
{
    return m_data->m_name;
}


/*!
    Returns the user-friendly, internationalized name for this service that can
    be displayed to the end user.
 */
QString QBluetoothAbstractService::displayName() const
{
    return m_data->m_displayName;
}



/*!
    \fn void QBluetoothAbstractService::start()

    Starts this service.

    This method will be called by Qtopia when the service should be started.
    This may be because an external party has required that the service be
    started (for example, through QBluetoothServiceController, or through the
    Bluetooth Settings application) or because Qtopia has been configured to
    start the service automatically.

    Subclasses must override this to start the service appropriately. The
    subclass must emit started() when the service has started, or failed
    while trying to start, to announce the result of the start() invocation.

    \warning This function must be implementated in such a way that any intermediate
    objects (which have been created up to the point where the error occurred)
    are cleaned up before the error signal is emitted.

    \sa started(), stop()
 */

/*!
    \fn void QBluetoothAbstractService::stop()

    Stops this service.

    This method will be called by Qtopia when the service should be stopped.

    Subclasses must override this to stop the service appropriately. The
    subclass must emit stopped() to announce that the service has stopped.

    \sa stopped(), start()
 */

/*!
    \fn void QBluetoothAbstractService::setSecurityOptions(QBluetooth::SecurityOptions options)

    Sets the security options for this service to the given \a options.

    This method will be called by Qtopia when the security options should be
    changed.

    Subclasses must override this to set the security options for this service.
 */

/*!
    \fn void QBluetoothAbstractService::started(bool error, const QString &description)

    When implementing the start() function, this signal must be emitted by the
    subclass when the service has started or failed while attempting to start,
    to announce the result of the start() invocation.

    If the service failed to start, \a error should be true and \a description 
    should be a human-readable description of the error. Otherwise, \a error 
    should be false and \a description should be a null QString.

    \sa start(), stopped()
 */

/*!
    \fn void QBluetoothAbstractService::stopped()

    This signal must be emitted by the subclass inside the implementation of
    the stop() function, to announce that the service has stopped.

    \sa started(), stop()
 */

#include "qbluetoothabstractservice.moc"
