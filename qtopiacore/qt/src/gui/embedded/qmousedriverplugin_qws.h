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

#ifndef QMOUSEDRIVERPLUGIN_QWS_H
#define QMOUSEDRIVERPLUGIN_QWS_H

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_HEADER

QT_MODULE(Gui)

#ifndef QT_NO_LIBRARY

class QWSMouseHandler;

struct Q_GUI_EXPORT QWSMouseHandlerFactoryInterface : public QFactoryInterface
{
    virtual QWSMouseHandler* create(const QString &name, const QString &device) = 0;
};

#define QWSMouseHandlerFactoryInterface_iid "com.trolltech.Qt.QWSMouseHandlerFactoryInterface"
Q_DECLARE_INTERFACE(QWSMouseHandlerFactoryInterface, QWSMouseHandlerFactoryInterface_iid)

class Q_GUI_EXPORT QMouseDriverPlugin : public QObject, public QWSMouseHandlerFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QWSMouseHandlerFactoryInterface:QFactoryInterface)
public:
    explicit QMouseDriverPlugin(QObject *parent = 0);
    ~QMouseDriverPlugin();

    virtual QStringList keys() const = 0;
    virtual QWSMouseHandler* create(const QString& driver, const QString &device) = 0;
};

#endif // QT_NO_LIBRARY

QT_END_HEADER

#endif // QMOUSEDRIVERPLUGIN_QWS_H
