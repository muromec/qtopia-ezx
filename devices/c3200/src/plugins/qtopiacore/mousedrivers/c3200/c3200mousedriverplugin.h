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

#ifndef C3200MOUSEDRIVERPLUGIN_H
#define C3200MOUSEDRIVERPLUGIN_H

#include <QtGui/QWSMouseHandlerFactoryInterface>

class C3200MouseDriverPlugin : public QMouseDriverPlugin {
    Q_OBJECT
public:
    C3200MouseDriverPlugin( QObject *parent  = 0 );
    ~C3200MouseDriverPlugin();

    QWSMouseHandler* create(const QString& driver);
    QWSMouseHandler* create(const QString& driver, const QString& device);
    QStringList keys()const;
};

#endif // C3200MOUSEDRIVERPLUGIN_H
