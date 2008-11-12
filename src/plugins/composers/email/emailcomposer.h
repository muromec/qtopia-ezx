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

#ifndef EMAILCOMPOSER_H
#define EMAILCOMPOSER_H

#include <QContent>
#include <QList>
#include <QTextEdit>

#include <qtopia/mail/qmailcomposer.h>
#include <qtopia/mail/qmailcomposerplugin.h>

class AddAttDialog;
class QLabel;
class QWidget;

class EmailComposer : public QTextEdit
{
    Q_OBJECT

public:
    EmailComposer( QWidget *parent = 0, const char* name = 0 );

    AddAttDialog *addAttDialog();

    using QTextEdit::setPlainText;

    void setPlainText( const QString& text, const QString& signature );

    bool isEmpty() const;

signals:
    void contentChanged();
    void attachmentsChanged();
    void finished();

protected slots:
    void selectAttachment();
    void updateLabel();
    void setCursorPosition();

protected:
    void keyPressEvent( QKeyEvent *e );

private:
    AddAttDialog *m_addAttDialog;
    int m_index;
};

class EmailComposerInterface : public QMailComposerInterface
{
    Q_OBJECT

public:
    EmailComposerInterface( QWidget *parent = 0 );
    ~EmailComposerInterface();

    bool isEmpty() const;
    QMailMessage message() const;

    QWidget *widget() const;

    virtual void addActions(QMenu* menu) const;

public slots:
    void setMessage( const QMailMessage &mail );
    void setText( const QString &txt, const QString &type );
    void clear();
    void attach( const QContent &lnk, QMailMessage::AttachmentsAction = QMailMessage::LinkToAttachments );
    void setSignature( const QString &sig );
    void attachmentsChanged();

private:
    EmailComposer *m_composer;
    QLabel *m_label;
    QWidget *m_widget;
    QList<QContent> m_temporaries;
    QString m_signature;
};

class EmailComposerPlugin : public QMailComposerPlugin
{
    Q_OBJECT

public:
    EmailComposerPlugin();

    virtual QString key() const;
    virtual QMailMessage::MessageType messageType() const;

    virtual QString name() const;
    virtual QString displayName() const;
    virtual QIcon displayIcon() const;

    QMailComposerInterface* create( QWidget* parent );
};

#endif
