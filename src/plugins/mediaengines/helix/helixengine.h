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

#ifndef __QTOPIA_HELIXENGINE_H
#define __QTOPIA_HELIXENGINE_H

#include <qmediaengine.h>
#include <qmediasessionbuilder.h>

class QTimerEvent;
class QMediaEngineInformation;
class QMediaSessionRequest;
class QMediaServerSession;
class IHXClientEngine;


#ifdef Q_QDOC
class qtopia_helix
{
public:
#else
namespace qtopia_helix
{
#endif

class HelixEnginePrivate;

class HelixEngine :
    public QMediaEngine,
    public QMediaSessionBuilder
{
    Q_OBJECT

public:
    HelixEngine();
    ~HelixEngine();

    // QMediaEngine
    void initialize();

    void start();
    void stop();

    void suspend();
    void resume();

    QMediaEngineInformation const* engineInformation();

    // QMediaSessionBuilder
    QString type() const;
    QMediaSessionBuilder::Attributes const& attributes() const;

    QMediaServerSession* createSession(QMediaSessionRequest sessionRequest);
    void destroySession(QMediaServerSession* serverSession);

private:
    void timerEvent(QTimerEvent* timerEvent);

    HelixEnginePrivate* d;
};

}   // ns qtopia_helix


#endif  //__QTOPIA_HELIXENGINE_H


