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

#ifndef __QTOPIA_SERVER_AUDIOVOLUMEMANAGERTASK_H
#define __QTOPIA_SERVER_AUDIOVOLUMEMANAGERTASK_H

#include <qstring.h>
#include <qlist.h>
#include <qmap.h>
#include <qtopiaipcadaptor.h>

class QValueSpaceObject;
class AudioVolumeManager : public QtopiaIpcAdaptor
{
    Q_OBJECT
    typedef QMap<QString, QString>  VolumeServiceProviders;
    typedef QList<QString>          VolumeDomains;

public:
    AudioVolumeManager();
    ~AudioVolumeManager();

    bool canManageVolume() const;

signals:
    void volumeChanged(int volume);

public slots:
    void setVolume(int volume);
    void increaseVolume(int increment);
    void decreaseVolume(int decrement);
    void setMuted(bool mute);

    void registerHandler(QString const& domain, QString const& channel);
    void unregisterHandler(QString const& domain, QString const& channel);
    void setActiveDomain(QString const& domain);
    void resetActiveDomain(QString const& domain);
    void currentVolume(QString const& volume);

private:
    QString findProvider() const;

    VolumeDomains           m_domains;
    VolumeServiceProviders  m_vsps;
    QValueSpaceObject *m_vsVolume;
};

#endif  // __QTOPIA_SERVER_AUDIOVOLUMEMANAGERTASk_H
