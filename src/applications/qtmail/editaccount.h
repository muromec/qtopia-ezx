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



#ifndef EDITACCOUNT_H
#define EDITACCOUNT_H

#include "ui_editaccountbasephone.h"

#include <qlistwidget.h>
#include <qtextedit.h>
#include <qtimer.h>

class QMailAccount;
class QTabWidget;

class EditAccount : public QDialog, public Ui::EditAccountBase
{
    Q_OBJECT

public:
    EditAccount( QWidget* parent = 0, const char* name = 0,Qt::WFlags fl = 0 );
    virtual ~EditAccount(){};
    void setAccount(QMailAccount *in, bool newOne = true);
    bool eventFilter( QObject *, QEvent * );
//  bool event(QEvent* e);

protected slots:
    void accept();
    void deleteAccount();
    void emailModified();
    void typeChanged(int);
    void sigPressed();
//  void configureFolders();
    void authChanged(int index);
    void createTabbedView();
    void currentTabChanged(int index);

private:
    QMailAccount *account;
    bool emailTyped;
    QString sig;
    QTabWidget* tabWidget;
    QLineEdit* accountNameInput;
};

class SigEntry : public QDialog
{
    Q_OBJECT
public:
    SigEntry(QWidget *parent, const char* name, Qt::WFlags fl = 0 );
    void setEntry(QString sig) { input->insertPlainText(sig); };
    QString entry() { return input->toPlainText(); };

private:
    QTextEdit *input;
};
/*
class MailboxView : public QListWidget
{
    Q_OBJECT
  public:
    MailboxView(QWidget *parent, const char *name);

  public slots:
    void showMessageOptions();

  protected:
    void mousePressEvent( QMouseEvent * e );
    void mouseReleaseEvent( QMouseEvent * e );
  protected slots:
    void cancelMenuTimer();

    void changeMessageSettings(QAction* e);

  private:
    QTimer menuTimer;
    QMenu *pop;
};

class MailboxSelector : public QDialog
{
    Q_OBJECT
  public:
    MailboxSelector(QWidget *parent, const char *name, QMailAccount *account );

  protected slots:
    void accept();

    void selectAll();
    void clearAll();
    void itemSelected();

  private:
    QMailAccount *_account;
    MailboxView *view;
};*/

#endif
