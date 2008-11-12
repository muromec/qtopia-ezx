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

#ifndef QTRANSPORTAUTH_QWS_H
#define QTRANSPORTAUTH_QWS_H

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qpointer.h>

#if !defined(QT_NO_SXE) || defined(SXE_INSTALLER)

#include <sys/types.h>

QT_BEGIN_HEADER

QT_MODULE(Gui)

class QAuthDevice;
class QWSClient;
class QIODevice;
class QTransportAuthPrivate;
class QMutex;

class Q_GUI_EXPORT QTransportAuth : public QObject
{
    Q_OBJECT
public:
    static QTransportAuth *getInstance();

    enum Result {
        // Error codes
        Pending = 0x00,
        TooSmall = 0x01,
        CacheMiss = 0x02,
        NoMagic = 0x03,
        NoSuchKey = 0x04,
        FailMatch = 0x05,
        OutOfDate = 0x06,
        // reserved for expansion
        Success = 0x1e,
        ErrMask = 0x1f,

        // Verification codes
        Allow = 0x20,
        Deny = 0x40,
        Ask = 0x60,
        // reserved
        StatusMask = 0xe0
    };

    enum Properties {
        Trusted = 0x01,
        Connection = 0x02,
        UnixStreamSock = 0x04,
        SharedMemory = 0x08,
        MessageQueue = 0x10,
        UDP = 0x20,
        TCP = 0x40,
        UserDefined = 0x80,
        TransportType = 0xfc
    };

    struct Data
    {
        Data() { processId = -1; }
        Data( unsigned char p, int d )
            : properties( p )
            , descriptor( d )
            , processId( -1 )
        {
            if (( properties & TransportType ) == TCP ||
                ( properties & TransportType ) == UnixStreamSock )
                properties |= Connection;
        }

        unsigned char properties;
        unsigned char progId;
        unsigned char status;
        unsigned int descriptor;   // socket fd or shmget key
        pid_t processId;

        bool trusted() const;
        void setTrusted( bool );
        bool connection() const;
        void setConnection( bool );
    };

    static const char *errorString( const Data & );

    Data *connectTransport( unsigned char, int );

    QAuthDevice *authBuf( Data *, QIODevice * );
    QAuthDevice *recvBuf( Data *, QIODevice * );
    QIODevice *passThroughByClient( QWSClient * ) const;

    void setKeyFilePath( const QString & );
    QString keyFilePath() const;
    const unsigned char *getClientKey( unsigned char progId );
    void invalidateClientKeyCache();
    QMutex *getKeyFileMutex();
    void setLogFilePath( const QString & );
    QString logFilePath() const;
    void setPackageRegistry( QObject *registry );
    bool isDiscoveryMode() const;
    void setProcessKey( const char * );
    void setProcessKey( const char *, const char * );
    void registerPolicyReceiver( QObject * );
    void unregisterPolicyReceiver( QObject * );

    bool authToMessage( QTransportAuth::Data &d, char *hdr, const char *msg, int msgLen );
    bool authFromMessage( QTransportAuth::Data &d, const char *msg, int msgLen );

    bool authorizeRequest( QTransportAuth::Data &d, const QString &request );

Q_SIGNALS:
    void policyCheck( QTransportAuth::Data &, const QString & );
    void authViolation( QTransportAuth::Data & );
private Q_SLOTS:
    void bufferDestroyed( QObject * );

private:
    // users should never construct their own
    QTransportAuth();
    ~QTransportAuth();

    friend class QAuthDevice;
    Q_DECLARE_PRIVATE(QTransportAuth)
};

QT_END_HEADER
#endif // QT_NO_SXE

#endif // QTRANSPORTAUTH_QWS_H
