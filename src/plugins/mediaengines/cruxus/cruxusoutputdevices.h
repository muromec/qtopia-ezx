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


#ifndef __CRUXUX_OUTPUTDEVICES_H
#define __CRUXUX_OUTPUTDEVICES_H

class QMediaDevice;

namespace cruxus
{

class OutputThread;

class OutputDevices
{
    static int              created;
    static OutputThread*    output;

public:
    static QMediaDevice* createOutputDevice();
    static void destroyOutputDevice(QMediaDevice* outputDevice);
};

}   // ns cruxus

#endif  // __CRUXUX_OUTPUTDEVICES_H
