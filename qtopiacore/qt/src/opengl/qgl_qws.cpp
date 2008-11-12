/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include "qgl.h"

#if defined(Q_WS_QWS)
#include <GLES/egl.h>
#include <GLES/gl.h>
#include <qdirectpainter_qws.h>

#include <qglscreen_qws.h>
#include <private/qglwindowsurface_qws_p.h>

#endif
#include <private/qbackingstore_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qgl_p.h>
#include <private/qpaintengine_opengl_p.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qstack.h>
#include <qdesktopwidget.h>
#include <qdebug.h>
#include <qvarlengtharray.h>


#define Q_USE_QEGL
//#define Q_USE_DIRECTPAINTER

// this one for full QScreen implemented using EGL/GLES:
//#define Q_USE_EGLWINDOWSURFACE
#ifdef Q_USE_QEGL
#include "qegl_qws_p.h"

#ifdef Q_USE_EGLWINDOWSURFACE
#include "private/qglwindowsurface_qws_p.h"
#endif
#endif

/*****************************************************************************
  QOpenGL debug facilities
 *****************************************************************************/
//#define DEBUG_OPENGL_REGION_UPDATE

bool QGLFormat::hasOpenGL()
{
    return true;
}


bool QGLFormat::hasOpenGLOverlays()
{
#ifdef Q_USE_EGLWINDOWSURFACE
    return false;
#else
    return true;
#endif
}

#define QT_EGL_CHECK(x)                          \
    if (!(x)) {                                  \
        EGLint err = eglGetError();              \
        printf("egl " #x " failure %x!\n", err); \
    }                                            \

#define QT_EGL_ERR(txt)                         \
    do {                                        \
        EGLint err = eglGetError();             \
        if (err != EGL_SUCCESS)                 \
            printf( txt " failure %x!\n", err); \
    } while (0)


#define QT_EGL_CHECK_ATTR(attr, val)     success = eglGetConfigAttrib(dpy, config, attr, &value); \
    if (!success || value != val)                                       \
        return false

#if defined(Q_USE_QEGL) && !defined(Q_USE_EGLWINDOWSURFACE)
static bool checkConfig(EGLDisplay dpy, EGLConfig config, int r, int g, int b, int a)
{
    EGLint value;
    EGLBoolean success;
    QT_EGL_CHECK_ATTR(EGL_RED_SIZE, r);
    QT_EGL_CHECK_ATTR(EGL_GREEN_SIZE, g);
    QT_EGL_CHECK_ATTR(EGL_BLUE_SIZE, b);
    QT_EGL_CHECK_ATTR(EGL_ALPHA_SIZE, a);
    return true;
}
#endif

bool QGLContext::chooseContext(const QGLContext* shareContext)
{
#ifdef Q_USE_EGLWINDOWSURFACE
    // EGL Only works if drawable is a QGLWidget. QGLPixelBuffer not supported
    if (device() && device()->devType() == QInternal::Widget)
        return static_cast<QGLScreen*>(QScreen::instance())->chooseContext(this, shareContext);
    else
        return false;
#else
    Q_D(QGLContext);
    d->cx = 0;

    EGLint matchingConfigs;
    EGLint configAttribs[] = {
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_ALPHA_MASK_SIZE, 8,
        EGL_DEPTH_SIZE,     16,
        EGL_STENCIL_SIZE,   EGL_DONT_CARE,
        EGL_SURFACE_TYPE,   EGL_WINDOW_BIT,
        EGL_NONE,           EGL_NONE
    };

    if (d->paintDevice->devType() == QInternal::Image) {
        QImage *img = static_cast<QImage*>(d->paintDevice);
        if (img->format() == QImage::Format_RGB16) {
            configAttribs[1] = 5;
            configAttribs[3] = 6;
            configAttribs[5] = 5;
            configAttribs[7] = 0;
        }
    } else if (d->paintDevice->devType() == QInternal::Widget) {
#ifdef Q_USE_QEGL
        if (qt_screen->pixmapDepth() == 16) {
            configAttribs[1] = 5;
            configAttribs[3] = 6;
            configAttribs[5] = 5;
            configAttribs[7] = 0;
        }
        configAttribs[15] = EGL_PIXMAP_BIT;
#else
        configAttribs[1] = 0;
        configAttribs[3] = 0;
        configAttribs[5] = 0;
        configAttribs[7] = 0;
        configAttribs[9] = 0;
#endif
    }
    if (deviceIsPixmap() || d->paintDevice->devType() == QInternal::Image)
        configAttribs[15] = EGL_PIXMAP_BIT;

    //Ask for an available display
    d->dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    QT_EGL_CHECK(d->dpy);

    //Display initialization (don't care about the OGLES version numbers)
    if (!eglInitialize(d->dpy, NULL, NULL))
        return false;

    eglBindAPI(EGL_OPENGL_ES_API);

    if (!eglChooseConfig(d->dpy, configAttribs, 0, 0, &matchingConfigs))
        return false;

    //If there isn't any configuration good enough
    if (matchingConfigs < 1)
        return false;

    QVarLengthArray<EGLConfig> configs(matchingConfigs);

    if (!eglChooseConfig(d->dpy, configAttribs, configs.data(),
                         matchingConfigs, &matchingConfigs))
        return false;
#ifdef Q_USE_QEGL
    bool found = false;
    for (int i = 0; i < matchingConfigs; ++i) {
        if (checkConfig(d->dpy, configs[i], configAttribs[1], configAttribs[3], configAttribs[5], configAttribs[7])) {
            d->config = configs[i];
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning("QGLContext::chooseContext none of the %d 'matching' configs actually match", matchingConfigs);
        return false;
    }
#else
    d->config = configs[0];
#endif


    GLint res;
    eglGetConfigAttrib(d->dpy, d->config, EGL_LEVEL,&res);
    d->glFormat.setPlane(res);
    QT_EGL_ERR("eglGetConfigAttrib");
    /*
    if(deviceIsPixmap())
        res = 0;
    else
    eglDescribePixelFormat(fmt, EGL_DOUBLEBUFFER, &res);
    d->glFormat.setDoubleBuffer(res);
    */

    eglGetConfigAttrib(d->dpy,d->config, EGL_DEPTH_SIZE, &res);
    d->glFormat.setDepth(res);
    if (d->glFormat.depth())
        d->glFormat.setDepthBufferSize(res);

    //eglGetConfigAttrib(d->dpy,d->config, EGL_RGBA, &res);
    //d->glFormat.setRgba(res);

    eglGetConfigAttrib(d->dpy,d->config, EGL_ALPHA_SIZE, &res);
    d->glFormat.setAlpha(res);
    if (d->glFormat.alpha())
        d->glFormat.setAlphaBufferSize(res);

    //eglGetConfigAttrib(d->dpy,d->config, EGL_ACCUM_RED_SIZE, &res);
    //d->glFormat.setAccum(res);
    //if (d->glFormat.accum())
    //    d->glFormat.setAccumBufferSize(res);

    eglGetConfigAttrib(d->dpy, d->config, EGL_STENCIL_SIZE, &res);
    d->glFormat.setStencil(res);
    if (d->glFormat.stencil())
        d->glFormat.setStencilBufferSize(res);

    //eglGetConfigAttrib(d->dpy, d->config, EGL_STEREO, &res);
    //d->glFormat.setStereo(res);

    eglGetConfigAttrib(d->dpy, d->config, EGL_SAMPLE_BUFFERS, &res);
    d->glFormat.setSampleBuffers(res);

    if (d->glFormat.sampleBuffers()) {
        eglGetConfigAttrib(d->dpy, d->config, EGL_SAMPLES, &res);
        d->glFormat.setSamples(res);
    }

    if(shareContext &&
       (!shareContext->isValid() || !shareContext->d_func()->cx)) {
        qWarning("QGLContext::chooseContext(): Cannot share with invalid context");
        shareContext = 0;
    }

    EGLContext ctx = eglCreateContext(d->dpy, d->config, 0, 0);
                                      //(shareContext ? shareContext->d_func()->cx : 0),
                                      //configAttribs);
    if(!ctx) {
        GLenum err = eglGetError();
        qDebug("eglCreateContext err %x", err);
        if(err == EGL_BAD_MATCH || err == EGL_BAD_CONTEXT) {
            if(shareContext && shareContext->d_func()->cx) {
                qWarning("QGLContext::chooseContext(): Context sharing mismatch!");
                if(!(ctx = eglCreateContext(d->dpy, d->config, 0, configAttribs)))
                    return false;
                shareContext = 0;
            }
        }
        if(!ctx) {
            qWarning("QGLContext::chooseContext(): Unable to create QGLContext");
            return false;
        }
    }
    d->cx = ctx;
    if (shareContext && shareContext->d_func()->cx) {
        QGLContext *share = const_cast<QGLContext *>(shareContext);
        d->sharing = true;
        share->d_func()->sharing = true;
    }

    // vblank syncing
    GLint interval = d->reqFormat.swapInterval();
    if (interval != -1) {
        if (interval != 0)
            eglSwapInterval(d->dpy, interval);
    }

    if (deviceIsPixmap() || d->paintDevice->devType() == QInternal::Image) {
        d->surface = eglCreatePixmapSurface(d->dpy, d->config,
                                            (NativeWindowType)d->paintDevice,
                                            configAttribs);
    } else {
#ifndef Q_USE_QEGL
        d->surface = eglCreateWindowSurface(d->dpy, d->config,
                                            (NativeWindowType)d->paintDevice, 0);
        if (!d->surface) {
             GLenum err = eglGetError();
             qDebug("eglCreateWindowSurface err %x", err);
        }
        qDebug() << "create Window surface" << d->surface;
#endif
    }


    if (!d->surface)
        return false;
    return true;
#endif
}


void QGLContext::reset()
{
    Q_D(QGLContext);
    if (!d->valid)
        return;
    doneCurrent();
#ifndef Q_USE_EGLWINDOWSURFACE
    if (d->cx)
        eglDestroyContext(d->dpy, d->cx);
    d->crWin = false;
#endif
    d->cx = 0;
    d->sharing = false;
    d->valid = false;
    d->transpColor = QColor();
    d->initDone = false;
    qgl_share_reg()->removeShare(this);
}

void QGLContext::makeCurrent()
{
    Q_D(QGLContext);
    if(!d->valid) {
        qWarning("QGLContext::makeCurrent(): Cannot make invalid context current");
        return;
    }

#ifndef Q_USE_EGLWINDOWSURFACE
    bool ok = eglMakeCurrent(d->dpy, d->surface, d->surface, d->cx);
    if (!ok) {
        EGLint err = eglGetError();
        qWarning("QGLContext::makeCurrent(): Failed %x.", err);
    }
    if (ok) {
#endif
        if (!qgl_context_storage.hasLocalData() && QThread::currentThread())
            qgl_context_storage.setLocalData(new QGLThreadContext);
        if (qgl_context_storage.hasLocalData())
            qgl_context_storage.localData()->context = this;
        currentCtx = this;
#ifndef Q_USE_EGLWINDOWSURFACE
    }
#else
#if 0
    if (device()->devType() == QInternal::Widget) {
        // EGL Only works if drawable is a QGLWidget. QGLFramebufferObject, QGLPixelBuffer not supported
        static_cast<QGLWidget*>(device())->d_func()->wsurf->beginPaint(QRegion());
    }
#endif
#endif
}

void QGLContext::doneCurrent()
{
#ifndef Q_USE_EGLWINDOWSURFACE
    Q_D(QGLContext);
    eglMakeCurrent(d->dpy, d->surface, d->surface, 0);
#endif

    QT_EGL_ERR("QGLContext::doneCurrent");
    if (qgl_context_storage.hasLocalData())
        qgl_context_storage.localData()->context = 0;
    currentCtx = 0;

#if 0
#ifdef Q_USE_EGLWINDOWSURFACE
    if (device()->devType() == QInternal::Widget) {
        // EGL Only works if drawable is a QGLWidget, QGLFramebufferObject, QGLPixelBuffer not supported
        static_cast<QGLWidget*>(device())->d_func()->wsurf->endPaint(QRegion());
    }
#endif
#endif
}


void QGLContext::swapBuffers() const
{
    Q_D(const QGLContext);
    if(!d->valid)
        return;
#ifndef Q_USE_EGLWINDOWSURFACE
    eglSwapBuffers(d->dpy, d->surface);
#else
    if (device()->devType() == QInternal::Widget) {
        // EGL Only works if drawable is a QGLWidget, QGLPixelBuffer not supported
        QGLWidget *widget = static_cast<QGLWidget*>(device());
        widget->d_func()->wsurf->flush(widget, widget->frameGeometry(), QPoint());
    }
#endif
    QT_EGL_ERR("QGLContext::swapBuffers");
}

QColor QGLContext::overlayTransparentColor() const
{
    return QColor(0, 0, 0);                // Invalid color
}

uint QGLContext::colorIndex(const QColor &c) const
{
    //### color index doesn't work on egl
    Q_UNUSED(c);
    return 0;
}

void QGLContext::generateFontDisplayLists(const QFont & fnt, int listBase)
{
    Q_UNUSED(fnt);
    Q_UNUSED(listBase);
}

void *QGLContext::getProcAddress(const QString &proc) const
{
    return (void*)eglGetProcAddress(reinterpret_cast<const char *>(proc.toLatin1().data()));
}

#ifdef Q_USE_DIRECTPAINTER
class QGLDirectPainter : public QDirectPainter
{
public:
    QGLDirectPainter(QGLContextPrivate *cd, QGLWidgetPrivate *wd) :glxPriv(cd), glwPriv(wd), image(0), nativePix(0) {}
    ~QGLDirectPainter() {
    }
    void regionChanged(const QRegion &);
    void render();

    QRect geom;
    QGLContextPrivate *glxPriv;
    QGLWidgetPrivate *glwPriv;
    QImage *image;
    NativeWindowType nativePix;
};


void QGLDirectPainter::regionChanged(const QRegion&)
{
#ifdef Q_USE_QEGL
    if (geometry() != geom) {
        geom = geometry();
        uchar *fbp = QDirectPainter::frameBuffer() + geom.top() * QDirectPainter::linestep()
                     + ((QDirectPainter::screenDepth()+7)/8) * geom.left();

        QImage *oldImage = image;
        NativeWindowType oldPix = nativePix;
        image = new QImage(fbp, geom.width(), geom.height(), QDirectPainter::screenDepth(),
                           QDirectPainter::linestep(), 0, 0, QImage::IgnoreEndian);
#if 0 // debug
        static int i = 0;
        i = (i+13) %255;
        for (int y = 0; y < image->height(); ++y)
            for (int x = 0; x < image->width(); ++x)
                image->setPixel(x, y, 0xff4000 + i);
#endif
        QT_EGL_ERR("before eglDestroySurface");
        if (glxPriv->surface != EGL_NO_SURFACE) {
            eglDestroySurface(glxPriv->dpy, glxPriv->surface);
            QT_EGL_ERR("eglDestroySurface");
        }
#if 1
        nativePix = QEGL::createNativePixmap(image);
        glxPriv->surface =    eglCreatePixmapSurface(glxPriv->dpy, glxPriv->config, nativePix, 0);////const EGLint *attrib list);
#else

#endif
        QT_EGL_ERR("createEGLSurface");
        glxPriv->valid =  glxPriv->surface != EGL_NO_SURFACE;
        glwPriv->resizeHandler(geom.size());
        delete oldImage;
        QEGL::destroyNativePixmap(oldPix);
    }
#endif
    if (0) {
    QRegion alloc = allocatedRegion();
    int max = 0;
    QRect allocR;

    for (int i=0; i < alloc.rects().count(); ++i) {
        QRect r = alloc.rects().at(i);
        int a = r.width()*r.height();
        if (a  > max) {
            max = a;
            allocR = r;
        }
    }
    allocR.translate(-geom.topLeft());
    glScissor(allocR.left(), geom.height() - allocR.bottom(), allocR.width(), allocR.height());

    glwPriv->render(allocR);
    }
}
#else

class QWSGLPrivate
{
public:
    QWSGLPrivate() : img(0), oldbsimg(0), nativePix(0), dirty(true) {}
    QImage *img;
    QImage *oldbsimg;
    NativeWindowType nativePix;
    bool dirty;
};

class QGLDirectPainter : public QWSGLPrivate {}; //###

#endif // Q_USE_DIRECTPAINTER

void QGLWidgetPrivate::render(const QRegion &r)
{
    Q_Q(QGLWidget);
    QPaintEvent e(r);
    q->paintEvent(&e); //### slightly hacky...
}

void QGLWidgetPrivate::resizeHandler(const QSize &s)
{
    Q_Q(QGLWidget);

    q->makeCurrent();
    if (!glcx->initialized())
        q->glInit();

#ifndef Q_USE_EGLWINDOWSURFACE
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
#endif

    QT_EGL_ERR("QGLWidgetPrivate::resizeHandler");

    q->resizeGL(s.width(), s.height());
}

bool QGLWidget::event(QEvent *e)
{
#if 0 // ??? we have to make sure update() works...
    if (e->type() == QEvent::Paint)
        return true; // We don't paint when the GL widget needs to be painted, but when the directpainter does
#endif
#ifdef Q_USE_EGLWINDOWSURFACE
    return QWidget::event(e); // for EGL/GLES windowsurface do nothing in ::event()
#else

#ifndef Q_USE_DIRECTPAINTER
    if (e->type() == QEvent::Paint) {
        Q_D(QGLWidget);
        QWindowSurface *ws = d->currentWindowSurface();
        Q_ASSERT(ws);
        QImage *bsImage = static_cast<QImage*>(ws->paintDevice());
        if (bsImage
            && (bsImage != d->directPainter->oldbsimg || !d->directPainter->img ||d->directPainter->img->size() != size())) {
            QPoint offs = mapToGlobal(QPoint(0,0)) - window()->frameGeometry().topLeft();
            uchar *fbp = bsImage->bits() + offs.y() * bsImage->bytesPerLine()
                         + ((bsImage->depth()+7)/8) * offs.x();
            QImage *oldImage = d->directPainter->img;
            d->directPainter->img = new QImage(fbp, width(), height(), bsImage->depth(),
                                            bsImage->bytesPerLine(), 0, 0, QImage::IgnoreEndian);

            QGLContextPrivate *glxPriv = d->glcx->d_func();

            if (glxPriv->surface != EGL_NO_SURFACE) {
                eglDestroySurface(glxPriv->dpy, glxPriv->surface);
                QT_EGL_ERR("eglDestroySurface");
            }

            NativeWindowType nativePix = QEGL::createNativePixmap(d->directPainter->img);
            glxPriv->surface = eglCreatePixmapSurface(glxPriv->dpy, glxPriv->config, nativePix, 0);
            glxPriv->valid =  glxPriv->surface != EGL_NO_SURFACE;
            delete oldImage;
            QEGL::destroyNativePixmap(d->directPainter->nativePix);
            d->directPainter->nativePix = nativePix;
        }

    }
#endif
#endif
    return QWidget::event(e);
}

void QGLWidget::setMouseTracking(bool enable)
{
    QWidget::setMouseTracking(enable);
}


void QGLWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QGLWidget);
//     if (!isValid())
//         return;
#ifdef Q_USE_DIRECTPAINTER
    if (!d->directPainter)
        d->directPainter = new QGLDirectPainter(d->glcx->d_func(), d);
    d->directPainter->setGeometry(geometry());
#else
    if (!d->directPainter)
        d->directPainter = new QGLDirectPainter;
#endif
    //handle overlay
}

const QGLContext* QGLWidget::overlayContext() const
{
    return 0;
}

void QGLWidget::makeOverlayCurrent()
{
    //handle overlay
}

void QGLWidget::updateOverlayGL()
{
    //handle overlay
}

void QGLWidget::setContext(QGLContext *context, const QGLContext* shareContext, bool deleteOldContext)
{
    Q_D(QGLWidget);
    if(context == 0) {
        qWarning("QGLWidget::setContext: Cannot set null context");
        return;
    }

    if(d->glcx)
        d->glcx->doneCurrent();
    QGLContext* oldcx = d->glcx;
    d->glcx = context;
    if(!d->glcx->isValid())
        d->glcx->create(shareContext ? shareContext : oldcx);
    if(deleteOldContext)
        delete oldcx;
}

void QGLWidgetPrivate::init(QGLContext *context, const QGLWidget* shareWidget)
{
    Q_Q(QGLWidget);

    directPainter = 0;

#ifdef Q_USE_EGLWINDOWSURFACE
    wsurf = static_cast<QWSGLWindowSurface*>(QScreen::instance()->createSurface(q));
    q->setWindowSurface(wsurf);
#endif

    initContext(context, shareWidget);

    if(q->isValid() && glcx->format().hasOverlay()) {
        //no overlay
        qWarning("QtOpenGL ES doesn't currently support overlays");
    }
}

bool QGLWidgetPrivate::renderCxPm(QPixmap*)
{
    return false;
}

void QGLWidgetPrivate::cleanupColormaps()
{
}

const QGLColormap & QGLWidget::colormap() const
{
    return d_func()->cmap;
}

void QGLWidget::setColormap(const QGLColormap &)
{
}

void QGLExtensions::init()
{
    static bool init_done = false;

    if (init_done)
        return;
    init_done = true;

#if 0 //### to avoid confusing experimental EGL: don't create two GL widgets
    QGLWidget dmy;
    dmy.makeCurrent();
    init_extensions();
#endif
}

