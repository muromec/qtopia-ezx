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

#include "qdecorationplugin_qws.h"
#include "qdecoration_qws.h"

/*!
    \class QDecorationPlugin
    \ingroup qws
    \ingroup plugins

    \brief The QDecorationPlugin class is an abstract base class for
    window decoration plugins in Qtopia Core.

    Note that this class is only available in \l {Qtopia Core}.

    \l {Qtopia Core} provides three ready-made decoration styles: \c
    Default, \c Styled and \c Windows. Custom decorations can be
    implemented by subclassing the QDecoration class and creating a
    decoration plugin.

    A decoration plugin can be created by subclassing
    QDecorationPlugin and implementing the pure virtual keys() and
    create() functions. By exporting the derived class using the
    Q_EXPORT_PLUGIN2() macro, \l {Qtopia Core}'s implementation of the
    QDecorationFactory class will automatically detect the plugin and
    load the driver into the application at runtime. See \l{How to
    Create Qt Plugins} for details.

    To actually apply a decoration, use the
    QApplication::qwsSetDecoration() function.

    \sa QDecoration, QDecorationFactory
*/

/*!
    \fn QStringList QDecorationPlugin::keys() const

    Returns the list of valid keys, i.e., the decorations supported by
    this plugin.

    \sa create()
*/

/*!
    \fn QDecoration *QDecorationPlugin::create(const QString &key)

    Creates a decoration matching the given \a key. Note that keys are
    case-insensitive.

    \sa keys()
*/

/*!
    Constructs a decoration plugin with the given \a parent.

    Note that this constructor is invoked automatically by the
    Q_EXPORT_PLUGIN2() macro, so there is no need for calling it
    explicitly.
*/
QDecorationPlugin::QDecorationPlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the decoration plugin.

    Note that Qt destroys a plugin automatically when it is no longer
    used, so there is no need for calling the destructor explicitly.
*/
QDecorationPlugin::~QDecorationPlugin()
{
}
