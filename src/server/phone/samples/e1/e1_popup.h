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

#ifndef _EXZ_POPUP_H_
#define _EXZ_POPUP_H_

#include <QPixmap>
#include <QWidget>
#include <QSize>
class QListWidget;
class QMouseEvent;
class QKeyEvent;

class E1Popup : public QWidget
{
Q_OBJECT
public:
    E1Popup();

    void addItem(const QString &);
    void addItem(const QPixmap &);
    void addItem(E1Popup *);

    virtual QSize sizeHint() const;

    void popup(const QPoint &);


signals:
    void closed();
    void selected(int);

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void showEvent(QShowEvent *);

private slots:
    void doClose();

private:
    struct PopupItem {
        PopupItem() : popup(0) {}
        QString text;
        QPixmap pix;
        E1Popup *popup;

        QSize size;
    };
    QList<PopupItem> m_items;
    QSize sizeHint(const PopupItem &) const;
    void paint(QPainter *, const QRect &, const PopupItem &, bool);
    int findItem(const QPoint &);

    int m_selected;
};

#endif // _EXZ_POPUP_H_

