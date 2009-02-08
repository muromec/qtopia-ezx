#include "mp3decoder.h"
#include "mp3plugin.h"


class Mp3PluginPrivate
{
public:
    QStringList mimeTypes;
    QStringList fileExtensions;
};



Mp3Plugin::Mp3Plugin():
    d(new Mp3PluginPrivate)
{
    d->mimeTypes << "audio/mpeg";
    d->fileExtensions << "mp3";
}

Mp3Plugin::~Mp3Plugin()
{
    delete d;
}

QString Mp3Plugin::name() const
{
    return QLatin1String("MP3 decoder");
}

QString Mp3Plugin::comment() const
{
    return QString();
}

QStringList Mp3Plugin::mimeTypes() const
{
    return d->mimeTypes;
}

QStringList Mp3Plugin::fileExtensions()  const
{
    return d->fileExtensions;
}

double Mp3Plugin::version() const
{
    return 0.01;
}

bool Mp3Plugin::canEncode() const
{
    return false;
}

bool Mp3Plugin::canDecode() const
{
    return true;
}

QMediaEncoder* Mp3Plugin::encoder(QString const&)
{
    return 0;
}

QMediaDecoder* Mp3Plugin::decoder(QString const& mimeType)
{
    Q_UNUSED(mimeType);
    return new Mp3Decoder;
}


QTOPIA_EXPORT_PLUGIN(Mp3Plugin);

