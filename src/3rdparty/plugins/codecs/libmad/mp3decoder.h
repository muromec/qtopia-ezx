#ifndef __QTOPIA_CRUXUS_MP3CODER_H
#define __QTOPIA_CRUXUS_MP3CODER_H


#include <qtopiamedia/media.h>
#include <QMediaDecoder>


#define MAD_BUFFER_SIZE 8192


extern "C"
{
#include <mad.h>
}


class QMediaDevice;

class Mp3DecoderPrivate;

class Mp3Decoder : public QMediaDecoder
{
    Q_OBJECT

public:
    Mp3Decoder();
    ~Mp3Decoder();

    QMediaDevice::Info const& dataType() const;

    bool connectToInput(QMediaDevice* input);
    void disconnectFromInput(QMediaDevice* input);

    void start();
    void stop();
    void pause();

    quint64 length();
    bool seek(qint64 ms);

    void setVolume(int volume);
    int volume();

    void setMuted(bool mute);
    bool isMuted();

signals:
    void playerStateChanged(QtopiaMedia::State);
    void positionChanged(quint32);
    void lengthChanged(quint32);
    void volumeChanged(int);
    void volumeMuted(bool);

public:
    unsigned char *input_data;
    unsigned char *output_data;
    unsigned char *output_pos;
    int           buffered;
    int           offset;
    int           input_length;
    int           output_length;
    bool          resync;
    bool          header;
    bool          init;
    int           frames;
    int           framesize;
    quint32       duration;
    quint32       current;
    quint32       update_duration;

    int                 sFreq[6];
    int                 sLayer[6];
    int                 samples;

private:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    void   fill_input_buffer();
    void   decode_data();
    int    getFrames();
    void   nextFrame();

    inline signed int scale( mad_fixed_t sample );
    void audio_pcm( unsigned short *data, unsigned int nsamples, mad_fixed_t *left, mad_fixed_t *right );

    Mp3DecoderPrivate* d;
};


#endif  // __QTOPIA_CRUXUS_MP3CODER_H
