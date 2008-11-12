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

#include "qkbddriverplugin_qws.h"

#ifndef QT_NO_LIBRARY

#include "qkbd_qws.h"

/*!
    \class QKbdDriverPlugin
    \ingroup plugins
    \ingroup qws

    \brief The QKbdDriverPlugin class is an abstract base class for
    keyboard driver plugins in Qtopia Core.

    Note that this class is only available in \l {Qtopia Core}.

    \l {Qtopia Core} provides ready-made drivers for several keyboard
    protocols, see the \l {Qtopia Core Character Input}{character
    input} documentation for details. Custom keyboard drivers can be
    implemented by subclassing the QWSKeyboardHandler class and
    creating a keyboard driver plugin.

    A keyboard driver plugin can be created by subclassing
    QKbdDriverPlugin and reimplementing the pure virtual keys() and
    create() functions. By exporting the derived class using the
    Q_EXPORT_PLUGIN2() macro, \l {Qtopia Core}'s implementation of the
    QKbdDriverFactory class will automatically detect the plugin and
    load the driver into the server application at runtime. See \l
    {How to Create Qt Plugins} for details.

    \sa QKbdDriverFactory, QWSKeyboardHandler
*/

/*!
    \fn QStringList QKbdDriverPlugin::keys() const

    Implement this function to return the list of valid keys, i.e. the
    keyboard drivers supported by this plugin.

    \l {Qtopia Core} provides ready-made drivers for several keyboard
    protocols, see the \l {Qtopia Core Character Input}{character
    input} documentation for details.

    \sa create()
*/

/*!
    Constructs a keyboard driver plugin with the given \a parent.

    Note that this constructor is invoked automatically by the
    Q_EXPORT_PLUGIN2() macro, so there is no need for calling it
    explicitly.
*/
QKbdDriverPlugin::QKbdDriverPlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the keyboard driver plugin.

    Note that Qt destroys a plugin automatically when it is no longer
    used, so there is no need for calling the destructor explicitly.
*/
QKbdDriverPlugin::~QKbdDriverPlugin()
{
}

/*!
    \fn QScreen *QKbdDriverPlugin::create(const QString &key, const QString &device)

    Implement this function to create a driver matching the type
    specified by the given \a key and \a device parameters. Note that
    keys are case-insensitive.

    \sa keys()
*/

#endif // QT_NO_LIBRARY
