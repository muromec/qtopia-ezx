/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QWSDISPLAY_QWS_H
#define QWSDISPLAY_QWS_H

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>
#include <QtGui/qregion.h>
#include <QtGui/qimage.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qlist.h>

QT_BEGIN_HEADER

QT_MODULE(Gui)

class QWSEvent;
class QWSMouseEvent;
class QWSQCopMessageEvent;
class QVariant;
class QLock;

class QWSWindowInfo
{

public:

    int winid;
    unsigned int clientid;
    QString name;

};

#define QT_QWS_PROPERTY_CONVERTSELECTION 999
#define QT_QWS_PROPERTY_WINDOWNAME 998
#define QT_QWS_PROPERTY_MARKEDTEXT 997

class QWSDisplay;
extern QWSDisplay *qt_fbdpy;

class Q_GUI_EXPORT QWSDisplay
{
public:
    QWSDisplay();
    ~QWSDisplay();

    static QWSDisplay* instance() { return qt_fbdpy; }

    bool eventPending() const;
    QWSEvent *getEvent();
//    QWSRegionManager *regionManager() const;

    uchar* frameBuffer() const;
    int width() const;
    int height() const;
    int depth() const;
    int pixmapDepth() const;
    bool supportsDepth(int) const;

    uchar *sharedRam() const;
    int sharedRamSize() const;

#ifndef QT_NO_QWS_PROPERTIES
    void addProperty(int winId, int property);
    void setProperty(int winId, int property, int mode, const QByteArray &data);
    void setProperty(int winId, int property, int mode, const char * data);
    void removeProperty(int winId, int property);
    bool getProperty(int winId, int property, char *&data, int &len);
#endif // QT_NO_QWS_PROPERTIES

    QList<QWSWindowInfo> windowList();
    int windowAt(const QPoint &);

    void setIdentity(const QString &appName);
    void nameRegion(int winId, const QString& n, const QString &c);
    void requestRegion(int winId, const QString &surfacekey,
                       const QByteArray &surfaceData,
                       const QRegion &region);
    void repaintRegion(int winId, int windowFlags, bool opaque, QRegion);
    void moveRegion(int winId, int dx, int dy);
    void destroyRegion(int winId);
    void requestFocus(int winId, bool get);
    void setAltitude(int winId, int altitude, bool fixed = false);
    void setOpacity(int winId, int opacity);
    int takeId();
    void setSelectionOwner(int winId, const QTime &time);
    void convertSelection(int winId, int selectionProperty, const QString &mimeTypes);
    void defineCursor(int id, const QBitmap &curs, const QBitmap &mask,
                        int hotX, int hotY);
    void destroyCursor(int id);
    void selectCursor(QWidget *w, unsigned int id);
    void setCursorPosition(int x, int y);
    void grabMouse(QWidget *w, bool grab);
    void grabKeyboard(QWidget *w, bool grab);
    void playSoundFile(const QString&);
    void registerChannel(const QString &channel);
    void sendMessage(const QString &channel, const QString &msg,
                       const QByteArray &data);
    void flushCommands();
#ifndef QT_NO_QWS_INPUTMETHODS
    void sendIMUpdate(int type, int winId, int widgetid);
    void resetIM();
    void sendIMResponse(int winId, int property, const QVariant &result);
    void sendIMMouseEvent(int index, bool isPress);
#endif
    QWSQCopMessageEvent* waitForQCopResponse();
    void sendFontCommand(int type, const QByteArray &fontName);

    void setWindowCaption(QWidget *w, const QString &);

    // Lock display for access only by this process
    static bool initLock(const QString &filename, bool create = false);
    static bool grabbed();
    static void grab();
    static void grab(bool write);
    static void ungrab();

#ifdef QT_QWS_DYNAMIC_TRANSFORMATION
    static void setTransformation(int t);
#endif
    static void setRawMouseEventFilter(void (*filter)(QWSMouseEvent *));

private:
    friend int qt_fork_qapplication();
    friend void qt_app_reinit( const QString& newAppName );
    friend class QApplication;
    friend class QCopChannel;
    friend class QWSEmbedWidget;
    friend class QWSEmbedWidgetPrivate;
    class Data;
    friend class Data;
    Data *d;

    friend class QWSMemorySurface;
    friend class QWSOnScreenSurface;
    friend class QWSDirectPainterSurface;
    int getPropertyLen;
    char *getPropertyData;
    static QLock *lock;
};

QT_END_HEADER

#endif // QWSDISPLAY_QWS_H
