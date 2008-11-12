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

#include "qpaintdevice.h"
#include "qpainter.h"
#include "qwidget.h"
#include "qbitmap.h"
#include "qapplication.h"
#include <private/qt_x11_p.h>
#include "qx11info_x11.h"

/*!
    \class QPaintDevice
    \brief The QPaintDevice class is the base class of objects that
    can be painted.

    \ingroup multimedia

    A paint device is an abstraction of a two-dimensional space that
    can be drawn using a QPainter.  Its default coordinate system has
    its origin located at the top-left position. X increases to the
    right and Y increases downwards. The unit is one pixel.

    The drawing capabilities of QPaintDevice are currently implemented
    by the QWidget, QImage, QPixmap, QGLPixelBuffer, QPicture, and
    QPrinter subclasses.

    To implement support for a new backend, you must derive from
    QPaintDevice and reimplement the virtual paintEngine() function to
    tell QPainter which paint engine should be used to draw on this
    particular device. Note that you also must create a corresponding
    paint engine to be able to draw on the device, i.e derive from
    QPaintEngine and reimplement its virtual functions.

    \warning Qt requires that a QApplication object exists before
    any paint devices can be created. Paint devices access window
    system resources, and these resources are not initialized before
    an application object is created.

    The QPaintDevice class provides several functions returning the
    various device metrics: The depth() function returns its bit depth
    (number of bit planes). The height() function returns its height
    in default coordinate system units (e.g. pixels for QPixmap and
    QWidget) while heightMM() returns the height of the device in
    millimeters. Similiarily, the width() and widthMM() functions
    return the width of the device in default coordinate system units
    and in millimeters, respectively. Alternatively, the protected
    metric() function can be used to retrieve the metric information
    by specifying the desired PaintDeviceMetric as argument.

    The logicalDpiX() and logicalDpiY() functions return the
    horizontal and vertical resolution of the device in dots per
    inch. The physicalDpiX() and physicalDpiY() functions also return
    the resolution of the device in dots per inch, but note that if
    the logical and vertical resolution differ, the corresponding
    QPaintEngine must handle the mapping. Finally, the numColors()
    function returns the number of different colors available for the
    paint device.

    \sa QPaintEngine, QPainter, {The Coordinate System}, {The Paint
    System}
*/

/*!
    \enum QPaintDevice::PaintDeviceMetric

    Describes the various metrics of a paint device.

    \value PdmWidth The width of the paint device in default
    coordinate system units (e.g. pixels for QPixmap and QWidget). See
    also width().

    \value PdmHeight The height of the paint device in default
    coordinate system units (e.g. pixels for QPixmap and QWidget). See
    also height().

    \value PdmWidthMM The width of the paint device in millimeters. See
    also widthMM().

    \value PdmHeightMM  The height of the paint device in millimeters. See
    also heightMM().

    \value PdmNumColors The number of different colors available for
    the paint device. See also numColors().

    \value PdmDepth The bit depth (number of bit planes) of the paint
    device. See also depth().

    \value PdmDpiX The horizontal resolution of the device in dots per
    inch. See also logicalDpiX().

    \value PdmDpiY  The vertical resolution of the device in dots per inch. See
    also logicalDpiY().

    \value PdmPhysicalDpiX The horizontal resolution of the device in
    dots per inch. See also physicalDpiX().

    \value PdmPhysicalDpiY The vertical resolution of the device in
    dots per inch. See also physicalDpiY().

    \sa metric()
*/

/*!
    Constructs a paint device. This constructor can be invoked only from
    subclasses of QPaintDevice.
*/

QPaintDevice::QPaintDevice()
{
    painters = 0;
}

/*!
    Destroys the paint device and frees window system resources.
*/

QPaintDevice::~QPaintDevice()
{
    if (paintingActive())
        qWarning("QPaintDevice: Cannot destroy paint device that is being "
                  "painted");
    extern void qt_painter_removePaintDevice(QPaintDevice *); //qpainter.cpp
    qt_painter_removePaintDevice(this);
}

/*!
    \fn int QPaintDevice::devType() const

    \internal

    Returns the device type identifier, which is QInternal::Widget
    if the device is a QWidget, QInternal::Pixmap if it's a
    QPixmap, QInternal::Printer if it's a QPrinter,
    QInternal::Picture if it's a QPicture, or
    QInternal::UnknownDevice in other cases.
*/

/*!
    \fn bool QPaintDevice::paintingActive() const

    Returns true if the device is currently being painted on, i.e. someone has
    called QPainter::begin() but not yet called QPainter::end() for
    this device; otherwise returns false.

    \sa QPainter::isActive()
*/

/*!
    \fn QPaintEngine *QPaintDevice::paintEngine() const

    Returns a pointer to the paint engine used for drawing on the
    device.
*/

/*! \internal

    Returns the X11 Drawable of the paint device. 0 is returned if it
    can't be obtained.
*/

Drawable Q_GUI_EXPORT qt_x11Handle(const QPaintDevice *pd)
{
    if (!pd) return 0;
    if (pd->devType() == QInternal::Widget)
        return static_cast<const QWidget *>(pd)->handle();
    else if (pd->devType() == QInternal::Pixmap)
        return static_cast<const QPixmap *>(pd)->handle();
    return 0;
}

/*!
    \relates QPaintDevice

    Returns the QX11Info structure for the \a pd paint device. 0 is
    returned if it can't be obtained.
*/
const Q_GUI_EXPORT QX11Info *qt_x11Info(const QPaintDevice *pd)
{
    if (!pd) return 0;
    if (pd->devType() == QInternal::Widget)
        return &static_cast<const QWidget *>(pd)->x11Info();
    else if (pd->devType() == QInternal::Pixmap)
        return &static_cast<const QPixmap *>(pd)->x11Info();
    return 0;
}

/*!
    \fn int QPaintDevice::metric(PaintDeviceMetric metric) const

    Returns the metric information for  the given paint device \a metric.

    \sa PaintDeviceMetric
*/

int QPaintDevice::metric(PaintDeviceMetric) const
{
    qWarning("QPaintDevice::metrics: Device has no metric information");
    return 0;
}



#ifdef QT3_SUPPORT

/*!
    Use QX11Info::display() instead.

    \oldcode
        Display *display = widget->x11Display();
    \newcode
        Display *display = QX11Info::display();
    \endcode

    \sa QWidget::x11Info(), QX11Info::display()
*/
Display *QPaintDevice::x11Display() const
{
    return X11->display;
}

/*!
    Use QX11Info::screen() instead.

    \oldcode
        int screen = widget->x11Screen();
    \newcode
        int screen = widget->x11Info().screen();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11Screen() const
{
    const QX11Info *info = qt_x11Info(this);
    if (info)
	return info->screen();
    return QX11Info::appScreen();
}

/*!
    Use QX11Info::visual() instead.

    \oldcode
        void *visual = widget->x11Visual();
    \newcode
        void *visual = widget->x11Info().visual();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
void *QPaintDevice::x11Visual() const
{
    const QX11Info *info = qt_x11Info(this);
    if (info)
	return info->visual();
    return QX11Info::appVisual();
}

/*!
    Use QX11Info::depth() instead.

    \oldcode
        int depth = widget->x11Depth();
    \newcode
        int depth = widget->x11Info().depth();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11Depth() const
{
    const QX11Info *info = qt_x11Info(this);
    if (info)
        return info->depth();
    return QX11Info::appDepth();
}

/*!
    Use QX11Info::cells() instead.

    \oldcode
        int cells = widget->x11Cells();
    \newcode
        int cells = widget->x11Info().cells();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11Cells() const
{
    const QX11Info *info = qt_x11Info(this);
    if (info)
	return info->cells();
    return QX11Info::appCells();
}

/*!
    Use QX11Info::colormap() instead.

    \oldcode
        unsigned long screen = widget->x11Colormap();
    \newcode
        unsigned long screen = widget->x11Info().colormap();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
Qt::HANDLE QPaintDevice::x11Colormap() const
{
    const QX11Info *info = qt_x11Info(this);
    if (info)
	return info->colormap();
    return QX11Info::appColormap();
}

/*!
    Use QX11Info::defaultColormap() instead.

    \oldcode
        bool isDefault = widget->x11DefaultColormap();
    \newcode
        bool isDefault = widget->x11Info().defaultColormap();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
bool QPaintDevice::x11DefaultColormap() const
{
    const QX11Info *info = qt_x11Info(this);
    if (info)
	return info->defaultColormap();
    return QX11Info::appDefaultColormap();
}

/*!
    Use QX11Info::defaultVisual() instead.

    \oldcode
        bool isDefault = widget->x11DefaultVisual();
    \newcode
        bool isDefault = widget->x11Info().defaultVisual();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
bool QPaintDevice::x11DefaultVisual() const
{
    const QX11Info *info = qt_x11Info(this);
    if (info)
	return info->defaultVisual();
    return QX11Info::appDefaultVisual();
}

/*!
    Use QX11Info::visual() instead.

    \oldcode
        void *visual = QPaintDevice::x11AppVisual(screen);
    \newcode
        void *visual = qApp->x11Info(screen).visual();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
void *QPaintDevice::x11AppVisual(int screen)
{ return QX11Info::appVisual(screen); }

/*!
    Use QX11Info::colormap() instead.

    \oldcode
        unsigned long colormap = QPaintDevice::x11AppColormap(screen);
    \newcode
        unsigned long colormap = qApp->x11Info(screen).colormap();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
Qt::HANDLE QPaintDevice::x11AppColormap(int screen)
{ return QX11Info::appColormap(screen); }

/*!
    Use QX11Info::display() instead.

    \oldcode
        Display *display = QPaintDevice::x11AppDisplay();
    \newcode
        Display *display = qApp->x11Info().display();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
Display *QPaintDevice::x11AppDisplay()
{ return QX11Info::display(); }

/*!
    Use QX11Info::screen() instead.

    \oldcode
        int screen = QPaintDevice::x11AppScreen();
    \newcode
        int screen = qApp->x11Info().screen();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11AppScreen()
{ return QX11Info::appScreen(); }

/*!
    Use QX11Info::depth() instead.

    \oldcode
        int depth = QPaintDevice::x11AppDepth(screen);
    \newcode
        int depth = qApp->x11Info(screen).depth();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11AppDepth(int screen)
{ return QX11Info::appDepth(screen); }

/*!
    Use QX11Info::cells() instead.

    \oldcode
        int cells = QPaintDevice::x11AppCells(screen);
    \newcode
        int cells = qApp->x11Info(screen).cells();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11AppCells(int screen)
{ return QX11Info::appCells(screen); }

/*!
    Use QX11Info::appRootWindow() instead.

    \oldcode
        unsigned long window = QPaintDevice::x11AppRootWindow(screen);
    \newcode
        unsigned long window = qApp->x11Info(screen).appRootWindow();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
Qt::HANDLE QPaintDevice::x11AppRootWindow(int screen)
{ return QX11Info::appRootWindow(screen); }

/*!
    Use QX11Info::defaultColormap() instead.

    \oldcode
        bool isDefault = QPaintDevice::x11AppDefaultColormap(screen);
    \newcode
        bool isDefault = qApp->x11Info(screen).defaultColormap();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
bool QPaintDevice::x11AppDefaultColormap(int screen)
{ return QX11Info::appDefaultColormap(screen); }

/*!
    Use QX11Info::defaultVisual() instead.

    \oldcode
        bool isDefault = QPaintDevice::x11AppDefaultVisual(screen);
    \newcode
        bool isDefault = qApp->x11Info(screen).defaultVisual();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
bool QPaintDevice::x11AppDefaultVisual(int screen)
{ return QX11Info::appDefaultVisual(screen); }

/*!
    Use QX11Info::setAppDpiX() instead.
*/
void QPaintDevice::x11SetAppDpiX(int dpi, int screen)
{
    QX11Info::setAppDpiX(dpi, screen);
}

/*!
    Use QX11Info::setAppDpiY() instead.
*/
void QPaintDevice::x11SetAppDpiY(int dpi, int screen)
{
    QX11Info::setAppDpiY(dpi, screen);
}


/*!
    Use QX11Info::appDpiX() instead.

    \oldcode
        bool isDefault = QPaintDevice::x11AppDpiX(screen);
    \newcode
        bool isDefault = qApp->x11Info(screen).appDpiX();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11AppDpiX(int screen)
{
    return QX11Info::appDpiX(screen);
}

/*!
    Use QX11Info::appDpiY() instead.

    \oldcode
        bool isDefault = QPaintDevice::x11AppDpiY(screen);
    \newcode
        bool isDefault = qApp->x11Info(screen).appDpiY();
    \endcode

    \sa QWidget::x11Info(), QPixmap::x11Info()
*/
int QPaintDevice::x11AppDpiY(int screen)
{
    return QX11Info::appDpiY(screen);
}
#endif


/*!
    \fn int QPaintDevice::width() const

    Returns the width of the paint device in default coordinate system
    units (e.g. pixels for QPixmap and QWidget).

    \sa widthMM()
*/

/*!
    \fn int QPaintDevice::height() const

    Returns the height of the paint device in default coordinate
    system units (e.g. pixels for QPixmap and QWidget).

    \sa heightMM()
*/

/*!
    \fn int QPaintDevice::widthMM() const

    Returns the width of the paint device in millimeters.

    \sa width()
*/

/*!
    \fn int QPaintDevice::heightMM() const

    Returns the height of the paint device in millimeters.

    \sa height()
*/

/*!
    \fn int QPaintDevice::numColors() const

    Returns the number of different colors available for the paint
    device. Since this value is an int, it will not be sufficient to represent
    the number of colors on 32 bit displays, in this case INT_MAX is
    returned instead.
*/

/*!
    \fn int QPaintDevice::depth() const

    Returns the bit depth (number of bit planes) of the paint device.
*/

/*!
    \fn int QPaintDevice::logicalDpiX() const

    Returns the horizontal resolution of the device in dots per inch,
    which is used when computing font sizes. For X11, this is usually
    the same as could be computed from widthMM(), but it varies on
    Windows.

    Note that if the logicalDpiX() doesn't equal the physicalDpiX(),
    the corresponding QPaintEngine must handle the resolution mapping.

    \sa logicalDpiY(), physicalDpiX()
*/

/*!
    \fn int QPaintDevice::logicalDpiY() const

    Returns the vertical resolution of the device in dots per inch,
    which is used when computing font sizes. For X11, this is usually
    the same as could be computed from heightMM(), but it varies on
    Windows.

    Note that if the logicalDpiY() doesn't equal the physicalDpiY(),
    the corresponding QPaintEngine must handle the resolution mapping.

    \sa  logicalDpiX(), physicalDpiY()
*/

/*!
    \fn int QPaintDevice::physicalDpiX() const

    Returns the horizontal resolution of the device in dots per inch.

    Note that if the physicalDpiX() doesn't equal the logicalDpiX(),
    the corresponding QPaintEngine must handle the resolution mapping.

    \sa  physicalDpiY(),  logicalDpiX()
*/

/*!
    \fn int QPaintDevice::physicalDpiY() const

    Returns the horizontal resolution of the device in dots per inch.

    Note that if the physicalDpiY() doesn't equal the logicalDpiY(),
    the corresponding QPaintEngine must handle the resolution mapping.

    \sa  physicalDpiX(),  logicalDpiY()
*/

