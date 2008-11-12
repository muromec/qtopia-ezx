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

#include "c3200audioplugin.h"

#include <QAudioState>
#include <QAudioStateInfo>
#include <QValueSpaceItem>
#include <QtopiaIpcAdaptor>
#include <QtopiaIpcEnvelope>

#include <QDebug>
#include <qplugin.h>
#include <qtopialog.h>

#include <stdio.h>
#include <stdlib.h>
#include <QProcess>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <QDir>

static inline bool setAudioMode(int mode)
{
QString m_mode;
    switch (mode) {
    case 0:
        qLog(AudioState)<< "setAudioMode() normal, speaker output";
        m_mode = "speaker";
        break;
    case 1:
        qLog(AudioState)<< "setAudioMode() headset dual mode for Phone call";
        m_mode = "phonecall";
        break;
    case 2:
        qLog(AudioState)<< "setAudioMode() headset stereo output for media playback";
        m_mode = "headphone";
        break;
    case 3:
        qLog(AudioState)<< "setAudioMode() recording";
        m_mode = "recording";
        break;
    };

    QString confDir;
    if( QDir("/etc/alsa").exists())
        confDir="/etc/alsa/";
    else
        confDir="/etc/";

    QString cmd = "/usr/sbin/alsactl -f "+confDir+m_mode+".state restore";
    qLog(AudioState)<< "cmd="<<cmd;
    int result = system(cmd.toLocal8Bit());

   qLog(AudioState)<< "setAudioMode "<< QString( "/etc/alsa/%1.state").arg(m_mode);
   if(result == 0)
       return true;
   qLog(AudioState)<< QString("Setting audio mode to: %1 failed").arg( m_mode);
    return false;

    QtopiaIpcEnvelope e("QPE/AudioVolumeManager/C3200VolumeService", "changeAmpModeVS()");

}

class HandsetAudioState : public QAudioState
{
    Q_OBJECT

public:
    HandsetAudioState(bool isPhone,QObject *parent = 0);

    QAudioStateInfo info() const;
    QAudio::AudioCapabilities capabilities() const;

    bool isAvailable() const;
    bool enter(QAudio::AudioCapability capability);
    bool leave();

private:
    QAudioStateInfo m_info;
    bool m_isPhone;

};

HandsetAudioState::HandsetAudioState(bool isPhone, QObject *parent)
    : QAudioState(parent)
{
    qLog(AudioState)<<"HandsetAudioState"<<isPhone;
    m_isPhone = isPhone;

    m_info.setDomain("Phone");
    m_info.setProfile("DualMode");
    m_info.setDisplayName(tr("Headset"));

    m_info.setPriority(50);
}

QAudioStateInfo HandsetAudioState::info() const
{
    return m_info;
}

QAudio::AudioCapabilities HandsetAudioState::capabilities() const
{
    return QAudio::InputOnly | QAudio::OutputOnly;
}

bool HandsetAudioState::isAvailable() const
{
    return true;
}

bool HandsetAudioState::enter(QAudio::AudioCapability capability)
{
    qLog(AudioState)<<"HandsetAudioState::enter"<<"isPhone";

    Q_UNUSED(capability)

    return setAudioMode( 1);
}

bool HandsetAudioState::leave()
{
    return true;
}

class MediaSpeakerAudioState : public QAudioState
{
    Q_OBJECT

public:
    MediaSpeakerAudioState(bool isPhone, QObject *parent = 0);

    QAudioStateInfo info() const;
    QAudio::AudioCapabilities capabilities() const;

    bool isAvailable() const;
    bool enter(QAudio::AudioCapability capability);
    bool leave();

private:
    QAudioStateInfo m_info;
    bool m_Phone;
};

MediaSpeakerAudioState::MediaSpeakerAudioState(bool isPhone, QObject *parent)
    : QAudioState(parent)
{
    m_Phone = isPhone;
    qLog(AudioState)<<"MediaSpeakerAudioState"<<isPhone;

    m_info.setDomain("Media");
    m_info.setProfile("Speaker");
    m_info.setDisplayName(tr("Speaker"));

    m_info.setPriority(150);

}

QAudioStateInfo MediaSpeakerAudioState::info() const
{
    return m_info;
}

QAudio::AudioCapabilities MediaSpeakerAudioState::capabilities() const
{
    return QAudio::OutputOnly;
}

bool MediaSpeakerAudioState::isAvailable() const
{
    return true;
}

bool MediaSpeakerAudioState::enter(QAudio::AudioCapability capability)
{
    qLog(AudioState)<<"MediaSpeakerAudioState::enter "<<m_Phone;

    Q_UNUSED(capability)

    return setAudioMode( 0);
}

bool MediaSpeakerAudioState::leave()
{
    qLog(AudioState)<<"MediaSpeakerAudioState::leave";
    return true;
}

class HeadphonesAudioState : public QAudioState
{
    Q_OBJECT

public:
    HeadphonesAudioState(bool isPhone, QObject *parent = 0);

    QAudioStateInfo info() const;
    QAudio::AudioCapabilities capabilities() const;

    bool isAvailable() const;
    bool enter(QAudio::AudioCapability capability);
    bool leave();

private slots:
    void onHeadsetModified();

private:
    QAudioStateInfo m_info;
    bool m_isPhone;
    QValueSpaceItem *m_headset;
};

HeadphonesAudioState::HeadphonesAudioState(bool isPhone, QObject *parent)
    : QAudioState(parent)
{
    m_isPhone = isPhone;
    qLog(AudioState)<<"isPhone?"<<isPhone;

    if (isPhone) {
        m_info.setDomain("Phone");
        m_info.setProfile("DualMode");
        m_info.setDisplayName(tr("Headset"));
    } else {
        m_info.setDomain("Media");
        m_info.setProfile("Headphone");
        m_info.setDisplayName(tr("Headphones"));
    }

    m_info.setPriority(25);

    m_headset = new QValueSpaceItem("/Hardware/Accessories/PortableHandsfree/Present", this);
    connect( m_headset, SIGNAL(contentsChanged()),
             this, SLOT(onHeadsetModified()));
}

QAudioStateInfo HeadphonesAudioState::info() const
{
    return m_info;
}

QAudio::AudioCapabilities HeadphonesAudioState::capabilities() const
{
    return QAudio::InputOnly | QAudio::OutputOnly;
}

void HeadphonesAudioState::onHeadsetModified()
{
    qLog(AudioState)<<"HeadphonesAudioState::onHeadsetModified()";
    bool avail = m_headset->value().toBool();

    if(avail) {
        this->enter( QAudio::OutputOnly);
      } else {
        this->leave();
    }

    emit availabilityChanged(avail);
}

bool HeadphonesAudioState::isAvailable() const
{
    return m_headset->value().toBool();
}

bool HeadphonesAudioState::enter(QAudio::AudioCapability capability)
{
    Q_UNUSED(capability)
    qLog(AudioState)<<"HeadphonesAudioState::enter"<<"isPhone"<<m_isPhone;

    qLog(AudioState) << "HeadphonesAudioState::enter" << capability;

    QtopiaIpcEnvelope e("QPE/AudioVolumeManager/C3200VolumeService", "setAmp(QString)");
    e << QString("Headphones");

    m_info.setPriority(25);
    if(m_info.domain() == "Phone") {
        qLog(AudioState) << "HeadphonesAudioState::enter phone";
        return setAudioMode(1);
    } else {
        qLog(AudioState) << "HeadphonesAudioState::enter media";
        if(capability && QAudio::InputOnly)
            return setAudioMode(3);
        else
            return setAudioMode(2);
    }
    //return setAudioMode(1);
}

bool HeadphonesAudioState::leave()
{
    qLog(AudioState)<<" HeadphonesAudioState::leave()"<<m_isPhone;

    if(m_isPhone) {

  QtopiaIpcEnvelope e("QPE/AudioVolumeManager/C3200VolumeService", "setAmp(QString)");
  e << QString("Stereo Speakers + Headphones");

    } else {

      QtopiaIpcEnvelope e("QPE/AudioVolumeManager/C3200VolumeService", "setAmp(QString)");
      e << QString("Stereo Speakers + Headphones");
    }
    m_info.setPriority(200);

    return true;
}

class SpeakerphoneAudioState : public QAudioState
{
    Q_OBJECT

public:
    SpeakerphoneAudioState(QObject *parent = 0);

    QAudioStateInfo info() const;
    QAudio::AudioCapabilities capabilities() const;

    bool isAvailable() const;
    bool enter(QAudio::AudioCapability capability);
    bool leave();

private:
    QAudioStateInfo m_info;
};

SpeakerphoneAudioState::SpeakerphoneAudioState(QObject *parent)
    : QAudioState(parent)
{
    m_info.setDomain("Phone");
    m_info.setProfile("PhoneSpeakerphone");
    m_info.setDisplayName(tr("Speakerphone"));
    m_info.setPriority(100);
}

QAudioStateInfo SpeakerphoneAudioState::info() const
{
    return m_info;
}

QAudio::AudioCapabilities SpeakerphoneAudioState::capabilities() const
{
    return QAudio::InputOnly | QAudio::OutputOnly;
}

bool SpeakerphoneAudioState::isAvailable() const
{
    return true;
}

bool SpeakerphoneAudioState::enter(QAudio::AudioCapability capability)
{
    //handset
    Q_UNUSED(capability)
        qLog(AudioState)<< " SpeakerphoneAudioState::enter";
    return setAudioMode( 1);
}

bool SpeakerphoneAudioState::leave()
{
    qLog(AudioState)<<" SpeakerphoneAudioState::leave";
    return true;
}


class RingtoneAudioState : public QAudioState
{
    Q_OBJECT

public:
    RingtoneAudioState(QObject *parent = 0);

    QAudioStateInfo info() const;
    QAudio::AudioCapabilities capabilities() const;

    bool isAvailable() const;
    bool enter(QAudio::AudioCapability capability);
    bool leave();

private:
    QAudioStateInfo m_info;
};

RingtoneAudioState::RingtoneAudioState(QObject *parent)
    : QAudioState(parent)
{
    m_info.setDomain("RingTone");
    m_info.setProfile("RingToneSpeaker");
    m_info.setDisplayName(tr("Stereo"));
    m_info.setPriority(120);
}

QAudioStateInfo RingtoneAudioState::info() const
{
    return m_info;
}

QAudio::AudioCapabilities RingtoneAudioState::capabilities() const
{
    return  QAudio::InputOnly | QAudio::OutputOnly;
}

bool RingtoneAudioState::isAvailable() const
{
    return true;
}

bool RingtoneAudioState::enter(QAudio::AudioCapability)
{
    qLog(AudioState)<<" RingtoneAudioState::enter";

    return setAudioMode(0);
}

bool RingtoneAudioState::leave()
{
    qLog(AudioState)<<"RingtoneAudioState::leave()";
    return true;
}


class C3200AudioPluginPrivate
{
public:
    QList<QAudioState *> m_states;
};

C3200AudioPlugin::C3200AudioPlugin(QObject *parent)
    : QAudioStatePlugin(parent)
{

    m_data = new C3200AudioPluginPrivate;

    m_data->m_states.push_back(new HandsetAudioState(this));

    m_data->m_states.push_back(new MediaSpeakerAudioState(this));

    m_data->m_states.push_back(new HeadphonesAudioState(false, this));
    m_data->m_states.push_back(new HeadphonesAudioState(true, this));

    m_data->m_states.push_back(new SpeakerphoneAudioState(this));

    m_data->m_states.push_back(new RingtoneAudioState(this));
}

C3200AudioPlugin::~C3200AudioPlugin()
{
    for (int i = 0; m_data->m_states.size(); i++) {
        delete m_data->m_states.at(i);
    }

    delete m_data;
}

QList<QAudioState *> C3200AudioPlugin::statesProvided() const
{
    return m_data->m_states;
}

Q_EXPORT_PLUGIN2(C3200audio_plugin, C3200AudioPlugin)

#include "c3200audioplugin.moc"
