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

#ifndef __OBEXSERVICEMANAGER_H__
#define __OBEXSERVICEMANAGER_H__

#include <qglobal.h>
#include <QObject>
#include <qatomic.h>
#include <QMap>
#include <QHash>
#include <QtopiaAbstractService>

#include <qcommdevicesession.h>
#include <qcontent.h>

class ObexServiceManager;
class QWaitWidget;
class QDSActionRequest;
class QContact;

#ifdef QTOPIA_BLUETOOTH
#include <qbluetoothrfcommsocket.h>
#include <qbluetoothaddress.h>

class QBluetoothObexServer;
class QBluetoothSdpRecord;
class QBluetoothSdpQueryResult;
class QBluetoothLocalDevice;
class QBluetoothSdpQuery;
class ObexPushServiceProvider;

struct BluetoothPushRequest
{
    enum Type { File, Data };

    Type m_type;
    QByteArray m_vobj;
    QBluetoothAddress m_addr;
    QString m_mimeType;
    QString m_fileName;
    QString m_prettyFile;
    QString m_description;
    bool m_autodelete;
};

class BluetoothPushingService : public QtopiaAbstractService
{
    Q_OBJECT

public:
    BluetoothPushingService(ObexServiceManager *parent);
    ~BluetoothPushingService();

public slots:
    void pushPersonalBusinessCard();
    void pushPersonalBusinessCard(const QBluetoothAddress &addr);

    void pushBusinessCard(const QContact &contact);
    void pushBusinessCard(const QDSActionRequest& request);

    void pushCalendar(const QDSActionRequest &request);

    void pushFile(const QString &fileName, const QString &mimeType, const QString &description, bool autodelete);
    void pushFile(const QContentId &id);
    void pushFile(const QBluetoothAddress &addr, const QContentId &id);

private slots:
    void bluetoothDown();
    void sdapQueryComplete(const QBluetoothSdpQueryResult &result);
    void pushCommandFinished(int id, bool error);
    void donePushingVObj(bool error);
    void donePushingFile(bool error);

    void sessionOpen();
    void sessionFailed();
    void sessionClosed();

    void rfcommConnected();
    void handleConnectionFailed();
    void rfcommError(QBluetoothAbstractSocket::SocketError error);

private:
    void startPushingVObj(const QBluetoothAddress &addr,
                          const QString &mimetype);
    bool getPersonalVCard(QByteArray &arr);
    void startPushingFile();
    void startSession();
    void showBusyMessage();

    ObexServiceManager *m_parent;
    bool m_busy;
    QWaitWidget *m_waitWidget;
    QBluetoothSdpQuery *m_sdap;

    QDSActionRequest *m_current;
    QIODevice *m_device;
    BluetoothPushRequest m_req;
    QPointer<QCommDeviceSession> m_session;

    bool m_pushFailed;
    QPointer<QBluetoothRfcommSocket> m_socket;
};
#endif

class QObexPushService;
class QCommDeviceSession;
class QObexPushClient;
class CustomPushService;

#ifdef QTOPIA_INFRARED
class QIrRemoteDevice;
class QIrLocalDevice;
#include <qirsocket.h>

class QIrServer;

class InfraredBeamingService : public QtopiaAbstractService
{
    Q_OBJECT

public:
    InfraredBeamingService(ObexServiceManager *parent);
    ~InfraredBeamingService();

public slots:
    void beamPersonalBusinessCard();
    void beamBusinessCard(const QContact &contact);
    void beamBusinessCard(const QDSActionRequest &request);

    void beamFile(const QString &fileName, const QString &mimeType, const QString &description, bool autodelete);
    void beamFile(const QContentId &id);

    void beamCalendar(const QDSActionRequest &request);

private slots:
    void pushCommandFinished(int id, bool error);
    void doneBeamingVObj(bool error);
    void doneBeamingFile(bool error);

    void sessionOpen();
    void sessionFailed();

    void socketConnected();
    void handleConnectionFailed();
    void socketError(QIrSocket::SocketError error);
    void remoteDevicesFound(const QList<QIrRemoteDevice> &devices);

private:
    void startBeamingFile(const QString &fileName, const QString &mimeType, const QString &displayName, const QString &description, bool autodelete);
    void startBeamingVObj(const QByteArray &data, const QString &mimeType);
    void showBusyMessage();

    bool m_busy;
    ObexServiceManager *m_parent;
    QWaitWidget *m_waitWidget;
    QFile *m_file;
    QDSActionRequest *m_current;
    bool m_autodelete;
    QString m_fileName;
    QString m_mimeType;
    QString m_prettyFile;
    QString m_description;
    QPointer<QCommDeviceSession> m_session;
    QByteArray m_deviceName;

    bool m_pushFailed;

    enum Type {VObject, File};
    Type m_type;
    QByteArray m_vobj;

    QPointer<QIrSocket> m_socket;
    QIrLocalDevice *m_device;
};
#endif

class ObexServiceManager : public QObject
{
    Q_OBJECT

public:
    ObexServiceManager(QObject *parent = 0);
    ~ObexServiceManager();

    void setupConnection(QObexPushService *opush);
    void setupConnection(QObexPushClient *client, const QString &fileName, const QString &mimeType, const QString &description);

public slots:
    void abortTransfer(int id);

signals:
    void receiveInitiated(int id, const QString &fileName, const QString &mime, const QString &description);
    void sendInitiated(int id, const QString &fileName, const QString &mime, const QString &description);
    void progress(int id, qint64 bytes, qint64 total);
    void completed(int id, bool error);

private slots:
    void putRequested(const QString &fileName, const QString &mimeType, qint64, const QString &);
    void businessCardRequested();
    void requestFinished(bool error);
    void progress(qint64, qint64);
    void aboutToDelete();

#ifdef QTOPIA_INFRARED
    void newIrxferConnection();
    void irXferPushServiceDone();
    void irXferSocketDisconnected();
    void irSessionEnded();
#endif

private:
#ifdef QTOPIA_BLUETOOTH
    ObexPushServiceProvider *m_pushServiceProvider;
    BluetoothPushingService *m_bluetoothService;
#endif

#ifdef QTOPIA_INFRARED
    QIrServer *m_irxfer;

    int m_numIrSessions;
    QCommDeviceSession *m_irSession;
    InfraredBeamingService *m_infraredService;
    QHash<QIrSocket*, CustomPushService*> m_activeSockets;
#endif

    QMap<QObject *, int> m_map;
#if QT_VERSION < 0x040400
    static QBasicAtomic idCounter;
#else
    static QAtomicInt idCounter;
#endif
    static int nextId();
};

#endif
