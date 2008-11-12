/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QUNIXSOCKET_P_H
#define QUNIXSOCKET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/qabstractsocket.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>

extern "C" {
#include <sys/types.h>
};

class QUnixSocketRights;
class QUnixSocketRightsPrivate;
class QUnixSocketPrivate;
class QUnixSocketMessagePrivate;
class iovec;

class Q_GUI_EXPORT QUnixSocketRights {
public:
    QUnixSocketRights(int);
    ~QUnixSocketRights();

    QUnixSocketRights(const QUnixSocketRights &);
    QUnixSocketRights & operator=(const QUnixSocketRights &);

    bool isValid() const;

    int dupFd() const;
    int peekFd() const;

private:
    friend class QUnixSocket;
    QUnixSocketRights(int,int);
    QSharedDataPointer<QUnixSocketRightsPrivate> d;
};

class Q_GUI_EXPORT QUnixSocketMessage {
public:
    QUnixSocketMessage();
    QUnixSocketMessage(const QByteArray &);
    QUnixSocketMessage(const QByteArray &, const QList<QUnixSocketRights> &);
    QUnixSocketMessage(const QUnixSocketMessage &);
    QUnixSocketMessage(const iovec*, int);
    QUnixSocketMessage & operator=(const QUnixSocketMessage &);
    ~QUnixSocketMessage();

    void setBytes(const QByteArray &);
    void setRights(const QList<QUnixSocketRights> &);

    const QList<QUnixSocketRights> & rights() const;
    bool rightsWereTruncated() const;

    const QByteArray & bytes() const;

    pid_t processId() const;
    uid_t userId() const;
    gid_t groupId() const;

    void setProcessId(pid_t);
    void setUserId(uid_t);
    void setGroupId(gid_t);

    bool isValid() const;
private:
    friend class QUnixSocket;
    friend class QUnixSocketPrivate;
    QSharedDataPointer<QUnixSocketMessagePrivate> d;
};

class Q_GUI_EXPORT QUnixSocket : public QIODevice
{
    Q_OBJECT
public:
    QUnixSocket(QObject * = 0);
    QUnixSocket(qint64, qint64, QObject * = 0);
    virtual ~QUnixSocket();

    enum SocketState {
        UnconnectedState = QAbstractSocket::UnconnectedState,
        HostLookupState = QAbstractSocket::HostLookupState,
        ConnectingState = QAbstractSocket::ConnectingState,
        ConnectedState = QAbstractSocket::ConnectedState,
        BoundState = QAbstractSocket::BoundState,
        ClosingState = QAbstractSocket::ClosingState,
        ListeningState = QAbstractSocket::ListeningState,
    };

    enum SocketError { NoError, InvalidPath, ResourceError,
                       NonexistentPath, ConnectionRefused, UnknownError,
                       ReadFailure, WriteFailure };

    bool connect(const QByteArray & path);
    bool setSocketDescriptor(int socketDescriptor);
    int socketDescriptor() const;
    void abort();
    void close();

    bool flush();

    SocketError error() const;

    SocketState state() const;
    QByteArray address() const;

    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);
    qint64 rightsBufferSize() const;
    void setRightsBufferSize(qint64 size);

    bool canReadLine() const;

    qint64 write(const char * data, qint64 maxSize)
    { return QIODevice::write(data, maxSize); }
    qint64 write(const QByteArray & byteArray)
    { return QIODevice::write(byteArray); }
    qint64 read(char * data, qint64 maxSize)
    { return QIODevice::read(data, maxSize); }
    QByteArray read(qint64 maxSize)
    { return QIODevice::read(maxSize); }

    qint64 write(const QUnixSocketMessage &);
    QUnixSocketMessage read();

    virtual bool isSequential() const;
    virtual bool waitForReadyRead(int msec = 300);
    virtual bool waitForBytesWritten(int msec = 300);

Q_SIGNALS:
    void stateChanged(SocketState socketState);

protected:
    virtual qint64 readData(char * data, qint64 maxSize);
    virtual qint64 writeData (const char * data, qint64 maxSize);

private:
    QUnixSocket(const QUnixSocket &);
    QUnixSocket & operator=(const QUnixSocket &);

    QUnixSocketPrivate * d;
};

#endif // QUNIXSOCKET_P_H
