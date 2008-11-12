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

/*
 * KAsteroids - Copyright (c) Martin R. Jones 1997
 *
 * Part of the KDE project
 */

#ifndef __AST_VIEW_H__
#define __AST_VIEW_H__

#include <qwidget.h>
#include <qlist.h>
#include <QHash>
#include <qtimer.h>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "sprites.h"

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
 public:
    MyGraphicsView(QGraphicsScene* scene, QWidget* parent = 0)
        : QGraphicsView(scene,parent) { }

 protected:
    void resizeEvent(QResizeEvent* event);
};

class KAsteroidsView : public QWidget
{
    Q_OBJECT

 public:
    KAsteroidsView(QWidget* parent = 0);
    virtual ~KAsteroidsView();

    void newGame();
    void newShip();
    void raiseShield();
    void teleport();
    void pause(bool p);
    void reportMissileFired();
    void reportRockDestroyed(int rock_size);

    void showText(const QString &text);
    void hideText();

    void constructMessages(const QString& t);
    void reportStartGame();
    void reportShipKilled();
    void reportGameOver();
    void markVitalsChanged() { vitalsChanged_ = true; }

 signals:
    void missileFired();
    void shipKilled();
    void updateScore(int key);
    void allRocksDestroyed();
    void updateVitals();

 private slots:
    void dropShield();
    void mainTimerEvent();

 private:
    QGraphicsScene* 	scene_;
    MyGraphicsView* 	view_;
    QGraphicsSimpleTextItem* 	textSprite_;

    bool		instruct_user_;
    bool 		game_paused_;
    bool 		vitalsChanged_;
    bool		textPending_;

    int 		textDy_;
    int 		timerEventCount_;

    QTimer*		shieldTimer_;
    QTimer*             mainTimer_;

    QString		nextGameMessage_;
    QString		firstGameMessage_;
    QString		shipKilledMessage_;
    QString		gameOverMessage_;
};

#endif
