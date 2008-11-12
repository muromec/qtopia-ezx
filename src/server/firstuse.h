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

#ifndef FIRSTUSE_H
#define FIRSTUSE_H

#include <qdialog.h>
#include <qpixmap.h>
#include <qfont.h>

class InputMethods;
class QPushButton;
class QLabel;

class FirstUse : public QDialog
{
    Q_OBJECT
public:
    FirstUse(QWidget* parent=0, Qt::WFlags=0);
    ~FirstUse();

    bool restartNeeded() const { return needRestart; }
    void reloadLanguages();

private slots:
    void calcMaxWindowRect();
    void nextDialog();
    void previousDialog();
    void switchDialog();

protected:
    void paintEvent( QPaintEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void keyPressEvent( QKeyEvent *e );

private:
    void loadPixmaps();
    void drawText(QPainter &p, const QString &text);
    int findNextDialog(bool forwards);
    void updateButtons();

private:
    QPixmap splash;
    QPixmap buttons;
    int currDlgIdx;
    QDialog *currDlg;
    InputMethods *inputMethods;
    QPushButton *back;
    QPushButton *next;
    int controlHeight;
    QString lang;
    bool needCalibrate;
    QWidget *taskBar;
    QLabel *titleBar;
    bool needRestart;
    QFont defaultFont;
};

#endif

