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
#ifndef LOG_H
#define LOG_H

#include <trace.h>
#include <QTextBrowser>
extern QTextBrowser *qdsync_tb;
#define USERLOG(x)\
do {\
    if ( qdsync_tb ) {\
        LOG() << x;\
        qdsync_tb->append(x);\
    }\
} while ( 0 )

#endif
