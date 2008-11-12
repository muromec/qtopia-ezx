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

#ifndef DELAYEDWAITDIALOG_H
#define DELAYEDWAITDIALOG_H

#include <QDialog>
#include <QList>

class Icon;
class QTimer;
class QLabel;

class DelayedWaitDialog : public QDialog
{
    Q_OBJECT
public:
    DelayedWaitDialog( QWidget *parent = 0, Qt::WFlags fl = 0 );
    ~DelayedWaitDialog();

    void setText( const QString &str );
    void setDelay( int ms );

protected:
    virtual void showEvent( QShowEvent *se );
    virtual void hideEvent( QHideEvent *he );
    virtual void keyReleaseEvent( QKeyEvent *ke );
    virtual void timerEvent( QTimerEvent *te );

public slots:
    virtual void show();
    virtual void hide();

protected slots:
    void update();

private:
    QList<Icon*> mIconList;
    QLabel *text;
    static const int NUMBEROFICON = 10;
    QTimer *mTimer;
    int mDelay;
    int mTid;
};

#endif //DELAYEDWAITDIALOG_H
