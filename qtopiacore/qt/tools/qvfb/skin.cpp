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

#include "skin.h"
#include "qvfb.h"
#include "qvfbview.h"

#include <qnamespace.h>
#include <QApplication>
#include <QBitmap>
#include <QPixmap>
#include <QPainter>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTimer>
#include <QDir>
#include <QRegExp>
#include <QMouseEvent>
#include <QDebug>

#include <unistd.h>
#include <stdlib.h>

const int key_repeat_delay = 500;
const int key_repeat_period = 50;
const int joydistance = 10;

class CursorWindow : public QWidget
{
public:
    CursorWindow( const QString& fn, QPoint hot, QWidget *sk);

    void setView(QVFbView*);
    void setPos(QPoint);
    bool handleMouseEvent(QEvent *ev);

protected:
    bool event( QEvent *);
    bool eventFilter( QObject*, QEvent *);

private:
    QWidget *mouseRecipient;
    QVFbView *view;
    QWidget *skin;
    QPoint hotspot;
};

QSize Skin::screenSize(const QString &skinFile)
{
    QString _skinFileName = skinFileName(skinFile,0);
    if ( _skinFileName.isEmpty() )
        return QSize(0,0);
    QFile f( _skinFileName );
    f.open( QIODevice::ReadOnly );
    QTextStream ts( &f );
    QRect rect;
    parseSkinFileHeader(ts,
            &rect, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0);
    return rect.size();
}

QSize Skin::secondaryScreenSize(const QString &skinFile)
{
    QString _skinFileName = skinFileName(skinFile,0);
    if ( _skinFileName.isEmpty() )
        return QSize(0,0);
    QFile f( _skinFileName );
    f.open( QIODevice::ReadOnly );
    QTextStream ts( &f );
    QRect rect;
    QRect crect;
    parseSkinFileHeader(ts,
            0, &rect, &crect,
            0, 0, 0, 0, 0, 0, 0, 0, 0);
    return (rect.isNull() ? crect.size() : rect.size());
}

bool Skin::hasSecondaryScreen(const QString &skinFile)
{
    return secondaryScreenSize(skinFile) != QSize(0, 0);
}

QString Skin::skinFileName(const QString &rawSkinFile, QString* prefix)
{
    // remove ending '/' if present
    QString skinFile = rawSkinFile;
    if (skinFile.endsWith('/'))
        skinFile.truncate(skinFile.length() - 1);

    QFileInfo fi(skinFile);
    QString pref;
    QString fn;
    if ( fi.isDir() ) {
	pref = skinFile + "/";
	fn = pref + fi.baseName() + ".skin";
    } else if (fi.isFile()){
	fn = skinFile;
	pref = fi.path() + "/";
    }else if (!skinFile.isNull()){
	qDebug("Skin file \"%s\" not found", skinFile.toLatin1().constData());
	return "";
    }
    if ( prefix ) *prefix = pref;
    return fn;
}

void Skin::parseRect(const QString &value, QRect *rect) {
    QStringList l = value.split(" ");
    rect->setRect(l[0].toInt(), l[1].toInt(), l[2].toInt(), l[3].toInt());
}

bool Skin::parseSkinFileHeader(QTextStream& ts,
                    QRect *screen, QRect *backScreen,
                    QRect *closedScreen,
		    int *numberOfAreas,
		    QString* skinImageUpFileName,
		    QString* skinImageDownFileName,
		    QString* skinImageClosedFileName,
		    QString* skinCursorFileName,
		    QPoint* cursorHot,
                    QStringList* closedAreas,
		    QStringList *toggleAreas,
		    QStringList *toggleActiveAreas)
{
    QString mark;
    ts >> mark;
    if ( mark == "[SkinFile]" ) {
	// New
	int nareas=0;
	while (!nareas) {
	    QString line = ts.readLine();
	    if ( line.isNull() )
		break;
	    if ( line[0] != '#' && !line.isEmpty() ) {
		int eq = line.indexOf('=');
		if ( eq >= 0 ) {
		    QString key = line.left(eq);
		    eq++;
		    while (eq<(int)line.length()-1 && line[eq]==' ')
			eq++;
		    QString value = line.mid(eq);
		    if ( key == "Up" ) {
			if ( skinImageUpFileName ) *skinImageUpFileName = value;
		    } else if ( key == "Down" ) {
			if ( skinImageDownFileName ) *skinImageDownFileName = value;
		    } else if ( key == "Closed" ) {
			if ( skinImageClosedFileName ) *skinImageClosedFileName = value;
		    } else if ( key == "ClosedAreas" ) {
			if ( closedAreas ) {
                            *closedAreas = value.split(" ");
                        }
		    } else if ( key == "ToggleAreas" ) {
			if ( toggleAreas ) {
                            *toggleAreas = value.split(" ");
                        }
		    } else if ( key == "ToggleActiveAreas" ) {
			if ( toggleActiveAreas ) {
                            *toggleActiveAreas = value.split(" ");
                        }
		    } else if ( key == "Screen" ) {
                        if ( screen ) parseRect( value, screen );
		    } else if ( key == "BackScreen" ) {
                        if ( backScreen ) parseRect( value, backScreen );
		    } else if ( key == "ClosedScreen" ) {
                        if ( closedScreen ) parseRect( value, closedScreen );
		    } else if ( key == "Cursor" ) {
			QStringList l = value.split(" ");
			if ( skinCursorFileName ) *skinCursorFileName = l[0];
			if ( cursorHot ) *cursorHot = QPoint(l[1].toInt(),l[2].toInt());
		    } else if ( key == "Areas" ) {
			nareas = value.toInt();
		    }
		} else {
		    qDebug("Broken line: %s",line.toLatin1().constData());
		}
	    }
	}
	if ( numberOfAreas )
	    *numberOfAreas = nareas;
    } else {
	// Old
	if ( skinImageUpFileName ) *skinImageUpFileName = mark;
	QString s;
	int x,y,w,h,na;
	ts >> s >> x >> y >> w >> h >> na;
	if ( skinImageDownFileName ) *skinImageDownFileName = s;
        if ( screen ) screen->setRect(x, y, w, h);
	if ( numberOfAreas ) *numberOfAreas = na;
    }
    return true;
}

Skin::Skin( QVFb *p, const QString &skinFile, int &viewW, int &viewH ) :
    QWidget(p), view(0), secondaryView(0),
    buttonPressed(false), buttonIndex(0), skinValid(false),
    zoom(1.0), numberOfAreas(0), areas(0),
    cursorw(0),
    joystick(-1), joydown(0),
    flipped_open(true)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_NoSystemBackground);
    parent = p;

    QString _skinFileName = skinFileName(skinFile,&prefix);
    if ( _skinFileName.isEmpty() ) {
        skinValid = false;
        return;
    }
    QFile f( _skinFileName );
    f.open( QIODevice::ReadOnly );
    QStringList closedAreas;
    QStringList toggleAreas;
    QStringList toggleActiveAreas;
    QTextStream ts( &f );
    parseSkinFileHeader(ts,
            &screenRect, &backScreenRect, &closedScreenRect,
            &numberOfAreas,
	&skinImageUpFileName, &skinImageDownFileName, &skinImageClosedFileName,
	&skinCursorFileName, &cursorHot, &closedAreas,
	&toggleAreas, &toggleActiveAreas);

    viewW = screenRect.width();
    viewH = screenRect.height();

//  Debug the skin file parsing
    areas = new ButtonAreas[numberOfAreas];

    skinImageUpFileName = prefix + skinImageUpFileName;
    skinImageDownFileName = prefix + skinImageDownFileName;
    if ( !skinImageClosedFileName.isEmpty() )
	skinImageClosedFileName = prefix + skinImageClosedFileName;
    if ( !skinCursorFileName.isEmpty() )
	skinCursorFileName = prefix + skinCursorFileName;

    // verify skin files exist
    if (!QFile(skinImageUpFileName).exists()) {
        qWarning() << "failed to find skin up image file named: " << skinImageUpFileName;
        skinValid = false;
        return;
    }
    if (!QFile(skinImageDownFileName).exists()) {
        qWarning() << "failed to find skin down image file named: " << skinImageDownFileName;
        skinValid = false;
        return;
    }
    if (!skinImageClosedFileName.isEmpty() && !QFile(skinImageClosedFileName).exists()) {
        qWarning() << "failed to find skin closed image file named: " << skinImageClosedFileName;
        skinValid = false;
        return;
    }
    if (!skinCursorFileName.isEmpty() && !QFile(skinCursorFileName).exists()) {
        qWarning() << "failed to find skin cursor image file named: " << skinCursorFileName;
        skinValid = false;
        return;
    }

    int i = 0;
    ts.readLine(); // eol
    joystick = -1;
    while (i < numberOfAreas && !ts.atEnd() ) {
	QString line = ts.readLine();
	if ( line[0] != '#' && !line.isEmpty() ) {
	    QStringList tok = line.split(QRegExp("[ \t][ \t]*"));
	    if ( tok.count()<6 ) {
		qDebug("Broken line: %s",line.toLatin1().constData());
	    } else {
		areas[i].name = tok[0];
		QString k = tok[1];
		if ( k.left(2).toLower() == "0x" ) {
		    areas[i].keyCode = k.mid(2).toInt(0,16);
		} else {
		    areas[i].keyCode = k.toInt();
		}

		int p=0;
		for (int j=2; j < tok.count() - 1; ) {
		    int x = tok[j++].toInt();
		    int y = tok[j++].toInt();
		    areas[i].area.putPoints(p++,1,x,y);
		}

		if ( areas[i].name[0] == '"' && areas[i].name.right(1) == "\"" )
		    areas[i].name = areas[i].name.mid(1,areas[i].name.length()-2);
		if ( areas[i].name.length() == 1 )
		    areas[i].text = areas[i].name;
		if ( areas[i].name == "Joystick" )
		    joystick = i;
		areas[i].activeWhenClosed = closedAreas.contains(areas[i].name)
                    || areas[i].keyCode == Qt::Key_Flip; // must be to work
		areas[i].toggleArea = toggleAreas.contains(areas[i].name);
		areas[i].toggleActiveArea =
		    toggleActiveAreas.contains(areas[i].name);
		if ( areas[i].toggleArea )
		    toggleAreaList += i;
		i++;
	    }
	}
    }
    setZoom(1.0);
    t_skinkey = new QTimer( this );
    connect( t_skinkey, SIGNAL(timeout()), this, SLOT(skinKeyRepeat()) );
    t_parentmove = new QTimer( this );
    t_parentmove->setSingleShot( true );
    connect( t_parentmove, SIGNAL(timeout()), this, SLOT(moveParent()) );

    skinValid = true;
}

void Skin::skinKeyRepeat()
{
    if ( view ) {
	view->skinKeyReleaseEvent( areas[buttonIndex].keyCode, areas[buttonIndex].text, true );
	view->skinKeyPressEvent( areas[buttonIndex].keyCode, areas[buttonIndex].text, true );
	t_skinkey->start(key_repeat_period);
    }
}

void Skin::calcRegions()
{
    for (int i=0; i<numberOfAreas; i++) {
	QPolygon xa(areas[i].area.count());
	int n = areas[i].area.count();
	for (int p=0; p<n; p++) {
	    xa.setPoint(p,
		int(areas[i].area[p].x()*zoom),
		int(areas[i].area[p].y()*zoom));
	}
	if ( n == 2 ) {
	    areas[i].region = QRegion(xa.boundingRect());
	} else {
	    areas[i].region = QRegion(xa);
	}
    }
}

void Skin::loadImages()
{
    QImage iup;
    if ( !skinImageUpFileName.isEmpty() )
	iup.load(skinImageUpFileName);
    QImage idown(skinImageDownFileName);
    if ( !skinImageDownFileName.isEmpty() )
	idown.load(skinImageDownFileName);
    QImage iclosed;
    if ( !skinImageClosedFileName.isEmpty() )
	iclosed.load(skinImageClosedFileName);
    QImage icurs;
    if ( !skinCursorFileName.isEmpty() )
	icurs.load(skinCursorFileName);

    if ( zoom != int(zoom) ) {
	iup = iup.scaled(int(iup.width()*zoom),int(iup.height()*zoom), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	idown = idown.scaled(int(idown.width()*zoom),int(idown.height()*zoom), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	if ( !skinImageClosedFileName.isEmpty() )
	    iclosed = iclosed.scaled(int(idown.width()*zoom),int(idown.height()*zoom), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	if ( !skinCursorFileName.isEmpty() )
	    icurs = icurs.scaled(int(idown.width()*zoom),int(idown.height()*zoom), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    Qt::ImageConversionFlags conv = Qt::ThresholdAlphaDither|Qt::AvoidDither;
    if ( !skinImageUpFileName.isEmpty() )
	skinImageUp = QPixmap::fromImage(iup);
    if ( !skinImageDownFileName.isEmpty() )
	skinImageDown = QPixmap::fromImage(idown, conv);
    if ( !skinImageClosedFileName.isEmpty() )
	skinImageClosed = QPixmap::fromImage(iclosed, conv);
    if ( !skinCursorFileName.isEmpty() )
	skinCursor = QPixmap::fromImage(icurs, conv);

    if ( zoom == int(zoom) ) {
	QMatrix scale; scale = scale.scale(zoom,zoom);
	skinImageUp = skinImageUp.transformed(scale);
	skinImageDown = skinImageDown.transformed(scale);
	if ( !skinImageClosedFileName.isEmpty() )
	    skinImageClosed = skinImageClosed.transformed(scale);
	if ( !skinCursorFileName.isEmpty() )
	    skinCursor = skinCursor.transformed(scale);
    }

    setFixedSize( skinImageUp.size() );
    if (!skinImageUp.mask())
	skinImageUp.setMask(skinImageUp.createHeuristicMask());
    if (!skinImageClosed.mask())
	skinImageClosed.setMask(skinImageClosed.createHeuristicMask());

    QWidget* parent = parentWidget();
    parent->setMask( skinImageUp.mask() );
    parent->setFixedSize( skinImageUp.size() );

    delete cursorw;
    cursorw = 0;
    if ( !skinCursorFileName.isEmpty() ) {
	cursorw = new CursorWindow(skinCursorFileName,cursorHot,this);
	if ( view )
	    cursorw->setView(view);
    }
}

Skin::~Skin( )
{
    delete cursorw;
}

void Skin::setZoom( double z )
{
    zoom = z;
    calcRegions();
    loadImages();
    if ( view )
	view->move( int(screenRect.x()*zoom), int(screenRect.y()*zoom) );
    updateSecondaryScreen();
}

void Skin::updateSecondaryScreen()
{
    if (!secondaryView)
        return;
    if (flipped_open) {
        if (backScreenRect.isNull()) {
            secondaryView->hide();
        } else {
            secondaryView->move( int(backScreenRect.x()*zoom), int(backScreenRect.y()*zoom) );
            secondaryView->show();
        }
    } else {
        if (closedScreenRect.isNull()) {
            secondaryView->hide();
        } else {
            secondaryView->move( int(closedScreenRect.x()*zoom), int(closedScreenRect.y()*zoom) );
            secondaryView->show();
        }
    }
}

void Skin::setView( QVFbView *v )
{
    view = v;
    view->setFocus();
    view->move( int(screenRect.x()*zoom), int(screenRect.y()*zoom) );
    if ( cursorw )
	cursorw->setView(v);

    setupDefaultButtons();
}

void Skin::setSecondaryView( QVFbView *v )
{
    secondaryView = v;
    updateSecondaryScreen();
}

void Skin::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    if ( flipped_open ) {
	p.drawPixmap( 0, 0, skinImageUp );
    } else {
	p.drawPixmap( 0, 0, skinImageClosed );
    }
    QList<int> toDraw;
    if (buttonPressed == true) {
	toDraw += buttonIndex;
    }
    foreach (int toggle, toggleAreaList) {
        ButtonAreas *ba = &areas[toggle];
	if ( flipped_open || ba->activeWhenClosed ) {
	    if ( ba->toggleArea && ba->toggleActiveArea )
		toDraw += toggle;
	}
    }
    foreach (int button, toDraw ) {
        ButtonAreas *ba = &areas[button];
        QRect r = ba->region.boundingRect();
        if ( ba->area.count() > 2 )
            p.setClipRegion(ba->region);
        p.drawPixmap( r.topLeft(), skinImageDown, r);
    }
}


void Skin::mousePressEvent( QMouseEvent *e )
{
    if (e->button() == Qt::RightButton) {
	parent->popupMenu();
    } else {
	buttonPressed = false;

	onjoyrelease = -1;
        for (int i = 0; i < numberOfAreas; i++) {
            ButtonAreas *ba = &areas[i];
            if ( ba->region.contains( e->pos() ) ) {
                if ( flipped_open || ba->activeWhenClosed ) {
                    if ( joystick == i ) {
                        joydown = true;
                    } else {
                        if ( joydown )
                            onjoyrelease = i;
                        else
                            startPress(i);
                        break;
//		Debug message to be sure we are clicking the right areas
//		printf("%s clicked\n", areas[i].name);
                    }
                }
            }
        }

//	This is handy for finding the areas to define rectangles for new skins
//	printf("Clicked in %i,%i\n",  e->pos().x(),  e->pos().y());
	clickPos = e->pos();
    }
}

void Skin::flip(bool open)
{
    if ( flipped_open == open )
	return;
    if ( open ) {
	parent->setMask( skinImageUp.mask() );
	view->skinKeyReleaseEvent( Qt::Key(Qt::Key_Flip), QString() );
    } else {
	parent->setMask( skinImageClosed.mask() );
	view->skinKeyPressEvent( Qt::Key(Qt::Key_Flip), QString() );
    }
    flipped_open = open;
    updateSecondaryScreen();
    repaint();
}

void Skin::startPress(int i)
{
    buttonPressed = true;
    buttonIndex = i;
    if (view) {
	if ( areas[buttonIndex].keyCode == Qt::Key_Flip ) {
	    flip(!flipped_open);
	} else if ( areas[buttonIndex].toggleArea ) {
	    bool active = !areas[buttonIndex].toggleActiveArea;
	    areas[buttonIndex].toggleActiveArea = active;
	    if ( active )
		view->skinKeyPressEvent( areas[buttonIndex].keyCode, areas[buttonIndex].text );
	    else
		view->skinKeyReleaseEvent( areas[buttonIndex].keyCode, areas[buttonIndex].text );
	} else {
	    view->skinKeyPressEvent( areas[buttonIndex].keyCode, areas[buttonIndex].text );
	    t_skinkey->start(key_repeat_delay);
	}
	ButtonAreas *ba = &areas[buttonIndex];
	repaint( ba->region.boundingRect() );
    }
}

void Skin::endPress()
{
    if (view && areas[buttonIndex].keyCode != Qt::Key_Flip &&
        !areas[buttonIndex].toggleArea)
	view->skinKeyReleaseEvent( areas[buttonIndex].keyCode, areas[buttonIndex].text );
    t_skinkey->stop();
    buttonPressed = false;
    ButtonAreas *ba = &areas[buttonIndex];
    repaint( ba->region.boundingRect() );
}

void Skin::mouseMoveEvent( QMouseEvent *e )
{
    if ( e->buttons() & Qt::LeftButton ) {
	QPoint newpos =  e->globalPos() - clickPos;
	if ( joydown ) {
	    int k1=0, k2=0;
	    if ( newpos.x() < -joydistance ) {
		k1 = joystick+1;
	    } else if ( newpos.x() > +joydistance ) {
		k1 = joystick+3;
	    }
	    if ( newpos.y() < -joydistance ) {
		k2 = joystick+2;
	    } else if ( newpos.y() > +joydistance ) {
		k2 = joystick+4;
	    }
	    if ( k1 || k2 ) {
		if ( !buttonPressed ) {
		    onjoyrelease = -1;
		    if ( k1 && k2 ) {
			startPress(k2);
			endPress();
		    }
		    startPress(k1 ? k1 : k2);
		}
	    } else if ( buttonPressed ) {
		endPress();
	    }
	} else if ( buttonPressed == false ) {
	    parentpos = newpos;
	    if ( !t_parentmove->isActive() )
		t_parentmove->start(50);
	}
    }
    if ( cursorw )
	cursorw->setPos(e->globalPos());
}

void Skin::moveParent()
{
    parent->move( parentpos );
}

void Skin::mouseReleaseEvent( QMouseEvent * )
{
    if ( buttonPressed )
	endPress();
    if ( joydown ) {
	joydown = false;
	if ( onjoyrelease >= 0 ) {
	    startPress(onjoyrelease);
	    endPress();
	}
    }
}


bool Skin::hasCursor() const
{
    return !skinCursorFileName.isEmpty();
}

void Skin::setupDefaultButtons()
{
    QString destDir = QString("/tmp/qtembedded-%1/").arg(view->displayId());
    QFileInfo src(prefix + "defaultbuttons.conf");
    QFileInfo dst(destDir + "defaultbuttons.conf");
    unlink(dst.absoluteFilePath().toLatin1().constData());
    if (src.exists()) {
        QFile::copy(src.absoluteFilePath(), dst.absoluteFilePath());
    }
}

// ====================================================================

bool CursorWindow::eventFilter( QObject *, QEvent *ev)
{
    handleMouseEvent(ev);
    return false;
}

bool CursorWindow::event( QEvent *ev )
{
    if (handleMouseEvent(ev))
        return true;
    return QWidget::event(ev);
}

bool CursorWindow::handleMouseEvent(QEvent *ev)
{
    bool handledEvent = false;
    static int inhere=0;
    if ( !inhere ) {
	inhere++;
	if ( view ) {
	    if ( ev->type() >= QEvent::MouseButtonPress && ev->type() <= QEvent::MouseMove ) {
		QMouseEvent *e = (QMouseEvent*)ev;
		QPoint gp = e->globalPos();
		QPoint vp = view->mapFromGlobal(gp);
		QPoint sp = skin->mapFromGlobal(gp);
		if ( e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick ) {
		    if ( view->rect().contains(vp) )
			mouseRecipient = view;
		    else if ( skin->parentWidget()->geometry().contains(gp) )
			mouseRecipient = skin;
		    else
			mouseRecipient = 0;
		}
		if ( mouseRecipient ) {
		    setPos(gp);
		    QMouseEvent me(e->type(),mouseRecipient==skin ? sp : vp,gp,e->button(),e->buttons(),e->modifiers());
		    QApplication::sendEvent(mouseRecipient, &me);
		} else if ( !skin->parentWidget()->geometry().contains(gp) ) {
		    hide();
		} else {
		    setPos(gp);
		}
		if ( e->type() == QEvent::MouseButtonRelease )
		    mouseRecipient = 0;
		handledEvent = true;
	    }
	}
	inhere--;
    }
    return handledEvent;
}

void CursorWindow::setView(QVFbView* v)
{
    if ( view ) {
	view->removeEventFilter(this);
	view->removeEventFilter(this);
    }
    view = v;
    view->installEventFilter(this);
    view->installEventFilter(this);
    mouseRecipient = 0;
}

CursorWindow::CursorWindow( const QString& fn, QPoint hot, QWidget* sk)
	:QWidget(0),
	view(0), skin(sk),
	hotspot(hot)
{
    setWindowFlags( Qt::FramelessWindowHint );
    mouseRecipient = 0;
    setMouseTracking(true);
    setCursor(Qt::BlankCursor);
    QImage img( fn );
    QPixmap p;
    p = QPixmap::fromImage( img );
    if ( !p.mask() )
	if ( img.hasAlphaChannel() ) {
	    QBitmap bm;
	    bm = QPixmap::fromImage(img.createAlphaMask());
	    p.setMask( bm );
	} else {
	    QBitmap bm;
	    bm = QPixmap::fromImage(img.createHeuristicMask());
	    p.setMask( bm );
	}
    QPalette palette;
    palette.setBrush(backgroundRole(), QBrush(p));
    setPalette(palette);
    setFixedSize( p.size() );
    if ( !p.mask().isNull() )
	setMask( p.mask() );
}

void CursorWindow::setPos(QPoint p)
{
    move(p-hotspot);
    show();
    raise();
}
