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

#ifndef _SLIDEINMESSAGEBOX_H_
#define _SLIDEINMESSAGEBOX_H_

#include <QWidget>
#include "qabstractmessagebox.h"
class QString;
class SlideInMessageBoxPrivate;

class SlideInMessageBox : public QAbstractMessageBox
{
Q_OBJECT
public:
    SlideInMessageBox(QWidget *parent = 0, Qt::WFlags flags = 0);
    virtual ~SlideInMessageBox();

    virtual void setButtons(Button button1, Button button2);
    virtual void setButtons(const QString &button0Text, const QString &button1Text, const QString &button2Text,
            int defaultButtonNumber, int escapeButtonNumber);

    virtual Icon icon() const;
    virtual void setIcon(Icon);

    virtual QString title() const;
    virtual void setTitle(const QString &);

    virtual QString text() const;
    virtual void setText(const QString &);

    virtual QPixmap pixmap() const;
    virtual void setPixmap(const QPixmap &);

protected:
    virtual void showEvent(QShowEvent *);
    virtual void paintEvent(QPaintEvent *);
    virtual void keyPressEvent(QKeyEvent *);

private slots:
    void valueChanged(qreal);

private:
    void renderBox();

    void animate();
    void drawFrame(QPainter *painter, const QRect &r, const QString &title);

    SlideInMessageBoxPrivate *d;
};

#endif // _SLIDEINMESSAGEBOX_H_

