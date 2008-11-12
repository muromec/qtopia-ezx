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

#ifndef QMAILCOMPOSER_H
#define QMAILCOMPOSER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QIconSet>

#include <qtopiaglobal.h>
#include <qmailmessage.h>

class QContent;
class QMenu;
class QWidget;

class QTOPIAMAIL_EXPORT QMailComposerInterface : public QObject
{
    Q_OBJECT

public:
    QMailComposerInterface( QWidget *parent = 0 );
    virtual ~QMailComposerInterface();

    QString key() const;
    QMailMessage::MessageType messageType() const;

    QString name() const;
    QString displayName() const;
    QIcon displayIcon() const;

    virtual bool isEmpty() const = 0;
    virtual QMailMessage message() const = 0;

    virtual QWidget *widget() const = 0;

    virtual void addActions(QMenu* menu) const;

public slots:
    virtual void setMessage( const QMailMessage& mail ) = 0;
    virtual void clear() = 0;
    virtual void setText( const QString &text, const QString &type );
    virtual void attach( const QContent &lnk, QMailMessage::AttachmentsAction action = QMailMessage::LinkToAttachments );
    virtual void setSignature( const QString &sig );

signals:
    void contentChanged();
    void finished();
};

class QTOPIAMAIL_EXPORT QMailComposerFactory
{
public:
    // Yield the key for each interface supporting the supplied type
    static QStringList keys( QMailMessage::MessageType type = QMailMessage::AnyType );

    // Yield the default key for the supplied type
    static QString defaultKey( QMailMessage::MessageType type = QMailMessage::AnyType );

    // Properties available for each interface
    static QMailMessage::MessageType messageType( const QString& key );
    static QString name( const QString& key );
    static QString displayName( const QString& key );
    static QIcon displayIcon( const QString& key );

    // Use the interface identified by the supplied key to create a composer
    static QMailComposerInterface *create( const QString& key, QWidget *parent = 0 );
};

#endif // QMAILCOMPOSER_H
