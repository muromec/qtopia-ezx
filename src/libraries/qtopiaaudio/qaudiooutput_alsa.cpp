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

#include "qaudiooutput.h"
#include <qtopianamespace.h>
#include <qtopialog.h>
#include <qfile.h>

#include <alsa/asoundlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

QAudioOutput* __output = NULL;

class QAudioOutputPrivate
{
public:
    QAudioOutputPrivate(const QByteArray &device)
    {
        frequency = 44100;
        channels = 2;
        bitsPerSample = 16;
        m_device = device;
        handle = 0;
        access = SND_PCM_ACCESS_RW_INTERLEAVED;
        format = SND_PCM_FORMAT_S16;
        period_time=0;
        buffer_time=500000;
    }
    ~QAudioOutputPrivate()
    {
        close();
    }

    int frequency;
    int channels;
    int bitsPerSample;
    QByteArray m_device;
    snd_pcm_t *handle;
    snd_pcm_access_t  access;
    snd_pcm_format_t  format;
    snd_pcm_uframes_t buffer_size;
    snd_pcm_uframes_t period_size;
    unsigned int buffer_time;
    unsigned int period_time;
    int sample_size;


    bool open()
    {
        // Open the Alsa playback device.
        int err;
        unsigned int        freakuency = frequency;

        if ( ( err = snd_pcm_open
               ( &handle, m_device.constData(), SND_PCM_STREAM_PLAYBACK, 0 ) ) < 0 ) {
            qWarning( "QAudioOuput: snd_pcm_open: error %d", err );
            return false;
        }
        snd_pcm_nonblock( handle, 0 );

        // Set the desired parameters.
        snd_pcm_hw_params_t *hwparams;
        snd_pcm_hw_params_alloca( &hwparams );

        err = snd_pcm_hw_params_any( handle, hwparams );
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_any: err %d", err);
            return false;
        }

        err = snd_pcm_hw_params_set_access( handle, hwparams, access );
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_set_access: err %d", err);
            return false;
        }

        err = snd_pcm_hw_params_set_format( handle, hwparams,
             ( bitsPerSample == 16 ? SND_PCM_FORMAT_S16
                                   : SND_PCM_FORMAT_U8 ) );
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_set_format: err %d", err);
            return false;
        }

        err = snd_pcm_hw_params_set_channels
            ( handle, hwparams, (unsigned int)channels );
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_set_channels: err %d", err);
            return false;
        }

        err = snd_pcm_hw_params_set_rate_near
            ( handle, hwparams, &freakuency, 0 );
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_set_rate_near: err %d", err);
            return false;
        }

        // Set buffer and period sizes based on a 20ms block size.
        int samplesPerBlock = frequency * channels / 50;
        sample_size = (snd_pcm_uframes_t)( samplesPerBlock * channels / 8 );
        err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &freakuency, 0);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_set_rate_near: err %d",err);
        }
        err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, 0);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_set_buffer_time_near: err %d",err);
        }
        period_time = 1000000 * 256 / frequency;
        err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, 0);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params_set_period_time_near: err %d",err);
        }
        err = snd_pcm_hw_params(handle, hwparams);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_hw_params: err %d", err);
            return false;
        }

        int                  dir;
        unsigned int         vval, vval2;
        snd_pcm_access_t     val;
        snd_pcm_format_t     fval;
        snd_pcm_subformat_t  sval;


        qLog(QAudioOutput) << "PCM handle name =" << snd_pcm_name(handle);
        qLog(QAudioOutput) << "PCM state =" << snd_pcm_state_name(snd_pcm_state(handle));
        snd_pcm_hw_params_get_access(hwparams,&val);
        vval = (unsigned int)val;
        if ( (int)vval != (int)access ) {
            qWarning( "QAudioInput: access type not set, want %s got %s",
                    snd_pcm_access_name((snd_pcm_access_t)access),
                    snd_pcm_access_name((snd_pcm_access_t)vval) );
            access = (snd_pcm_access_t)vval;
        }
        qLog(QAudioOutput) << "access type =" << snd_pcm_access_name((snd_pcm_access_t)vval);
        snd_pcm_hw_params_get_format(hwparams, &fval);
        vval = (unsigned int)fval;
        if ( (int)vval != (int)format ) {
            qWarning( "QAudioInput: format type not set, want %s got %s",
                    snd_pcm_format_name((snd_pcm_format_t)format),
                    snd_pcm_format_name((snd_pcm_format_t)vval) );
            format = (snd_pcm_format_t)vval;
        }
        qLog(QAudioOutput) << QString("format = '%1' (%2)").arg(snd_pcm_format_name((snd_pcm_format_t)vval))
            .arg(snd_pcm_format_description((snd_pcm_format_t)vval))
            .toLatin1().constData();
        snd_pcm_hw_params_get_subformat(hwparams,&sval);
        vval = (unsigned int)sval;
        qLog(QAudioOutput) << QString("subformat = '%1' (%2)").arg(snd_pcm_subformat_name((snd_pcm_subformat_t)vval))
            .arg(snd_pcm_subformat_description((snd_pcm_subformat_t)vval))
            .toLatin1().constData();
        snd_pcm_hw_params_get_channels(hwparams, &vval);
        if ( (int)vval != (int)channels ) {
            qWarning( "QAudioInput: channels type not set, want %d got %d",channels,vval);
            channels = vval;
        }
        qLog(QAudioOutput) << "channels =" << vval;
        snd_pcm_hw_params_get_rate(hwparams, &vval, &dir);
        if ( (int)vval != (int)frequency ) {
            qWarning( "QAudioInput: frequency type not set, want %d got %d",frequency,vval);
            frequency = vval;
        }
        qLog(QAudioOutput) << "rate =" << vval << "bps";
        snd_pcm_hw_params_get_period_time(hwparams,&period_time, &dir);
        qLog(QAudioOutput) << "period time =" << period_time << "us";
        snd_pcm_hw_params_get_period_size(hwparams,&period_size, &dir);
        qLog(QAudioOutput) << "period size =" << (int)period_size << "frames";
        snd_pcm_hw_params_get_buffer_time(hwparams,&buffer_time, &dir);
        qLog(QAudioOutput) << "buffer time =" << (int)buffer_time << "us";
        snd_pcm_hw_params_get_buffer_size(hwparams,(snd_pcm_uframes_t *) &buffer_size);
        qLog(QAudioOutput) << "buffer size =" << (int)buffer_size << "frames";
        snd_pcm_hw_params_get_periods(hwparams, &vval, &dir);
        qLog(QAudioOutput) << "periods per buffer =" << vval << "frames";
        snd_pcm_hw_params_get_rate_numden(hwparams, &vval, &vval2);
        qLog(QAudioOutput) << QString("exact rate = %1/%2 bps").arg(vval).arg(vval2).toLatin1().constData();
        vval = snd_pcm_hw_params_get_sbits(hwparams);
        qLog(QAudioOutput) << "significant bits =" << vval;
        snd_pcm_hw_params_get_tick_time(hwparams,&vval, &dir);
        qLog(QAudioOutput) << "tick time =" << vval << "us";
        vval = snd_pcm_hw_params_is_batch(hwparams);
        qLog(QAudioOutput) << "is batch =" << vval;
        vval = snd_pcm_hw_params_is_block_transfer(hwparams);
        qLog(QAudioOutput) << "is block transfer =" << vval;
        vval = snd_pcm_hw_params_is_double(hwparams);
        qLog(QAudioOutput) << "is double =" << vval;
        vval = snd_pcm_hw_params_is_half_duplex(hwparams);
        qLog(QAudioOutput) << "is half duplex =" << vval;
        vval = snd_pcm_hw_params_is_joint_duplex(hwparams);
        qLog(QAudioOutput) << "is joint duplex =" << vval;
        vval = snd_pcm_hw_params_can_overrange(hwparams);
        qLog(QAudioOutput) << "can overrange =" << vval;
        vval = snd_pcm_hw_params_can_mmap_sample_resolution(hwparams);
        qLog(QAudioOutput) << "can mmap =" << vval;
        vval = snd_pcm_hw_params_can_pause(hwparams);
        qLog(QAudioOutput) << "can pause =" << vval;
        vval = snd_pcm_hw_params_can_resume(hwparams);
        qLog(QAudioOutput) << "can resume =" << vval;
        vval = snd_pcm_hw_params_can_sync_start(hwparams);
        qLog(QAudioOutput) << "can sync start =" << vval;

        snd_pcm_sw_params_t *swparams;
        snd_pcm_sw_params_alloca(&swparams);
        err = snd_pcm_sw_params_current(handle, swparams);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_sw_params_current: err %d",err);
        }
        err = snd_pcm_sw_params_set_start_threshold(handle,swparams,period_size);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_sw_params_set_start_threshold: err %d",err);
        }
        err = snd_pcm_sw_params_set_avail_min(handle, swparams,period_size);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_sw_params_set_avail_min: err %d",err);
        }
        err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_sw_params_set_xfer_align: err %d",err);
        }
        err = snd_pcm_sw_params(handle, swparams);
        if ( err < 0 ) {
            qWarning( "QAudioOutput: snd_pcm_sw_params: err %d",err);
        }

        // Prepare for audio output.
        //snd_pcm_prepare( handle );

        return true;
    }

    void close()
    {
        if ( handle ) {
            snd_pcm_drain( handle );
            snd_pcm_close( handle );
            handle = 0;
        }
    }

    void startOutput()
    {
    }

    void write( const char *data, qint64 len )
    {
        if ( !handle )
            return;

        int count=0;

        while ( len > 0 ) {
            if(count > 5) break;
            int err=0;
            int frames = snd_pcm_bytes_to_frames( handle, (int)len );
#if defined(MIN_ALSA_BUFFER)
            if(frames > buffer_size) {
                err = snd_pcm_writei( handle, data, buffer_size );
            } else {
                err = snd_pcm_writei( handle, data, frames );
            }
#else
            err = snd_pcm_writei( handle, data, frames );
#endif
            if ( err >= 0 ) {
                int bytes = snd_pcm_frames_to_bytes( handle, err );
                qLog(QAudioOutput) << QString("write out = %1 (f=%2)").arg(bytes).arg(frames).toLatin1().constData();
                data += bytes;
                len -= bytes;
                count++;
                //break;
            } else {
                if((err == -EAGAIN)||(err == -EINTR)) {
                    qLog(QAudioOutput) << "snd_pcm_writei failed with EAGAIN or EINTR";
                    err=0;
                } else if(err == -EPIPE) {
                    qLog(QAudioOutput) << "QAudioOutput:: ALSA: underrun!!!";
                    err = snd_pcm_prepare(handle);
                    err = 0;
                } else if(err == -ESTRPIPE) {
                    while((err = snd_pcm_resume(handle)) == -EAGAIN)
                        sleep(1);
                    if(err < 0) {
                        qLog(QAudioOutput) << "ALSA: Unable to wake up pcm device, restart it!!!";
                        err = snd_pcm_prepare(handle);
                        if(err < 0) {
                            qLog(QAudioOutput) << "ALSA: restart failed!!!";
                        }
                        err = 0;
                        close();
                        break;
                    }
                } else {
                    qLog(QAudioOutput) << "ALSA: Unknown alsa writei error=" << err;
                    err = 0;
                    close();
                    break;
                }
            }
        }
    }
};

QAudioOutput::QAudioOutput( const QByteArray &device, QObject *parent )
    : QIODevice( parent )
{
    __output = this;
    d = new QAudioOutputPrivate(device);
}

QAudioOutput::QAudioOutput( QObject *parent )
    : QIODevice( parent )
{
    __output = this;
    d = new QAudioOutputPrivate("default");
}

QAudioOutput::~QAudioOutput()
{
    __output = NULL;
    delete d;
}

int QAudioOutput::frequency() const
{
    return d->frequency;
}

void QAudioOutput::setFrequency( int value )
{
    d->frequency = value;
}

int QAudioOutput::channels() const
{
    return d->channels;
}

void QAudioOutput::setChannels( int value )
{
    d->channels = value;
}

int QAudioOutput::bitsPerSample() const
{
    return d->bitsPerSample;
}

void QAudioOutput::setBitsPerSample( int value )
{
    d->bitsPerSample = value;
}

bool QAudioOutput::open( QIODevice::OpenMode mode )
{
    if ( isOpen() )
        return false;
    if ( !d->open() )
        return false;
    setOpenMode( mode | QIODevice::Unbuffered );
    return true;
}

void QAudioOutput::close()
{
    d->close();
    setOpenMode( NotOpen );
}

bool QAudioOutput::isSequential() const
{
    return true;
}

qint64 QAudioOutput::readData( char *, qint64 )
{
    // Cannot read from audio output devices.
    return 0;
}

qint64 QAudioOutput::writeData( const char *data, qint64 len )
{
    if ( !isOpen() )
        return len;
    d->write( data, len );
    return len;
}

void QAudioOutput::deviceReady( int id )
{
    Q_UNUSED(id)
}

void QAudioOutput::deviceError( int id )
{
    Q_UNUSED(id)
}
