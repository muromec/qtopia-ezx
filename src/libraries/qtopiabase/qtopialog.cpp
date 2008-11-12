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

#include <qtopialog.h>
#include <QString>
#include <QSettings>

Q_GLOBAL_STATIC_WITH_ARGS(QSettings, logSettings, ("Trolltech", "Log"));

bool qtopiaLogRequested( const char* category )
{
    return logSettings()->value(QLatin1String(category)+"/Enabled",0).toBool();
}

bool qtopiaLogEnabled( const char* category )
{
#undef QTOPIA_LOG_OPTION
#undef QLOG_DISABLE
#undef QLOG_ENABLE
#define QTOPIA_LOG_OPTION(dbgcat)
#define QLOG_DISABLE(dbgcat) if ( strcmp(category,#dbgcat)==0 ) return 0;
#define QLOG_ENABLE(dbgcat) if ( strcmp(category,#dbgcat)==0 ) return 1;
#include <qtopialog-config.h>
    return qtopiaLogRequested(category);
}

bool qtopiaLogOptional( const char* category )
{
#undef QTOPIA_LOG_OPTION
#undef QLOG_DISABLE
#undef QLOG_ENABLE
#define QTOPIA_LOG_OPTION(dbgcat)
#define QLOG_DISABLE(dbgcat) if ( strcmp(category,#dbgcat)==0 ) return 0;
#define QLOG_ENABLE(dbgcat) if ( strcmp(category,#dbgcat)==0 ) return 0;
#include <qtopialog-config.h>
    return 1;
}

