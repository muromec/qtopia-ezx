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

#ifndef __QTOIPA_CRUXUS_ENGINE_H
#define __QTOPIA_CRUXUS_ENGINE_H


#include <qmediaengine.h>
#include <qmediasessionbuilder.h>


class QMediaSessionRequest;
class QMediaServerSession;
class SimpleSession;

namespace cruxus
{

class DecodeSession;

class CruxusEnginePrivate;

class Engine : public QMediaEngine
{
    Q_OBJECT

public:
    Engine();
    ~Engine();

    void initialize();

    void start();
    void stop();

    void suspend();
    void resume();

    QMediaEngineInformation const* engineInformation();

    void registerSession(QMediaServerSession* session);
    void unregisterSession(QMediaServerSession* session);

private:
    QList<SimpleSession*>   s;
    CruxusEnginePrivate*    d;
};

}   // ns cruxus

#endif  // __QTOPIA_CRUXUS_ENGINE_H

