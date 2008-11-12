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

#ifndef __GSTEAMER_PLAYBINSESSION_H
#define __GSTEAMER_PLAYBINSESSION_H

#include <QUrl>
#include <QUuid>

#include <QMediaServerSession>



namespace gstreamer
{

class Engine;
class Message;
class PlaybinSessionPrivate;

class PlaybinSession : public QMediaServerSession
{
    Q_OBJECT

public:
    PlaybinSession(Engine* engine, QUuid const& id, QUrl const& url);
    ~PlaybinSession();

    bool isValid() const;

    void start();
    void pause();
    void stop();

    void suspend();
    void resume();

    void seek(quint32 ms);
    quint32 length();

    void setVolume(int volume);
    int volume() const;

    void setMuted(bool mute);
    bool isMuted() const;

    QtopiaMedia::State playerState() const;

    QString errorString();

    void setDomain(QString const& domain);
    QString domain() const;

    QStringList interfaces();

    QString id() const;
    QString reportData() const;

private slots:
    void busMessage(Message const& msg);

private:
    void getStreamsInfo();
    void readySession();

    PlaybinSessionPrivate*  d;
};

}   // ns gstreamer


#endif  // __GSTREAMER_PLAYBINSESSION_H
