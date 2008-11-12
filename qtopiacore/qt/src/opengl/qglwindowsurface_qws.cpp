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

#include <QtGui/QPaintDevice>
#include <QtGui/QWidget>
#include <QtOpenGL/QGLWidget>
#include "private/qglwindowsurface_qws_p.h"
#include "private/qglpaintdevice_qws_p.h"
#include "private/qpaintengine_opengl_p.h"

/*!
    \class QWSGLWindowSurface
    \since 4.3
    \ingroup qws
    \preliminary

    \brief The QWSGLWindowSurface class provides the drawing area for top-level
    windows in Qtopia Core on EGL/OpenGLES. It also provides the drawing area for
    QGLWidgets whether they are top-level windows or child widget of another QWidget.

    Note that this class is only available in Qtopia Core and only available if
    Qt is configured with OpenGL support.
*/

class QWSGLWindowSurfacePrivate
{
public:
    QWSGLWindowSurfacePrivate() :
        qglContext(0), ownsContext(false) {}

    QGLContext *qglContext;
    bool ownsContext;
};

/*!
    Constructs an empty QWSGLWindowSurface for the given top-level \a window.
    The window surface is later initialized from chooseContext() and resources for it
    is typically allocated in setGeometry().
*/
QWSGLWindowSurface::QWSGLWindowSurface(QWidget *window)
    : QWSWindowSurface(window),
      d_ptr(new QWSGLWindowSurfacePrivate)
{
}

/*!
    Constructs an empty QWSGLWindowSurface.
*/
QWSGLWindowSurface::QWSGLWindowSurface()
    : d_ptr(new QWSGLWindowSurfacePrivate)
{
}

/*!
    Destroys the QWSGLWindowSurface object and frees any
    allocated resources.
 */
QWSGLWindowSurface::~QWSGLWindowSurface()
{
    Q_D(QWSGLWindowSurface);
    if (d->ownsContext)
        delete d->qglContext;
    delete d;
}

/*!
    Returns the QGLContext of the window surface.
*/
QGLContext *QWSGLWindowSurface::context() const
{
    Q_D(const QWSGLWindowSurface);
    if (!d->qglContext) {
        QWSGLWindowSurface *that = const_cast<QWSGLWindowSurface*>(this);
        that->setContext(new QGLContext(QGLFormat::defaultFormat()));
        that->d_func()->ownsContext = true;
    }
    return d->qglContext;
}

/*!
    Sets the QGLContext for this window surface to \a context.
*/
void QWSGLWindowSurface::setContext(QGLContext *context)
{
    Q_D(QWSGLWindowSurface);
    if (d->ownsContext) {
        delete d->qglContext;
        d->ownsContext = false;
    }
    d->qglContext = context;
}
