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
#ifndef BEAMING_H
#define BEAMING_H

#include <QDialog>

class IRController;
class QListWidget;
class QListWidgetItem;

class Beaming : public QDialog
{
    Q_OBJECT

public:
    Beaming( QWidget* parent = 0, Qt::WFlags fl = 0 );
    ~Beaming();

protected:
    void accept();

private slots:
    void chooseState(int c);
    void chooseProtocol(QListWidgetItem *);

private:
    IRController *irc;
    QListWidget *lb;
    int state;
    int protocol;
};

#endif // BEAMING_H

