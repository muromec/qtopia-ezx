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

#ifndef GENERICCOMPOSER_H
#define GENERICCOMPOSER_H

#include <QLineEdit>

#include <qtopia/mail/qmailcomposer.h>
#include <qtopia/mail/qmailcomposerplugin.h>

class AddAttDialog;
class QTextEdit;
class QLabel;
class QAction;

class GenericComposer : public QWidget
{
    Q_OBJECT
public:
    GenericComposer( QWidget *parent = 0 );
    ~GenericComposer();

    void setText( const QString &t, const QString &type );
    QString text() const;
    bool isVCard() const { return m_vCard; }

    void addActions(QMenu* menu);

signals:
    void contentChanged();
    void finished();

protected slots:
    void updateSmsLimitIndicator();
    void templateText();
    void textChanged();

#ifndef QTOPIA_NO_SMS
    void smsLengthInfo(uint& estimatedBytes, bool& isUnicode);
    int smsCountInfo();
#endif

private:
    QTextEdit *m_textEdit;
    QLabel *m_smsLimitIndicator;
    QAction *m_showLimitAction;
    QAction *m_templateTextAction;
    bool m_vCard;
    QString m_vCardData;
};

class GenericComposerInterface : public QMailComposerInterface
{
    Q_OBJECT

public:
    GenericComposerInterface( QWidget *parent = 0 );
    ~GenericComposerInterface();

    bool isEmpty() const;
    QMailMessage message() const;

    QWidget *widget() const;

    virtual void addActions(QMenu* menu) const;

public slots:
    void setMessage( const QMailMessage &mail );
    void setText( const QString &txt, const QString &type );
    void clear();
    void attach( const QContent &lnk, QMailMessage::AttachmentsAction action = QMailMessage::LinkToAttachments );

private:
    GenericComposer *m_composer;
};

class GenericComposerPlugin : public QMailComposerPlugin
{
    Q_OBJECT

public:
    GenericComposerPlugin();

    virtual QString key() const;
    virtual QMailMessage::MessageType messageType() const;

    virtual QString name() const;
    virtual QString displayName() const;
    virtual QIcon displayIcon() const;

    virtual bool isSupported( QMailMessage::MessageType type ) const;

    QMailComposerInterface* create( QWidget* parent );
};

#endif
