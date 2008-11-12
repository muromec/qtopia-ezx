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



#ifndef READMAIL_H
#define READMAIL_H

#include <qaction.h>
#include <qmainwindow.h>
#include <qmenudata.h>
#include <qmenubar.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qmime.h>

#include <quuid.h>
#include <qurl.h>
#include <qmap.h>

#include "emaillistitem.h"
#include "viewatt.h"
#include <QMailMessage>

class QLabel;
class MailListView;
class QMenu;
class QStackedWidget;
class QContactModel;
class QMailViewerInterface;
class AccountList;
class QContactModel;

class ReadMail : public QMainWindow
{
    Q_OBJECT

public:
    enum ResendAction { Reply = 1, ReplyToAll = 2, Forward = 3 };

    ReadMail( QWidget* parent = 0, const QString name = QString(), Qt::WFlags fl = 0 );
    ~ReadMail();

    void setAccountList(AccountList* list);

    void viewSelectedMail(MailListView *view);
    void mailUpdated(const QMailId& message);

private slots:
    void updateView();

signals:
    void resendRequested(const QMailMessage&, int);
    void mailto(const QString &);
    void modifyRequested(const QMailMessage&);
    void removeItem(EmailListItem *);
    void viewingMail(const QMailMessage&);
    void getMailRequested(const QMailMessage&);
    void sendMailRequested(const QMailMessage&);
    void readReplyRequested(const QMailMessage&);
    void cancelView();

public slots:
    void cleanup();
    void isSending(bool);
    void isReceiving(bool);

protected slots:
    void linkClicked(const QUrl &lnk);
    void closeView();

    void next();
    void previous();

    void deleteItem();
    void viewAttachments();

    void reply();
    void replyAll();
    void forward();
    void modify();

    void setStatus(int);
    void getThisMail();
    void sendThisMail();

    void storeContact();

protected:
    void keyPressEvent(QKeyEvent *);

private:
    void viewMms();

    void init();
    void showMail(const QMailId& message);
    void updateButtons();
    void buildMenu(const QString &mailbox);
    void initImages(QMailViewerInterface* view);

    bool hasGet(const QString &mailbox);
    bool hasSend(const QString &mailbox);
    bool hasEdit(const QString &mailbox);
    bool hasReply(const QString &mailbox);
    bool hasDelete(const QString &mailbox);

    void dialNumber(const QString&);

    void switchView(QWidget* widget, const QString& title);

private slots:
    void mmsFinished();
    void contactModelReset();

private:
    QStackedWidget *views;
    MailListView *mailView;
    bool sending, receiving;
    QMailMessage mail;
    QMailId lastMailId;
    ViewAtt *viewAtt;
    bool isMms;
    bool isSmil;
    bool firstRead;

    QMenu *context;

    QAction *deleteButton;
    bool initialized;
    QAction *nextButton;
    QMailViewerInterface *emailView;
    QMailViewerInterface *smilView;
    QAction *attachmentsButton;
    QAction *previousButton;
    QAction *replyButton;
    QAction *replyAllAction;
    QAction *forwardAction;
    QAction *getThisMailButton;
    QAction *sendThisMailButton;
    QAction *modifyButton;
    QAction *storeButton;
    AccountList *accountList;
    QContactModel *contactModel;
    bool modelUpdatePending;
};

#endif // READMAIL_H
