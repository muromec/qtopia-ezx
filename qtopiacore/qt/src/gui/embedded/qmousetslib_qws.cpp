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

#include "qmousetslib_qws.h"

#if !defined(QT_NO_QWS_MOUSE_TSLIB) || defined(QT_PLUGIN)

#include "qsocketnotifier.h"
#include "qscreen_qws.h"

#include <tslib.h>
#include <errno.h>

#ifndef QT_QWS_TP_PRESSURE_THRESHOLD
#define QT_QWS_TP_PRESSURE_THRESHOLD 1
#endif

#ifndef QT_QWS_TP_JITTER_LIMIT
#define QT_QWS_TP_JITTER_LIMIT 3
#endif

/*!
    \internal

    \class QWSTslibMouseHandler
    \ingroup qws

    \brief The QWSTslibMouseHandler class implements a mouse driver
    for the Universal Touch Screen Library, tslib.

    QWSTslibMouseHandler inherits the QWSCalibratedMouseHandler class,
    providing calibration and noise reduction functionality in
    addition to generating mouse events, for devices using the
    Universal Touch Screen Library.

    To be able to compile this mouse handler, \l {Qtopia Core} must be
    configured with the \c -qt-mouse-tslib option, see the \l {Pointer
    Handling} documentation for details. In addition, the tslib
    headers and library must be present in the build environment.  The
    tslib sources can be downloaded from \l
    {http://tslib.berlios.de/}.  Use the \c -L and \c -I options
    with \c configure to explicitly specify the location of the
    library and its headers:

    \code
        configure  -L <path to tslib library> -I <path to tslib headers>
    \endcode

    In order to use this mouse handler, tslib must also be correctly
    installed on the target machine. This includes providing a \c
    ts.conf configuration file and setting the necessary environment
    variables, see the README file provided with tslib for details.

    The ts.conf file will usually contain the following two lines

    \code
        module_raw input
        module linear
    \endcode

    To make \l {Qtopia Core} explicitly choose the tslib mouse
    handler, set the QWS_MOUSE_PROTO environment variable.

    \sa {Pointer Handling}, {Qtopia Core}
*/

class QWSTslibMouseHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QWSTslibMouseHandlerPrivate(QWSTslibMouseHandler *h,
                                const QString &device);
    ~QWSTslibMouseHandlerPrivate();

    void suspend();
    void resume();

    void calibrate(const QWSPointerCalibrationData *data);
    void clearCalibration();

private:
    QWSTslibMouseHandler *handler;
    struct tsdev *dev;
    QSocketNotifier *mouseNotifier;

    struct ts_sample lastSample;
    bool wasPressed;
    int lastdx;
    int lastdy;

    bool calibrated;
    QString devName;

    void open();
    void close();
    inline bool get_sample(struct ts_sample *sample);

private slots:
    void readMouseData();
};

QWSTslibMouseHandlerPrivate::QWSTslibMouseHandlerPrivate(QWSTslibMouseHandler *h,
                                                         const QString &device)
    : handler(h)
{
    devName = device;

    if (devName.isNull()) {
        const char *str = getenv("TSLIB_TSDEVICE");
        if (str)
            devName = QString::fromLocal8Bit(str);
    }
    if (devName.isNull())
        devName = QLatin1String("/dev/ts");

    open();
    calibrated = true;

    int fd = ts_fd(dev);
    mouseNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(mouseNotifier, SIGNAL(activated(int)),this, SLOT(readMouseData()));
    resume();
}

QWSTslibMouseHandlerPrivate::~QWSTslibMouseHandlerPrivate()
{
    close();
}

void QWSTslibMouseHandlerPrivate::open()
{
    dev = ts_open(devName.toLocal8Bit().constData(), 1);
    if (!dev) {
        qCritical("QWSTslibMouseHandlerPrivate: ts_open() failed"
                  " with error: '%s'", strerror(errno));
        qCritical("Please check your tslib installation!");
        return;
    }

    if (ts_config(dev)) {
        qCritical("QWSTslibMouseHandlerPrivate: ts_config() failed"
                  " with error: '%s'", strerror(errno));
        qCritical("Please check your tslib installation!");
        close();
        return;
    }
}

void QWSTslibMouseHandlerPrivate::close()
{
    if (dev)
        ts_close(dev);
}

void QWSTslibMouseHandlerPrivate::suspend()
{
    mouseNotifier->setEnabled(false);
}

void QWSTslibMouseHandlerPrivate::resume()
{
    memset(&lastSample, 0, sizeof(lastSample));
    wasPressed = false;
    lastdx = 0;
    lastdy = 0;
    mouseNotifier->setEnabled(true);
}

bool QWSTslibMouseHandlerPrivate::get_sample(struct ts_sample *sample)
{
    if (!calibrated)
        return (ts_read_raw(dev, sample, 1) == 1);

    return (ts_read(dev, sample, 1) == 1);
}

void QWSTslibMouseHandlerPrivate::readMouseData()
{
    if(!qt_screen)
        return;

    for(;;) {
        struct ts_sample sample = lastSample;
        bool pressed = wasPressed;

        // Fast return if there's no events.
        if (!get_sample(&sample))
            return;
        pressed = (sample.pressure >= QT_QWS_TP_PRESSURE_THRESHOLD);

        // Only return last sample unless there's a press/release event.
        while (pressed == wasPressed) {
            if (!get_sample(&sample))
                break;
            pressed = (sample.pressure >= QT_QWS_TP_PRESSURE_THRESHOLD);
        }

        // work around missing coordinates on mouse release in raw mode
        if (!calibrated && !pressed && sample.x == 0 && sample.y == 0) {
            sample.x = lastSample.x;
            sample.y = lastSample.y;
        }

        int dx = sample.x - lastSample.x;
        int dy = sample.y - lastSample.y;

        // Remove small movements in oppsite direction
        if (dx * lastdx < 0 && qAbs(dx) < QT_QWS_TP_JITTER_LIMIT) {
            sample.x = lastSample.x;
            dx = 0;
        }
        if (dy * lastdy < 0 && qAbs(dy) < QT_QWS_TP_JITTER_LIMIT) {
            sample.y = lastSample.y;
            dy = 0;
        }

        if (wasPressed == pressed && dx == 0 && dy == 0)
            return;

#ifdef TSLIBMOUSEHANDLER_DEBUG
        qDebug() << "last" << QPoint(lastSample.x, lastSample.y)
                 << "curr" << QPoint(sample.x, sample.y)
                 << "dx,dy" << QPoint(dx, dy)
                 << "ddx,ddy" << QPoint(dx*lastdx, dy*lastdy)
                 << "pressed" << wasPressed << pressed;
#endif

        lastSample = sample;
        wasPressed = pressed;
        if (dx != 0)
            lastdx = dx;
        if (dy != 0)
            lastdy = dy;

        // tslib should do all the translation and filtering, so we send a
        // "raw" mouse event
        handler->QWSMouseHandler::mouseChanged(QPoint(sample.x, sample.y),
                                               pressed);
    }
}

void QWSTslibMouseHandlerPrivate::clearCalibration()
{
    suspend();
    close();
    handler->QWSCalibratedMouseHandler::clearCalibration();
    calibrated = false;
    open();
    resume();
}

void QWSTslibMouseHandlerPrivate::calibrate(const QWSPointerCalibrationData *data)
{
    suspend();
    close();
    // default implementation writes to /etc/pointercal
    // using the same format as the tslib linear module.
    handler->QWSCalibratedMouseHandler::calibrate(data);
    calibrated = true;
    open();
    resume();
}

/*!
    \internal
*/
QWSTslibMouseHandler::QWSTslibMouseHandler(const QString &driver,
                                           const QString &device)
    : QWSCalibratedMouseHandler(driver, device)
{
    d = new QWSTslibMouseHandlerPrivate(this, device);
}

/*!
    \internal
*/
QWSTslibMouseHandler::~QWSTslibMouseHandler()
{
    delete d;
}

/*!
    \reimp
*/
void QWSTslibMouseHandler::suspend()
{
    d->suspend();
}

/*!
    \reimp
*/
void QWSTslibMouseHandler::resume()
{
    d->resume();
}

/*!
    \reimp
*/
void QWSTslibMouseHandler::clearCalibration()
{
    d->clearCalibration();
}

/*!
    \reimp
*/
void QWSTslibMouseHandler::calibrate(const QWSPointerCalibrationData *data)
{
    d->calibrate(data);
}

#include "qmousetslib_qws.moc"
#endif //QT_NO_QWS_MOUSE_TSLIB
