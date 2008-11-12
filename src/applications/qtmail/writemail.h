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

#ifndef WRITEMAIL_H
#define WRITEMAIL_H

#include <QMainWindow>
#include <QString>
#include <QMailMessage>

class QAction;
class QContent;
class QMailComposerInterface;
class QStackedWidget;

class AccountList;
class DetailsPage;
class SelectListWidget;

class WriteMail : public QMainWindow
{
    Q_OBJECT

public:
    enum ComposeAction { Create = 0, Reply = 1, ReplyToAll = 2, Forward = 3 };

    WriteMail( QWidget* parent, const char* name, Qt::WFlags fl = 0 );
    ~WriteMail();
    void reply(const QMailMessage& replyMail, int type);
    void modify(const QMailMessage& previousMessage);
    void setRecipients(const QString &emails, const QString &numbers);
    void setRecipient(const QString &recipient);
    bool readyToSend() const;
    void setSubject(const QString &subject);
    void setBody(const QString &text, const QString &type);
    bool hasContent();
#ifndef QTOPIA_NO_SMS
    void setSmsRecipient(const QString &recipient);
#endif
    void setAccountList(AccountList *list);

    QString composer() const;
    void setComposer( const QString &id );

    bool isComplete() const;
    bool changed() const;
    bool canClose();

    void setAction(ComposeAction action);

    bool forcedClosure();

public slots:
    bool saveChangesOnRequest();
    void reset();
    void discard();
    bool draft();
    bool composerSelected(const QString &key);
    void selectionCanceled();

signals:
    void autosaveMail(const QMailMessage&);
    void saveAsDraft(const QMailMessage&);
    void enqueueMail(const QMailMessage&);
    void discardMail();
    void noSendAccount(QMailMessage::MessageType);

public slots:
    bool newMail( const QString &cmpsr = QString(), bool detailsOnly = false );
    void attach( const QContent &dl, QMailMessage::AttachmentsAction );

protected slots:
    void previousStage();
    void nextStage();
    void composeStage();
    void detailsStage();
    void sendStage();

    void enqueue();

    void messageChanged();
    void detailsChanged();

private:
    bool largeAttachments();
    uint largeAttachmentsLimit() const;

    bool buildMail();
    void init();
    QString signature() const;

    void showDetailsPage();

    QMailMessage mail;

    QMailComposerInterface *m_composerInterface;

    QWidget *m_composerWidget;
    DetailsPage *m_detailsPage;
    QAction *m_previousAction;
    QAction *m_cancelAction, *m_draftAction;

    QStackedWidget* widgetStack;

    QMainWindow *m_mainWindow;
    AccountList *accountList;
    bool hasMessageChanged;
    bool _detailsOnly;
    ComposeAction _action;
    QWidget *_selectComposer;
    SelectListWidget *_composerList;
    QList<QMailMessage::MessageType> _sendTypes;
};

#endif // WRITEMAIL_H
