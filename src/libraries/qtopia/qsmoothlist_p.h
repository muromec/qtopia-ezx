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

#ifndef QSMOOTHLIST_P_H
#define QSMOOTHLIST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "motionpath_p.h"
#include <qtopiaglobal.h>
#include <QWidget>
#include <QTimeLine>
#include <QAbstractItemModel>
#include <QAbstractItemDelegate>

class ListItem;
class ItemHighlight;
class Scrollbar;
class FPainter;

class QTOPIA_EXPORT QSmoothList : public QWidget
{
Q_OBJECT
public:
    QSmoothList(QWidget * = 0, Qt::WFlags = 0);
    virtual ~QSmoothList();

    void setModel(QAbstractItemModel *);
    void setItemDelegate(QAbstractItemDelegate *);
    void setIconSize(const QSize &);
    QSize iconSize() { return m_iconSize; }
    void reset();
    
signals:
    void activated(const QModelIndex &index);

private slots:
    void updateList();
    void updateHighlight();
    void updateCompleted();
    void tryHideScrollBar();
    void doUpdate(bool force = false);
    void tickStart();
    void emitActivated();
    
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void completeDisable();

protected:
    virtual bool event(QEvent *);
    virtual void paintEvent(QPaintEvent *);
    virtual void showEvent(QShowEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);

private:
    void enableDirect();
    void disableDirect();

    void init();
    QRect fill();
    void refresh();
    void trim();
    void fixupPosition();
    bool flickList(qreal);
    void correctHighlight(qreal);
    qreal maxListPos() const;

    QAbstractItemModel *m_model;
    QAbstractItemDelegate *m_delegate;
    QList<ListItem *> m_items;

    MotionTimeLine m_listTimeline;
    ValueGroup m_listPosition;
    ConcreteValue m_concretePosition;

    MotionTimeLine m_pauseTimeline;

    MotionTimeLine m_highlightAlpha;
    MotionTimeLine m_highlightTimeline;
    ItemHighlight *m_highlight;

    QPoint m_origMousePos;
    QPoint m_prevMousePos;
    bool m_mouseClick;
    bool m_inFlick;

    int m_focusItem;

    MotionVelocity m_velocity;

    MotionTimeLine m_scrollTime;
    Scrollbar *m_scrollbar;
    bool scrollBarIsVisible() const;
    void showScrollBar();
    void hideScrollBar();
    bool m_scrollOn;
    bool m_needRefreshOnShow;

    FPainter *fp;
    QRect reserve;

    QImage background;
    
    QSize m_iconSize;
    
    MotionTimeLine m_activatedTimeline;
};

#endif // QSMOOTHLIST_P_H
