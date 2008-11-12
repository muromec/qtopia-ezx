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

#include "qdecorationfactory_qws.h"
#include "qdecorationplugin_qws.h"
#include "private/qfactoryloader_p.h"
#include "qmutex.h"

#include "qapplication.h"
#include "qdecorationdefault_qws.h"
#include "qdecorationwindows_qws.h"
#include "qdecorationstyled_qws.h"

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QDecorationFactoryInterface_iid, QCoreApplication::libraryPaths(),
     QLatin1String("/decorations"), Qt::CaseInsensitive))
#endif



/*!
    \class QDecorationFactory
    \ingroup qws
    \ingroup appearance

    \brief The QDecorationFactory class creates window decorations in
    Qtopia Core.

    Note that this class is only available in \l {Qtopia Core}.

    QDecorationFactory is used to detect and instantiate the available
    decorations, allowing \l {Qtopia Core} to load the preferred
    decoration into the application at runtime. The create() function
    returns a QDecoration object representing the decoration
    identified by a given key. The valid keys (i.e. the supported
    decorations) can be retrieved using the keys() function.

    \l {Qtopia Core} provides three built-in decorations: \c Default,
    \c Styled and \c Windows. In addition, custom decorations can be
    added using Qt's \l {How to Create Qt Plugins}{plugin mechanism},
    i.e. by subclassing the QDecoration class and creating a mouse
    driver plugin (QDecorationPlugin).

    \sa QDecoration, QDecorationPlugin
*/

/*!
    Creates the decoration specified by the given \a key. Note that
    the keys are case-insensitive.

    \sa keys()
*/

QDecoration *QDecorationFactory::create(const QString& key)
{
    QDecoration *ret = 0;
    QString decoration = key.toLower();
#ifndef QT_NO_QWS_DECORATION_DEFAULT
    if (decoration == QLatin1String("default"))
        ret = new QDecorationDefault;
    else
#endif
#ifndef QT_NO_QWS_DECORATION_WINDOWS
    if (decoration == QLatin1String("windows"))
        ret = new QDecorationWindows;
    else
#endif
#ifndef QT_NO_QWS_DECORATION_STYLED
    if (decoration == QLatin1String("styled"))
        ret = new QDecorationStyled;
    else
#endif
    { } // Keep these here - they make the #ifdefery above work
#ifndef QT_NO_LIBRARY
    if (!ret) {
        if (QDecorationFactoryInterface *factory = qobject_cast<QDecorationFactoryInterface*>(loader()->instance(decoration))) {
            ret = factory->create(decoration);
        }
    }
#endif
    return ret;
}

/*!
    Returns the list of valid keys, i.e., the available decorations.

    \sa create()
*/
QStringList QDecorationFactory::keys()
{
    QStringList list;
#ifndef QT_NO_QWS_DECORATION_DEFAULT
    list << QLatin1String("Default");
#endif
#ifndef QT_NO_QWS_DECORATION_WINDOWS
    list << QLatin1String("Windows");
#endif
#ifndef QT_NO_QWS_DECORATION_STYLED
    list << QLatin1String("Styled");
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
