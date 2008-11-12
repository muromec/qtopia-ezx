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

#ifndef QTRANSPORTAUTH_QWS_P_H
#define QTRANSPORTAUTH_QWS_P_H

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

#include "qtransportauth_qws.h"
#include "qbuffer.h"

#if !defined(QT_NO_SXE) || defined(SXE_INSTALLER)

#include <qmutex.h>
#include <qdatetime.h>
#include "private/qobject_p.h"

#include <QtCore/qcache.h>

// Uncomment to generate debug output
// #define QTRANSPORTAUTH_DEBUG 1

#ifdef QTRANSPORTAUTH_DEBUG
void hexstring( char *buf, const unsigned char* key, size_t sz );
#endif

// proj id for ftok usage in sxe
#define SXE_PROJ 10022

/*!
  \internal
  memset for security purposes, guaranteed not to be optimized away
  http://www.faqs.org/docs/Linux-HOWTO/Secure-Programs-HOWTO.html
*/
void *guaranteed_memset(void *v,int c,size_t n);

class QUnixSocketMessage;

class Q_GUI_EXPORT RequestAnalyzer
{
public:
    RequestAnalyzer();
    virtual ~RequestAnalyzer();
    QString operator()( QByteArray *data ) { return analyze( data ); }
    bool requireMoreData() const { return moreData; }
    qint64 bytesAnalyzed() const { return dataSize; }
protected:
    virtual QString analyze( QByteArray * );
    bool moreData;
    qint64 dataSize;
};

/*!
  \internal
  \class QAuthDevice

  \brief Pass-through QIODevice sub-class for authentication.

   Use this class to forward on or receive forwarded data over a real
   device for authentication.
*/
class Q_GUI_EXPORT QAuthDevice : public QIODevice
{
    Q_OBJECT
public:
    enum AuthDirection {
        Receive,
        Send
    };
    QAuthDevice( QIODevice *, QTransportAuth::Data *, AuthDirection );
    ~QAuthDevice();
    void setTarget( QIODevice *t ) { m_target = t; }
    QIODevice *target() const { return m_target; }
    void setClient( QObject* );
    QObject *client() const;
    void setRequestAnalyzer( RequestAnalyzer * );
    bool isSequential() const;
    bool atEnd() const;
    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;
    bool seek( qint64 );
    QByteArray & buffer();

protected:
    qint64 readData( char *, qint64 );
    qint64 writeData(const char *, qint64 );
private Q_SLOTS:
    void recvReadyRead();
    void targetBytesWritten( qint64 );
private:
    bool authorizeMessage();

    QTransportAuth::Data *d;
    AuthDirection way;
    QIODevice *m_target;
    QObject *m_client;
    QByteArray msgQueue;
    qint64 m_bytesAvailable;
    qint64 m_skipWritten;

    RequestAnalyzer *analyzer;
};

inline bool QAuthDevice::isSequential() const
{
    return true;
}

inline bool QAuthDevice::seek( qint64 )
{
    return false;
}

inline bool QAuthDevice::atEnd() const
{
    return msgQueue.isEmpty();
}

inline qint64 QAuthDevice::bytesAvailable() const
{
    if ( way == Receive )
        return m_bytesAvailable;
    else
        return ( m_target ? m_target->bytesAvailable() : 0 );
}

inline qint64 QAuthDevice::bytesToWrite() const
{
    return msgQueue.size();
}

inline QByteArray &QAuthDevice::buffer()
{
    return msgQueue;
}

#define QSXE_KEY_LEN 16
#define QSXE_MAGIC_BYTES 4

// Number of bytes of each message to authenticate.  Just need to ensure
// that the command at the beginning hasn't been tampered with.  This value
// does not matter for trusted transports.
#define AMOUNT_TO_AUTHENTICATE 200

/*!
  \internal
  \class AuthCookie
  Struct to carry process authentication key and id
*/
#define QSXE_HEADER_LEN 24

/*!
  \macro AUTH_ID
  Macro to manage authentication header.  Format of header is:
  \table
  \header \i BYTES  \i  CONTENT
     \row \i 0-3    \i  magic numbers
     \row \i 4      \i  length of authenticated data (max 255 bytes)
     \row i\ 5      \i  reserved
     \row \i 6-21   \i  MAC digest, or shared secret in case of simple auth
     \row \i 22     \i  program id
     \row \i 23     \i  sequence number
  \endtable
  Total length of the header is 24 bytes

  However this may change.  Instead of coding these numbers use the AUTH_ID,
  AUTH_KEY, AUTH_DATA and AUTH_SPACE macros.
*/

#define AUTH_ID(k) ((unsigned char)(k[QSXE_KEY_LEN]))
#define AUTH_KEY(k) ((unsigned char *)(k))

// must be a largish -ve number under any endianess when cast as an int
const unsigned char magic[QSXE_MAGIC_BYTES] = { 0xBA, 0xD4, 0xD4, 0xBA };
const int magicInt = 0xBAD4D4BA;

#define AUTH_DATA(x) (unsigned char *)((x) + QSXE_HEADER_LEN)
#define AUTH_SPACE(x) ((x) + QSXE_HEADER_LEN)
#define QSXE_LEN_IDX 4
#define QSXE_KEY_IDX 6
#define QSXE_PROG_IDX 22
#define QSXE_SEQ_IDX 23

#define QSXE_KEYFILE "keyfile"

/*
  Header in above format, less the magic bytes.
  Useful for reading off the socket
*/
struct AuthHeader
{
    unsigned char len;
    unsigned char pad;
    unsigned char digest[QSXE_KEY_LEN];
    unsigned char id;
    unsigned char seq;
};

/*
  Header in a form suitable for authentication routines
*/
struct AuthMessage
{
    AuthMessage()
    {
        ::memset( authData, 0, sizeof(authData) );
        ::memcpy( pad_magic, magic, QSXE_MAGIC_BYTES );
    }
    unsigned char pad_magic[QSXE_MAGIC_BYTES];
    union {
        AuthHeader hdr;
        char authData[sizeof(AuthHeader)];
    };
    char payLoad[AMOUNT_TO_AUTHENTICATE];
};

/**
  Auth data as stored in _key
*/
struct AuthCookie
{
    unsigned char key[QSXE_KEY_LEN];
    unsigned char pad;
    unsigned char progId;
};

/*
  Auth data as written to the key file - SUPERSEDED by usr_key_entry

  This is still used internally for some functions, ie the socket
  related calls.
*/
struct AuthRecord
{
    union {
        AuthCookie auth;
        char data[sizeof(struct AuthCookie)];
    };
    time_t change_time;
};

/*!
  \class usr_key_entry
  This comes from the SXE kernel patch file include/linux/lidsif.h

  This is the (new) data record for the key file (version 2).

  The key file is (now) either /proc/lids/keys (and the per-process
  keys in /proc/<pid>/lids_key) OR for desktop/development ONLY (not
  for production) it is $QPEDIR/etc/keyfile

  The key file maps keys to files.

  File are identified by inode and device numbers, not paths.

  (See the "installs" file for path to inode/device mapping)
*/
struct usr_key_entry
{
    char key[QSXE_KEY_LEN];
    ino_t ino;
    dev_t dev;
};


/*!
  \class IdBlock
  \brief Data record for the manifest file.
  The manifest file maps program id's to files
*/
struct IdBlock
{
    quint64 inode;
    quint64 device;
    unsigned char pad;
    unsigned char progId;
    unsigned short installId;
    unsigned int keyOffset;
    qint64 install_time;
};

class SxeRegistryLocker : public QObject
{
    Q_OBJECT
public:
    SxeRegistryLocker( QObject * );
    ~SxeRegistryLocker();
    bool success() const { return m_success; }
private:
    bool m_success;
    QObject *m_reg;
};

class QTransportAuthPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTransportAuth)
public:
    QTransportAuthPrivate();
    ~QTransportAuthPrivate();

    const unsigned char *getClientKey( unsigned char progId );
    void invalidateClientKeyCache();

    bool keyInitialised;
    QString m_logFilePath;
    QString m_keyFilePath;
    QObject *m_packageRegistry;
    AuthCookie authKey;
    QCache<unsigned char, char> keyCache;
    QHash< QObject*, QIODevice*> buffersByClient;
    QMutex keyfileMutex;
};

/*!
  \internal
  Enforces the False Authentication Rate.  If more than 4 authentications
  are received per minute the sxemonitor is notified that the FAR has been exceeded
*/
class FAREnforcer
{
    public:
        static FAREnforcer *getInstance();
        void logAuthAttempt( QDateTime time = QDateTime::currentDateTime() );
        void reset();

    #ifndef TEST_FAR_ENFORCER
    private:
    #endif
        FAREnforcer();
        FAREnforcer( const FAREnforcer & );
        FAREnforcer &operator=(FAREnforcer const & );
        
        static const QString FARMessage;
        static const int minutelyRate;
        static const QString SxeTag;
        static const int minute;
        
        QList<QDateTime> authAttempts;
};

#endif // QT_NO_SXE
#endif // QTRANSPORTAUTH_QWS_P_H

