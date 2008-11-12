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


#ifndef __QTOPIA_ENGINELOADER_H
#define __QTOPIA_ENGINELOADER_H

#include <qobject.h>
#include <qlist.h>

#include <qmediaengine.h>


namespace mediaserver
{

class EngineLoaderPrivate;

class EngineLoader : public QObject
{
    Q_OBJECT

public:
    EngineLoader();
    ~EngineLoader();

    void load();
    void unload();

    QMediaEngineList const& engines();

private:
    EngineLoaderPrivate*    d;
};

}   // ns mediaserver

#endif  // __QTOPIA_ENGINELOADER_H

