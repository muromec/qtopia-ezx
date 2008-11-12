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

#ifndef PACKAGEMANAGER_UTILS_H
#define PACKAGEMANAGER_UTILS_H

#include <qstring.h>

class SizeUtils
{
    public:
    static bool isSufficientSpace(qlonglong size, QString &neededSpace);
    static QString getSizeString( qlonglong size );
    static qlonglong parseInstalledSize( QString installedSize );
};

class LidsUtils
{
    public:
        static bool isLidsEnabled();
        static bool maxRulesExceeded();
};

namespace ScriptRunner
{
    void runScript(const QString &cmd);
}

#endif
