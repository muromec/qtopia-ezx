#ifndef __QTOPIA_MP3PLUGIN_H
#define __QTOPIA_MP3PLUGIN_H

#include <QObject>
#include <QStringList>

#include <QMediaCodecPlugin>

#include <qtopiaglobal.h>



class Mp3PluginPrivate;

class Mp3Plugin :
    public QObject,
    public QMediaCodecPlugin
{
    Q_OBJECT
    Q_INTERFACES(QMediaCodecPlugin)

public:
    Mp3Plugin();
    ~Mp3Plugin();

    QString name() const;
    QString comment() const;
    QStringList mimeTypes() const;
    QStringList fileExtensions() const;

    double version() const;

    bool canEncode() const;
    bool canDecode() const;

    QMediaEncoder* encoder(QString const& mimeType);
    QMediaDecoder* decoder(QString const& mimeType);

private:
    Mp3PluginPrivate*  d;
};


#endif  // __QTOPIA_MP3PLUGIN_H
