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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <QTimer>
#include <QDebug>
#include <QValueSpaceItem>
#include <qtopiaserverapplication.h>
#include <qtopiaipcenvelope.h>

#include <QAudioStateConfiguration>
#include <QAudioStateInfo>
#include <QValueSpaceObject>

#include "ficgta01volumeservice.h"

Ficgta01VolumeService::Ficgta01VolumeService():
    QtopiaIpcAdaptor("QPE/AudioVolumeManager/Ficgta01VolumeService")
{
    publishAll(Slots);
    qLog(AudioState)<<"Ficgta01VolumeService::Ficgta01VolumeService()";

    adaptor = new QtopiaIpcAdaptor("QPE/Ficgta01Modem", this);

    vsoVolumeObject = new QValueSpaceObject("/Hardware/Audio");

     QValueSpaceItem *ampmode = new QValueSpaceItem("/System/Tasks/Ficgta01VolumeService/ampMode");
     QObject::connect(ampmode, SIGNAL(contentsChanged()),
                      this, SLOT(toggleAmpMode()));

    QTimer::singleShot(0, this, SLOT(registerService()));
}

Ficgta01VolumeService::~Ficgta01VolumeService()
{
}

//public slots:
void Ficgta01VolumeService::setVolume(int volume)
{
    adjustVolume(volume, volume, Absolute);
}

void Ficgta01VolumeService::setVolume(int leftChannel, int rightChannel)
{
    adjustVolume(leftChannel, rightChannel, Absolute);
}

void Ficgta01VolumeService::increaseVolume(int increment)
{
    adjustVolume(increment, increment, Relative);
}

void Ficgta01VolumeService::decreaseVolume(int decrement)
{
    decrement *= -1;

    adjustVolume(decrement, decrement, Relative);
}

void Ficgta01VolumeService::setMute(bool)
{
}

void Ficgta01VolumeService::registerService()
{
    QtopiaIpcEnvelope e("QPE/AudioVolumeManager", "registerHandler(QString,QString)");
    e << QString("Headset") << QString("QPE/AudioVolumeManager/Ficgta01VolumeService");

//    QTimer::singleShot(0, this, SLOT(setCallDomain()));
    QTimer::singleShot(0, this, SLOT(initVolumes()));

}

void Ficgta01VolumeService::initVolumes()
{
    qLog(AudioState)<<" Ficgta01VolumeService::initVolumes()";

    initMixer();
//alsa
    snd_mixer_elem_t *elem;
    long minPVolume;
    long maxPVolume;
    long volume;

    for ( elem = snd_mixer_first_elem( mixerFd); elem; elem = snd_mixer_elem_next( elem) ) {
        if ( snd_mixer_elem_get_type( elem ) == SND_MIXER_ELEM_SIMPLE &&
             snd_mixer_selem_is_active( elem) ) {

            elemName = QString(snd_mixer_selem_get_name( elem));

            if(elemName == "Headphone") { // Master output
                snd_mixer_selem_get_playback_volume_range( elem, &minPVolume, &maxPVolume);

                adaptor->send(MESSAGE(setSpeakerVolumeRange(int,int )), (int)minPVolume, (int)maxPVolume);

                if(snd_mixer_selem_has_playback_volume( elem) > 0)
                    snd_mixer_selem_get_playback_volume( elem,SND_MIXER_SCHN_FRONT_LEFT, &volume);

                adaptor->send(MESSAGE(setOutputVolume(int)), (int)volume);

            }

            if(elemName == "ALC Capture Target") { // mic??
                snd_mixer_selem_get_capture_volume_range( elem, &minPVolume, &maxPVolume);

                adaptor->send(MESSAGE(setMicVolumeRange(int,int )), (int)minPVolume, (int)maxPVolume);


                if(snd_mixer_selem_has_capture_volume( elem) > 0)
                    snd_mixer_selem_get_capture_volume( elem,SND_MIXER_SCHN_FRONT_LEFT, &volume);
                adaptor->send(MESSAGE(setMicVolume(int)), (int)volume);

            }
        }
    }

    closeMixer();

}

void Ficgta01VolumeService::setCallDomain()
{
    qLog(AudioState)<<"Ficgta01VolumeService::setCallDomain()";
    QtopiaIpcEnvelope e("QPE/AudioVolumeManager", "setActiveDomain(QString)");
    e << QString("Headset");
}

void Ficgta01VolumeService::adjustVolume(int leftChannel, int rightChannel, AdjustType adjust)
{
    qLog(AudioState)<<" Ficgta01VolumeService::adjustVolume";
    unsigned int leftright;
    int left;
    int right;

    if (adjust == Relative) {

        left = (leftright & 0xff00) >> 8;
        right = (leftright & 0x00ff);

        left = qBound(minOutputVolume, left + leftChannel, maxOutputVolume);
        right = qBound(minOutputVolume, right + rightChannel, maxOutputVolume);
    } else {
        left = leftChannel;
        right = rightChannel;
    }

    leftright = (left << 8) | right;

    leftright = (maxOutputVolume << 8) | maxOutputVolume;

    adjustSpeakerVolume( left, right );
}




void Ficgta01VolumeService::adjustSpeakerVolume( int left, int right )
{
    initMixer();

    for ( elem = snd_mixer_first_elem( mixerFd); elem; elem = snd_mixer_elem_next( elem) ) {
        if ( snd_mixer_elem_get_type( elem ) == SND_MIXER_ELEM_SIMPLE &&
             snd_mixer_selem_is_active( elem) ) {

            elemName = QString(snd_mixer_selem_get_name( elem));

            if(elemName == "Headphone") { // Master output // could use PCM

                if(snd_mixer_selem_has_playback_volume( elem) > 0) {
                    snd_mixer_selem_set_playback_volume( elem,SND_MIXER_SCHN_FRONT_LEFT, (long)&left);
                    snd_mixer_selem_set_playback_volume( elem,SND_MIXER_SCHN_FRONT_RIGHT, (long)&right);
                }
            }
        }
    }

    closeMixer();
    saveState();
}

void Ficgta01VolumeService::adjustMicrophoneVolume( int volume )
{

    initMixer();
    for ( elem = snd_mixer_first_elem( mixerFd); elem; elem = snd_mixer_elem_next( elem) ) {
        if ( snd_mixer_elem_get_type( elem ) == SND_MIXER_ELEM_SIMPLE &&
             snd_mixer_selem_is_active( elem) ) {

            elemName = QString(snd_mixer_selem_get_name( elem));

            if(elemName == "VLC Capture Target") { // Mic Input

                if(snd_mixer_selem_has_playback_volume( elem) > 0) {
                    snd_mixer_selem_set_capture_volume( elem,SND_MIXER_SCHN_FRONT_LEFT, (long)&volume);
                }
            }
        }
    }

    closeMixer();
    saveState();
}

int Ficgta01VolumeService::initMixer()
{
    int result;
  if ((result = snd_mixer_open( &mixerFd, 0)) < 0) {
        qWarning()<<"snd_mixer_open error"<< result;
        mixerFd = NULL;
        return result;
    }
/*  hw:0
  hw:0,0*/
    if ((result = snd_mixer_attach( mixerFd, "default")) < 0) {
        qWarning()<<"snd_mixer_attach error"<< result;
        snd_mixer_close(mixerFd);
        mixerFd = NULL;
        return result;
    }
    if ((result = snd_mixer_selem_register( mixerFd, NULL, NULL)) < 0) {
        qWarning()<<"snd_mixer_selem_register error"<<result;
        snd_mixer_close(mixerFd);
        mixerFd = NULL;
        return result;
    }
    if ((result = snd_mixer_load( mixerFd)) < 0) {
        qWarning()<<"snd_mixer_load error"<<result;
        snd_mixer_close(mixerFd);
        mixerFd = NULL;
        return result;
    }
    return result;
}

int Ficgta01VolumeService::closeMixer()
{
     int result = snd_mixer_detach( mixerFd, "default" );
     result = snd_mixer_close( mixerFd );
//     snd_mixer_free( mixerFd ); //causes segfault
    return 0;

}


int Ficgta01VolumeService::saveState()
{

    QAudioStateConfiguration *audioState;
    audioState = new QAudioStateConfiguration(this);

    QString currentProfile( audioState->currentState().profile());
    QString m_mode;

    if(currentProfile == "MediaSpeaker"
       || currentProfile == "MediaHeadphones"
       || currentProfile == "RingToneSpeaker") {
        m_mode = "stereoout";
    }

    if(currentProfile == "PhoneSpeaker" ) {
        m_mode = "gsmhandset";
    }
    if(currentProfile == "PhoneHeadphones") {
        m_mode = "gsmheadset";
    }
    if(currentProfile == "PhoneBluetoothHeadset" /*|| "MediaBluetoothHeadset"*/) {
        m_mode = "gsmbluetooth";
    }
//Ficgta01CallVolume
    QString confDir;
    if( QDir("/usr/share/openmoko/scenarios").exists())
        confDir="/usr/share/openmoko/scenarios/";
    else
        confDir="/etc/";

    QString cmd = "/usr/sbin/alsactl -f "+confDir+m_mode+".state store";
    qLog(AudioState)<<cmd;
    int result = system(cmd.toLocal8Bit());
return 0;
}


/*
  sets the vso to current amp mode from mixer
 */
void Ficgta01VolumeService::changeAmpModeVS()
{
    char itemname[40];
    unsigned int item = 0;
    initMixer();



      for ( elem = snd_mixer_first_elem( mixerFd); elem; elem = snd_mixer_elem_next( elem) ) {
        if ( snd_mixer_elem_get_type( elem ) == SND_MIXER_ELEM_SIMPLE &&
             snd_mixer_selem_is_enumerated( elem) &&
             snd_mixer_selem_is_active( elem) ) {

            elemName = QString(snd_mixer_selem_get_name( elem));
            if(elemName == "Amp Spk") {

//current selection
                snd_mixer_selem_get_enum_item( elem, (snd_mixer_selem_channel_id_t)0, &item);
                snd_mixer_selem_get_enum_item_name(elem, item, sizeof(itemname) - 1, itemname);

                vsoVolumeObject->setAttribute("Amp",itemname);

            }
        }
      }
      closeMixer();
}

/*
set Amp spk on neo's audio card
*/
void Ficgta01VolumeService::setAmp( bool mode)
{
    qLog(AudioState)<<" Ficgta01VolumeService::setAmp" << mode;
    QValueSpaceItem *device = new QValueSpaceItem("/Hardware/Neo/Device");

    if ( device->value().toString() == "GTA01") {
        set1973Amp(mode);
        return;
    }

    QValueSpaceItem ampVS("/Hardware/Audio/Amp");
    QString ok = ampVS.value().toString();

    initMixer();
    char itemname[40];
    unsigned int item = 0;

    for ( elem = snd_mixer_first_elem( mixerFd); elem; elem = snd_mixer_elem_next( elem) ) {
        if ( snd_mixer_elem_get_type( elem ) == SND_MIXER_ELEM_SIMPLE &&
             //   snd_mixer_selem_is_enumerated( elem) &&
             snd_mixer_selem_is_active( elem) ) {

            elemName = QString(snd_mixer_selem_get_name( elem));


            if(/*elemName == "Amp Mode" ||*/ elemName.contains("Amp Spk")) {
                qLog(AudioState)<<"switch says"<< elemName << mode;

//current selection
//                 snd_mixer_selem_get_enum_item( elem, (snd_mixer_selem_channel_id_t)0, &item);
//                 snd_mixer_selem_get_enum_item_name(elem, item, sizeof(itemname) - 1, itemname);

//                 qLog(AudioState)<<"switch says"<< itemname << mode;

                if(snd_mixer_selem_has_playback_switch( elem))
                    snd_mixer_selem_set_playback_switch( elem,
                                                         snd_mixer_selem_channel_id_t(item),
                                                         mode? 1:0);
                vsoVolumeObject->setAttribute("Amp",mode);
            }
        }
    }

    closeMixer();

}

/*
  1973 needs this to switch off headphones
*/
void Ficgta01VolumeService::set1973Amp( bool mode)
{

 qLog(AudioState)<<" Ficgta01VolumeService::set1973Amp" << mode;

    char itemname[40];
    unsigned int item = 0;
    QString elemName;
    QString currentMode;
    QValueSpaceItem ampVS("/Hardware/Audio/Amp");
    QString ampMode = ampVS.value().toString();

    if(mode) {
        vsoVolumeObject->setAttribute("Amp", mode);
        ampMode = "Stereo Speakers";
    } else {
        vsoVolumeObject->setAttribute("Amp", mode);
        ampMode = "Headphones";
    }

    initMixer();


// unsigned int
    for ( elem = snd_mixer_first_elem( mixerFd); elem; elem = snd_mixer_elem_next( elem) ) {
        if ( snd_mixer_elem_get_type( elem ) == SND_MIXER_ELEM_SIMPLE &&
             snd_mixer_selem_is_enumerated( elem) &&
             snd_mixer_selem_is_active( elem) ) {

            elemName = QString(snd_mixer_selem_get_name( elem));
            if(elemName == "Amp Mode") {

//current selection
                snd_mixer_selem_get_enum_item( elem, (snd_mixer_selem_channel_id_t)0, &item);
                snd_mixer_selem_get_enum_item_name(elem, item, sizeof(itemname) - 1, itemname);

                currentMode = itemname;

                int enumItems = snd_mixer_selem_get_enum_items(elem);

                for( item = 0;item < enumItems; item++ ) {
                    snd_mixer_selem_get_enum_item_name(elem, item, sizeof(itemname) - 1, itemname);
                    if( QString(itemname) == ampMode) {
                        snd_mixer_selem_set_enum_item(elem, (snd_mixer_selem_channel_id_t)0, item);
                        break;
                    }
                }
            }
        }
    }

    if(currentMode != "Off") ampMode = currentMode;
    vsoVolumeObject->setAttribute("Amp",ampMode);

    closeMixer();
}


void Ficgta01VolumeService::toggleAmpMode()
{
    QValueSpaceItem ampVS("/Hardware/Audio/Amp");
    QString ampMode = ampVS.value().toString();

    if( ampMode == "Off") {
        setAmp(false);
    } else {
        setAmp(true);
    }
}



QTOPIA_TASK(Ficgta01VolumeService, Ficgta01VolumeService);

