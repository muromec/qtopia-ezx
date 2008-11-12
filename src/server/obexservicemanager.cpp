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

#include "obexservicemanager.h"
#include <qobexpushservice.h>
#include <qobexpushclient.h>
#include <qcommdevicesession.h>
#include <qcommdevicecontroller.h>
#include <qtopiaapplication.h>

#ifdef QTOPIA_BLUETOOTH
#include <qbluetoothsdprecord.h>
#include <qbluetoothabstractservice.h>
#include <qbluetoothrfcommserver.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothlocaldevicemanager.h>
#include <qbluetoothnamespace.h>
#include <qbluetoothsdpquery.h>
#include <qbluetoothremotedevicedialog.h>
#include <qbluetoothrfcommsocket.h>
#endif

#ifdef QTOPIA_INFRARED
#include <qirlocaldevice.h>
#include <qirserver.h>
#include <qirnamespace.h>
#include <qirremotedevice.h>
#endif

#include <qvaluespace.h>
#include <qwaitwidget.h>
#include <qcontent.h>
#include <qmimetype.h>
#include <qdsactionrequest.h>
#include <qdsdata.h>
#include <qtopialog.h>
#include <qtopianamespace.h>
#include <qtopia/pim/qcontactmodel.h>
#include <qtopiaservices.h>
#include <qphoneprofile.h>

#include <QProcess>
#include <QStringList>
#include <QTimer>
#include <QTemporaryFile>
#include <QDebug>

#include <unistd.h>
#include <sys/vfs.h>

static QString obexservicemanager_prettyPrintSize(qint64 fsize)
{
    static const char *size_suffix[] = {
        QT_TRANSLATE_NOOP("CustomPushService", "B"),
        QT_TRANSLATE_NOOP("CustomPushService", "KB"),
        QT_TRANSLATE_NOOP("CustomPushService", "MB"),
        QT_TRANSLATE_NOOP("CustomPushService", "GB"),
    };

    double max = fsize;

    int i = 0;
    for (; i < 4; i++) {
        if (max > 1024.0) {
            max /= 1024.0;
        }
        else {
            break;
        }
    }

    // REALLY big file?
    if (i == 4)
        i = 0;

    if (fsize < 1024) {
        return QString::number(fsize) + qApp->translate("CustomPushService", size_suffix[i]);
    } else {
        return QString::number(max, 'f', 2)
                + qApp->translate("CustomPushService", size_suffix[i]);
    }
}


class CustomPushService : public QObexPushService
{
    Q_OBJECT
public:
    CustomPushService(QIODevice *device, QObject *parent = 0);
    ~CustomPushService();

    QByteArray businessCard() const;
    QDialog *confirmationDialog() const { return m_msgBox; }

    static QString incomingDirectory();
    static QFile *createReceivingFile(const QString &name);

private slots:
    void completedRequest();
    void stopMessageRingtone();

protected:
    virtual QIODevice *acceptFile(const QString &name, const QString &mimeType, qint64 size, const QString &description);

private:
    QMessageBox *m_msgBox;
};

CustomPushService::CustomPushService(QIODevice *device, QObject *parent)
    : QObexPushService(device, parent)
{
    QObject::connect(this, SIGNAL(requestFinished(bool)),
                       this, SLOT(completedRequest()));

    m_msgBox = new QMessageBox(0);
    m_msgBox->setWindowTitle(tr("Accept file?"));
    m_msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    m_msgBox->setDefaultButton(QMessageBox::No);
}

CustomPushService::~CustomPushService()
{
    delete m_msgBox;
}

QString CustomPushService::incomingDirectory()
{
    return Qtopia::tempDir() + "obex/in/";
}

QFile *CustomPushService::createReceivingFile(const QString &name)
{
    // Filename is not that important, as long as it's a valid filename,
    // because it's just used to create a temporary file that will be
    // deleted later by receivewindow.cpp.
    QString fileName = name;
    int sepIndex = name.lastIndexOf(QDir::separator());
    if (sepIndex != -1)
        fileName = fileName.mid(sepIndex + 1);
    if (fileName.isEmpty())
        fileName = tr("unnamed");

    // assuming incomingDirectory() ends with a dir separator
    QFile *file = new QFile(incomingDirectory() + fileName);
    if (file->open(QIODevice::WriteOnly)) {
        return file;
    }

    delete file;
    return 0;
}

void CustomPushService::completedRequest()
{
    qLog(Obex) << "CustomPushService: request has finished";

    QIODevice *d = currentDevice();
    if (d) {
        d->close();
        d->deleteLater();
    }
}

QIODevice *CustomPushService::acceptFile(const QString &name,
                                         const QString &mimeType,
                                         qint64 size,
                                         const QString &description)
{
    qLog(Obex) << "CustomPushService::acceptFile";

    // brighten the screen
    QtopiaServiceRequest e1("QtopiaPowerManager", "setBacklight(int)");
    e1 << -1; // read brightness setting from config
    e1.send();

    // wake up (otherwise the next keypress is swallowed even though the
    // screen has been brightened)
    QtopiaServiceRequest e2("QtopiaPowerManager", "setActive(bool)");
    e2 << false;
    e2.send();

    // sound a message tone
    QtopiaServiceRequest e3("Ringtone", "startMessageRingtone()");
    e3.send();
    QPhoneProfileManager manager;
    QPhoneProfile profile = manager.activeProfile();
    QTimer::singleShot(profile.msgAlertDuration(), this,
            SLOT(stopMessageRingtone()));


    qint64 availableStorage;

    struct statfs fs;
    if ( statfs( incomingDirectory().toLocal8Bit(), &fs ) == 0 ) {
        availableStorage = fs.f_bavail * fs.f_bsize;
    }
    else {
        qWarning("Could not stat: %s",
                incomingDirectory().toLocal8Bit().constData());
        return 0;
    }

    if (size >= availableStorage) {
        qLog(Obex) << "No space available for incoming file, auto-rejecting file";
        return 0;
    }

    // Accept files of mimetype vCard or vCalendar
    QString mime = mimeType.toLower();
    if (mime == "text/x-vcard" || mime == "text/x-vcalendar") {
        QFile *file = createReceivingFile(name);
        if (file)
            file->setParent(this);
        return file;
    }

    QString desc = ( description.isEmpty() ? name : description );
    if (size > 0) {
        m_msgBox->setText( tr("<qt>Accept incoming file <b>%1</b> of size %2? "
                    "You have %3 of storage remaining.</qt>", "%1=name %2=3Kb %3=3MB"
                            ).arg(desc).
                              arg(obexservicemanager_prettyPrintSize(size)).
                              arg(obexservicemanager_prettyPrintSize(availableStorage)) );
    } else {
        m_msgBox->setText( tr("<qt>Accept incoming file <b>%1</b>? "
                    "You have %2 of storage remaining.</qt>", "%1=name %2=3MB"
                            ).arg(desc).
                              arg(obexservicemanager_prettyPrintSize(availableStorage)) );
    }

    // stop ringtone if msg box has been accepted/rejected
    int result = QtopiaApplication::execDialog(m_msgBox);
    stopMessageRingtone();

    if (result == QMessageBox::Yes) {
        QFile *file = createReceivingFile(name);
        if (file)
            file->setParent(this);
        return file;
    }

    return 0;
}

QByteArray CustomPushService::businessCard() const
{
    QContactModel model(0);

    if (!model.hasPersonalDetails()) {
        return QByteArray();
    }

    QContact myVCard = model.personalDetails();
    QByteArray arr;
    QDataStream stream(&arr, QIODevice::WriteOnly);
    myVCard.writeVCard(&stream);
    return arr;
}


void CustomPushService::stopMessageRingtone()
{
    QtopiaServiceRequest e("Ringtone", "stopMessageRingtone()");
    e.send();
}



#ifdef QTOPIA_INFRARED
/*!
    \service InfraredBeamingService InfraredBeaming
    \brief Provides the Qtopia InfraredBeaming service.

    The \i InfraredBeaming service enables applications to send
    files over the Infrared link.  The service takes care of
    establishing a connection, tracking progress and notifying the user
    of events that occur during the transmission.
 */

/*!
    \internal
*/
InfraredBeamingService::InfraredBeamingService(ObexServiceManager *parent)
    : QtopiaAbstractService("InfraredBeaming", parent)
        , m_waitWidget(0)
        , m_file(0)
        , m_session(0)
{
    QStringList irDevices = QIrLocalDevice::devices();
    m_deviceName = irDevices[0].toLatin1();
    m_device = new QIrLocalDevice(m_deviceName);

    m_parent = parent;
    m_busy = false;
    m_waitWidget = new QWaitWidget(0);
    m_file = 0;
    m_current = 0;

    m_session = 0;
    m_socket = 0;

    publishAll();

    connect(m_device, SIGNAL(remoteDevicesFound(QList<QIrRemoteDevice>)),
            this, SLOT(remoteDevicesFound(QList<QIrRemoteDevice>)));
}

/*!
    \internal
 */
InfraredBeamingService::~InfraredBeamingService()
{
    delete m_waitWidget;
    delete m_session;
    delete m_file;
}

/*!
    \internal
 */
void InfraredBeamingService::sessionFailed()
{
    QTimer::singleShot(0, m_session, SLOT(deleteLater()));  // try a new session next time

    if (m_type == VObject) {
        m_waitWidget->setText(tr( "<P>There was an error using the local infrared device."));
        m_waitWidget->setCancelEnabled(true);
        QTimer::singleShot(5000, m_waitWidget, SLOT(hide()));
    }
    else {
        QMessageBox::critical( 0, tr( "Beam File" ),
                              tr( "<P>Error while trying to use the infrared device." ));
        m_waitWidget->hide();
    }

    m_busy = false;
}

/*!
    \internal
 */
void InfraredBeamingService::sessionOpen()
{
    bool ret = m_device->discoverRemoteDevices(QIr::OBEX);

    // This should never happen in practice if we havean open session
    if (!ret) {
        m_session->endSession();
        sessionFailed();
    }
}

void InfraredBeamingService::remoteDevicesFound(const QList<QIrRemoteDevice> &devices)
{
    if (devices.size() == 0) {
        handleConnectionFailed();
    }

    // Now make an Infrared Connection to the remote host
    m_socket = new QIrSocket(this);
    QObject::connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    QObject::connect(m_socket, SIGNAL(error(QIrSocket::SocketError)),
                     this, SLOT(handleConnectionFailed()));
    // Pick the first available device for now
    m_socket->connect("OBEX", devices[0].address());
}

void InfraredBeamingService::handleConnectionFailed()
{
    if (!m_socket)
        return;

    m_session->endSession();
    m_waitWidget->setText(tr( "<P>Could not connect to the remote device.  Please make sure that the Infrared ports are aligned and the device's Infrared capabilities are turned on."));

    m_waitWidget->setCancelEnabled(true);
    QTimer::singleShot(5000, m_waitWidget, SLOT(hide()));
    m_busy = false;

    delete m_socket;
    m_socket = 0;
}

void InfraredBeamingService::socketConnected()
{
    m_pushFailed = false;
    QObexPushClient *pushClient = new QObexPushClient(m_socket, this);
    connect(pushClient, SIGNAL(done(bool)), pushClient, SLOT(deleteLater()));
    connect(pushClient, SIGNAL(commandFinished(int,bool)), this,
            SLOT(pushCommandFinished(int,bool)));
    disconnect(m_socket, SIGNAL(error(QIrSocket::SocketError)),
               this, SLOT(handleConnectionFailed()));
    connect(m_socket, SIGNAL(error(QIrSocket::SocketError)),
            this, SLOT(socketError(QIrSocket::SocketError)));

    if (m_type == File) {
        m_waitWidget->hide();
        QObject::connect(pushClient, SIGNAL(done(bool)), this, SLOT(doneBeamingFile(bool)));
        m_parent->setupConnection(pushClient, m_fileName, m_mimeType, m_description);
    }
    else {
        QObject::connect(pushClient, SIGNAL(done(bool)), this, SLOT(doneBeamingVObj(bool)));
        m_waitWidget->setText(tr("Sending..."));
    }

    pushClient->connect();
    if (m_file) {
        pushClient->send(m_file, m_prettyFile, m_mimeType, m_description);
    }
    else {
        pushClient->send(m_vobj, m_fileName, m_mimeType, m_description);
    }
    pushClient->disconnect();
}

void InfraredBeamingService::socketError(QIrSocket::SocketError error)
{
    qLog(Infrared) << "InfraredBeamingService::ConnectionFailed with error: " << error;
}

/*!
    \internal
 */
void InfraredBeamingService::pushCommandFinished(int /*id*/, bool error)
{
    QObexPushClient *pushClient = qobject_cast<QObexPushClient *>(sender());
    if (!pushClient)
        return;

    // don't worry about disconnect errors
    if (error && pushClient->currentCommand() != QObexPushClient::Disconnect)
        m_pushFailed = true;
}

/*!
    \internal
 */
void InfraredBeamingService::startBeamingVObj(const QByteArray &data, const QString &mimeType)
{
    QMessageBox::information( 0, tr( "Beam File" ),
                              tr( "<P>Please align the infrared receivers and hit OK once ready." ));

    m_waitWidget->setText(tr("Connecting..."));
    m_waitWidget->setCancelEnabled(false);
    m_waitWidget->show();

    m_mimeType = mimeType;
    m_description = QString();
    if (m_mimeType == "text/x-vcard") {
        m_fileName = "BusinessCard.vcf";
        m_prettyFile = m_fileName;
    }
    else if (m_mimeType == "text/x-vcalendar") {
        m_fileName = "vcal.vcs";
        m_prettyFile = m_fileName;
    }
    m_vobj = data;
    m_type = VObject;

    m_busy = true;

    if (!m_session) {
        qLog(Infrared) << "Lazy initializing a QCommDeviceSession object";
        m_session = new QCommDeviceSession(m_deviceName, this);
        QObject::connect(m_session, SIGNAL(sessionOpen()), this, SLOT(sessionOpen()));
        QObject::connect(m_session, SIGNAL(sessionFailed()), this, SLOT(sessionFailed()));
    }

    m_session->startSession();
}

/*!
    \internal
 */
void InfraredBeamingService::doneBeamingVObj(bool error)
{
    Q_UNUSED(error);
    m_busy = false;

    if (m_pushFailed) {
        if (m_current) {
            m_current->respond(tr("Transmission error"));
            delete m_current;
            m_current = 0;
        }

        m_waitWidget->setText(tr( "<P>An error has occurred during transmission.  Please make sure that the Infrared ports are kept aligned and the device's Infrared capabilities are turned on.") );
        m_waitWidget->setCancelEnabled(true);
        QTimer::singleShot(5000, m_waitWidget, SLOT(hide()));
        return;
    }

    if (m_current) {
        m_current->respond();
        delete m_current;
        m_current = 0;
    }

    m_session->endSession();
    m_waitWidget->setText(tr("Done"));
    QTimer::singleShot(1000, m_waitWidget, SLOT(hide()));
}

/*!
    \internal
 */
void InfraredBeamingService::startBeamingFile(const QString &fileName,
                                              const QString &mimeType,
                                              const QString &displayName,
                                              const QString &description,
                                              bool autodelete)
{
    m_file = new QFile(fileName);

    if (!m_file->open(QIODevice::ReadOnly)) {
        delete m_file;
        m_file = 0;
        QMessageBox::warning( 0, tr( "Beam File" ),
                              tr( "<P>The file you are trying to beam could not be opened!" ));
        return;
    }

    QMessageBox::information( 0, tr( "Beam File" ),
                            tr( "<P>Please align the infrared receivers and hit OK once ready." ));

    m_type = File;
    m_fileName = fileName;
    m_mimeType = mimeType;
    m_prettyFile = displayName;
    m_description = description;
    m_autodelete = autodelete;

    m_busy = true;

    m_waitWidget->setText(tr("Connecting..."));
    m_waitWidget->setCancelEnabled(false);
    m_waitWidget->show();

    if (!m_session) {
        qLog(Infrared) << "Lazy initializing a QCommDeviceSession object";
        m_session = new QCommDeviceSession(m_deviceName, this);
        QObject::connect(m_session, SIGNAL(sessionOpen()), this, SLOT(sessionOpen()));
        QObject::connect(m_session, SIGNAL(sessionFailed()), this, SLOT(sessionFailed()));
    }

    m_session->startSession();
}

/*!
    \internal
 */
void InfraredBeamingService::doneBeamingFile(bool error)
{
    Q_UNUSED(error);
    qLog(Infrared) << "InfraredBeamingService::doneBeamingFile()" << error;

    m_session->endSession();
    m_busy = false;
    delete m_file;
    m_file = 0;

    if (m_autodelete)
        ::unlink(m_fileName.toLocal8Bit());
}

/*!
    \internal
 */
void InfraredBeamingService::showBusyMessage()
{
    QMessageBox::information(0, tr("Transmission error"), tr("A transfer operation is in progress."));
}

/*!
    Asks the service to beam the currently set personal business card to a remote
    device.  If no business card is set, the user will be notified appropriately.

    \sa beamBusinessCard()
*/
void InfraredBeamingService::beamPersonalBusinessCard()
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    QContactModel model(this);

    if (!model.hasPersonalDetails()) {
        QMessageBox::warning( 0, tr( "Beam Personal vCard" ),
                                 tr( "<P>There is no personal business card set.  Please set a personal business card in the Contacts application." ));
        return;
    }

    QContact myVCard = model.personalDetails();
    QByteArray arr;
    QDataStream stream(&arr, QIODevice::WriteOnly);
    myVCard.writeVCard(&stream);

    startBeamingVObj(arr, "text/x-vcard");
}

/*!
    Asks the service to beam the business card represented by \a contact.

    \sa beamPersonalBusinessCard()
*/
void InfraredBeamingService::beamBusinessCard(const QContact &contact)
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    QByteArray arr;
    QDataStream stream(&arr, QIODevice::WriteOnly);
    contact.writeVCard(&stream);

    startBeamingVObj(arr, "text/x-vcard");
}

/*!
    Asks the service to beam the business card represented by \a request.  The
    request object should contain raw serialized QContact data.

    \sa beamPersonalBusinessCard()
*/
void InfraredBeamingService::beamBusinessCard(const QDSActionRequest &request)
{
    if (m_busy) {
        QDSActionRequest(request).respond(tr("A transfer operation is in progress."));
        return;
    }

    QByteArray arr = request.requestData().data();
    m_current = new QDSActionRequest(request);
    startBeamingVObj(arr, "text/x-vcard");
}

/*!
    Asks the service to beam a vCalendar object represented by \a request.  The
    request object should contain raw serialized QTask or QAppointment data.
 */
void InfraredBeamingService::beamCalendar(const QDSActionRequest &request)
{
    if (m_busy) {
        QDSActionRequest(request).respond(tr("A transfer operation is in progress."));
        return;
    }

    QByteArray arr = request.requestData().data();
    m_current = new QDSActionRequest(request);
    startBeamingVObj(arr, "text/x-vcalendar");
}

/*!
    Asks the service to beam \a fileName to the remote device.  Mimetype
    of the file is given by \a mimeType.  The \a autodelete parameter
    specifies whether the service should delete the file after it has performed
    the operation.  The \a description parameter holds the optional
    description of the file.
*/
void InfraredBeamingService::beamFile(const QString &fileName,
                                      const QString &mimeType,
                                      const QString &description,
                                      bool autodelete)
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    QString display = fileName;
    int pos = fileName.lastIndexOf("/");
    if ( pos != -1 )
        display = fileName.mid( pos + 1 );

    startBeamingFile(fileName, mimeType, display, description, autodelete);
}

/*!
    Asks the service to beam a fileName represented by \a id.
    A QContent object is built using the \a id.  The fileName and
    mimeType are then inferred from the QContent object.
 */
void InfraredBeamingService::beamFile(const QContentId &id)
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    QContent content(id);
    QMimeType mime(content);
    QString display = content.fileName();
    int pos = content.fileName().lastIndexOf("/");
    if ( pos != -1 )
        display = content.fileName().mid( pos + 1 );

    startBeamingFile(content.fileName(), mime.id(), display, content.name(), false);
}
#endif

#ifdef QTOPIA_BLUETOOTH
/*!
    \service BluetoothPushingService BluetoothPush
    \brief Provides the Qtopia BluetoothPush service.

    The \i BluetoothPush service enables applications to send
    files over the Bluetooth link.  This service takes care of user interaction
    to select a device to send to (if required), looking up the SDP record
    of the device to find out the RFCOMM channel to connect to, establishing
    the connection to the remote service, tracking progress and notifying the user
    of events that occur during the transmission.
 */

/*!
    \internal
*/
BluetoothPushingService::BluetoothPushingService(ObexServiceManager *parent)
    : QtopiaAbstractService("BluetoothPush")
{
    m_parent = parent;
    m_busy = false;
    m_waitWidget = new QWaitWidget(0);

    m_sdap = new QBluetoothSdpQuery();
    QObject::connect(m_sdap, SIGNAL(searchComplete(QBluetoothSdpQueryResult)),
                     this, SLOT(sdapQueryComplete(QBluetoothSdpQueryResult)));

    QBluetoothLocalDeviceManager mgr;
    QCommDeviceController *controller = new QCommDeviceController(
            mgr.defaultDevice().toLatin1(), this);
    QObject::connect(controller, SIGNAL(down()), SLOT(bluetoothDown()));

    m_device = 0;
    m_current = 0;

    m_session = 0;
    m_socket = 0;

    publishAll();
}

/*!
    \internal
*/
BluetoothPushingService::~BluetoothPushingService()
{
    delete m_waitWidget;
    delete m_sdap;
    delete m_session;
}

void BluetoothPushingService::bluetoothDown()
{
    // ensure state is reset
    m_busy = false;
}

/*!
    \internal
 */
void BluetoothPushingService::sessionOpen()
{
    if (!m_req.m_addr.isValid()) {
        QSet<QBluetooth::SDPProfile> profiles;
        profiles.insert(QBluetooth::ObjectPushProfile);
        m_req.m_addr =
                QBluetoothRemoteDeviceDialog::getRemoteDevice(0, profiles);

        if ( !m_req.m_addr.isValid() ) {
            m_session->endSession();
            m_busy = false;
            return;
        }
    }

    if (m_req.m_type == BluetoothPushRequest::Data) {
        startPushingVObj(m_req.m_addr, m_req.m_mimeType);
    }
    else {
        startPushingFile();
    }
}

/*!
    \internal
 */
void BluetoothPushingService::sessionFailed()
{
    m_busy = false;
    QTimer::singleShot(0, m_session, SLOT(deleteLater()));  // try a new session next time

    QMessageBox::warning( 0, tr( "Bluetooth" ),
                          tr( "<P>There is a problem with the Bluetooth device!" ));
}

/*!
    \internal
*/
// Called when BT device session has closed.
// This may be called as a result of the session being closed from
// donePushingVObj() or donePushinFile(), or as a result of the session being
// closed from elsewhere, e.g. if Bluetooth is turned off in Bluetooth
// settings.
void BluetoothPushingService::sessionClosed()
{
    qLog(Bluetooth) << "BluetoothPushingService: session closed";

    m_busy = false;

    if (m_socket) {
        if (m_socket->state() == QBluetoothAbstractSocket::ConnectedState) {
            // if push client is still running, it will stop and emit done() when
            // its transport is disconnected
            connect(m_socket, SIGNAL(disconnected()), m_socket, SLOT(deleteLater()));
            m_socket->disconnect();
        } else {
            delete m_socket;
            m_socket = 0;
        }
    }
}

/*!
    \internal
 */
void BluetoothPushingService::startSession()
{
    if (!m_session) {
        qLog(Bluetooth) << "Lazy initializing a new QCommDeviceSession object";
        QBluetoothLocalDevice local;
        m_session = new QCommDeviceSession(local.deviceName().toLatin1(), this);
        QObject::connect(m_session, SIGNAL(sessionOpen()), this, SLOT(sessionOpen()));
        QObject::connect(m_session, SIGNAL(sessionFailed()), this, SLOT(sessionFailed()));
        QObject::connect(m_session, SIGNAL(sessionClosed()), this, SLOT(sessionClosed()));
    }

    m_session->startSession();
}

/*!
    \internal
 */
void BluetoothPushingService::sdapQueryComplete(const QBluetoothSdpQueryResult &result)
{
    qLog(Bluetooth) << "Service searching complete";

    bool success = false;
    int channel = -1;

    foreach ( QBluetoothSdpRecord service, result.services() ) {
        if ( service.isInstance( QBluetooth::ObjectPushProfile ) ) {
            channel = QBluetoothSdpRecord::rfcommChannel(service);
            success = true;
            break;
        }
    }

    if (!success) {
        m_busy = false;
        m_session->endSession();
        // This should not happen if the DeviceSelector is working correctly
        m_waitWidget->setText(tr("<qt>The selected device does not support this service.</qt>"));
        m_waitWidget->setCancelEnabled(true);
        QTimer::singleShot(5000, m_waitWidget, SLOT(hide()));
        return;
    }

    // Now make a RFCOMM Connection to the remote host
    m_socket = new QBluetoothRfcommSocket(this);
    QObject::connect(m_socket, SIGNAL(connected()), this, SLOT(rfcommConnected()));
    QObject::connect(m_socket, SIGNAL(error(QBluetoothAbstractSocket::SocketError)),
                     this, SLOT(handleConnectionFailed()));
    m_socket->connect(QBluetoothAddress::any, m_req.m_addr, channel);
}

void BluetoothPushingService::handleConnectionFailed()
{
    m_busy = false;
    qLog(Bluetooth) << "BluetoothPushingService: ending session";
    m_session->endSession();

    m_waitWidget->setText(tr("Unable to connect.  Please ensure the Bluetooth device is in range and try again."));
    m_waitWidget->setCancelEnabled(true);
    QTimer::singleShot(5000, m_waitWidget, SLOT(hide()));

    delete m_socket;
    m_socket = 0;
}

void BluetoothPushingService::rfcommConnected()
{
    m_pushFailed = false;
    QObexPushClient *pushClient = new QObexPushClient(m_socket, this);
    connect(pushClient, SIGNAL(done(bool)), pushClient, SLOT(deleteLater()));
    connect(pushClient, SIGNAL(commandFinished(int,bool)),
            this, SLOT(pushCommandFinished(int,bool)));
    disconnect(m_socket, SIGNAL(error(QBluetoothAbstractSocket::SocketError)),
               this, SLOT(handleConnectionFailed()));
    connect(m_socket, SIGNAL(error(QBluetoothAbstractSocket::SocketError)),
            this, SLOT(rfcommError(QBluetoothAbstractSocket::SocketError)));

    if ((m_req.m_mimeType == "text/x-vcard") || (m_req.m_mimeType == "text/x-vcalendar")) {
        QObject::connect(pushClient, SIGNAL(done(bool)), this, SLOT(donePushingVObj(bool)));
        m_waitWidget->setText(tr("Sending..."));
    }
    else {
        m_waitWidget->hide();
        QObject::connect(pushClient, SIGNAL(done(bool)), this, SLOT(donePushingFile(bool)));
        m_parent->setupConnection(pushClient, m_req.m_fileName, m_req.m_mimeType, m_req.m_description);
    }

    pushClient->connect();
    if (m_device) {
        qLog(Bluetooth) << "BluetoothPushingService: sending file" << m_req.m_description;
        pushClient->send(m_device, m_req.m_prettyFile, m_req.m_mimeType, m_req.m_description);
    }
    else {
        qLog(Bluetooth) << "BluetoothPushingService: sending v-obj" << m_req.m_description;
        pushClient->send(m_req.m_vobj, m_req.m_fileName, m_req.m_mimeType, m_req.m_description);
    }
    pushClient->disconnect();

}

void BluetoothPushingService::rfcommError(QBluetoothAbstractSocket::SocketError error)
{
    qLog(Bluetooth) << "BluetoothPushingService::ConnectionFailed with error: " << error;
}

/*!
    \internal
 */
void BluetoothPushingService::pushCommandFinished(int id, bool error)
{
    QObexPushClient *pushClient = qobject_cast<QObexPushClient *>(sender());
    if (!pushClient)
        return;

    qLog(Bluetooth) << "BT Push client finished request" << id << "with error:" << pushClient->error()
        << "and response:" << pushClient->lastCommandResponse();

    // don't worry about disconnect errors
    if (error && pushClient->currentCommand() != QObexPushClient::Disconnect)
        m_pushFailed = true;
}

/*!
    \internal
 */
void BluetoothPushingService::startPushingVObj(const QBluetoothAddress &addr,
                                               const QString &mimeType)
{
    if (mimeType == "text/x-vcard") {
        m_req.m_fileName = "BusinessCard.vcf";
        m_req.m_prettyFile = m_req.m_fileName;
        m_req.m_description = QString();
    }
    else {
        m_req.m_fileName = "vcal.vcs";
        m_req.m_prettyFile = m_req.m_fileName;
        m_req.m_description = QString();
    }

    QBluetoothLocalDevice localDev;
    m_sdap->searchServices(addr, localDev, QBluetooth::ObjectPushProfile);

    m_waitWidget->setText(tr("Connecting..."));
    m_waitWidget->setCancelEnabled(false);
    m_waitWidget->show();
}

/*!
    \internal
 */
void BluetoothPushingService::donePushingVObj(bool error)
{
    Q_UNUSED(error);
    m_busy = false;

    if (m_pushFailed) {
        // Respond to the request
        if (m_current) {
            m_current->respond(tr("Error during transmission"));
            delete m_current;
            m_current = 0;
        }

        m_waitWidget->setText(tr( "<P>An error has occurred during transmission.  Please ensure that the selected Bluetooth device is kept on and in range.") );
        QTimer::singleShot(5000, m_waitWidget, SLOT(hide()));
        m_waitWidget->setCancelEnabled(true);
        m_session->endSession();
        return;
    }

    m_waitWidget->setText(tr("Done"));
    QTimer::singleShot(1000, m_waitWidget, SLOT(hide()));

    // Respond to the request
    if (m_current) {
        m_current->respond();
        delete m_current;
        m_current = 0;
    }

    m_session->endSession();
}

/*!
    \internal
 */
void BluetoothPushingService::startPushingFile()
{
    m_device = new QFile(m_req.m_fileName);

    if (!m_device->open(QIODevice::ReadOnly)) {
        delete m_device;
        m_device = 0;
        QMessageBox::warning( 0, tr( "Push File" ),
                              tr( "<P>The file you are trying to beam could not be opened!" ));
        m_busy = false;
        return;
    }

    QBluetoothLocalDevice localDev;
    m_sdap->searchServices(m_req.m_addr, localDev, QBluetooth::ObjectPushProfile);

    m_waitWidget->setText(tr("Connecting..."));
    m_waitWidget->setCancelEnabled(false);
    m_waitWidget->show();
}

/*!
    \internal
 */
void BluetoothPushingService::donePushingFile(bool error)
{
    Q_UNUSED(error);
    qLog(Bluetooth) << "BluetoothPushingService::donePushingFile()" << error;

    m_busy = false;
    delete m_device;
    m_device = 0;

    if (m_req.m_autodelete)
        ::unlink(m_req.m_fileName.toLocal8Bit());

    m_session->endSession();
}

/*!
    \internal
 */
void BluetoothPushingService::showBusyMessage()
{
    QMessageBox::information(0, tr("Transmission error"), tr("A transfer operation is in progress."));
}

/*!
    \internal
 */
bool BluetoothPushingService::getPersonalVCard(QByteArray &arr)
{
    QContactModel model(this);

    if (!model.hasPersonalDetails()) {
        QMessageBox::warning( 0, tr( "Push Personal vCard" ),
                              tr( "<P>There is no personal business card set.  Please set a personal business card in the Contacts application." ));
        return false;
    }

    QContact myVCard = model.personalDetails();
    QDataStream stream(&arr, QIODevice::WriteOnly);
    myVCard.writeVCard(&stream);

    return true;
}

/*!
    Asks the service to send the currently set personal business card to a remote
    device.  If no business card is set, the user will be notified appropriately.

    \sa pushBusinessCard()
 */
void BluetoothPushingService::pushPersonalBusinessCard()
{
    qLog(Bluetooth) << "BluetoothPushingService::pushPersonalBusinessCard()" << m_busy;

    if (m_busy) {
        showBusyMessage();
        return;
    }

    QByteArray arr;
    if (!getPersonalVCard(arr))
        return;

    m_req.m_type = BluetoothPushRequest::Data;
    m_req.m_vobj = arr;
    m_req.m_addr = QBluetoothAddress();
    m_req.m_mimeType = "text/x-vcard";
    m_req.m_autodelete = false;

    m_busy = true;
    startSession();
}

/*!
    Asks the service to beam the currently set personal business card to a remote
    device.  If no business card is set, the user will be notified appropriately.

    The user will not be prompted to select a remote device to send to, instead
    the a device with address given by \a addr will be used.

    \sa pushPersonalBusinessCard()
 */
void BluetoothPushingService::pushPersonalBusinessCard(const QBluetoothAddress &addr)
{
    qLog(Bluetooth) << "BluetoothPushingService::pushPersonalBusinessCard(addr)" << m_busy;

    if (m_busy) {
        showBusyMessage();
        return;
    }

    QByteArray arr;
    if (!getPersonalVCard(arr))
        return;

    if ( !addr.isValid() ) {
        return;
    }

    m_req.m_type = BluetoothPushRequest::Data;
    m_req.m_vobj = arr;
    m_req.m_addr = addr;
    m_req.m_mimeType = "text/x-vcard";
    m_req.m_autodelete = false;

    m_busy = true;
    startSession();
}

/*!
    Asks the service to send the business card represented by \a contact.

    \sa pushPersonalBusinessCard()
 */
void BluetoothPushingService::pushBusinessCard(const QContact &contact)
{
    qLog(Bluetooth) << "BluetoothPushingService::pushBusinessCard" << m_busy;

    if (m_busy) {
        showBusyMessage();
        return;
    }

    QByteArray arr;
    QDataStream stream(&arr, QIODevice::WriteOnly);
    contact.writeVCard(&stream);

    m_req.m_type = BluetoothPushRequest::Data;
    m_req.m_vobj = arr;
    m_req.m_addr = QBluetoothAddress();
    m_req.m_mimeType = "text/x-vcard";
    m_req.m_autodelete = false;

    m_busy = true;
    startSession();
}

/*!
    Asks the service to send the business card represented by \a request.  The
    request object should contain raw serialized QContact data.

    \sa pushPersonalBusinessCard()
 */
void BluetoothPushingService::pushBusinessCard(const QDSActionRequest &request)
{
    qLog(Bluetooth) << "BluetoothPushingService::pushBusinessCard" << m_busy;

    if (m_busy) {
        showBusyMessage();
        return;
    }

    QByteArray arr = request.requestData().data();

    m_req.m_type = BluetoothPushRequest::Data;
    m_req.m_vobj = arr;
    m_req.m_addr = QBluetoothAddress();
    m_req.m_mimeType = "text/x-vcard";
    m_req.m_autodelete = false;

    m_current = new QDSActionRequest(request);

    m_busy = true;
    startSession();
}

/*!
    Asks the service to beam a vCalendar object represented by \a request.  The
    request object should contain raw serialized QTask or QAppointment data.

    \sa pushBusinessCard()
 */
void BluetoothPushingService::pushCalendar(const QDSActionRequest &request)
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    qLog(Bluetooth) << "BluetoothPushingService::pushCalendar()";

    QByteArray arr = request.requestData().data();

    m_req.m_type = BluetoothPushRequest::Data;
    m_req.m_vobj = arr;
    m_req.m_addr = QBluetoothAddress();
    m_req.m_mimeType = "text/x-vcalendar";
    m_req.m_autodelete = false;

    m_current = new QDSActionRequest(request);

    m_busy = true;
    startSession();
}

/*!
    Asks the service to send \a fileName to the remote device.  Mimetype
    of the file is given by \a mimeType.  The \a autodelete parameter
    specifies whether the service should delete the file after it has performed
    the operation.  The \a description parameter holds the optional
    description of the file.
 */
void BluetoothPushingService::pushFile(const QString &fileName,
                                       const QString &mimeType,
                                       const QString &description,
                                       bool autodelete)
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    qLog(Bluetooth) << "BluetoothPushingService::pushFile()" << fileName
            << mimeType << description << autodelete;

    m_req.m_type = BluetoothPushRequest::File;
    m_req.m_addr = QBluetoothAddress();
    m_req.m_fileName = fileName;
    m_req.m_mimeType = mimeType;
    m_req.m_autodelete = autodelete;

    m_req.m_prettyFile = fileName;
    int pos = fileName.lastIndexOf( "/" );
    if ( pos != -1 )
        m_req.m_prettyFile = fileName.mid( pos + 1 );

    m_req.m_description = description;

    m_busy = true;
    startSession();
}

/*!
    Asks the service to beam a fileName represented by \a id.
    A QContent object is built using the \a id.  The fileName and
    mimeType are then inferred from the QContent object.
 */
void BluetoothPushingService::pushFile(const QContentId &id)
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    qLog(Bluetooth) << "BluetoothPushingService::pushFile() (QContent)" << id;

    QContent content(id);
    QMimeType mime(content);

    m_req.m_type = BluetoothPushRequest::File;
    m_req.m_addr = QBluetoothAddress();
    m_req.m_fileName = content.fileName();
    m_req.m_mimeType = mime.id();
    m_req.m_autodelete = false;
    m_req.m_prettyFile = content.fileName();
    int pos = m_req.m_prettyFile.lastIndexOf( "/" );
    if ( pos != -1 )
        m_req.m_prettyFile = m_req.m_prettyFile.mid( pos + 1 );
    m_req.m_description = content.name();

    m_busy = true;
    startSession();
}

/*!
    Asks the service to beam a fileName represented by \a id.
    A QContent object is built using the \a id.  The fileName and
    mimeType are then inferred from the QContent object.

    The user will not be prompted to select a device to send to, instead the device
    at address \a addr will be used.
 */
void BluetoothPushingService::pushFile(const QBluetoothAddress &addr, const QContentId &id)
{
    if (m_busy) {
        showBusyMessage();
        return;
    }

    qLog(Bluetooth) << "BluetoothPushingService::pushFile() (QBluetoothAddress,QContent)"
            << addr.toString() << id;

    QContent content(id);
    QMimeType mime(content);

    m_req.m_type = BluetoothPushRequest::File;
    m_req.m_addr = addr;
    m_req.m_fileName = content.fileName();
    m_req.m_mimeType = mime.id();
    m_req.m_autodelete = false;
    m_req.m_prettyFile = content.name();
    m_req.m_description = content.name();

    m_busy = true;
    startSession();
}
#endif

#ifdef QTOPIA_BLUETOOTH
class ObexPushServiceProvider : public QBluetoothAbstractService
{
    Q_OBJECT
public:
    ObexPushServiceProvider(ObexServiceManager *parent = 0);
    ~ObexPushServiceProvider();

    virtual void start();
    virtual void stop();

    virtual void setSecurityOptions(QBluetooth::SecurityOptions options);

private slots:
    void newOPushConnection();
    void pushServiceDone();
    void rfcommSocketDisconnected();
    void sessionEnded();
    void btDeviceSessionClosed();

private:
    void close();

    ObexServiceManager *m_parent;
    QBluetoothRfcommServer *m_rfcommServer;
    QBluetooth::SecurityOptions m_securityOptions;
    QBluetoothLocalDevice *m_local;
    int m_numBtSessions;
    QCommDeviceSession *m_btDeviceSession;
    quint32 m_sdpRecordHandle;
    QHash<QBluetoothRfcommSocket*, CustomPushService*> m_activeSockets;
};

ObexPushServiceProvider::ObexPushServiceProvider(ObexServiceManager *parent)
    : QBluetoothAbstractService("ObexObjectPush", tr("OBEX Object Push"), parent),
      m_parent(parent),
      m_rfcommServer(0),
      m_securityOptions(0),
      m_local(new QBluetoothLocalDevice(this)),
      m_numBtSessions(0),
      m_btDeviceSession(0),
      m_sdpRecordHandle(0)
{
}

ObexPushServiceProvider::~ObexPushServiceProvider()
{
    delete m_btDeviceSession;
}

void ObexPushServiceProvider::close()
{
    qLog(Bluetooth) << "ObexPushServiceProvider close";

    if (m_rfcommServer) {
        m_rfcommServer->close();
        delete m_rfcommServer;
        m_rfcommServer = 0;
    }

    delete m_btDeviceSession;
    m_btDeviceSession = 0;
}

void ObexPushServiceProvider::start()
{
    qLog(Bluetooth) << "ObexPushServiceProvider start";
    if (m_rfcommServer)
        close();

    if (!m_local->isValid()) {
        delete m_local;
        m_local = new QBluetoothLocalDevice(this);
        if (!m_local->isValid()) {
            emit started(true, tr("Cannot access local bluetooth device"));
            return;
        }
    }

    m_sdpRecordHandle = 0;
    QBluetoothSdpRecord sdpRecord;

    // register the SDP service
    QFile sdpRecordFile(Qtopia::qtopiaDir() + "etc/bluetooth/sdp/opp.xml");
    if (sdpRecordFile.open(QIODevice::ReadOnly)) {
        sdpRecord = QBluetoothSdpRecord::fromDevice(&sdpRecordFile);
        if (!sdpRecord.isNull())
            m_sdpRecordHandle = registerRecord(sdpRecord);
    }

    if (sdpRecord.isNull())
        qWarning() << "ObexPushServiceProvider: cannot read" << sdpRecordFile.fileName();

    if (m_sdpRecordHandle == 0) {
        emit started(true, tr("Error registering with SDP server"));
        return;
    }

    m_rfcommServer = new QBluetoothRfcommServer(this);
    connect(m_rfcommServer, SIGNAL(newConnection()),
            this, SLOT(newOPushConnection()));

    int channel = QBluetoothSdpRecord::rfcommChannel(sdpRecord);
    qLog(Bluetooth) << "Starting OBEX Push server on channel" << channel;

    if (!m_rfcommServer->listen(m_local->address(), channel)) {
        unregisterRecord(m_sdpRecordHandle);
        close();
        emit started(true, tr("Error listening on OBEX Push Server"));
        return;
    }
    m_rfcommServer->setSecurityOptions(m_securityOptions);
    emit started(false, QString());

    if (!m_btDeviceSession) {
        QBluetoothLocalDevice dev;
        m_btDeviceSession = new QCommDeviceSession(dev.deviceName().toLatin1());
        connect(m_btDeviceSession, SIGNAL(sessionClosed()),
                SLOT(btDeviceSessionClosed()));
    }
}

void ObexPushServiceProvider::newOPushConnection()
{
    QBluetoothRfcommSocket *rfcommSocket = qobject_cast<QBluetoothRfcommSocket*>(
            m_rfcommServer->nextPendingConnection());
    if (!rfcommSocket)
        return;

    CustomPushService *opush = new CustomPushService(rfcommSocket, this);
    m_parent->setupConnection(opush);
    rfcommSocket->setParent(opush);

    m_activeSockets.insert(rfcommSocket, opush);

    // if transport is disconnected, the push service will emit done()
    connect(opush, SIGNAL(done(bool)), this, SLOT(pushServiceDone()));
    connect(rfcommSocket, SIGNAL(disconnected()), this, SLOT(rfcommSocketDisconnected()));
    connect(rfcommSocket, SIGNAL(destroyed(QObject*)), this, SLOT(sessionEnded()));

    m_numBtSessions++;

    if (m_numBtSessions == 1) { // First session
        qLog(Bluetooth) << "ObexPushServiceProvider starting BT Session";
        m_btDeviceSession->startSession();
    }
}

void ObexPushServiceProvider::rfcommSocketDisconnected()
{
    qLog(Bluetooth) << "ObexPushServiceProvider: RFCOMM socket has been disconnected";

    QBluetoothRfcommSocket *rfcomm =
            qobject_cast<QBluetoothRfcommSocket*>(sender());


    if (rfcomm && m_activeSockets.contains(rfcomm)) {
        CustomPushService *service = m_activeSockets[rfcomm];
        m_activeSockets.remove(rfcomm);

        // might be disconnected while the "Accept file?" dialog is
        // showing
        if (service->confirmationDialog()->isVisible()) {
            service->confirmationDialog()->reject();
        }

        // delete the rfcomm & obex sockets and also the push service
        QTimer::singleShot(0, service, SLOT(deleteLater()));
    }
}

void ObexPushServiceProvider::pushServiceDone()
{
    QObject *s = sender();
    CustomPushService *opush = qobject_cast<CustomPushService *>(s);
    if (!opush)
        return;

    QBluetoothRfcommSocket *rfcomm =
            qobject_cast<QBluetoothRfcommSocket*>(opush->sessionDevice());
    if (rfcomm) {
        qLog(Bluetooth) << "Disconnecting RFCOMM socket for Obex Push session...";
        rfcomm->disconnect();
    }
}

void ObexPushServiceProvider::sessionEnded()
{
    m_numBtSessions--;
    qLog(Bluetooth) << "Bluetooth Obex Push session finished, m_numBtSessions: " << m_numBtSessions;

    if (m_numBtSessions == 0) {
        qLog(Bluetooth) << "Ending Bluetooth Obex Push session";
        m_btDeviceSession->endSession();
        qLog(Bluetooth) << "Session ended";
    }
}

// Called when BT device session has closed.
// This may be called as a result of the session being closed in sessionEnded(),
// or as a result of the session being closed from elsewhere, e.g. if Bluetooth
// is turned off in Bluetooth settings.
void ObexPushServiceProvider::btDeviceSessionClosed()
{
    QListIterator<QBluetoothRfcommSocket*> i(m_activeSockets.keys());
    while (i.hasNext()) {
        i.next()->disconnect();
    }
}

void ObexPushServiceProvider::stop()
{
    qLog(Bluetooth) << "ObexPushServiceProvider stop";

    if (m_rfcommServer && m_rfcommServer->isListening())
        close();

    if (!unregisterRecord(m_sdpRecordHandle))
        qLog(Bluetooth) << "ObexPushServiceProvider::stop() error unregistering SDP service";

    emit stopped();
}

void ObexPushServiceProvider::setSecurityOptions(QBluetooth::SecurityOptions options)
{
    m_securityOptions = options;
    if (m_rfcommServer && m_rfcommServer->isListening())
        m_rfcommServer->setSecurityOptions(options);
}

#endif


/*!
    \class ObexServiceManager
    \ingroup QtopiaServer
    \brief The ObexServiceManager class is responsible for managing OBEX related services over Bluetooth and Infrared.

    ObexServiceManager provides and manages the Bluetooth OBEX Push
    service and the Infrared IrXfer server.  It also provides a common
    infrastructure for these services, including user notification of
    files transferred and received.

    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
  */

/*!
    Constructs an ObexServiceManager.  The \a parent argument contains the
    QObject parent.
*/
ObexServiceManager::ObexServiceManager(QObject *parent) : QObject(parent)
#ifdef QTOPIA_INFRARED
        , m_irxfer(0), m_numIrSessions(0), m_irSession(0),
        m_infraredService(0)
#endif
{
#ifdef QTOPIA_BLUETOOTH
    m_bluetoothService = new BluetoothPushingService(this);
#endif

    //just blow it all away and recreate the dir
    // Note that any files the user has not accepted from his incoming directory
    // (using the receive window widget) will be gone forever
    QProcess::execute("rm", QStringList() << "-rf" << CustomPushService::incomingDirectory());
    QProcess::execute("mkdir", QStringList() << "-p" << "-m" << "0755" << CustomPushService::incomingDirectory());

#ifdef QTOPIA_BLUETOOTH
    new ObexPushServiceProvider(this);
#endif

#ifdef QTOPIA_INFRARED
    QStringList irDevices = QIrLocalDevice::devices();
    qLog(Infrared) << "Got Infrared devices:" << irDevices;

    if (irDevices.size() > 0) {
        m_irxfer = new QIrServer(this);
        m_irxfer->listen("OBEX:IrXfer", QIr::OBEX | QIr::Telephony);
        connect(m_irxfer, SIGNAL(newConnection()), SLOT(newIrxferConnection()));

        //TODO: Register other IR services here
        m_irSession = new QCommDeviceSession(irDevices[0].toLatin1());
        m_infraredService = new InfraredBeamingService(this);
    }
    else {
        qLog(Infrared) << "No IRDA devices found!";
    }
#endif
}

/*!
    Destructor.
*/
ObexServiceManager::~ObexServiceManager()
{
#ifdef QTOPIA_INFRARED
    delete m_irxfer;
    delete m_irSession;
    delete m_infraredService;
#endif

#ifdef QTOPIA_BLUETOOTH
    delete m_bluetoothService;
#endif
}

/*!
    \internal

    Sets up a service connection.
*/
void ObexServiceManager::setupConnection(QObexPushService *opush)
{
    connect(opush, SIGNAL(putRequested(QString,QString,qint64,QString)),
            SLOT(putRequested(QString,QString,qint64,QString)));
    connect(opush, SIGNAL(businessCardRequested()),
            SLOT(businessCardRequested()));
    connect(opush, SIGNAL(dataTransferProgress(qint64,qint64)), SLOT(progress(qint64,qint64)));
    connect(opush, SIGNAL(requestFinished(bool)), SLOT(requestFinished(bool)));
    connect(opush, SIGNAL(destroyed(QObject*)),
            SLOT(aboutToDelete()));

    m_map.insert(opush, 0);
}

/*!
    \internal

    Sets up a client connection.
*/
void ObexServiceManager::setupConnection(QObexPushClient *client,
                                         const QString &fileName,
                                         const QString &mimeType,
                                         const QString &description)
{
    // We never do multiple put commands here, so this is safe
    connect(client, SIGNAL(dataTransferProgress(qint64,qint64)), SLOT(progress(qint64,qint64)));
    connect(client, SIGNAL(done(bool)), SLOT(requestFinished(bool)));
    connect(client, SIGNAL(destroyed(QObject*)),
            SLOT(aboutToDelete()));

    QMap<QObject *, int>::iterator i = m_map.insert(client, 0);

    i.value() = nextId();
    emit sendInitiated(i.value(), fileName, mimeType, description);
}

/*!
    Aborts the request identified by \a id if the request is currently in
    progress. Otherwise, this method does nothing.
*/
void ObexServiceManager::abortTransfer(int id)
{
    QMapIterator<QObject *, int> i(m_map);
    while (i.hasNext()) {
        if (i.next().value() == id) {
            // use QTimer to force disconnection if other side doesn't respond
            qLog(Obex) << "Trying to abort request" << id
                << ", will force transport disconnection regardless";
            if (QObexPushClient *c = qobject_cast<QObexPushClient*>(i.key())) {
                c->abort();
                QTimer::singleShot(1000, c->sessionDevice(), SLOT(deleteLater()));
            } else if (QObexPushService *s = qobject_cast<QObexPushService*>(i.key())) {
                s->abort();
                QTimer::singleShot(1000, s->sessionDevice(), SLOT(deleteLater()));
            }
            break;
        }
    }
}

#ifdef QTOPIA_INFRARED
/*!
    \internal
 */
void ObexServiceManager::newIrxferConnection()
{
    qLog(Infrared) << "New IrXfer Connection";
    if (!m_irxfer->hasPendingConnections())
        return;

    QIrSocket *irSocket = m_irxfer->nextPendingConnection();
    qLog(Infrared) << "Connection from:" << irSocket->remoteAddress();

    qLog(Infrared) << "Creating ObexPushService";
    CustomPushService *opush = new CustomPushService(irSocket, this);
    irSocket->setParent(opush);

    m_activeSockets.insert(irSocket, opush);

    setupConnection(opush);
    connect(irSocket, SIGNAL(destroyed(QObject*)), SLOT(irSessionEnded()));

    // if transport is disconnected, the push service will emit done()
    connect(opush, SIGNAL(done(bool)), this, SLOT(irXferPushServiceDone()));
    connect(irSocket, SIGNAL(disconnected()), irSocket, SLOT(irXferSocketDisconnected()));

    m_numIrSessions++;


    qLog(Infrared) << "Num IR Sessions: " << m_numIrSessions;

    if (m_numIrSessions == 1) {
        // We don't need to receive the notification of session start here
        // as that just notifies us that the device is up.  We would not be receiving
        // connections here if the device wasn't up though
        qLog(Infrared) << "ObexServiceManager starting IR Session";
        m_irSession->startSession();
    }
}

void ObexServiceManager::irXferPushServiceDone()
{
    QObject *s = sender();
    CustomPushService *opush = qobject_cast<CustomPushService *>(s);
    if (!opush)
        return;

    QIrSocket *irSocket =
            qobject_cast<QIrSocket*>(opush->sessionDevice());
    if (irSocket) {
        qLog(Infrared) << "Disconnecting IR socket for Obex Push session...";
        irSocket->disconnect();
    }
}

void ObexServiceManager::irXferSocketDisconnected()
{
    qLog(Infrared) << "IR socket has been disconnected";

    QIrSocket *irSocket = qobject_cast<QIrSocket*>(sender());

    if (irSocket && m_activeSockets.contains(irSocket)) {
        CustomPushService *service = m_activeSockets[irSocket];
        m_activeSockets.remove(irSocket);

        // might be disconnected while the "Accept file?" dialog is
        // showing
        if (service->confirmationDialog()->isVisible()) {
            service->confirmationDialog()->reject();
        }

        // delete the IR & obex sockets and also the push service
        QTimer::singleShot(0, service, SLOT(deleteLater()));
    }
}
#endif

/*!
    \internal
 */
void ObexServiceManager::putRequested(const QString &/*fileName*/, const QString &mimeType, qint64, const QString &description)
{
    qLog(Obex) << "ObexServiceManager Put Request";
    QObject *s = sender();
    if (!s)
        return;

    QObexPushService *push = qobject_cast<QObexPushService*>(s);
    if (!push)
        return;
    QFile *file = qobject_cast<QFile*>(push->currentDevice());
    if (!file)
        return;

    qLog(Obex) << "Sender is valid";

    QMap<QObject *, int>::iterator i = m_map.find(s);
    if (i == m_map.end())
        return;

    qLog(Obex) << "Sender in map";

    i.value() = nextId();

    emit receiveInitiated(i.value(), QFileInfo(*file).fileName(), mimeType, description);
}

/*!
    \internal
 */
void ObexServiceManager::businessCardRequested()
{
    QObject *s = sender();
    if (!s)
        return;

    QMap<QObject *, int>::iterator i = m_map.find(s);
    if (i == m_map.end())
        return;

    i.value() = nextId();

    emit sendInitiated(i.value(), "", "text/x-vCard", QString());
}

/*!
    \internal
 */
void ObexServiceManager::requestFinished(bool error)
{
    QObject *s = sender();
    if (!s)
        return;

    QObexPushService *service = qobject_cast<QObexPushService*>(s);
    if (service)
        qLog(Obex) << "OBEX request finished with error:" << service->error();

    qLog(Obex) << "Request finished:" << error;

    QMap<QObject *, int>::iterator i = m_map.find(s);
    if (i == m_map.end()) {
        qLog(Obex) << "Unknown request object!";
        return;
    }

    if (i.value() != 0) {
        emit completed(i.value(), error);
        i.value() = 0;
    }
}

/*!
    \internal
 */
void ObexServiceManager::progress(qint64 done, qint64 total)
{
    QObject *s = sender();
    if (!s)
        return;

    QMap<QObject *, int>::const_iterator i = m_map.find(s);
    if (i == m_map.end())
        return;

    emit progress(i.value(), done, total);
}

/*!
    \internal
 */
void ObexServiceManager::aboutToDelete()
{
    qLog(Obex) << "Service Object about to delete itself";

    QObject *s = sender();
    if (!s)
        return;

    QMap<QObject *, int>::iterator i = m_map.find(s);
    if (i == m_map.end())
        return;

    m_map.erase(i);
}

#ifdef QTOPIA_INFRARED
/*!
    \internal
 */
void ObexServiceManager::irSessionEnded()
{
    m_numIrSessions--;
    qLog(Infrared) << "Infrared Receive finished, NumIrSessions: " << m_numIrSessions;

    if (m_numIrSessions == 0) {
        qLog(Infrared) << "Ending Infrared session";
        m_irSession->endSession();
        qLog(Infrared) << "Session ended";
    }
}
#endif

#if QT_VERSION < 0x040400
QBasicAtomic ObexServiceManager::idCounter = Q_ATOMIC_INIT(1);
#else
QAtomicInt ObexServiceManager::idCounter(1);
#endif

/*!
    \internal
 */
int ObexServiceManager::nextId()
{
#if QT_VERSION < 0x040400
    register int id;
    for (;;) {
        id = idCounter;
        if (idCounter.testAndSet(id, id + 1))
            break;
    }
    return id;
#else
    register int id;
    for (;;) {
        id = idCounter;
        if (idCounter.testAndSetOrdered(id, id + 1))
            break;
    }
    return id;
#endif
}

/*!
    \fn void ObexServiceManager::receiveInitiated(int id, const QString &fileName, const QString &mimeType, const QString &description)

    This signal is sent whenever an object is being received.  The \a id parameter
    contains a unique id of the request.  The \a fileName parameter contains the fileName
    of the object being received.  This file name will be in an incoming directory (the inbox.)
    The \a mimeType contains the MIME type of the object being received. The \a description
    contains the description of the object.

    One can track the progress of the request by using the completed() and progress() signals.

    \sa completed(), progress()
*/

/*!
    \fn void ObexServiceManager::sendInitiated(int id, const QString &fileName, const QString &mimeType, const QString &description)

    This signal is emitted whenever an object is being sent by the local device.  The \a id
    parameter contains a unique id of the request.  The \a fileName parameter contains
    the file name of the object being send.  The \a mimeType contains the MIME type of the object
    being received. The \a description is a user-friendly name for the file (for example, if the file has another name under Qtopia's content system).

    One can track the progress of the request by using the completed() and progress() signals.

    \sa completed(), progress()
*/

/*!
    \fn void ObexServiceManager::progress(int id, qint64 bytes, qint64 total)

    This signal is emitted to report progress of an operation.  The \a id parameter
    contains the unique id of the request.  The \a bytes parameter contains the number
    of bytes sent.  The \a total contains the total number of bytes.  If the total is
    not known, it will be set to 0.
*/

/*!
    \fn void ObexServiceManager::completed(int id, bool error)

    This signal is emitted to report the completion of an operation.  The \a id parameter
    contains the unique id of the request completed.  The \a error parameter
    is set to true if the operation finished due to an error, and false otherwise.
*/

#include "obexservicemanager.moc"



