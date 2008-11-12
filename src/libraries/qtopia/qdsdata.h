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

#ifndef QDS_DATA_H
#define QDS_DATA_H

// Qtopia includes
#include <Qtopia>
#include <qtopiaipcadaptor.h>
#include <qtopiaipcmarshal.h>

// ============================================================================
//
//  Forward class declarations
//
// ============================================================================

class QDSDataPrivate;
class QUniqueId;
class QByteArray;
class QMimeType;
class QString;
class QFile;
class QDataStream;

// ============================================================================
//
//  QDSData
//
// ============================================================================

class QTOPIA_EXPORT QDSData
{
public:
    QDSData();
    QDSData( const QDSData& other );
    explicit QDSData( const QUniqueId& key );
    QDSData( const QByteArray& data, const QMimeType& type );
    QDSData( const QString& data, const QMimeType& type );
    QDSData( QFile& data, const QMimeType& type );

    ~QDSData();

    // Operators
    const QDSData& operator=( const QDSData& other );
    bool operator==( const QDSData& other ) const;
    bool operator!=( const QDSData& other ) const;

    // Access
    bool isValid() const;
    QMimeType type() const;
    QByteArray data() const;
    QString toString() const;
    QIODevice* toIODevice() const;

    // Modification
    QUniqueId store();
    bool remove();
    bool modify( const QByteArray& data );
    bool modify( const QString& data );
    bool modify( QFile& data );
    bool modify( const QByteArray& data, const QMimeType& type );
    bool modify( const QString& data, const QMimeType& type );
    bool modify( QFile& data, const QMimeType& type );

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    QDSDataPrivate* d;
};

Q_DECLARE_USER_METATYPE( QDSData );


#endif //QDS_DATA_H
