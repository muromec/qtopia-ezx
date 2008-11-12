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

#ifndef _ID3CONTENTPLUGIN_H
#define _ID3CONTENTPLUGIN_H

#include <qcontentplugin.h>
#include <qtopiaglobal.h>

class Id3Tag;

class QTOPIA_PLUGIN_EXPORT Id3ContentPlugin : public QObject, public QContentPlugin
{
    Q_OBJECT
    Q_INTERFACES(QContentPlugin)
public:
    Id3ContentPlugin();
    ~Id3ContentPlugin();

    virtual QStringList keys() const;
    virtual bool installContent( const QString &filePath, QContent *content );
    virtual bool updateContent( QContent *content );

private:
    QString name( const QString &filePath, Id3Tag &tag ) const;

    QString string( Id3Tag &tag, qint64 framePosition ) const;
    QString genre( Id3Tag &tag, qint64 framePosition ) const;

};

#endif
