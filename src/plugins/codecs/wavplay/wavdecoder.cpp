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

#include <QTimer>
#include <QFileInfo>

#include <qendian.h>
#include <qtopianamespace.h>
#include <qtopialog.h>

#include "wavdecoder.h"


// {{{ Wave Info
namespace
{
static const char* riffId = "RIFF";
static const char* waveId = "WAVE";
static const char* fmtId = "fmt ";
static const char* dataId = "data";

struct chunk
{
    char        id[4];
    quint32     size;
};

struct RIFFHeader
{
    chunk       descriptor;
    char        type[4];
};

struct WAVEHeader
{
    chunk       descriptor;
    quint16     audioFormat;        // PCM = 1
    quint16     numChannels;
    quint32     sampleRate;
    quint32     byteRate;
    quint16     blockAlign;
    quint16     bitsPerSample;
//    quint16 extraParamSize  // if !PCM, so not supported
//    uchar extraParams     
};

struct DATAHeader
{
    chunk       descriptor;
    quint8      data[];
};

struct CombinedHeader
{
    RIFFHeader  riff;
    WAVEHeader  wave;
    DATAHeader  data;
};
}
// }}}

// {{{ WavDecoderPrivate
class WavDecoderPrivate
{
public:
    bool                initialized;
    bool                muted;
    int                 volume;
    quint32             length;
    quint32             position;
    quint32             rawDataRead;
    QMediaDevice*       inputDevice;
    QtopiaMedia::State  state;
    CombinedHeader      header;
    QMediaDevice::Info  outputInfo;
};
// }}}

/*!
    \class WavDecoder
    \brief The WavDecoder class is used to read and PCM data from a Standard
    WAVE format data source.
*/

// {{{ WavDecoder

/*!
    Construct a WavDecoder.
*/

WavDecoder::WavDecoder():
    d(new WavDecoderPrivate)
{
    // init
    d->initialized = false;
    d->muted = false;
    d->volume = 50;
    d->length = 0;
    d->position = 0;
    d->rawDataRead = 0;
    d->state = QtopiaMedia::Stopped;

    d->outputInfo.type = QMediaDevice::Info::PCM;
}

/*!
    Destroy the WavDecoder object.
*/

WavDecoder::~WavDecoder()
{
    delete d;
}

/*!
    Return information about the Wave data as well as the current volume at which the Wave
    data should be played.

    \sa QMediaDecoder
*/

QMediaDevice::Info const& WavDecoder::dataType() const
{
    return d->outputInfo;
}

/*!
    Connect to \a input as a source of data for this decoder, return true if the source
    is compatible.
*/

bool WavDecoder::connectToInput(QMediaDevice* input)
{
    if (input->dataType().type != QMediaDevice::Info::Raw)
        return false;

    d->inputDevice = input;

    return true;
}

/*!
    Disconnect from the \a input source of data.
*/

void WavDecoder::disconnectFromInput(QMediaDevice* input)
{
    Q_UNUSED(input);

    d->inputDevice = 0;
}

/*!
    Starting actively decoding the Wave data from the input QMediaDevice.
*/

void WavDecoder::start()
{
    if (!d->initialized)
    {
        if (QIODevice::open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        {
            if (d->inputDevice->read((char*)&d->header, sizeof(CombinedHeader)) == sizeof(CombinedHeader))
            {
                if (strncmp(d->header.riff.descriptor.id, riffId, 4) == 0 &&
                    strncmp(d->header.riff.type, waveId, 4) == 0 &&
                    strncmp(d->header.wave.descriptor.id, fmtId, 4) == 0) {
                    if (d->header.wave.audioFormat == 1)
                    {
                        d->outputInfo.type = QMediaDevice::Info::PCM;
                        d->outputInfo.frequency = qFromLittleEndian<quint32>(d->header.wave.sampleRate);
                        d->outputInfo.bitsPerSample = qFromLittleEndian<quint16>(d->header.wave.bitsPerSample);
                        d->outputInfo.channels = qFromLittleEndian<quint16>(d->header.wave.numChannels);
                        d->outputInfo.volume = d->volume;

                        d->length = quint32((double(d->header.riff.descriptor.size) /
                                        d->outputInfo.frequency /
                                        d->outputInfo.channels /
                                        (d->outputInfo.bitsPerSample / 8)) * 1000);

                        qLog(Media) << "WavDecoder::start(); Info" <<
                                    d->outputInfo.frequency <<
                                    d->outputInfo.bitsPerSample <<
                                    d->outputInfo.channels <<
                                    "length:" << d->length;

                        emit lengthChanged(d->length);

                        d->initialized = true;
                    }
                }
            }
        }
    }

    if (d->initialized)
    {
        if (d->state == QtopiaMedia::Stopped)
            seek(0);

        d->state = QtopiaMedia::Playing;

        emit readyRead();
        emit playerStateChanged(d->state);
    }
}

/*!
    Stop decoding data from the input QMediaDevice.
*/

void WavDecoder::stop()
{
    emit playerStateChanged(d->state = QtopiaMedia::Stopped);
    seek(0);
}

/*!
    Pause decoding data from the input QMediaDevice.
*/

void WavDecoder::pause()
{
    emit playerStateChanged(d->state = QtopiaMedia::Paused);
}

/*!
    Return the length, in milliseconds of the Wave data.
*/

quint64 WavDecoder::length()
{
    return d->length;
}

/*!
    Seek to \a ms milliseconds from the beginning of that data. Returning
    true if the seek was able to be performed.
*/

bool WavDecoder::seek(qint64 ms)
{
    ms /= 1000;

    int     rawPos = sizeof(CombinedHeader) +
                         (ms * d->outputInfo.frequency *
                         d->outputInfo.channels *
                         (d->outputInfo.bitsPerSample / 8));

    if (d->inputDevice->seek(rawPos))
    {
        d->rawDataRead = rawPos;
        emit positionChanged(d->position = ms * 1000);

        return true;
    }

    return false;
}

/*!
    Set the volume of the Wave data to \a volume.
*/

void WavDecoder::setVolume(int volume)
{
    d->volume = qMin(qMax(volume, 0), 100);

    if (!d->muted)
        d->outputInfo.volume = d->volume;

    emit volumeChanged(d->volume);
}

/*!
    Return the current volume of the wave data.
*/

int WavDecoder::volume()
{
    return d->volume;
}

/*!
    Set the mute status to \a mute. if true the volume will be set
    to 0.
*/

void WavDecoder::setMuted(bool mute)
{
    d->outputInfo.volume = mute ? 0 : d->volume;

    emit volumeMuted(d->muted = mute);
}

/*!
    Return the mute status.
*/

bool WavDecoder::isMuted()
{
    return d->muted;
}

//private:
/*!
    \internal
*/

qint64 WavDecoder::readData(char *data, qint64 maxlen)
{
    qint64      rc = 0;

    if (maxlen > 0)
    {
        if (d->state == QtopiaMedia::Playing)
        {
            quint32 position = quint32((double(d->rawDataRead) /
                                    (double(d->outputInfo.frequency) *
                                     d->outputInfo.channels *
                                     (d->outputInfo.bitsPerSample / 8))) * 1000);

            if (d->position != position)
            {
                d->position = position;
                emit positionChanged(d->position);
            }

            rc = d->inputDevice->read(data, maxlen);

            if (rc == 0)
                emit playerStateChanged(d->state = QtopiaMedia::Stopped);
            else
                d->rawDataRead += rc;
        }
    }

    return rc;
}

/*!
    \internal
*/

qint64 WavDecoder::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}
// }}}


