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

#ifndef __CRUXUS_SIMPLESESSION_H
#define __CRUXUS_SIMPLESESSION_H

#include <QMediaHandle>
#include <QMediaServerSession>


class QMediaDevice;
class QMediaDecoder;

namespace cruxus
{

class SimpleSessionPrivate;

class SimpleSession : public QMediaServerSession
{
    Q_OBJECT

public:
    SimpleSession(QMediaHandle const& handle,
                  QMediaDevice* source,
                  QMediaDecoder* coder,
                  QMediaDevice* sink);
    ~SimpleSession();

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
    void stateChanged(QtopiaMedia::State state);

private:
    SimpleSessionPrivate* d;
};

}   // ns cruxus

#endif  // __CRUXUS_SIMPLESESSION_H
