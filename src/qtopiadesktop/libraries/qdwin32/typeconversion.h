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
#ifndef TYPECONVERSION_H
#define TYPECONVERSION_H

#include <qdwin32config.h>
#include <windows.h>
#include <tchar.h>
#include <QString>

namespace QDWIN32 {

QDWIN32_EXPORT QString tchar_to_qstring( TCHAR *string, int length );
QDWIN32_EXPORT TCHAR *qstring_to_tchar( const QString &string );

};

#endif
