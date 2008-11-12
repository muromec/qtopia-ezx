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

#ifndef AUDIO_PARAMETERS_H
#define AUDIO_PARAMETERS_H

// Qt includes
#include <QString>

// Qtopia includes
#include <QMimeType>

// Forward class declarations
class QDataStream;

// ============================================================================
//
// AudioParameters class
//
// ============================================================================

class AudioParameters
{
public:
    AudioParameters();
    AudioParameters( const QMimeType& mimeType,
                     const QString&   subFormat,
                     const int        frequency,
                     const int        channels );

    const QMimeType& mimeType() const;
    QString          subFormat() const;
    int              frequency() const;
    int              channels() const;

    void setMimeType( const QMimeType& mimeType );
    void setSubFormat( const QString& subFormat );
    void setFrequency( int frequency );
    void setChannels( int channels );

private:
    QMimeType mMimeType;
    QString   mSubFormat;
    int       mFrequency;
    int       mChannels;
};

QDataStream& operator>>( QDataStream& stream, AudioParameters& parameters );
QDataStream& operator<<( QDataStream& stream, const AudioParameters& parameters );

#endif //AUDIO_PARAMETERS_H
