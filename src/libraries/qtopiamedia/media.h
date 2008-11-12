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

#ifndef _QTOPIA_MEDIALIBRARY_MEDIA_H
#define _QTOPIA_MEDIALIBRARY_MEDIA_H

#include <qtopiaipcmarshal.h>

// Media Server definitions XXX: move out to private
#define QTOPIA_MEDIASERVER_CHANNEL  "MediaServer"
#define QTOPIA_MEDIALIBRARY_CHANNEL "QPE/MediaLibrary/%1"
#define QT_MEDIASERVER_CHANNEL      "QPE/MediaServer"

#ifndef Q_QDOC
namespace QtopiaMedia
{
#else
class QtopiaMedia
{
public:
#endif
enum State { Playing, Paused, Stopped, Buffering, Error };
enum Offset { Beginning, Current, End };
}

Q_DECLARE_USER_METATYPE_ENUM(QtopiaMedia::State);

#endif  // _QTOPIA_MEDIALIBRARY_MEDIA_H

