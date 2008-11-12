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

#ifndef OMAP730KBDDRIVERPLUGIN_H
#define OMAP730KBDDRIVERPLUGIN_H

#include <QtGui/QWSKeyboardHandlerFactoryInterface>

class Omap730KbdDriverPlugin : public QKbdDriverPlugin {
    Q_OBJECT
public:
    Omap730KbdDriverPlugin( QObject *parent  = 0 );
    ~Omap730KbdDriverPlugin();

    QWSKeyboardHandler* create(const QString& driver, const QString& device);
    QWSKeyboardHandler* create(const QString& driver);
    QStringList keys()const;
};

#endif // OMAP730KBDC0DRIVERPLUGIN_H
