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

#ifndef __DIRDELETERDIALOG_H__
#define __DIRDELETERDIALOG_H__

#include <QDialog>
#include <QString>

class QObexFtpClient;
class DirDeleterDialogPrivate;
class QObexFolderListingEntryInfo;
class QPushButton;
class QLabel;
class QKeyEvent;
class DirDeleter;

class DirDeleterDialog : public QDialog
{
    Q_OBJECT

public:
    enum Result { Success, Canceled, Failed };

    static DirDeleterDialog::Result
            deleteDirectory(const QString &dir, QObexFtpClient *client, QWidget *parent = 0);

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:
    void cancel();
    void deleteDone(bool error);

private:
    DirDeleterDialog(const QString &dir, QObexFtpClient *client, QWidget *parent = 0);
    ~DirDeleterDialog();

    QPushButton *m_cancelButton;
    QLabel *m_currentLabel;
    QLabel *m_removingLabel;
    DirDeleter *m_deleter;
};

#endif
