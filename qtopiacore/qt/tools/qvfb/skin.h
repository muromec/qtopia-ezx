/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef SKIN_H
#define SKIN_H

#include <QWidget>
#include <QPolygon>
#include <QRegion>
#include <QPixmap>

class QVFb;
class QVFbView;
class CursorWindow;
class QTextStream;

class Skin : public QWidget
{
    Q_OBJECT
public:
    Skin( QVFb *p, const QString &skinFile, int &viewW, int &viewH );
    ~Skin( );
    void setView( QVFbView *v );
    void setSecondaryView( QVFbView *v );
    void setZoom( double );
    bool isValid() {return skinValid;}

    bool hasCursor() const;
    static QSize screenSize(const QString &skinFile);
    static QSize secondaryScreenSize(const QString &skinFile);
    static bool hasSecondaryScreen(const QString &skinFile);

protected slots:
    void skinKeyRepeat();
    void moveParent();

protected:
    virtual void paintEvent( QPaintEvent * );
    virtual void mousePressEvent( QMouseEvent *e );
    virtual void mouseMoveEvent( QMouseEvent *e );
    virtual void mouseReleaseEvent( QMouseEvent * );

private:
    QVFb *parent;
    QVFbView *view;
    QVFbView *secondaryView;
    QPoint parentpos;
    QPoint clickPos;
    bool buttonPressed;
    int buttonIndex;
    bool skinValid;
    double zoom;
    void calcRegions();
    void flip(bool open);
    void updateSecondaryScreen();

    static QString skinFileName(const QString &skinFile, QString* prefix);
    static void parseRect(const QString &, QRect *);
    static bool parseSkinFileHeader(QTextStream& ts,
                    QRect *screen, QRect *outsideScreen,
                    QRect *outsideScreenClosed,
		    int *numberOfAreas,
		    QString* skinImageUpFileName,
		    QString* skinImageDownFileName,
		    QString* skinImageClosedFileName,
		    QString* skinCursorFileName,
		    QPoint* cursorHot,
                    QStringList* closedAreas,
		    QStringList* toggleAreas,
		    QStringList* toggleActiveAreas);

    void loadImages();
    QString skinImageUpFileName;
    QString skinImageDownFileName;
    QString skinImageClosedFileName;
    QString skinCursorFileName;
    QPixmap skinImageUp;
    QPixmap skinImageDown;
    QPixmap skinImageClosed;
    QPixmap skinCursor;
    QRect screenRect;
    QRect backScreenRect;
    QRect closedScreenRect;
    int numberOfAreas;

    typedef struct {
	QString	name;
        int	keyCode;
        QPolygon area;
        QRegion region;
	QString text;
        bool activeWhenClosed;
	bool toggleArea;
	bool toggleActiveArea;
    } ButtonAreas;

    void startPress(int);
    void endPress();

    ButtonAreas *areas;
    QList<int> toggleAreaList;

    CursorWindow *cursorw;
    QPoint cursorHot;

    int joystick;
    bool joydown;
    QTimer *t_skinkey;
    QTimer *t_parentmove;
    int onjoyrelease;

    bool flipped_open;

    void setupDefaultButtons();
    QString prefix;
};

#endif
