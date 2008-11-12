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
#include <qtopiadesktoplog.h>
#include <desktopsettings.h>

bool qtopiadesktopLogEnabled( const char *_category )
{
    static QMap<QString,bool> *cache = 0;
    if ( cache && _category == 0 ) {
        delete cache;
        cache = 0;
        return false;
    }
    if ( !cache ) {
        cache = new QMap<QString,bool>;
    }
    QLatin1String category( _category );
    if ( cache->contains(category) )
        return (*cache)[category];

    DesktopSettings settings( "Log" );
    QVariant v = settings.value( category );
    if ( v.isNull() ) {
        v = DesktopSettings::debugMode()?1:0;
        if ( category == "TRACE" )
            v = 0;
        settings.setValue( category, v );
    }
    bool ret = v.toBool();
    (*cache)[QString(category)] = ret;
    //qDebug() << "category" << category << ret;
    return ret;
}

/*!
  \headerfile <qtopiadesktoplog.h>
  \title <qtopiadesktoplog.h>
  \ingroup headers
  \brief The <qtopiadesktoplog.h> header contains the category definitions used in Qtopia Sync Agent.

  The <qtopiadesktoplog.h> header contains the category definitions used in Qtopia Sync Agent.

  \quotefromfile libraries/qtopiadesktop/qtopiadesktoplog.h
  \skipto QLOG_UNCATEGORIZED
  \printuntil QDSync

  \sa <qlog.h>
*/

/*!
  \macro QDLOG_OPTION(CATEGORY)
  \relates <qtopiadesktoplog.h>

  Register a category for conditional logging. This enables qLog(\a{CATEGORY}) but
  the log messages are not displayed unless \a CATEGORY is enabled. This macro uses the
  qtopiadesktopLogEnabled() function to check if \a CATEGORY is enabled.
*/

/*!
  \fn bool qtopiadesktopLogEnabled( const char *category )
  \relates <qtopiadesktoplog.h>
  This function returns true if \a category has been enabled.

  \quotefromfile libraries/qtopiadesktop/qtopiadesktoplog.cpp
  \skipto DesktopSettings
  \printuntil return
*/
