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

#include "qapplication.h"

#ifndef QT_NO_SOUND

#include "qsound.h"
#include "qpaintdevice.h"
#include "qwsdisplay_qws.h"
#include "qsound_p.h"

#include "qsoundqss_qws.h"

#include "qhash.h"
#include "qfileinfo.h"

#ifdef MEDIA_SERVER
#include "qbytearray.h"
#include "quuid.h"
#include "qdatastream.h"
#include "qcopchannel_qws.h"

#define SERVER_CHANNEL "QPE/MediaServer"

class QCopMessage : public QDataStream
{
public:
    QCopMessage( const QString& channel, const QString& message )
        : QDataStream( new QBuffer ), m_channel( channel ), m_message( message )
    {
        device()->open( QIODevice::WriteOnly );
    }

    ~QCopMessage()
    {
        QCopChannel::send( m_channel, m_message, ((QBuffer*)device())->buffer() );
        delete device();
    }

private:
    QString m_channel;
    QString m_message;
};

#endif // MEDIA_SERVER

class QAuServerQWS;

class QAuBucketQWS : public QObject, public QAuBucket
{
    Q_OBJECT
public:
    QAuBucketQWS( QAuServerQWS*, QSound*, QObject* parent = 0 );

    ~QAuBucketQWS();

#ifndef MEDIA_SERVER
    int id() const { return id_; }
#endif

    QSound* sound() const { return sound_; }

#ifdef MEDIA_SERVER
    void play();

    void stop();
#endif

signals:
    // Only for Media Server
    void done( QAuBucketQWS* );

private slots:
    // Only for Media Server
    void processMessage( const QString& msg, const QByteArray& data );

private:
#ifdef MEDIA_SERVER
    QCopChannel *m_channel;
    QUuid m_id;
#endif

#ifndef MEDIA_SERVER
    int id_;
#endif
    QSound *sound_;
    QAuServerQWS *server_;

    static int next;
};

int QAuBucketQWS::next = 0;

class QAuServerQWS : public QAuServer
{
    Q_OBJECT
public:
    QAuServerQWS( QObject* parent );

    void init( QSound* s )
    {
        QAuBucketQWS *bucket = new QAuBucketQWS( this, s );
#ifdef MEDIA_SERVER
        connect( bucket, SIGNAL(done(QAuBucketQWS*)),
            this, SLOT(complete(QAuBucketQWS*)) );
#endif
        setBucket( s, bucket );
    }

#ifndef MEDIA_SERVER
    // Register bucket
    void insert( QAuBucketQWS *bucket )
    {
        buckets.insert( bucket->id(), bucket );
    }

    // Remove bucket from register
    void remove( QAuBucketQWS *bucket )
    {
        buckets.remove( bucket->id() );
    }
#endif

    void play( QSound* s )
    {
        QString filepath = QFileInfo( s->fileName() ).absoluteFilePath();
#if defined(QT_NO_QWS_SOUNDSERVER)
        server->playFile( bucket( s )->id(), filepath );
#elif defined(MEDIA_SERVER)
        bucket( s )->play();
#else
        client->play( bucket( s )->id(), filepath );
#endif
    }

    void stop( QSound* s )
    {
#if defined(QT_NO_QWS_SOUNDSERVER)
        server->stopFile( bucket( s )->id() );
#elif defined(MEDIA_SERVER)
        bucket( s )->stop();
#else
        client->stop( bucket( s )->id() );
#endif
    }

    bool okay() { return true; }

private slots:
    // Continue playing sound if loops remain
    void complete( int id )
    {
#ifndef MEDIA_SERVER
        QAuBucketQWS *bucket = find( id );
        if( bucket ) {
            QSound *sound = bucket->sound();
            if( decLoop( sound ) ) {
                play( sound );
            }
        }
#endif
    }

    // Only for Media Server
    void complete( QAuBucketQWS* bucket )
    {
#ifndef MEDIA_SERVER
        Q_UNUSED(bucket);
#else
        QSound *sound = bucket->sound();
        if( decLoop( sound ) ) {
            play( sound );
        }
#endif
    }

protected:
    QAuBucketQWS* bucket( QSound *s )
    {
        return (QAuBucketQWS*)QAuServer::bucket( s );
    }

private:
#ifndef MEDIA_SERVER
    // Find registered bucket with given id, return null if none found
    QAuBucketQWS* find( int id )
    {
        QHash<int, QAuBucketQWS*>::Iterator it = buckets.find( id );
        if( it != buckets.end() ) {
            return it.value();
        }

        return 0;
    }

    QHash<int, QAuBucketQWS*> buckets; // ### possible problem with overlapping keys

#ifdef QT_NO_QWS_SOUNDSERVER
    QWSSoundServer *server;
#else
    QWSSoundClient *client;
#endif

#endif // MEDIA_SERVER
};

QAuServerQWS::QAuServerQWS(QObject* parent) :
    QAuServer(parent)
{
#ifndef MEDIA_SERVER
    setObjectName(QLatin1String("qauserverqws"));

#ifdef QT_NO_QWS_SOUNDSERVER
    server = new QWSSoundServer( this ); // ### only suitable for single application

    connect( server, SIGNAL(soundCompleted(int)),
        this, SLOT(complete(int)) );
#else
    client = new QWSSoundClient( this ); // ### requires successful connection

    connect( client, SIGNAL(soundCompleted(int)),
        this, SLOT(complete(int)) );
#endif

#endif // MEDIA_SERVER
}

QAuBucketQWS::QAuBucketQWS( QAuServerQWS *server, QSound *sound, QObject* parent )
    : QObject( parent ), sound_( sound ), server_( server )
{
#ifdef MEDIA_SERVER
    m_id = QUuid::createUuid();

    sound->setObjectName( m_id.toString() );

    m_channel = new QCopChannel( QString( "QPE/QSound/" ).append( m_id ), this );
    connect( m_channel, SIGNAL(received(QString,QByteArray)),
        this, SLOT(processMessage(QString,QByteArray)) );

    {
        QCopMessage message( SERVER_CHANNEL, "subscribe(QUuid)" );
        message << m_id;
    }

    {
        QString filepath = QFileInfo( sound_->fileName() ).absoluteFilePath();
        QCopMessage message( SERVER_CHANNEL, "open(QUuid,QString)" );
        message << m_id << filepath;
    }
#else
    id_ = next++;
    server_->insert( this );
#endif
}

#ifdef MEDIA_SERVER
void QAuBucketQWS::play()
{
    QString filepath = QFileInfo( sound_->fileName() ).absoluteFilePath();

    QCopMessage message( SERVER_CHANNEL, "play(QUuid)" );
    message << m_id;
}

void QAuBucketQWS::stop()
{
    QCopMessage message( SERVER_CHANNEL, "stop(QUuid)" );
    message << m_id;
}
#endif // MEDIA_SERVER

void QAuBucketQWS::processMessage( const QString& msg, const QByteArray& data )
{
    Q_UNUSED(data);
#ifndef MEDIA_SERVER
    Q_UNUSED(msg);
#else
    if( msg == "done()" ) {
        emit done( this );
    }
#endif
}

QAuBucketQWS::~QAuBucketQWS()
{
#ifdef MEDIA_SERVER
    QCopMessage message( SERVER_CHANNEL, "revoke(QUuid)" );
    message << m_id;
#else
    server_->remove( this );
#endif
}


QAuServer* qt_new_audio_server()
{
    return new QAuServerQWS(qApp);
}

#include "qsound_qws.moc"

#endif // QT_NO_SOUND
