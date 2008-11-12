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

#include "qscreendriverfactory_qws.h"

#include "qscreen_qws.h"
#include "qapplication.h"
#include "qscreenlinuxfb_qws.h"
#include "qscreentransformed_qws.h"
#include "qscreenvfb_qws.h"
#include "qscreenvnc_qws.h"
#include "qscreenmulti_qws_p.h"
#include <stdlib.h>
#include "private/qfactoryloader_p.h"
#include "qscreendriverplugin_qws.h"

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QScreenDriverFactoryInterface_iid, QCoreApplication::libraryPaths(),
     QLatin1String("/gfxdrivers")))

#endif //QT_NO_LIBRARY
#endif //QT_MAKEDLL

/*!
    \class QScreenDriverFactory
    \ingroup qws

    \brief The QScreenDriverFactory class creates screen drivers in
    Qtopia Core.

    Note that this class is only available in \l {Qtopia Core}.

    QScreenDriverFactory is used to detect and instantiate the
    available screen drivers, allowing \l {Qtopia Core} to load the
    preferred driver into the server application at runtime.  The
    create() function returns a QScreen object representing the screen
    driver identified by a given key. The valid keys (i.e. the
    supported drivers) can be retrieved using the keys() function.


    \l {Qtopia Core} provides several built-in screen drivers. In
    addition, custom screen drivers can be added using Qt's plugin
    mechanism, i.e. by subclassing the QScreen class and creating a
    screen driver plugin (QScreenDriverPlugin). See the \l {Qtopia
    Core Display Management}{display management} documentation for
    details.

    \sa QScreen, QScreenDriverPlugin
*/

/*!
    Creates the screen driver specified by the given \a key, using the
    display specified by the given \a displayId.

    Note that the keys are case-insensitive.

    \sa keys()
*/
QScreen *QScreenDriverFactory::create(const QString& key, int displayId)
{
    QString driver = key.toLower();
#ifndef QT_NO_QWS_QVFB
    if (driver == QLatin1String("qvfb") || driver.isEmpty())
        return new QVFbScreen(displayId);
#endif
#ifndef QT_NO_QWS_LINUXFB
    if (driver == QLatin1String("linuxfb") || driver.isEmpty())
        return new QLinuxFbScreen(displayId);
#endif
#ifndef QT_NO_QWS_TRANSFORMED
    if (driver == QLatin1String("transformed"))
        return new QTransformedScreen(displayId);
#endif
#ifndef QT_NO_QWS_VNC
    if (driver == QLatin1String("vnc"))
        return new QVNCScreen(displayId);
#endif
#ifndef QT_NO_QWS_MULTISCREEN
    if (driver == QLatin1String("multi"))
        return new QMultiScreen(displayId);
#endif

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY

    if (QScreenDriverFactoryInterface *factory = qobject_cast<QScreenDriverFactoryInterface*>(loader()->instance(key)))
        return factory->create(driver, displayId);

#endif
#endif
    return 0;
}

/*!
    Returns the list of valid keys, i.e. the available screen drivers.

    \sa create()
*/
QStringList QScreenDriverFactory::keys()
{
    QStringList list;

#ifndef QT_NO_QWS_QVFB
    list << QLatin1String("QVFb");
#endif
#ifndef QT_NO_QWS_LINUXFB
    list << QLatin1String("LinuxFb");
#endif
#ifndef QT_NO_QWS_TRANSFORMED
    list << QLatin1String("Transformed");
#endif
#ifndef QT_NO_QWS_VNC
    list << QLatin1String("VNC");
#endif
#ifndef QT_NO_QWS_MULTISCREEN
    list << QLatin1String("Multi");
#endif

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY
    QStringList plugins = loader()->keys();
    for (int i = 0; i < plugins.size(); ++i) {
        if (!list.contains(plugins.at(i)))
            list += plugins.at(i);
    }
#endif //QT_NO_LIBRARY
#endif //QT_MAKEDLL

    return list;
}
