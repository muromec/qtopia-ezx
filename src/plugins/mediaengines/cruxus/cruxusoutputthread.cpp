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

#include <math.h>

#include <QList>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QSemaphore>
#include <QMediaDecoder>
#include <QAudioOutput>
#include <QDebug>

#include <qtopialog.h>

#include "cruxusoutputthread.h"


namespace cruxus
{

// {{{ OutputThreadPrivate
class OutputThreadPrivate : public QThread
{
    Q_OBJECT

    static const int MAX_VOLUME = 100;

    static const int request_frequency = 44100;
    static const int request_bitsPerSample = 16;
    static const int request_channels = 2;

    static const int frame_milliseconds = 100;  // NOTE: no more than 1 second given below frame size calc

    static const int default_frame_size = (request_frequency / (1000 / frame_milliseconds)) *
                                          (request_bitsPerSample / 8) *
                                          request_channels;

public:
    bool                    opened;
    bool                    running;
    bool                    quit;
    QMutex                  mutex;
    QWaitCondition          condition;
    QAudioOutput*           audioOutput;
    QMediaDevice::Info      inputInfo;
    QList<QMediaDevice*>    activeSessions;

protected:
    void run();

private:
    int readFromDevice(QMediaDevice* device, QMediaDevice::Info const& info, char* working);
    int resampleAndMix(QMediaDevice::Info const& deviceInfo, char* src, int dataAmt, bool first);

    char    mixbuf[default_frame_size];
};

void OutputThreadPrivate::run()
{
    unsigned long   timeout = 30000;

    quit = false;

    audioOutput = new QAudioOutput;

    audioOutput->setFrequency(request_frequency);
    audioOutput->setChannels(request_channels);
    audioOutput->setBitsPerSample(request_bitsPerSample);

    audioOutput->open(QIODevice::ReadWrite | QIODevice::Unbuffered);

    inputInfo.type = QMediaDevice::Info::PCM;
    inputInfo.frequency = audioOutput->frequency();
    inputInfo.bitsPerSample = audioOutput->bitsPerSample();
    inputInfo.channels = audioOutput->channels();

    qLog(Media) << "OutputThreadPrivate::run(); opened device with " <<
                inputInfo.frequency << inputInfo.bitsPerSample << inputInfo.channels;

    do
    {
        QMutexLocker    conditionLock(&mutex);

        int             sc = activeSessions.size();

        if (sc == 0) {
            if(!condition.wait(&mutex, timeout) && opened) {
                audioOutput->close();
                opened = false;
                timeout = ULONG_MAX;
            }
        } else {
            bool    first = true;
            int     mixLength = 0;
            char    working[default_frame_size];

            for (int i = 0; i < sc; ++i) {
                QMediaDevice* input = activeSessions.at(i);
                QMediaDevice::Info const& info = input->dataType();

                int read = readFromDevice(input, info, working);

                if (read > 0) {
                    if (info.volume > 0) {
                        mixLength = qMax(resampleAndMix(info, working, read, first), mixLength);
                        first = false;
                    }
                }
                else {
                    activeSessions.removeAt(i);
                    --sc;
                }
            }

            if (mixLength > 0)
                audioOutput->write(mixbuf, mixLength);

            timeout = activeSessions.size() > 0 ? 0 : 30000;
        }

    } while (!quit);

    delete audioOutput;

    qLog(Media) << "OutputThreadPrivate::run(); exiting";
}

inline int OutputThreadPrivate::readFromDevice
(
 QMediaDevice* device,
 QMediaDevice::Info const& info,
 char* working
)
{
    return device->read(working, (info.frequency / (1000 / frame_milliseconds)) *
                                 (info.bitsPerSample / 8) *
                                 info.channels);
}

inline qint32 getNextSamplePart(char*& working, const int sampleSize)
{
    qint32  rc;

    switch (sampleSize)
    {
    case 1: rc = qint32(*(quint8*)working) - 128; break;
    case 2: rc = *(qint16*)working; break;
    case 4: rc = *(qint32*)working; break;
    default: rc = 0;
    }

    working += sampleSize;

    return rc;
}

inline void setNextSamplePart(char*& working, qint32 sample, const int sampleSize)
{
    switch (sampleSize)
    {
    case 1: *(quint8*)working = quint8(sample + 128); break;
    case 2: *(qint16*)working = qint16(sample); break;
    case 4: *(qint32*)working = sample; break;
    }

    working += sampleSize;
}

int OutputThreadPrivate::resampleAndMix
(
 QMediaDevice::Info const& deviceInfo,
 char* src,
 int dataAmt,
 bool first
)
{
    const int   oss = inputInfo.bitsPerSample / 8;   // output sample size
    const int   iss = deviceInfo.bitsPerSample / 8; // input sample size
    char*       mix = mixbuf;                   // src of data to mix
    char*       dst = mixbuf;                   // dst of all data (mixed or not)
    int         converted;

    if (deviceInfo.frequency == inputInfo.frequency &&
        deviceInfo.bitsPerSample == inputInfo.bitsPerSample &&
        deviceInfo.channels == inputInfo.channels)
    {   // Just send off
        converted = dataAmt;

        if (first)
        {
            for (;dataAmt > 0; dataAmt -= iss)
                setNextSamplePart(dst, getNextSamplePart(src, iss) * deviceInfo.volume / MAX_VOLUME, oss);
        }
        else
        {
            for (;dataAmt > 0; dataAmt -= iss)
                setNextSamplePart(dst, ((getNextSamplePart(src, iss) * deviceInfo.volume / MAX_VOLUME) + getNextSamplePart(mix, oss)) / 2, oss);
        }
    }
    else
    {   // re-sample (forgive me)
        const double srcSampleRate = deviceInfo.frequency / inputInfo.frequency;
        const double dstSampleRate = inputInfo.frequency / deviceInfo.frequency;
        const int rsr = int(ceil(srcSampleRate)) + 1;
        const int rdr = int(floor(dstSampleRate));
        const int srcChannelRate = deviceInfo.channels / inputInfo.channels;
        const int dstChannelRate = inputInfo.channels / deviceInfo.channels;
        const int resshift = 1 << qAbs(deviceInfo.bitsPerSample - inputInfo.bitsPerSample);

        converted = (dataAmt / iss / deviceInfo.channels) * rdr * oss * inputInfo.channels;

        int offset = 0;
        while (dataAmt > 0)
        {
            int     requiredSrcSamples = rsr;
            int     requiredDstSamples = rdr;
            qint32  sample[inputInfo.channels];

            // handle case freqs equal and mono to stereo
            if((requiredDstSamples == 1) && (dstChannelRate == 2))
                requiredDstSamples++;

            memset(sample, 0, sizeof(sample));

            while (requiredSrcSamples-- > 0 && dataAmt > 0)
            {
                qint32 cs[inputInfo.channels];

                memset(cs, 0, sizeof(cs));

                // Channels
                if (srcChannelRate > 1)
                {
                    // terrible
                    for (int i = 0; i < deviceInfo.channels; ++i)
                        cs[i % inputInfo.channels] += getNextSamplePart(src, iss) / srcChannelRate;
                }
                else if (dstChannelRate > 1)
                {
                    qint32  tmp = getNextSamplePart(src, iss);

                    for (int i = 0; i < inputInfo.channels; ++i)
                        cs[i] = tmp;
                }
                else
                {
                    for (int i = 0; i < inputInfo.channels; ++i)
                        cs[i] = getNextSamplePart(src, iss);
                }

                // Resolution
                if (iss == oss)
                    memcpy(sample, cs, sizeof(sample));
                else
                {
                    for (int i = 0; i < inputInfo.channels; ++i)
                        sample[i] = iss > oss ? cs[i] / resshift : cs[i] * resshift;
                }

                dataAmt -= iss * deviceInfo.channels;
            }

            if(deviceInfo.frequency ==  8000 && inputInfo.frequency == 44100)
                offset++;
            if(offset > requiredDstSamples*inputInfo.channels-1) {
                // Handle special cases of 8000Hz to 11025 or 22050 or 44100!
                qint32 samplei = 0, ss = 0;
                offset = 0;
                for (int i = requiredDstSamples * inputInfo.channels; i > 0; --i)
                {
                    samplei = (sample[i % inputInfo.channels] / rsr) * deviceInfo.volume / MAX_VOLUME;
                    if (first) {
                        setNextSamplePart(dst, samplei, oss);
                    } else {
                        qint32 ss = (samplei + getNextSamplePart(mix, oss)) / 2;
                        setNextSamplePart(dst, ss, oss);
                    }
                }
                for (int i = requiredDstSamples * inputInfo.channels; i > 0; --i)
                {
                    if (first) {
                        setNextSamplePart(dst, samplei, oss);
                    } else {
                        setNextSamplePart(dst, ss, oss);
                    }
                    converted+=2;
                }
            } else {
                for (int i = requiredDstSamples * inputInfo.channels; i > 0; --i)
                {
                    qint32 samplei = (sample[i % inputInfo.channels] / rsr) * deviceInfo.volume / MAX_VOLUME;

                    if (first)
                        setNextSamplePart(dst, samplei, oss);
                    else
                        setNextSamplePart(dst, (samplei + getNextSamplePart(mix, oss)) / 2, oss);
                }
            }
        }
    }

    return converted;
}
// }}}

// {{{ OutputThread

/*!
    \class cruxus::OutputThread
    \internal
*/

OutputThread::OutputThread():
    d(new OutputThreadPrivate)
{
    d->opened = false;

    d->start(QThread::HighPriority);
}

OutputThread::~OutputThread()
{
    d->quit = true;

    d->condition.wakeOne();
    d->wait();

    delete d;
}

QMediaDevice::Info const& OutputThread::dataType() const
{
    Q_ASSERT(false);        // Should never be called

    return d->inputInfo;
}

bool OutputThread::connectToInput(QMediaDevice* input)
{
    if (input->dataType().type != QMediaDevice::Info::PCM)
        return false;

    connect(input, SIGNAL(readyRead()), SLOT(deviceReady()));

    return true;
}

void OutputThread::disconnectFromInput(QMediaDevice* input)
{
    QMutexLocker    lock(&d->mutex);

    input->disconnect(this);

    d->activeSessions.removeAll(input);
}

bool OutputThread::open(QIODevice::OpenMode mode)
{
    if (! isOpen() ) 
        QIODevice::open(QIODevice::WriteOnly | QIODevice::Unbuffered);

    return isOpen();
 
    Q_UNUSED(mode);
}

void OutputThread::close()
{
    // Do nothing.
}

// private slots:
void OutputThread::deviceReady()
{
    QMutexLocker    lock(&d->mutex);

    if(!d->opened) {
        d->audioOutput->open(QIODevice::ReadWrite | QIODevice::Unbuffered);
        d->opened = true;
    }

    d->activeSessions.append(qobject_cast<QMediaDevice*>(sender()));

    d->condition.wakeOne();
}

// private:
qint64 OutputThread::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);

    return 0;
}

qint64 OutputThread::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}
// }}}

}   // ns cruxus

#include "cruxusoutputthread.moc"

