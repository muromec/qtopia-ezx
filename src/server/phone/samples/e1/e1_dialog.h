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

#ifndef _e1_DIALOG_H_
#define _e1_DIALOG_H_

#include <QDialog>
#include <QColor>

class QVBoxLayout;
class E1Bar;
class E1Dialog : public QDialog
{
public:
    enum Type {
        Generic,
        Return,
        NewMessage
    };

    E1Dialog( QWidget* parent, E1Dialog::Type t );

    E1Bar *bar() const;
    void setContentsWidget( QWidget* contentsWidget );

protected:
    virtual void showEvent(QShowEvent *);
    void paintEvent( QPaintEvent* e );
    void resizeEvent( QResizeEvent* e );
    void moveEvent( QMoveEvent* e );

    QColor highlightColor() const;

private:
    QVBoxLayout* m_layout;
    QWidget* m_contentsWidget;
    E1Bar* m_bar;
};

#endif // _e1_DIALOG_H_

