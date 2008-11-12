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

#ifndef __GSTREAMENGINEINFORMATION_H
#define __GSTREAMENGINEINFORMATION_H

#include <qmediaengineinformation.h>
#include <qmediasessionbuilder.h>


namespace gstreamer
{

class Engine;
class EngineInformationPrivate;

class EngineInformation : public QMediaEngineInformation
{
public:
    EngineInformation(Engine* engine);
    ~EngineInformation();

    QString name() const;
    QString version() const;

    int idleTime() const;

    bool hasExclusiveDeviceAccess() const;

    QMediaSessionBuilderList sessionBuilders() const;

private:
    EngineInformationPrivate*   d;
};

}   // ns gstreamer

#endif  // __GSTREAMENGINEINFORMATION_H
