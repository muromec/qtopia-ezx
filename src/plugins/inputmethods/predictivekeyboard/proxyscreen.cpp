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

#include <QRegExp>
#include <qdebug.h>

#ifdef Q_WS_QWS

#include "proxyscreen.h"
ProxyScreen::ProxyScreen(int displayId)
    : QScreen(displayId), realScreen(0)
{
}

ProxyScreen::~ProxyScreen()
{
}

void ProxyScreen::setScreen(QScreen *screen)
{
    realScreen = screen;
    configure();
}

void ProxyScreen::configure()
{
    if (!realScreen)
        return;

    d = realScreen->depth();
    w = realScreen->width();
    h = realScreen->height();
    dw = realScreen->deviceWidth();
    dh = realScreen->deviceHeight();
    lstep = realScreen->linestep();
    data = realScreen->base();
    lstep = realScreen->linestep();
    size = realScreen->screenSize();
    physWidth = realScreen->physicalWidth();
    physHeight = realScreen->physicalHeight();

    setOffset(realScreen->offset());
    setPixelFormat(realScreen->pixelFormat());
}

static int getDisplayId(const QString &spec)
{
    QRegExp regexp(QLatin1String(":(\\d+)\\b"));
    if (regexp.lastIndexIn(spec) != -1) {
        const QString capture = regexp.cap(1);
        return capture.toInt();
    }
    return 0;
}

bool ProxyScreen::connect(const QString &displaySpec)
{
    const int id = getDisplayId(displaySpec);
    realScreen = qt_get_screen(id, displaySpec.toLatin1().constData());
    configure();

    return true;
}

void ProxyScreen::blit(const QImage &image, const QPoint &topLeft,
                       const QRegion &region)
{
    if (!realScreen) {
        QScreen::blit(image, topLeft, region);
        return;
    }

    realScreen->blit(image, topLeft, region);
}

void ProxyScreen::solidFill(const QColor &color, const QRegion &region)
{
    if (!realScreen) {
        QScreen::solidFill(color, region);
        return;
    }
    realScreen->solidFill(color, region);
}

QSize ProxyScreen::mapToDevice(const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapToDevice(s);

    return realScreen->mapToDevice(s);
}

QSize ProxyScreen::mapFromDevice(const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapFromDevice(s);

    return realScreen->mapFromDevice(s);
}

QPoint ProxyScreen::mapToDevice(const QPoint &p, const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapToDevice(p, s);

    return realScreen->mapToDevice(p, s);
}

QPoint ProxyScreen::mapFromDevice(const QPoint &p, const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapFromDevice(p, s);

    return realScreen->mapFromDevice(p, s);
}

QRect ProxyScreen::mapToDevice(const QRect &r, const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapToDevice(r, s);

    return realScreen->mapToDevice(r, s);
}

QRect ProxyScreen::mapFromDevice(const QRect &r, const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapFromDevice(r, s);

    return realScreen->mapFromDevice(r, s);
}

QRegion ProxyScreen::mapToDevice(const QRegion &r, const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapToDevice(r, s);

    return realScreen->mapToDevice(r, s);
}

QRegion ProxyScreen::mapFromDevice(const QRegion &r, const QSize &s) const
{
    if (!realScreen)
        return QScreen::mapFromDevice(r, s);

    return realScreen->mapFromDevice(r, s);
}

void ProxyScreen::disconnect()
{
    if (realScreen) {
        realScreen->disconnect();
        delete realScreen;
        realScreen = 0;
    }
}

void ProxyScreen::exposeRegion(QRegion region, int windowIndex)
{
    if (realScreen)
        realScreen->exposeRegion(region, windowIndex);
    else
        QScreen::exposeRegion(region, windowIndex);
}

bool ProxyScreen::initDevice()
{
    if (realScreen)
        return realScreen->initDevice();

    return false;
}

void ProxyScreen::shutdownDevice()
{
    if (realScreen)
        realScreen->shutdownDevice();
}

void ProxyScreen::setMode(int w,int h, int d)
{
    if (realScreen) {
        realScreen->setMode(w, h, d);
        configure();
        exposeRegion(region(), 0);
    }
}

bool ProxyScreen::supportsDepth(int depth) const
{
    if (realScreen)
        return realScreen->supportsDepth(depth);
    return false;
}

void ProxyScreen::save()
{
    if (realScreen)
        realScreen->save();
    QScreen::save();
}

void ProxyScreen::restore()
{
    if (realScreen)
        realScreen->restore();
    QScreen::restore();
}

void ProxyScreen::blank(bool on)
{
    if (realScreen)
        realScreen->blank(on);
}

bool ProxyScreen::onCard(const unsigned char *ptr) const
{
    if (realScreen)
        return realScreen->onCard(ptr);
    return false;
}

bool ProxyScreen::onCard(const unsigned char *ptr, ulong &offset) const
{
    if (realScreen)
        return realScreen->onCard(ptr, offset);
    return false;
}

bool ProxyScreen::isInterlaced() const
{
    if (realScreen)
        return realScreen->isInterlaced();
    return false;
}

bool ProxyScreen::isTransformed() const
{
    if (realScreen)
        return realScreen->isTransformed();
    return QScreen::isTransformed();
}

int ProxyScreen::memoryNeeded(const QString &str)
{
    if (realScreen)
        return realScreen->memoryNeeded(str);
    else
        return QScreen::memoryNeeded(str);
}

int ProxyScreen::sharedRamSize(void *ptr)
{
    if (realScreen)
        return realScreen->sharedRamSize(ptr);
    else
        return QScreen::sharedRamSize(ptr);
}

void ProxyScreen::haltUpdates()
{
    if (realScreen)
        realScreen->haltUpdates();
}

void ProxyScreen::resumeUpdates()
{
    if (realScreen)
        realScreen->resumeUpdates();
}

void ProxyScreen::setDirty(const QRect &rect)
{
    if (realScreen)
        realScreen->setDirty(rect);
}

QWSWindowSurface* ProxyScreen::createSurface(QWidget *widget) const
{
    if (realScreen)
        return realScreen->createSurface(widget);

    return QScreen::createSurface(widget);
}

QWSWindowSurface* ProxyScreen::createSurface(const QString &key) const
{
    if (realScreen)
        return realScreen->createSurface(key);

    return QScreen::createSurface(key);
}

QList<QScreen*> ProxyScreen::subScreens() const
{
    if (realScreen)
        return realScreen->subScreens();

    return QScreen::subScreens();
}

QRegion ProxyScreen::region() const
{
    if (realScreen)
        return realScreen->region();
    else
        return QScreen::region();
}

#endif
