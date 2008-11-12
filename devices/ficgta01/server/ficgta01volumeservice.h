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

#ifndef __QTOPIA_MEDIA_DEFAULTVOLUMEPROVIDER_H
#define __QTOPIA_MEDIA_DEFAULTVOLUMEPROVIDER_H

#include <QtopiaIpcAdaptor>


#include <alsa/asoundlib.h>
#include <QtopiaIpcAdaptor>
#include <QValueSpaceObject>


class Ficgta01VolumeService : public QtopiaIpcAdaptor
{
    Q_OBJECT
    enum AdjustType { Relative, Absolute };

public:
    Ficgta01VolumeService();
    ~Ficgta01VolumeService();

public slots:
    void setVolume(int volume);
    void setVolume(int leftChannel, int rightChannel);
    void increaseVolume(int increment);
    void decreaseVolume(int decrement);
    void setMute(bool mute);

    void adjustMicrophoneVolume(int volume);

    void changeAmpModeVS();

    void setAmp(bool);
    void set1973Amp(bool);

    void toggleAmpMode();

private slots:
    void registerService();
    void setCallDomain();
    void initVolumes();
private:
    void adjustVolume(int leftChannel, int rightChannel, AdjustType);

    int m_leftChannelVolume;
    int m_rightChannelVolume;

    QtopiaIpcAdaptor *adaptor;
    QValueSpaceObject *vsoVolumeObject;


protected:
    snd_mixer_t *mixerFd;
    snd_mixer_elem_t *elem;
    QString elemName;

    int minOutputVolume;
    int maxOutputVolume;

    int minInputVolume;
    int maxInputVolume;

    int initMixer();
    int closeMixer();
    int saveState();
    void adjustSpeakerVolume(int left, int right);
};


#endif  // __QTOPIA_MEDIA_DEFAULTVOLUMEPROVIDER_H
