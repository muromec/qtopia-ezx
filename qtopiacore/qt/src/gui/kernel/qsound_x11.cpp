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

#include "qsound.h"

#ifndef QT_NO_SOUND

#include "qhash.h"
#include "qsocketnotifier.h"
#include "qapplication.h"
#include "qsound_p.h"

#ifndef QT_NO_NAS

#include <audio/audiolib.h>
#include <audio/soundlib.h>

static AuServer *nas=0;

static AuBool eventPred(AuServer *, AuEvent *e, AuPointer p)
{
    if (e && (e->type == AuEventTypeElementNotify)) {
        if (e->auelementnotify.flow == *((AuFlowID *)p))
            return true;
    }
    return false;
}

class QAuBucketNAS : public QAuBucket {
public:
    QAuBucketNAS(AuBucketID b, AuFlowID f = 0) : id(b), flow(f), stopped(true), numplaying(0) { }
    ~QAuBucketNAS()
    {
        if (nas) {
            AuSync(nas, false);
            AuDestroyBucket(nas, id, NULL);

            AuEvent ev;
            while (AuScanEvents(nas, AuEventsQueuedAfterFlush, true, eventPred, &flow, &ev))
                ;
        }
    }

    AuBucketID id;
    AuFlowID flow;
    bool     stopped;
    int      numplaying;
};

class QAuServerNAS : public QAuServer {
    Q_OBJECT

    QSocketNotifier* sn;

public:
    QAuServerNAS(QObject* parent);
    ~QAuServerNAS();

    void init(QSound*);
    void play(const QString& filename);
    void play(QSound*);
    void stop(QSound*);
    bool okay();
    void setDone(QSound*);

public slots:
    void dataReceived();
    void soundDestroyed(QObject *o);

private:
    QAuBucketNAS* bucket(QSound* s)
    {
        return (QAuBucketNAS*)QAuServer::bucket(s);
    }
};

QAuServerNAS::QAuServerNAS(QObject* parent) :
    QAuServer(parent)
{
    setObjectName(QLatin1String("Network Audio System"));
    nas = AuOpenServer(NULL, 0, NULL, 0, NULL, NULL);
    if (nas) {
        AuSetCloseDownMode(nas, AuCloseDownDestroy, NULL);
        // Ask Qt for async messages...
        sn=new QSocketNotifier(AuServerConnectionNumber(nas),
                QSocketNotifier::Read);
        QObject::connect(sn, SIGNAL(activated(int)),
                this, SLOT(dataReceived()));
    } else {
        sn = 0;
    }
}

QAuServerNAS::~QAuServerNAS()
{
    if (nas)
        AuCloseServer(nas);
    delete sn;
    nas = 0;
}

typedef QHash<void*,QAuServerNAS*> AuServerHash;
static AuServerHash *inprogress=0;

void QAuServerNAS::soundDestroyed(QObject *o)
{
    if (inprogress) {
        QSound *so = static_cast<QSound *>(o);
        while (inprogress->remove(so))
            ; // Loop while remove returns true
    }
}

void QAuServerNAS::play(const QString& filename)
{
    if (nas) {
        int iv=100;
        AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
        AuSoundPlayFromFile(nas, filename.toLocal8Bit().constData(), AuNone, volume,
                            NULL, NULL, NULL, NULL, NULL, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flush();
    }
}

static void callback(AuServer*, AuEventHandlerRec*, AuEvent* e, AuPointer p)
{
    if (inprogress->contains(p) && e) {
        if (e->type==AuEventTypeElementNotify &&
                    e->auelementnotify.kind==AuElementNotifyKindState) {
            if (e->auelementnotify.cur_state == AuStateStop) {
                AuServerHash::Iterator it = inprogress->find(p);
                if (it != inprogress->end())
                    (*it)->setDone((QSound*)p);
            }
        }
    }
}

void QAuServerNAS::setDone(QSound* s)
{
    if (nas) {
        decLoop(s);
        if (s->loopsRemaining() && !bucket(s)->stopped) {
            bucket(s)->stopped = true;
            play(s);
        } else {
            if (--(bucket(s)->numplaying) == 0)
                bucket(s)->stopped = true;
            inprogress->remove(s);
        }
    }
}

void QAuServerNAS::play(QSound* s)
{
    if (nas) {
        ++(bucket(s)->numplaying);
        if (!bucket(s)->stopped) {
            stop(s);
        }

        bucket(s)->stopped = false;
        if (!inprogress)
            inprogress = new AuServerHash;
        inprogress->insert(s,this);
        int iv=100;
        AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
        QAuBucketNAS *b = bucket(s);
        AuSoundPlayFromBucket(nas, b->id, AuNone, volume,
                              callback, s, 0, &b->flow, NULL, NULL, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flush();
    }
}

void QAuServerNAS::stop(QSound* s)
{
    if (nas && !bucket(s)->stopped) {
        bucket(s)->stopped = true;
        AuStopFlow(nas, bucket(s)->flow, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flush();
    }
}

void QAuServerNAS::init(QSound* s)
{
    connect(s, SIGNAL(destroyed(QObject*)),
            this, SLOT(soundDestroyed(QObject*)));

    if (nas) {
        AuBucketID b_id =
            AuSoundCreateBucketFromFile(nas, s->fileName().toLocal8Bit().constData(),
                                        0 /*AuAccessAllMasks*/, NULL, NULL);
        setBucket(s, new QAuBucketNAS(b_id));
    }
}

bool QAuServerNAS::okay()
{
    return !!nas;
}

void QAuServerNAS::dataReceived()
{
    AuHandleEvents(nas);
}

#include "qsound_x11.moc"

#endif


class QAuServerNull : public QAuServer
{
public:
    QAuServerNull(QObject* parent);

    void play(const QString&) { }
    void play(QSound*s) { while(decLoop(s) > 0) /* nothing */ ; }
    void stop(QSound*) { }
    bool okay() { return false; }
};

QAuServerNull::QAuServerNull(QObject* parent)
    : QAuServer(parent)
{
}


QAuServer* qt_new_audio_server()
{
#ifndef QT_NO_NAS
    QAuServer* s = new QAuServerNAS(qApp);
    if (s->okay())
        return s;
    else
        delete s;
#endif
    return new QAuServerNull(qApp);
}

#endif // QT_NO_SOUND
