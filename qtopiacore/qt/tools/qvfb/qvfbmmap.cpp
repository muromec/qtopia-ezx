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

#include "qvfbmmap.h"
#include "qvfbhdr.h"

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

QMMapViewProtocol::QMMapViewProtocol(int displayid, const QSize &s,
                                     int d, QObject *parent)
    : QVFbViewProtocol(displayid, parent), hdr(0), dataCache(0)
{
    int actualdepth = d;

    switch (d) {
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

    fileName = QString("/tmp/.qtvfb_map-%1").arg(displayid);

    int w = s.width();
    int h = s.height();


    kh = new QVFbKeyPipeProtocol(displayid);
    mh = new QVFbMouseLinuxTP(displayid);

    int bpl;
    if (d == 1)
	bpl = (w *d + 7) / 8;
    else if (d == 18)
        bpl = ((w * 24 + 31) / 32) * 4;
    else
	bpl = ((w * actualdepth + 31) / 32) * 4;

    displaySize = bpl * h;

    unsigned char *data;
    uint data_offset_value = sizeof(QVFbHeader);
    const int page_size = getpagesize();
    if (data_offset_value % page_size)
        data_offset_value += page_size - (data_offset_value % page_size);

    dataSize = bpl * h + data_offset_value;

    unlink(fileName.toLocal8Bit().data());
    fd = ::open( fileName.toLocal8Bit().data(), O_CREAT|O_RDWR, 0666 );
    ::lseek(fd, dataSize, SEEK_SET);
    ::write(fd, "\0", 1);
    if (fd < 0) {
        data = (unsigned char *)-1;
    } else {
        // might need to do something about size?
        data = (unsigned char *)mmap(NULL, dataSize, PROT_WRITE | PROT_READ,
                                     MAP_SHARED, fd, 0);
        if (data == MAP_FAILED)
            data = (unsigned char *)-1;
    }

    if ( (long)data == -1 ){
        delete kh;
        delete mh;
	qFatal( "Cannot attach to mapped file %s", fileName.toLocal8Bit().data());
    }
    dataCache = (unsigned char *)malloc(displaySize);
    memset(dataCache, 0, displaySize);
    memset(data+sizeof(QVFbHeader), 0, displaySize);

    hdr = (QVFbHeader *)data;
    hdr->width = w;
    hdr->height = h;
    hdr->depth = actualdepth;
    hdr->linestep = bpl;
    hdr->numcols = 0;
    hdr->dataoffset = data_offset_value;
    hdr->update = QRect();

    mRefreshTimer = new QTimer(this);
    connect(mRefreshTimer, SIGNAL(timeout()), this, SLOT(flushChanges()));
}

QMMapViewProtocol::~QMMapViewProtocol()
{
    munmap((char *)hdr, dataSize);
    ::close(fd);
    unlink(fileName.toLocal8Bit().constData());
    free(dataCache);
    delete kh;
    delete mh;
}

int QMMapViewProtocol::width() const
{
    return hdr->width;
}

int QMMapViewProtocol::height() const
{
    return hdr->height;
}

int QMMapViewProtocol::depth() const
{
    return hdr->depth;
}

int QMMapViewProtocol::linestep() const
{
    return hdr->linestep;
}

int  QMMapViewProtocol::numcols() const
{
    return hdr->numcols;
}

QVector<QRgb> QMMapViewProtocol::clut() const
{
    QVector<QRgb> vector(hdr->numcols);
    for (int i=0; i < hdr->numcols; ++i)
        vector[i] = hdr->clut[i];

    return vector;
}

unsigned char *QMMapViewProtocol::data() const
{
    return dataCache;
    //return ((unsigned char *)hdr)+hdr->dataoffset;
}

void QMMapViewProtocol::flushChanges()
{
    // based of dirty rect, copy changes from hdr to hdrcopy
    memcpy(dataCache, ((char *)hdr) + hdr->dataoffset, displaySize);
    emit displayDataChanged(QRect(0, 0, width(), height()));
}

void QMMapViewProtocol::setRate(int interval)
{
    if (interval > 0)
        return mRefreshTimer->start(1000/interval);
    else
        mRefreshTimer->stop();
}

int QMMapViewProtocol::rate() const
{
    int i = mRefreshTimer->interval();
    if (i > 0)
        return 1000/i;
    else
        return 0;
}
