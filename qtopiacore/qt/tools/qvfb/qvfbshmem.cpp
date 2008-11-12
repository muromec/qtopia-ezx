/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qlock_p.h"

#include "qvfbshmem.h"
#include "qvfbhdr.h"

#define QTE_PIPE "QtEmbedded-%1"

#include <QFile>
#include <QTimer>

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>


#ifdef Q_WS_QWS
#error qvfb must be compiled with  the Qt/X11 package
#endif

// Get the name of the directory where Qtopia Core temporary data should
// live.
static QString qws_dataDir(int qws_display_id)
{
    QByteArray dataDir = QString("/tmp/qtembedded-%1").arg(qws_display_id).toLocal8Bit();
    if (mkdir(dataDir, 0700)) {
        if (errno != EEXIST) {
            qFatal("Cannot create Qtopia Core data directory: %s", dataDir.constData());
        }
    }

    struct stat buf;
    if (lstat(dataDir, &buf))
        qFatal("stat failed for Qtopia Core data directory: %s", dataDir.constData());

    if (!S_ISDIR(buf.st_mode))
        qFatal("%s is not a directory", dataDir.constData());
    if (buf.st_uid != getuid())
        qFatal("Qtopia Core data directory is not owned by user %uh", getuid());

    if ((buf.st_mode & 0677) != 0600)
        qFatal("Qtopia Core data directory has incorrect permissions: %s", dataDir.constData());
    dataDir += "/";

    return QString(dataDir);
}


static QString displayPipe;
static QString displayPiped;
class DisplayLock
{
public:
    DisplayLock() : qlock(0) {
        if (QFile::exists(displayPiped)) {
            qlock = new QLock(displayPipe, 'd', false);
            qlock->lock(QLock::Read);
        }
    }
    ~DisplayLock() {
        if (qlock) {
            qlock->unlock();
            delete qlock;
            qlock = 0;
        }
    }
private:
    QLock *qlock;
};

QShMemViewProtocol::QShMemViewProtocol(int displayid, const QSize &s,
                                       int d, QObject *parent)
    : QVFbViewProtocol(displayid, parent), hdr(0), dataCache(0), lockId(-1)
{
    int actualdepth=d;

    switch ( d ) {
    case 12:
        actualdepth=16;
        break;
    case 1:
    case 4:
    case 8:
    case 16:
    case 18:
    case 24:
    case 32:
        break;

    default:
        qFatal("Unsupported bit depth %d\n", d);
    }

    int w = s.width();
    int h = s.height();

    QString username = "unknown";
    const char *logname = getenv("LOGNAME");
    if ( logname )
        username = logname;

    QString oldPipe = "/tmp/qtembedded-" + username + "/" + QString( QTE_PIPE ).arg( displayid );
    int oldPipeSemkey = ftok( oldPipe.toLatin1().constData(), 'd' );
    if (oldPipeSemkey != -1) {
        int oldPipeLockId = semget( oldPipeSemkey, 0, 0 );
        if (oldPipeLockId >= 0){
            sembuf sops;
            sops.sem_num = 0;
            sops.sem_op = 1;
            sops.sem_flg = SEM_UNDO;
            int rv;
            do {
                rv = semop(lockId,&sops,1);
            } while ( rv == -1 && errno == EINTR );
            qFatal("Cannot create lock file as an old version of QVFb has opened %s. Close other QVFb and try again", oldPipe.toLatin1().constData());
        }
    }

    kh = new QVFbKeyPipeProtocol(displayid);
    /* should really depend on receiving qt version, but how can
       one tell? */
    mh = new QVFbMousePipe(displayid);

    QString mousePipe = mh->pipeName();

    key_t key = ftok( mousePipe.toLatin1().constData(), 'b' );

    int bpl;
    if ( d == 1 )
	bpl = (w*d+7)/8;
    else if ( d == 18 )
        bpl = ((w*24+31)/32)*4;
    else
	bpl = ((w*actualdepth+31)/32)*4;

    displaySize = bpl * h;

    unsigned char *data;
    uint data_offset_value = sizeof(QVFbHeader);

    int dataSize = bpl * h + data_offset_value;
    shmId = shmget( key, dataSize, IPC_CREAT|0666);
    if ( shmId != -1 )
	data = (unsigned char *)shmat( shmId, 0, 0 );
    else {
	struct shmid_ds shm;
	shmctl( shmId, IPC_RMID, &shm );
	shmId = shmget( key, dataSize, IPC_CREAT|0666);
	if ( shmId == -1 )
	    qFatal( "Cannot get shared memory 0x%08x", key );
	data = (unsigned char *)shmat( shmId, 0, 0 );
    }

    if ( (long)data == -1 ){
        delete kh;
        delete mh;
	qFatal( "Cannot attach to shared memory %d",shmId );
    }
    dataCache = (unsigned char *)malloc(displaySize);
    memset(dataCache, 0, displaySize);
    memset(data+sizeof(QVFbHeader), 0, displaySize);

    hdr = (QVFbHeader *)data;
    hdr->width = w;
    hdr->height = h;
    hdr->depth = actualdepth;
    hdr->linestep = bpl;
    hdr->dataoffset = data_offset_value;
    hdr->update = QRect();
    hdr->dirty = 0;
    hdr->numcols = 0;
    hdr->viewerVersion = QT_VERSION;

    displayPipe = qws_dataDir(displayid) + QString( QTE_PIPE ).arg( displayid );

    displayPiped = displayPipe + 'd';


    mRefreshTimer = new QTimer( this );
    connect( mRefreshTimer, SIGNAL(timeout()), this, SLOT(flushChanges()) );
}

QShMemViewProtocol::~QShMemViewProtocol()
{
    struct shmid_ds shm;
    shmdt( (char*)hdr );
    shmctl( shmId, IPC_RMID, &shm );
    free(dataCache);
    delete kh;
    delete mh;
}

int QShMemViewProtocol::width() const
{
    return hdr->width;
}

int QShMemViewProtocol::height() const
{
    return hdr->height;
}

int QShMemViewProtocol::depth() const
{
    return hdr->depth;
}

int QShMemViewProtocol::linestep() const
{
    return hdr->linestep;
}

int  QShMemViewProtocol::numcols() const
{
    return hdr->numcols;
}

QVector<QRgb> QShMemViewProtocol::clut() const
{
    QVector<QRgb> vector(hdr->numcols);
    for (int i=0; i < hdr->numcols; ++i)
        vector[i]=hdr->clut[i];

    return vector;
}

unsigned char *QShMemViewProtocol::data() const
{
    return dataCache;
    //return ((unsigned char *)hdr)+hdr->dataoffset;
}

void QShMemViewProtocol::flushChanges()
{
    // based of dirty rect, copy changes from hdr to hdrcopy
    QRect r;
    {
        DisplayLock();
        if (hdr->dirty) {
            r = hdr->update;
            hdr->dirty = false;
            hdr->update = QRect();
            /* copy the memory area */
            /* for now, be inefficient. */
            memcpy(dataCache, ((char *)hdr) + hdr->dataoffset, displaySize);
        }
    }
    emit displayDataChanged(r);
}

void QShMemViewProtocol::setRate(int interval)
{
    if (interval > 0)
        return mRefreshTimer->start(1000/interval);
    else
        mRefreshTimer->stop();
}

int QShMemViewProtocol::rate() const
{
    int i = mRefreshTimer->interval();
    if (i > 0)
        return 1000/i;
    else
        return 0;
}
