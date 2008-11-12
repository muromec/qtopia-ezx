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

#include <qtopialog.h>
#include <QValueSpaceObject>
#include <custom.h>
#include <qtopianamespace.h>
#include <QTranslatableSettings>

#ifndef QT_NO_SXE
#include <private/qtransportauth_qws_p.h>
#include <qsxepolicy.h>
#include <qpackageregistry.h>
#endif

#include <stdlib.h>
#include <time.h>

#include <qfile.h>
#include <qlist.h>
#include <qqueue.h>
#include <qpainter.h>
#include <qvalidator.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <QTextBrowser>
#include <qdesktopwidget.h>
#include <qtranslator.h>
#ifdef Q_WS_QWS
#include <qsoundqss_qws.h>
#endif
#include <QTextCursor>
#include <qtopiaipcenvelope.h>
#include <qtopiaservices.h>
#ifdef Q_WS_QWS
#include <qwindowsystem_qws.h>
#endif
#include <qtextstream.h>
#include <qpalette.h>
#include <qbuffer.h>
#include <qregexp.h>
#include <qdir.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qtextcodec.h>
#include <qevent.h>
#include <qmessagebox.h>
#include <qclipboard.h>
#include <qtimer.h>
#include <qpixmapcache.h>
#include <qpushbutton.h>
#include <QSettings>
#include <qspinbox.h>
#include <qcombobox.h>
#include <QMenu>
#include <QLocale>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QDialogButtonBox>
#include <QAbstractSpinBox>
#include <QLayout>
#include <QResizeEvent>
#include <QLibrary>
#include <QPluginLoader>
#include <private/qlocale_p.h>
#include <QDateTimeEdit>
#ifdef Q_WS_QWS
#include <qwsdisplay_qws.h>
#endif
#include <qtopiaapplication.h>
#include <qtopia/private/qtopiaresource_p.h>
#include <qstylefactory.h>
#include <qstorage.h>
#include <qtopia/qphonestyle.h>
#ifdef Q_WS_QWS
#include <qtopia/private/qpedecoration_p.h>
#endif
#include <qtopianamespace.h>
#include "qpluginmanager.h"

#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <math.h>

#include <qtopia/private/contextkeymanager_p.h>
#include <qsoftmenubar.h>

bool mousePreferred = false;

#ifdef QTOPIA_DBUS_IPC
#include <qtdbus/qdbusconnection.h>
#include <qtdbus/qdbuserror.h>
#include <qtdbus/qdbusmessage.h>
#include <qtdbus/qdbusconnectioninterface.h>
#include "dbusipccommon_p.h"
#include "dbusapplicationchannel_p.h"
#endif

#ifdef Q_WS_X11
#include <qcopchannel_x11.h>
#include <QX11Info>
#include <QtGui/private/qt_x11_p.h>
#endif

#include "ezxphonestyle_p.h"
#include "qthumbstyle_p.h"

#include <qtopia/private/qcontentstore_p.h>
#include "qcontent_p.h"

#ifndef QTOPIA_HOST
#include <qtopia/private/qtopiasql_p.h>
#endif

#if !defined(QTOPIA_CONTENT_INSTALLER) && !defined(Q_WS_X11)
# include <qtopia/private/testslaveinterface_p.h>
#define QTOPIA_USE_TEST_SLAVE 1
#endif

enum QPEWidgetFlagsEnum {
    MenuLikeDialog = 0x01,
} QPEWidgetFlags;

QMap<const QWidget *, int> qpeWidgetFlags;

// This temporary define is for use while integrating the new QSoftKeyHelper API
// into Qtopia, and can be removed as soon as this is done.
//#define QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING

//#define GREENPHONE_EFFECTS

#ifdef GREENPHONE_EFFECTS

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <QDirectPainter>
#include <QLinuxFbScreen>

#ifdef QT_QWS_GREENPHONE
class GreenphoneScreenPrivate;
class GreenphoneScreen : public QLinuxFbScreen
{
public:
    GreenphoneScreen(int displayId);
    ~GreenphoneScreen();

    void exposeRegion(QRegion region, int windowIndex);
    virtual void addReserved(int id, const QRect &rect);
    virtual void removeReserved(int id);

    int cookie;
private:
    GreenphoneScreenPrivate *d;
};
#endif

class SyncPainter : public QDirectPainter
{
public:
    SyncPainter(QObject *parent = 0);
    virtual ~SyncPainter();

    void syncRegion(const QRect &);

private:
    QTcpSocket *socket;
};


SyncPainter::SyncPainter(QObject *parent)
: QDirectPainter(parent), socket(0)
{
#ifdef QT_QWS_GREENPHONE
    if(!qwsServer) {
        struct ::sockaddr_un addr;
        int crv;
        int fd;
        fd = ::socket(PF_UNIX, SOCK_STREAM, 0);
        if(fd == -1) {
            qWarning() << "SyncPainter: Unable to create socket";
            goto connect_error;
        }

        ::bzero(&addr, sizeof(sockaddr_un));
        addr.sun_family = AF_UNIX;
        ::memcpy(addr.sun_path, "/tmp/region_reserve", strlen("/tmp/region_reserve") + 1);

        crv = ::connect(fd, (sockaddr *)&addr, sizeof(sockaddr_un));
        if(-1 == crv) {
            qWarning() << "SyncPainter: Unable to connect";
            perror("connect");
            goto connect_error;
        }

        socket = new QTcpSocket(this);
        socket->setSocketDescriptor(fd);
        return;

    connect_error:
        ::close(fd);
    }
#endif
}

SyncPainter::~SyncPainter()
{
#ifdef QT_QWS_GREENPHONE
    if(qwsServer) 
        static_cast<GreenphoneScreen *>(qt_screen)->removeReserved((int)this);
#endif
}

void SyncPainter::syncRegion(const QRect &r)
{
#ifdef QT_QWS_GREENPHONE
    if(qwsServer) {
        static_cast<GreenphoneScreen *>(qt_screen)->addReserved((int)this, r);
    } else {
        if(!socket) {
            qWarning() << "SyncPainter: Cannot sync region - not connected";
            return;
        }

        uint data[4];
        data[0] = r.x();
        data[1] = r.y();
        data[2] = r.width();
        data[3] = r.height();

        socket->write((char *)data, sizeof(uint) * 4);

        socket->waitForReadyRead(-1);
        char d;
        socket->read(&d, 1);
    }
#endif
}

#ifdef QT_QWS_GREENPHONE
#include <QDirectPainter>
#include <QMutex>
#include <QWaitCondition>
#include <sched.h>
#include <blend.h>
class AImage
{
public:
    AImage(const QImage &);
    ~AImage();

    int height() const { return m_height; }
    int width() const { return m_width; }
    int stepwidth() const { return m_stepwidth; }

    ushort *colorline(int ii) const { return color + ii * m_stepwidth; }
    uchar *alphaline(int ii) const { return alpha + ii * m_stepwidth; }

private:

    int m_height;
    int m_width;
    int m_stepwidth;

    ushort *color;
    uchar *alpha;
};

AImage::AImage(const QImage &_img)
{
    Q_ASSERT(img.format() == QImage::Format_ARGB32 ||
             img.format() == QImage::Format_ARGB32_Premultiplied);

    QImage img;
    if(_img.format() == QImage::Format_ARGB32)
        img = _img;
    else
        img = _img.convertToFormat(QImage::Format_ARGB32);


    m_height = img.height();
    m_width = img.width();
    if(m_width % 4)
        m_stepwidth = (m_width & 0xFFFFFFFC) + 4;
    else
        m_stepwidth = m_width;

    color = new ushort[m_stepwidth * m_height];
    alpha = new uchar[m_stepwidth * m_height];

    uint *img_bits = (uint *)img.bits();
    uint img_step = img.bytesPerLine();

    for(int yy = 0; yy < m_height; ++yy) {
        ushort *color_l = color + yy * m_stepwidth;
        uchar *alpha_l = alpha + yy * m_stepwidth;

        for(int xx = 0; xx < m_width; ++xx) {
            uint color = img_bits[xx];

            alpha_l[xx] = (color & 0xFF000000) >> 24;
            color_l[xx] = ((color & 0x00F80000) >> 8) |
                          ((color & 0x0000FC00) >> 5) |
                          ((color & 0x000000F8) >> 3);
        }

        img_bits = (uint *)((uchar *)(img_bits) + img_step);
    }
};

AImage::~AImage()
{
    delete [] color;
    delete [] alpha;
}

class FadeThread : public QThread
{
Q_OBJECT
public:
    FadeThread(QObject *parent);

    void fade(QWidget *wid, const QRect &screenRect, const QImage &begin, const QImage &end, bool leftToRight);

    bool inProgress();
    void menu(QMenu *wid);

    void addTab(QTabWidget *tab);
    void removeTab(QTabWidget *tab);

protected:
    virtual void run();
    virtual bool eventFilter(QObject *, QEvent *); 

signals:
    void done();

private slots:
    void doDone();
    void tabChanged(int newTab);

private:
    void menu(QMenu *wid, const QRect &screenRect, const QImage &menu, const QImage &back); 
    void doFade();
    void doMenu();

    SyncPainter dp;

    enum Effect { None, Reserving, Tab, Menu, Waiting };

    Effect m_effect;

    QWidget *m_wid;
    QMutex m_lock;
    QWaitCondition m_wait;
    QRect m_screenRect;
    QImage m_begin;
    QImage m_end;
    bool m_leftToRight;

    bool m_ignoreTabChanged;

    QHash<QTabWidget *, int> m_oldTab;
};

FadeThread::FadeThread(QObject *parent)
: QThread(parent) , 
  m_effect(None) , m_wid(0)
{
    QObject::connect(this, SIGNAL(done()), 
                     this, SLOT(doDone()), 
                     Qt::QueuedConnection);

    m_ignoreTabChanged = false;

    m_lock.lock();
    start();
    m_wait.wait(&m_lock);
    m_lock.unlock();
}

void FadeThread::addTab(QTabWidget *tab)
{
    Q_ASSERT(tab);
    Q_ASSERT(!m_oldTab.contains(tab));
    m_oldTab[tab] = tab->currentIndex();
    QObject::connect(tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

void FadeThread::removeTab(QTabWidget *tab)
{
    Q_ASSERT(tab);
    Q_ASSERT(m_oldTab.contains(tab));
    m_oldTab.remove(tab);
    QObject::disconnect(tab, SIGNAL(currentChanged(int)), 
                        this, SLOT(tabChanged(int)));
}

void FadeThread::fade(QWidget *wid, 
                      const QRect &screenRect, 
                      const QImage &begin, 
                      const QImage &end, 
                      bool leftToRight)
{
    m_lock.lock();
    if(None == m_effect) {
        m_screenRect = screenRect;
        m_begin = begin;
        m_end = end;
        m_leftToRight = leftToRight;
        m_effect = Reserving;
        m_wid = wid;
        m_wid->installEventFilter(this);
        dp.syncRegion(m_screenRect);
        m_effect = Tab;
        m_wait.wakeAll();
    } else {
        qWarning() << "Effect conflict detected";
    }
    m_lock.unlock();
}

void FadeThread::tabChanged(int newTab)
{
    if(m_ignoreTabChanged)
        return;

    QTabWidget *tb = qobject_cast<QTabWidget *>(sender());
    Q_ASSERT(tab);
    Q_ASSERT(m_oldTab.contains(tab));

    if(!tb->isActiveWindow())
        return;

    int currentTab = m_oldTab[tb];
    if(currentTab == newTab)
        return;

    if(currentTab >= tb->count())
        return;

    m_ignoreTabChanged = true;

    bool leftToRight = (newTab > currentTab);

    QWidget *widget = tb->widget(currentTab);
    QRect globalRect(tb->mapToGlobal(QPoint(0,0)), tb->size());

    QBrush brush = 
        widget->window()->palette().brush(widget->window()->backgroundRole());

    // Take a snapshot
    QImage img_orig(globalRect.width(), 
                    globalRect.height(), 
                    QImage::Format_RGB16);
    {
        QPainter p(&img_orig);
        p.setBrushOrigin(-1 * globalRect.x(), -1 * globalRect.y());
        p.fillRect(0, 0, globalRect.width(), globalRect.height(), brush);
        p.end();
    }
    tb->setCurrentIndex(currentTab);
    tb->render(&img_orig);

    tb->setCurrentIndex(newTab);
    QImage img_new(globalRect.width(), 
                   globalRect.height(), 
                   QImage::Format_RGB16);
    {
        QPainter p(&img_new);
        p.setBrushOrigin(-1 * globalRect.x(), -1 * globalRect.y());
        p.fillRect(0, 0, globalRect.width(), globalRect.height(), brush);
        p.end();
    }
    tb->render(&img_new);
    m_oldTab[tb] = newTab;

    fade(tb, globalRect, img_orig, img_new, leftToRight);

    m_ignoreTabChanged = false;
}

bool FadeThread::inProgress() 
{
    bool rv;
    m_lock.lock();
    rv = (None != m_effect);
    m_lock.unlock();
    return rv;
}

void FadeThread::menu(QMenu *wid)
{
    QRect rect(wid->mapToGlobal(QPoint(0,0)), wid->rect().size());

    QPalette pal = wid->palette();
    QPalette pal_copy = pal;
    pal.setBrush(QPalette::Window, QColor(0,0,0,0));
    wid->setPalette(pal);

    QImage back(rect.size(), QImage::Format_RGB16);
    QPainter p(&back);
    p.setBrush(pal_copy.brush(QPalette::Window));
    p.setBrushOrigin(-rect.x(), -rect.y());
    p.setPen(Qt::NoPen);
    p.drawRect(0, 0, rect.width(), rect.height());
    p.end();

    QImage img(rect.size(), QImage::Format_ARGB32);
    img.fill(0x00FFFFFF);
    qWarning() << "XXXXXXXXXXX Render start";
    wid->render(&img);
    qWarning() << "XXXXXXXXXXX Render end";
    wid->setPalette(pal_copy);

    menu(wid, rect, img, back);
}

void FadeThread::menu(QMenu *wid,
                      const QRect &screenRect, 
                      const QImage &menu, 
                      const QImage &back)
{
    m_lock.lock();
    if(None == m_effect) {
        m_screenRect = screenRect;
        m_begin = menu;
        m_end = back;
        m_effect = Reserving;
        m_wid = wid;
        m_wid->installEventFilter(this);
        dp.syncRegion(m_screenRect);
        m_effect = Menu;
        m_wait.wakeAll();
    } else {
        qWarning() << "Effect conflict detected";
    }
    m_lock.unlock();
}

void FadeThread::run()
{
    sched_param p;
    ::bzero(&p, sizeof(sched_param));
    p.sched_priority = 1;

    m_lock.lock();
    m_wait.wakeAll();

    while(true) {

        
        if(m_effect != None) {

            m_lock.unlock();

            qWarning() << "Start effect";
            if(m_effect == Tab || m_effect == Menu) {

                sched_param p;
                ::bzero(&p, sizeof(sched_param));
                p.sched_priority = 1;
                sched_setscheduler(0, SCHED_RR, &p);

                if(m_effect == Tab)
                    doFade();
                else if(m_effect == Menu)
                    doMenu();

                p.sched_priority = 0;
                sched_setscheduler(0, SCHED_OTHER, &p);
            }
            qWarning() << "End effect";

            m_lock.lock();

            qWarning() << "Emit done";
            if(m_wid)
                m_effect = Waiting;
            else
                emit done();
        }

        qWarning() << "Wait start";
        m_wait.wait(&m_lock);
        qWarning() << "Wait end";
    }
}

bool FadeThread::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::Paint ) {
        m_lock.lock();
        m_wid->removeEventFilter(this);
        m_wid = 0;
        // This must be done on a timer so the paint event itself has time to
        // complete before the region is removed
        if(m_effect == Waiting) 
            QTimer::singleShot(0, this, SLOT(doDone()));

        m_lock.unlock();
    }

    return false;
}

void FadeThread::doDone()
{
    qWarning() << "DoDone" << QThread::currentThread();
    m_lock.lock();
    if(m_wid || None == m_effect || Reserving == m_effect) {
        m_lock.unlock();
        return;
    }
    m_effect = Reserving;
    m_lock.unlock();
    qWarning() << "clear start";
    dp.syncRegion(QRect());
//    dp.setRegion(QRegion());
    qWarning() << "clear end";
    m_lock.lock();
    m_effect = None;
    m_lock.unlock();
}

void FadeThread::doMenu()
{
    uchar *back_bits = m_end.bits();
    int back_step = m_end.bytesPerLine();

    AImage aimg(m_begin);

    ushort *framebuffer = 
        (ushort *)(dp.frameBuffer() + m_screenRect.y() * dp.linestep() + m_screenRect.x() * 2);

    int linestep = dp.linestep() / 2;

    int width = aimg.width();

    int height = m_screenRect.height();
    for(int ii = 1; ii <= height; ++ii) {
        for(int jj = 0; jj < ii; ++jj) {

            ushort *dest = (ushort *)(back_bits + height * back_step - ii * back_step + jj * back_step);
            ushort *src = aimg.colorline(jj);
            uchar *alpha = aimg.alphaline(jj);
            int bwidth = width & 0xFFFFFFFC;

            ushort *output = (ushort *)(framebuffer + height * linestep - ii * linestep + jj * linestep);

            blend_rgba16_rgb16(dest, src, alpha, bwidth, output);

        } 
    }
}

void FadeThread::doFade()
{
    ushort *img_orig_data = (ushort *)m_begin.bits();
    int imagestep = m_begin.width();

    ushort *img_new_data = (ushort *)m_end.bits();

    uint linestep = dp.linestep() / 2;
    ushort *frameBuffer = 
        (ushort *)(dp.frameBuffer() + m_screenRect.y() * linestep * 2 + 
                   m_screenRect.x() * 2);

    int width = m_screenRect.width();
    int height = m_screenRect.height();

    uchar *alpha = new uchar[width];
    ::memset(alpha, 0xFF, width);

    for(int ii = 0; ii < 495; ii += 10) {

        for(int aa = 0; aa < width; ++aa) {
            if((m_leftToRight && aa < ii && alpha[aa]) ||
               (!m_leftToRight && aa > (width - ii) && alpha[aa])) {
                if(alpha[aa] > 10)
                    alpha[aa]-=10;
                else 
                    alpha[aa]=0;
            }
        }

        ushort *image_new_data_l = img_new_data;
        ushort *image_orig_data_l = img_orig_data;
        ushort *dest_l = frameBuffer;

        for(int jj = 0; jj < height; ++jj) {


            blend_rgba16_rgb16(image_new_data_l, image_orig_data_l, 
                               alpha, width, dest_l); 

            image_new_data_l+=imagestep;
            image_orig_data_l+=imagestep;
            dest_l+=linestep;
        }
    }

    delete [] alpha;
}

#endif // QT_QWS_GREENPHONE

#endif













// declare QtopiaApplicationLifecycle
/*
  \internal
  \class QtopiaApplicationLifeCycle
  \mainclass
  \brief The QtopiaApplicationLifeCycle class controls the lifecycle of a QtopiaApplication based application.

  Applications in Qtopia are launched either directly through executable
  invocation, or indirectly through reception of a message on their QCop
  application channel.

  Application termination is a two stage process.  When an application becomes
  idle and could otherwise terminate, it enters a "lazy shutdown" state.

 */
class QtopiaApplicationLifeCycle : public QObject
{
Q_OBJECT
public:
    QtopiaApplicationLifeCycle(QtopiaApplication * app);

    void registerRunningTask(const QString &, QObject *);
    void unregisterRunningTask(const QString &);

    void reinit();
    bool willKeepRunning() const;

public slots:
    void unregisterRunningTask(QObject *);

signals:
    void quit();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *e);

private slots:
    void doQuit();

private:
    QValueSpaceObject *vso();
    void updateLazyShutdown();
    void recalculateQuit();
    void recalculateInfo();

    bool m_queuedQuit;
    bool m_lazyShutdown;
    QMap<QString, QObject *> m_runningTasks;
    bool m_canQuit;
    bool m_uiActive;
    QtopiaApplication *m_app;
    QValueSpaceObject *m_vso;
    QString m_name;
};

// define QtopiaApplicationLifeCycle
QtopiaApplicationLifeCycle::QtopiaApplicationLifeCycle(QtopiaApplication *app)
: QObject(app), m_queuedQuit(false), m_lazyShutdown(false), m_canQuit(true),
  m_uiActive(false), m_app(app), m_vso(0)
{
    Q_ASSERT(m_app);
    m_app->installEventFilter(this);
    m_name = QCoreApplication::applicationName();

    updateLazyShutdown();
    recalculateInfo();
    recalculateQuit();
}

bool QtopiaApplicationLifeCycle::willKeepRunning() const
{
    return !m_canQuit;
}

void QtopiaApplicationLifeCycle::reinit()
{
    QString newName = QCoreApplication::applicationName();
    if(newName != m_name) {
        m_name = newName;

        // Ahhh.  Name change
        delete m_vso;
        m_vso = 0;

        // Reinitialise the ValueSpace layers
        // QValueSpace::reinitValuespace();

        QValueSpaceObject *obj = vso();
        for(QMap<QString, QObject *>::ConstIterator iter = m_runningTasks.begin();
                iter != m_runningTasks.end();
                ++iter)
            obj->setAttribute("Tasks/" + iter.key(), true);

        recalculateQuit();
        recalculateInfo();
    }
}

void QtopiaApplicationLifeCycle::doQuit()
{
    if(!m_canQuit) {
        m_queuedQuit = false;
        return;
    }
    recalculateQuit();
    m_queuedQuit = false;

    if(!m_canQuit)
        return;

    if(!m_lazyShutdown)
        emit quit();
}

bool QtopiaApplicationLifeCycle::eventFilter(QObject *obj, QEvent *e)
{
    if(e->type() == QEvent::Show || e->type() == QEvent::Hide) {
        if(obj->isWidgetType() && static_cast<QWidget *>(obj)->isTopLevel()) {
            recalculateQuit();
        }
    }

    return QObject::eventFilter(obj, e);
}

void QtopiaApplicationLifeCycle::updateLazyShutdown()
{
    QSettings cfg(QLatin1String("Trolltech"), QLatin1String("Launcher"));
    cfg.beginGroup(QLatin1String("AppLoading"));
    m_lazyShutdown = cfg.value(QLatin1String("LazyShutdown"), false).toBool();
}

void QtopiaApplicationLifeCycle::recalculateInfo()
{
    vso()->setAttribute("Info/Pid", ::getpid());
    vso()->setAttribute("Info/Name", QtopiaApplication::applicationName());
}

void QtopiaApplicationLifeCycle::recalculateQuit()
{
    bool runningTasks = !m_runningTasks.isEmpty();
    bool uiActive = false;

    QWidgetList widgets = m_app->topLevelWidgets();
    for(int ii = 0; ii < widgets.count() && !uiActive; ++ii) {
        if(!widgets.at(ii)->isHidden() || widgets.at(ii)->isMinimized())
            uiActive = true;
    }

    if(uiActive != m_uiActive) {
        vso()->setAttribute("Tasks/UI", uiActive);
        m_uiActive = uiActive;
    }
    m_canQuit = !runningTasks && !uiActive;

    if(m_canQuit && !m_queuedQuit) {
        m_queuedQuit = true;
        QTimer::singleShot(0, this, SLOT(doQuit()));
    }
}

void QtopiaApplicationLifeCycle::registerRunningTask(const QString &name,
                                                     QObject *obj)
{
    Q_ASSERT(name != "UI");

    if (m_runningTasks.contains(name)) {
        QObject *oldObj = m_runningTasks.value(name);
        if (oldObj)
            QObject::disconnect(oldObj, SIGNAL(destroyed(QObject*)),
                    this, SLOT(unregisterRunningTask(QObject*)));
        m_runningTasks[name] = obj;
    } else {
        m_runningTasks.insert(name, obj);
        vso()->setAttribute("Tasks/" + name, true);
    }

    if(obj)
        QObject::connect(obj, SIGNAL(destroyed(QObject*)),
                         this, SLOT(unregisterRunningTask(QObject*)));

    recalculateQuit();
}

void QtopiaApplicationLifeCycle::unregisterRunningTask(const QString &name)
{
    QMap<QString, QObject *>::Iterator iter = m_runningTasks.find(name);
    if(iter != m_runningTasks.end()) {
        vso()->removeAttribute("Tasks/" + name);
        m_runningTasks.erase(iter);
    }

    recalculateQuit();
}

void QtopiaApplicationLifeCycle::unregisterRunningTask(QObject *obj)
{
    Q_ASSERT(obj);

    for(QMap<QString, QObject *>::ConstIterator iter = m_runningTasks.begin();
        iter != m_runningTasks.end();
        ++iter) {
        if(obj == (*iter)) {
            unregisterRunningTask(iter.key());
            return;
        }
    }
}

QValueSpaceObject *QtopiaApplicationLifeCycle::vso()
{
    if(!m_vso) {
        m_vso = new QValueSpaceObject("/System/Applications/" + m_name.toLatin1());
        m_vso->setAttribute("Tasks/UI", m_uiActive);
    }

    return m_vso;
}

/*
   Currently only modifies the short date format string and time format string
*/
class QtopiaSystemLocale : public QSystemLocale
{
public:
    QtopiaSystemLocale()
        : QSystemLocale()
    {
        readSettings();
    }
    virtual ~QtopiaSystemLocale() {}

    virtual QVariant query( QueryType type, QVariant in ) const {
        switch ( type ) {
        case DateFormatShort:
            if ( !mDateFormat.isEmpty() )
                return QVariant( mDateFormat );
            break;
        case TimeFormatShort:
            if ( !mTimeFormat.isEmpty() )
                return QVariant( mTimeFormat );
            break;
        default: break;
        }
        return QSystemLocale::query( type, in );
    }

    void readSettings() {
        QSettings config("Trolltech", "qpe");
        config.beginGroup( "Date" );
        QVariant v = config.value("DateFormat");
        if (v.isValid()) {
            mDateFormat = v.toString();
            // need to turn it into the QT format.
            mDateFormat.replace("%Y", "yyyy");
            mDateFormat.replace("%M", "MM");
            mDateFormat.replace("%D", "dd");
        } else {
            mDateFormat.clear();
        }
        config.endGroup();
        config.beginGroup( "Time" );
        v = config.value("AMPM");
        // time format is either 12 or 24 hour time.  We don't show seconds (ss) in Qtopia
        if (v.toBool()) {
            mTimeFormat = "h:mm AP";
        } else {
            mTimeFormat = "H:mm";
        }
    }

private:
    QString mDateFormat;
    QString mTimeFormat;
};

class QSpinBoxLineEditAccessor : public QSpinBox
{
public:
    QLineEdit *getLineEdit() { return lineEdit(); }
};

class CalendarMenu : public QMenu
{
    Q_OBJECT
public:
    CalendarMenu(QWidget *parent=0)
        : QMenu(parent), w(0)
    {
        addAction(QIcon(":icon/month"), tr("Calendar"),
                this, SLOT(showCalendar()));
    }

    void setTargetWidget(QDateEdit *widget) {
        if ( targetWidget )
            targetWidget->removeEventFilter(this);
        targetWidget = widget;
        //if ( targetWidget )
        //    targetWidget->installEventFilter(this);
    }

private slots:
    void showCalendar()
    {
        /* will need a cancel as well */
        if (!w) {
            w = new QCalendarWidget();
            w->setWindowFlags(Qt::Popup);
            w->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
            w->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
            connect(w, SIGNAL(activated(QDate)),
                    this, SLOT(selectDate(QDate)));
            QWidget *table = w->findChild<QWidget*>("qt_calendar_calendarview");
            table->installEventFilter(this);
            QSoftMenuBar::setLabel(w, Qt::Key_Context1, QSoftMenuBar::NoLabel);
        }
        if (targetWidget) {
            w->blockSignals(true);
            w->setSelectedDate(targetWidget->date());
            w->blockSignals(false);
        }
        w->setEditFocus(true);
        w->showMaximized();
    }

    void hideCalendar()
    {
        w->hide();
    }


    void selectDate(const QDate &date)
    {
        if (targetWidget)
            targetWidget->setDate(date);
        hideCalendar();
    }

    bool eventFilter(QObject *o, QEvent *e)
    {
        if ( e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease ) {
            QKeyEvent* ke = static_cast<QKeyEvent*>(e);
            if (o == targetWidget) {
                if (ke->key() == Qt::Key_Select) {
                    if ( e->type() == QEvent::KeyPress )
                        showCalendar();
                    return true;
                }
            } else {
                if (e->type() == QEvent::KeyRelease && ke->key() == Qt::Key_Back) {
                    hideCalendar();
                    ke->accept();
                    return true;
                }
            }
        }
        return QMenu::eventFilter(o,e);
    }

private:
    QCalendarWidget *w;
    QPointer<QDateEdit> targetWidget;
};

#define POPUP_SHADOWS   //for now, allows define, and use style hint to control
#ifdef POPUP_SHADOWS

class ShadowWidget : public QWidget
{
public:
    ShadowWidget(int size);

    int shadowSize() const { return shSize; }
    void setTarget(QWidget *w);

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *r);
    bool eventFilter(QObject *o, QEvent *e);

private:
    inline int alphaAt(int dist, int maxDist) const;
    void generateShadow();

private:
    static QMap<int,QBrush*> brushes;
    int shSize;
    QPointer<QWidget> widget;
};

QMap<int,QBrush*> ShadowWidget::brushes;

ShadowWidget::ShadowWidget(int size)
    : QWidget(0, Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint),
        shSize(size)
{
    generateShadow();
    QPalette pal;
    pal.setBrush(QPalette::Background, QColor(0,0,0,0));
    setPalette(pal);
}

void ShadowWidget::setTarget(QWidget *w)
{
    if (widget != w) {
        if (w) {
            setGeometry(w->x()+shadowSize(), w->y()+shadowSize(),
                    w->width(), w->height());
            w->installEventFilter(this);
        } else if (widget) {
            widget->removeEventFilter(this);
        }
        widget = w;
    }
}

bool ShadowWidget::eventFilter(QObject *o, QEvent *e)
{
    if ((e->type() == QEvent::Resize || e->type() == QEvent::Move) && widget && (QWidget*)o == widget) {
        setGeometry(widget->x()+shadowSize(),
                widget->y()+shadowSize(),
                widget->width(), widget->height());
    }

    return false;
}

int ShadowWidget::alphaAt(int dist, int maxDist) const
{
    return qMax(0,95-dist*dist*95/(maxDist*maxDist));
}

void ShadowWidget::generateShadow()
{
    if (brushes.contains(shSize))
        return;

    QBrush *br = new QBrush [5];
    // Right
    {
        QImage img(shSize, 8, QImage::Format_ARGB32_Premultiplied);
        for (int x = 0; x < shSize; x++) {
            int alpha = alphaAt(x, shSize);
            int g = 63*alpha/255;
            img.setPixel(x, 0, qRgba(g,g,g,alpha));
        }
        for (int y = 1; y < img.height(); y++)
            memcpy(img.scanLine(y), img.scanLine(y-1), img.bytesPerLine());
        QPixmap pm = QPixmap::fromImage(img);
        br[1] = QBrush(Qt::black, pm);
    }

    // Bottom
    {
        QImage img(8, shSize, QImage::Format_ARGB32_Premultiplied);
        for (int y = 0; y < shSize; y++) {
            int alpha = alphaAt(y, shSize);
            int g = 63*alpha/255;
            QRgb pix = qRgba(g,g,g,alpha);
            for (int x = 0; x < img.width(); x++) {
                img.setPixel(x, y, pix);
            }
        }
        QPixmap pm = QPixmap::fromImage(img);
        br[3] = QBrush(Qt::black, pm);
    }

    // Radial pixmap for corners
    QImage radImg(shSize*2+1, shSize*2+1, QImage::Format_ARGB32_Premultiplied);
    QPoint c(shSize, shSize);
    for (int y = 0; y < radImg.height(); y++) {
        for (int x = 0; x < radImg.width(); x++) {
            QPoint dp = c - QPoint(x, y);
            int dist = (int)(::sqrt(dp.x()*dp.x() + dp.y()*dp.y()) + 0.5);
//            int dist = dp.manhattanLength();
            int alpha = alphaAt(dist, shSize);
            int g = 63*alpha/255;
            radImg.setPixel(x, y, qRgba(g,g,g,alpha));
        }
    }
    QPixmap radPm = QPixmap::fromImage(radImg);

    // Top-Right
    br[0] = QBrush(Qt::black, radPm.copy(shSize, 1, shSize, shSize));
    // Bottom-Right
    br[2] = QBrush(Qt::black, radPm.copy(shSize, shSize, shSize, shSize));
    // Bottom-Left
    br[4] = QBrush(Qt::black, radPm.copy(1, shSize, shSize, shSize));

    brushes[shSize] = br;
}

void ShadowWidget::paintEvent(QPaintEvent *)
{
    QBrush *br = brushes.value(shSize);

    QPainter p(this);
    // Top-right
    p.setBrushOrigin(width()-shSize, 0);
    p.fillRect(width()-shSize, 0, shSize, shSize, br[0]);
    // Right
    p.setBrushOrigin(width()-shSize, 0);
    p.fillRect(width()-shSize, shSize, shSize, height()-shSize*2, br[1]);
    // Bottom-Right
    p.setBrushOrigin(width()-shSize, height()-shSize);
    p.fillRect(width()-shSize, height()-shSize, shSize, shSize, br[2]);
    // Bottom
    p.setBrushOrigin(0, height()-shSize);
    p.fillRect(shSize, height()-shSize, width()-shSize*2, shSize, br[3]);
    // Bottom-Left
    p.setBrushOrigin(0, height()-shSize);
    p.fillRect(0, height()-shSize, shSize, shSize, br[4]);
}

void ShadowWidget::resizeEvent(QResizeEvent *r)
{
    QRegion rgn(r->size().width()-shSize, 0, shSize, r->size().height());
    rgn |= QRect(0, r->size().height()-shSize, r->size().width(), shSize);
    setMask(rgn);
}

#endif // POPUP_SHADOWS


// QETWidget is a friend of QApplication, which will allow us to call
// QApplication::sendSpontaneousEvent() private method.
// QETWidget is a class internal to one compilation unit in Qt,
// so it is safe to reuse it.
class QETWidget : public QWidget
{
public:
    static void sendSpontaneousEvent(QObject *receiver, QEvent *event)
    {
        QApplication::sendSpontaneousEvent(receiver, event);
    }
};

static const int npressticks=10;

class PressTickWidget : public QWidget
{
public:
    PressTickWidget()
        : QWidget(0, Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint),
        pressWidget(0), pressTick(0), rightPressed(false)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
        QSettings cfg(QLatin1String("Trolltech"),QLatin1String("presstick"));
        cfg.beginGroup(QLatin1String("PressTick"));
        pm = QPixmap(QLatin1String(":image/")+cfg.value(QLatin1String("Image")).toString());
        cfg.endGroup();
        offsets.resize(npressticks);
        tickCount = 0;
        for (int i = 0; i < npressticks; i++) {
            cfg.beginGroup(QLatin1String("Tick")+QString::number(i));
            if (cfg.contains("Dx") || cfg.contains("Dy")) {
                int dx = cfg.value(QLatin1String("Dx")).toInt();
                int dy = cfg.value(QLatin1String("Dy")).toInt();
                offsets[i] = QPoint(dx, dy);
                bound |= QRect(offsets[i], pm.size());
                tickCount++;
            }
            cfg.endGroup();
        }
    }

    void startPress(QWidget *w, const QPoint &pos)
    {
        pressTick = npressticks;
        rightPressed = false;
        pressWidget = w;
        pressPos = pos;
        pressTimer.start(500/pressTick, this); // #### pref.
        QPoint gpos = pressWidget->mapToGlobal(pos);
        setGeometry(gpos.x()+bound.x(), gpos.y()+bound.y(), bound.width(), bound.height());
    }

    void cancelPress()
    {
        pressTimer.stop();
        pressWidget = 0;
        hide();
    }

    bool endPress(QWidget *widget, const QPoint &pos)
    {
        pressTimer.stop();
        hide();
        if (rightPressed && pressWidget ) {
            QWidget *receiver = pressWidget;
            rightPressed = false;
            pressWidget = 0;
            // Right released
            QETWidget::sendSpontaneousEvent(widget,
                new QMouseEvent(QEvent::MouseButtonRelease, pos,
                        widget->mapToGlobal(pos),
                        Qt::RightButton, Qt::LeftButton, 0));
            // Left released, off-widget
            QETWidget::sendSpontaneousEvent(receiver,
                new QMouseEvent(QEvent::MouseMove, QPoint(-1,-1),
                        Qt::LeftButton, Qt::LeftButton, 0 ) );
            QETWidget::sendSpontaneousEvent(receiver,
                new QMouseEvent(QEvent::MouseButtonRelease, QPoint(-1,-1),
                        Qt::LeftButton, 0, 0 ) );
            return true; // don't send the real Left release
        }
        pressWidget = 0;

        return false;
    }

    bool active() const { return pressWidget; }
    const QPoint &pos() const { return pressPos; }
    QWidget *widget() const { return pressWidget; }

protected:
    void paintEvent(QPaintEvent *e)
    {
        Q_UNUSED(e);

        if (pressTick < tickCount) {
            int dx = offsets[pressTick].x();
            int dy = offsets[pressTick].y();
            QPainter p(this);
            if (pressTick == tickCount-1) {
                p.setCompositionMode(QPainter::CompositionMode_Clear);
                p.eraseRect(rect());
                p.setCompositionMode(QPainter::CompositionMode_Source);
            }
            p.drawPixmap(bound.width()/2+dx, bound.height()/2+dy,pm);
        }
    }

    void timerEvent(QTimerEvent *e)
    {
        if (e->timerId() == pressTimer.timerId() && pressWidget) {
            if (pressTick) {
                pressTick--;
                if (pressTick < tickCount) {
                    show();
                    update();
                }
            } else {
                // Right pressed
                pressTimer.stop();
                hide();
                QETWidget::sendSpontaneousEvent(pressWidget,
                        new QMouseEvent(QEvent::MouseButtonPress, pressPos,
                            pressWidget->mapToGlobal(pressPos),
                            Qt::RightButton, Qt::LeftButton|Qt::RightButton, 0 ) );
                rightPressed = true;
            }
        }
    }

private:
    QWidget *pressWidget;
    QPoint pressPos;
    QVector<QPoint> offsets;
    QRect bound;
    QPixmap pm;
    int tickCount;
    int pressTick;
    bool rightPressed;
    QBasicTimer pressTimer;
};

class QContentChangedChannel : public QtopiaIpcAdaptor
{
    Q_OBJECT
public:
    QContentChangedChannel(QObject *parent = NULL);
public slots:
    void contentChanged(QContentIdList idList, QContent::ChangeType ct);
signals:
    void contentChangedSignal(QContentIdList idList, QContent::ChangeType ct);
};

class QtopiaApplicationData {
public:
    QtopiaApplicationData() : pressHandler(0),
        notbusysent(false),
        preloaded(false), nomaximize(false), noshow(false),
        qpe_main_widget(0), skiptimechanged(0), qpe_system_locale(0),
        lifeCycle(0)
#ifndef QT_NO_SXE
        , sxeAuthorizerRole( QtopiaApplication::SxeAuthorizerClientOnly )
#endif
#ifdef QTOPIA_DBUS_IPC
        , m_appChannel(0)
#else
        , qcopQok(false)
#endif
        , editMenu(0)
        , calendarMenu(0)
        , sysChannel(0), pidChannel(0)
        , contentChangedChannel(0)
        , remapKeys(false)
#if defined(GREENPHONE_EFFECTS) && defined(QT_QWS_GREENPHONE)
        , m_greenphoneEffects(0)
#endif
    {
#if !defined(QT_NO_SXE) && QT_VERSION < 0x040400
        authorizerInitialized.init( 0 );
#endif
        loadKeyRemapTable();
    }

    PressTickWidget *pressHandler;
    bool notbusysent;
    QString appName;
    QString appFullPath;
    struct QCopRec {
        QCopRec(const QString &ch, const QString &msg,
                               const QByteArray &d) :
            channel(ch), message(msg), data(d)
            {
                channel.detach();
                message.detach();
                data.detach();
            }

        QString channel;
        QString message;
        QByteArray data;
    };

    bool preloaded;
    bool nomaximize;
    bool noshow;

    QPointer<QWidget> qpe_main_widget;
    QPointer<QWidget> lastraised;
    int skiptimechanged;
    QtopiaSystemLocale *qpe_system_locale;
    QtopiaApplicationLifeCycle *lifeCycle;

#ifndef QT_NO_SXE
#if QT_VERSION < 0x040400
    QBasicAtomic authorizerInitialized;
#else
    QAtomicInt authorizerInitialized;
#endif
    QtopiaApplication::SxeAuthorizerRole sxeAuthorizerRole;
#endif

#ifdef QTOPIA_DBUS_IPC
    DBUSQtopiaApplicationChannel *m_appChannel;
#else
    bool qcopQok;
    QQueue<QCopRec*> qcopq;

    void enqueueQCop(const QString &ch, const QString &msg,
                               const QByteArray &data)
    {
        qcopq.enqueue(new QCopRec(ch,msg,data));
    }

    void sendQCopQ()
    {
        if ( qcopQok ) {
            QCopRec* r;
            while( !qcopq.isEmpty() ) {
                r = qcopq.dequeue();
                // remove from queue before sending...
                // event loop can come around again before getting
                // back from the send.
                QtopiaApplication *app = qobject_cast<QtopiaApplication *>(qApp);
                if ( app )
                    app->pidMessage( r->message, r->data );
                delete r;
            }
        }
    }
#endif

    static void qpe_show_dialog( QDialog* d, bool nomax )
    {
        if( QString(d->objectName()) == "__nomove" ) // hack, don't do anything if the dialog
                                                        // specifically requests it through this mechanism
            return;

        QSize sh = d->sizeHint();

        QDesktopWidget *desktop = QApplication::desktop();
        int screen = desktop->screenNumber(d);
        if (d->parentWidget())
            screen = desktop->screenNumber(d->parentWidget());
        QRect screenRect(desktop->screenGeometry(screen));
        QRect desktopRect(desktop->availableGeometry(screen));
        QRect fg = d->frameGeometry();
        QRect cg = d->geometry();
        int frameWidth = fg.width() - cg.width();
        int maxY = desktopRect.height() - (fg.height() - cg.height());
        int h = -1;
        if (d->layout() && d->layout()->hasHeightForWidth())
            h = d->heightForWidth(desktopRect.width()-frameWidth);
        if (h == -1)
            h = sh.height();

        if ( d->isMaximized() || h >= maxY || ((!nomax) && (h > desktopRect.height()/2)) || (d->windowFlags()&Qt::WindowStaysOnTopHint)) {
            if (desktop->screenNumber(d) != screen)
                d->setGeometry(desktopRect); // appear on the correct screen
            d->setMaximumSize(screenRect.width(), screenRect.height());
            d->showMaximized();
        } else {
            fg = d->frameGeometry();
            cg = d->geometry();
            int lb = cg.left()-fg.left()+desktopRect.left();
            int bb = fg.bottom() - cg.bottom();
            d->setFixedSize(desktopRect.width() - frameWidth, h);
            QRect geom(lb, desktopRect.bottom() - h - bb + 1,
                       desktopRect.width() - frameWidth, h);
            d->setGeometry(geom);
            d->show();
        }
//        d->raise();
//        d->activateWindow();
    }

    static bool read_widget_rect(const QString &app, bool &maximized, QPoint &p, QSize &s)
    {
        maximized = true;

        // 350 is the trigger in qwsdefaultdecoration for providing a resize button
        QDesktopWidget *desktop = QApplication::desktop();
        if (desktop->screenGeometry(desktop->primaryScreen()).width() <= 350)
            return false;

        QSettings cfg(QLatin1String("Trolltech"),QLatin1String("qpe"));
        cfg.beginGroup(QLatin1String("ApplicationPositions"));
        QString str = cfg.value( app, QString() ).toString();
        QStringList l = str.split(QLatin1String(","));

        if ( l.count() == 5) {
            p.setX( l[0].toInt() );
            p.setY( l[1].toInt() );

            s.setWidth( l[2].toInt() );
            s.setHeight( l[3].toInt() );

            maximized = l[4].toInt();

            return true;
        }

        return false;
    }

    static bool validate_widget_size(const QWidget *w, QPoint &p, QSize &s)
    {
        QDesktopWidget *desktop = QApplication::desktop();
        QRect desktopRect(desktop->availableGeometry(desktop->primaryScreen()));
        int maxX = desktopRect.width();
        int maxY = desktopRect.height();
        int wWidth = s.width() + ( w->frameGeometry().width() - w->geometry().width() );
        int wHeight = s.height() + ( w->frameGeometry().height() - w->geometry().height() );

        // total window size is not allowed to be larger than desktop window size
        if ( ( wWidth >= maxX ) && ( wHeight >= maxY ) )
            return false;

        if ( wWidth > maxX ) {
            s.setWidth( maxX - (w->frameGeometry().width() - w->geometry().width() ) );
            wWidth = maxX;
        }

        if ( wHeight > maxY ) {
            s.setHeight( maxY - (w->frameGeometry().height() - w->geometry().height() ) );
            wHeight = maxY;
        }

        // any smaller than this and the maximize/close/help buttons will be overlapping
        if ( wWidth < 80 || wHeight < 60 )
            return false;

        if ( p.x() < 0 )
            p.setX(0);
        if ( p.y() < 0 )
            p.setY(0);

        if ( p.x() + wWidth > maxX )
            p.setX( maxX - wWidth );
        if ( p.y() + wHeight > maxY )
            p.setY( maxY - wHeight );

        return true;
    }

    static void store_widget_rect(QWidget *w, QString &app)
    {
        // 350 is the trigger in qwsdefaultdecoration for providing a resize button
        QDesktopWidget *desktop = QApplication::desktop();
        if (desktop->screenGeometry(desktop->primaryScreen()).width() <= 350 )
            return;

        // we use these to map the offset of geometry and pos.  ( we can only use normalGeometry to
        // get the non-maximized version, so we have to do it the hard way )
        int offsetX = w->x() - w->geometry().left();
        int offsetY = w->y() - w->geometry().top();

        QRect r;
        if ( w->isMaximized() )
            r = w->normalGeometry();
        else
            r = w->geometry();

        // Stores the window placement as pos(), size()  (due to the offset mapping)
        QSettings cfg(QLatin1String("Trolltech"),QLatin1String("qpe"));
        cfg.beginGroup(QLatin1String("ApplicationPositions"));
        QString s;
        s.sprintf("%d,%d,%d,%d,%d", r.left() + offsetX, r.top() + offsetY, r.width(), r.height(), w->isMaximized() );
        cfg.setValue( app, s );
    }

    static bool setWidgetCaptionFromAppName( QWidget *mw, const QString &appName )
    {
        QContent c(appName,false);
        if ( !c.isNull() ) {
            mw->setWindowIcon( c.icon() );
            mw->setWindowTitle( c.name() );
            return true;
        }
        return false;
    }

    void show(QWidget* mw, bool nomax)
    {
        if (mw->windowTitle().isEmpty())
            setWidgetCaptionFromAppName( mw, appName );
        nomaximize = nomax;
        qpe_main_widget = mw;

#ifndef QTOPIA_DBUS_IPC
        qcopQok = true;
        sendQCopQ();
#endif

        if ( qpe_main_widget ) {
            if ( !qpe_main_widget->isVisible() ) {
                if ( !nomaximize )
                    qpe_main_widget->showMaximized();
                else
                    qpe_main_widget->show();
            }
            qpe_main_widget->raise();
            qpe_main_widget->activateWindow();
        }
    }

    static void updateContext(QWidget *w)
    {
        if (!w->hasEditFocus())
            return;
        if (ContextKeyManager::instance()->haveLabelForWidget(w, Qt::Key_Back, w->hasEditFocus()))
            return;
        if (ContextKeyManager::instance()->findHelper(w))
            return;
#ifndef QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING
        QLineEdit *l = qobject_cast<QLineEdit*>(w);
        if (!l && w->inherits("QSpinBox"))
            l = ((QSpinBoxLineEditAccessor*)w)->getLineEdit();
        if (!l && w->inherits("QComboBox"))
            l = ((QComboBox*)w)->lineEdit();
        if (l) {
            if (l->text().length() == 0 || l->isReadOnly())
                ContextKeyManager::instance()->setStandard(w, Qt::Key_Back, QSoftMenuBar::RevertEdit);
            else if (l->cursorPosition() == 0 && !l->hasSelectedText())
                ContextKeyManager::instance()->setStandard(w, Qt::Key_Back, QSoftMenuBar::NoLabel);
            else
                ContextKeyManager::instance()->setStandard(w, Qt::Key_Back, QSoftMenuBar::BackSpace);
        } else if (w->inherits("QTextEdit") && !w->inherits("QTextBrowser")) {
            QTextEdit *l = (QTextEdit*)w;
            if (l->document()->isEmpty() || l->isReadOnly()) {
                ContextKeyManager::instance()->setStandard(w, Qt::Key_Back, QSoftMenuBar::RevertEdit);
            } else {
                QTextCursor cursor = l->textCursor();
                if (cursor.position() == 0 && !cursor.hasSelection())
                    ContextKeyManager::instance()->setStandard(w, Qt::Key_Back, QSoftMenuBar::NoLabel);
                else
                    ContextKeyManager::instance()->setStandard(w, Qt::Key_Back, QSoftMenuBar::BackSpace);
            }
        }
#endif // QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING
    }

    static void updateButtonSoftKeys(QWidget *w)
    {
#ifdef QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING
        Q_UNUSED(w);
#else
        if(ContextKeyManager::instance()->findHelper(w)){
            return;
        };
        QAbstractButton *b = qobject_cast<QAbstractButton*>(w);
        if (b && b->isCheckable()) {
            if (w->inherits("QCheckBox")) {
                if (b->isChecked())
                    ContextKeyManager::instance()->setStandard(w, Qt::Key_Select, QSoftMenuBar::Deselect);
                else
                    ContextKeyManager::instance()->setStandard(w, Qt::Key_Select, QSoftMenuBar::Select);
            }
        }
#endif // QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING
    }

    static void updateBrowserSoftKeys(QWidget *w)
    {
        if(ContextKeyManager::instance()->findHelper(w)){
            return;
        };
#ifndef QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING
        QTextBrowser *tb = qobject_cast<QTextBrowser*>(w);
        if (!tb || ContextKeyManager::instance()->haveCustomLabelForWidget(w, Qt::Key_Select, w->hasEditFocus()))
            return;
        if (tb->textCursor().hasSelection() &&
            !tb->textCursor().charFormat().anchorHref().isEmpty()) {
            ContextKeyManager::instance()->setStandard(w, Qt::Key_Select, QSoftMenuBar::Select);
        } else {
            ContextKeyManager::instance()->setStandard(w, Qt::Key_Select, QSoftMenuBar::NoLabel);
        }
#endif // QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING
    }

    void loadKeyRemapTable()
    {
        // This function loads key remappings from defaultbuttons.conf,
        // which is used to translate raw key codes into the special codes
        // that Qtopia expects.  Normally this isn't required because the
        // device vendor will set up the mappings correctly at the keyboard
        // driver level.  However, if Qtopia applications are running under
        // a foreign launcher environment, it may not be possible to map
        // the keys at the driver level.  The following is an example:
        //
        //      [RemapKeys]
        //      Back=Esc
        //      Context1=F4
        //      Select=Return
        //
        QSettings config(Qtopia::defaultButtonsFile(), QSettings::IniFormat);
        config.beginGroup( QLatin1String("RemapKeys") );
        QStringList keys = config.childKeys();
        foreach (QString key, keys) {
            int from = QKeySequence(config.value(key).toString())[0];
            int to = QKeySequence(key)[0];
            if (from != 0 && to != 0)
                keyRemapper.insert(from, to);
        }
        remapKeys = !keyRemapper.isEmpty();
    }

    const char *appKey;

    QString styleName;
    QString decorationName;
    QString decorationTheme;
    QPointer<QMenu> editMenu;
    QPointer<CalendarMenu> calendarMenu;
#ifdef QTOPIA_ENABLE_FADE_IN_WINDOW
    QBasicTimer fadeInTimer;
    double fadeInOpacity;
    QPointer<QWidget> fadeInWidget;
#endif
    QFileResourceFileEngineHandler *fileengine;
#ifdef POPUP_SHADOWS
    QMap<QWidget*,ShadowWidget*> shadowMap;
#endif

    QtopiaChannel *sysChannel;
    QtopiaChannel *pidChannel;
    QContentChangedChannel *contentChangedChannel;

    bool remapKeys;
    QMap<int, int> keyRemapper;

#ifdef Q_WS_X11
    QList<QtopiaApplication::X11EventFilter *> x11Filters;
#endif
#if defined(GREENPHONE_EFFECTS) && defined(QT_QWS_GREENPHONE)
    FadeThread * greenphoneEffects()
    {
        if(!m_greenphoneEffects)
            m_greenphoneEffects = new FadeThread(0);
        return m_greenphoneEffects;
    }
    FadeThread *m_greenphoneEffects;
#endif
    QBasicTimer focusOutTimer;
    QPointer<QWidget> focusOutWidget;
};

int qtopia_muted=0;

static void setVolume(int t=0, int percent=-1)
{
    Q_UNUSED(t);
    Q_UNUSED(percent);
#if 0
    switch (t) {
        case 0: {
            QSettings cfg(QLatin1String("Trolltech"),QLatin1String("Sound"));
            cfg.beginGroup(QLatin1String("System"));
            if ( percent < 0 ) {
                percent = cfg.value(QLatin1String("Volume"),50).toInt();
                qtopia_muted = cfg.value(QLatin1String("Muted"),false).toBool();
            }
            int fd = 0;
            if ((fd = open("/dev/mixer", O_RDWR))>=0) { // Some devices require this, O_RDONLY doesn't always work
                int vol = qtopia_muted ? 0 : percent;
                // set both channels to same volume
                vol |= vol << 8;
                ioctl(fd, MIXER_WRITE(0), &vol);
                ::close(fd);
            }
        } break;
    }
#endif
}


/*!
  \class QtopiaApplication
  \mainclass
  \brief The QtopiaApplication class implements the system services
   available to all Qtopia applications.

  By using QtopiaApplication instead of QApplication, a standard Qt
  application becomes a Qtopia application. It automatically follows
  style changes, quits and raises, and in the case of
  \l {Qtopia - Main Document Widget}{document-oriented} applications,
  changes the currently displayed document in response to the environment.

  The QtopiaApplication class also controls the life cycle of a Qtopia
  application.  Applications are automatically started by Qtopia when a QCop
  message or service request is sent to their application channel.  The QCop
  application channel is an implicit, application specific channel of the form
  \c {QPE/Application/<application name>}.

  Conceptually a Qtopia application will continue running as long as it has
  work to do.  Concretely, it will not terminate so long as;

  \list 1
  \i There are subsequent QCop messages or service requests to process,
  \i The application has visible UI, or
  \i The application has running "tasks"
  \endlist

  Visible UI is defined as top level widgets that are not
  \l QWidget::isHidden() hidden, although they may be obscured by other
  windows.  Most UI-centric applications can rely on the visible UI rule to
  implicitly indicate to QtopiaApplication that there is still "work to do", and
  thus correctly manage their execution.  As the user explores the application,
  new windows (or widgets in the case of an application that utilizes
  QStackedWidget) are created and when they back out, each window is closed
  until none remain and the application terminates.

  Conversely, tasks are used by an application to explicitly communicate to
  QtopiaApplication that there is still work to do.  Tasks are used by
  applications that need to continue to run even if they are displaying no UI.
  System daemons or background applications that respond to service requests
  but otherwise display no UI, are obvious cases, but even traditionally UI-centric
  applications may need to continue after the user has "closed" them to
  complete, for example, transmitting a file or saving state.  Applications
  may register and unregister tasks at any time by calling the
  registerRunningTask() and unregisterRunningTask() methods.

  There are two optimizations to the conceptual life cycle model.  To improve 
  system performance, Qtopia supports preloaded applications and a special 
  termination mode known as lazy application shutdown.  Preloaded applications 
  are automatically launched at system startup and remain running.  The
  applications are never shutdown, only hidden. 
  Applications may be marked as preloaded by adding their name 
  to the \c {AppLoading\PreloadApps} list in the \c {Trolltech/Launcher} 
  configuration file.  For example, the following would preload the 
  \c addressbook and \c qtmail applications.

  \code
  [AppLoading]
  PreloadApps=addressbook,qtmail
  \endcode

  Lazy application shutdown is a similar, system-wide optimization.  When
  enabled, applications will continue running in a hidden state even if they
  have no work to do until they are explicitly shutdown by the Qtopia system
  server when the system becomes too loaded, or too many such applications are
  superfluously running.  While preloaded applications allow a system
  configurator to improve performance of a preselected few applications, the
  rationale behind lazy application shutdown is to allow the system to
  dynamically tune itself to improve performance of the most frequently run
  applications.  For example, if the user constantly switched between the
  \c addressbook and \c todo applications, these would remain in the lazy
  shutdown state which would improve the startup time of subsequent
  "launches".  To enable lazy application shutdown, set the
  \c {AppLoading/LazyShutdown} key in the \c {Trolltech/Launcher} configuration
  file to true.  For example,

  \code
  [AppLoading]
  LazyShutdown=true
  \endcode

  For information on creating a new application to run within Qtopia, see
  \l {Tutorial: Creating a New Application}{Creating a New Application}.

  \ingroup environment
*/

/*!
  \fn void QtopiaApplication::clientMoused()

  \internal
*/

/*!
  \fn void QtopiaApplication::timeChanged();

  This signal is emitted when the time changes outside the normal
  passage of time, that is, if the time is set backwards or forwards.

  If the application offers the TimeMonitor service, it will get
  the QCop message that causes this signal even if it is not running,
  thus allowing it to update any alarms or other time-related records.
*/

/*!
  \fn void QtopiaApplication::categoriesChanged();

  This signal is emitted whenever a category is added, removed or edited.

  \sa QCategoryManager
*/

/*!
  \fn void QtopiaApplication::contentChanged(const QContentIdList &ids, QContent::ChangeType type)

  This signal is emitted whenever one or more QContent is stored, removed or edited.
  \a ids contains the list of Id's of the content that is being modified.
  \a type contains the type of change.

  \sa {Document System}
*/

/*!
  \fn void QtopiaApplication::resetContent()

  This signal is emitted whenever a new media database is attached, or the
  system needs to reset QContentSet instances to refresh their list of items.

  \sa {Document System}
 */

/*!
  \fn void QtopiaApplication::clockChanged( bool ampm );

  This signal is emitted when the clock style is changed. If
  \a ampm is true, the clock style is a 12-hour AM/PM clock, otherwise
  it is a 24-hour clock.

  \warning When using the QTimeString functions all strings obtained by QTimeString
  should be updated to reflect the changes.

  \sa dateFormatChanged()
*/

/*!
    \fn void QtopiaApplication::volumeChanged( bool muted )

    This signal is emitted whenever the mute state is changed. If \a
    muted is true, then sound output has been muted.
*/

/*!
    \fn void QtopiaApplication::weekChanged( bool startOnMonday )

    This signal is emitted if the week start day is changed. If \a
    startOnMonday is true then the first day of the week is Monday otherwise
    the first day of the week is Sunday.
*/

/*!
    \fn void QtopiaApplication::dateFormatChanged()

    This signal is emitted whenever the date format is changed.

    \warning When using QTimeString functions, all QTimeString strings should be updated
    to reflect the changes.

    \sa clockChanged()
*/

/*!
    \fn void QtopiaApplication::flush()

    \internal
*/

/*!
    \fn void QtopiaApplication::reload()

    \internal
*/

/*!
  \fn void QtopiaApplication::appMessage( const QString& msg, const QByteArray& data )

  This signal is emitted when a message is received on the
  application's QPE/Application/\i{appname}  \l {Qtopia IPC Layer}{Qtopia} channel.

  The slot to which you connect uses \a msg and \a data
  as follows:

\code
    void MyWidget::receive( const QString& msg, const QByteArray& data )
    {
        QDataStream stream( data );
        if ( msg == "someMessage(int,int,int)" ) {
            int a,b,c;
            stream >> a >> b >> c;
            ...
        } else if ( msg == "otherMessage(QString)" ) {
            ...
        }
    }
\endcode

*/

/*!
    Returns true if this application will keep running because there are
    visible widgets or registered tasks; otherwise returns false.
*/
bool QtopiaApplication::willKeepRunning() const
{
    if(type() == GuiServer) return true;
    Q_ASSERT(d->lifeCycle);

#ifndef QTOPIA_DBUS_IPC
    d->qcopQok = true;
    d->sendQCopQ();
#endif

    return d->lifeCycle->willKeepRunning();
}

/*!
  Register the task \a name as running.  If \a taskObj is supplied, the task
  will be automatically unregistered when \a taskObj is destroyed.  Tasks may
  be manually unregistered by calling unregisterRunningTask().  For a
  broader discussion of tasks, please refer to the QtopiaApplication overview
  documentation.

  It is not possible to register two tasks with the same \a name
  simultaneously.  The last registration with \a name will
  replace the previous registration.

  Certain task names are reserved for use by the system and should not be used
  directly by application programmers.  The following table describes the list
  of reserved task names.

  \table
  \header \o name \o Description
  \row \o \c {UI} \o An implied system task that is "running" whenever the application has visible UI.  Visible UI is defined as top level widgets that are not hidden (as determined by QWidget::isHidden()), although they may be obscured by other windows.
  \row \o \c {QtopiaPreload} \o A task used by the system to keep pre-loaded applications running indefinately.
  \row \o \c {Qtopia*} \o All task names beginning with "Qtopia" are reserved for future use.
  \endtable
  */
void QtopiaApplication::registerRunningTask(const QString &name,
                                            QObject *taskObj)
{
    if(type() == GuiServer) return; // Server doesn't support shutdown like this
    Q_ASSERT(d->lifeCycle);

    d->lifeCycle->registerRunningTask(name, taskObj);
}

/*!
  Unregister the task \a name previously registered with registerRunningTask().
  Attempting to unregister a task that has not been registered has no effect.
 */
void QtopiaApplication::unregisterRunningTask(const QString &name)
{
    if(type() == GuiServer) return; // Server doesn't support shutdown like this
    Q_ASSERT(d->lifeCycle);

    d->lifeCycle->unregisterRunningTask(name);
}

/*!
  Unregister the task \a taskObj previously registered with
  registerRunningTask().  Attempting to unregister a task that has not been
  registered has no effect.
 */
void QtopiaApplication::unregisterRunningTask(QObject *taskObj)
{
    if(type() == GuiServer) return; // Server doesn't support shutdown like this
    Q_ASSERT(d->lifeCycle);

    d->lifeCycle->unregisterRunningTask(taskObj);
}

#ifndef QTOPIA_DBUS_IPC
void QtopiaApplication::processQCopFile()
{
    QString qcopfn = d->appName;
    qcopfn.prepend( Qtopia::tempDir() + "qcop-msg-" );
    QFile qcopfile(qcopfn);

    if ( qcopfile.open(QIODevice::ReadWrite) ) {
#ifdef QTOPIA_POSIX_LOCKS
        struct flock fl;
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        fl.l_pid = getpid();
        if (fcntl(qcopfile.handle(), F_SETLKW, &fl) == -1) {
            qWarning("Could not acquire lock: %d %s", errno, strerror(errno));
        }
#else
        if (flock(qcopfile.handle(), LOCK_EX) == -1) {
            qWarning("Could not acquire lock: %d %s", errno, strerror(errno));
        }
#endif
        QDataStream ds( &qcopfile );
        QString channel, message;
        QByteArray data;
        while(!ds.atEnd()) {
            ds >> channel >> message >> data;
            d->enqueueQCop(channel,message,data);
        }
        Qtopia::truncateFile(qcopfile, 0);
        qcopfile.flush();
#ifdef QTOPIA_POSIX_LOCKS
        fl.l_type = F_UNLCK;
        if (fcntl(qcopfile.handle(), F_SETLK, &fl) == -1) {
            qWarning("Could not release lock: %d %s", errno, strerror(errno));
        }
#else
        if (flock(qcopfile.handle(), LOCK_UN) == -1) {
            qWarning("Could not release lock: %d %s", errno, strerror(errno));
        }
#endif
    }
}
#endif

/*!
    Loads the translation file specified by \a qms. \a qms can be the name
    of a library or application.
*/
#ifndef QT_NO_TRANSLATION
void QtopiaApplication::loadTranslations( const QString& qms )
{
    loadTranslations(QStringList(qms));
}


/*!
  It behaves essentially like the above function.

  It is the same as calling \c{loadTranslations(QString)} for each entry of \a qms.
*/
void QtopiaApplication::loadTranslations( const QStringList& qms )
{
    QStringList qmList( qms );
    QStringList langs = Qtopia::languageList();

    QStringList qpepaths = Qtopia::installPaths();
    for (QStringList::ConstIterator qit = qpepaths.begin(); qit!=qpepaths.end(); ++qit) {
        for (QStringList::ConstIterator it = langs.begin(); it!=langs.end(); ++it) {
            QString lang = *it;

            //Optimisation: Assumption the source is en_US => don't load translations
            if ( lang == QLatin1String("en_US")){
                qLog(I18n) << "Loading of en_US requested, skipping loading of translation files";
                return;
            }
            QTranslator * trans;
            QString tfn;
            QMutableStringListIterator qmit( qmList );
            while ( qmit.hasNext() ) {
                trans = new QTranslator(qApp);
                tfn = *qit + QLatin1String("i18n/") + lang + QLatin1String("/") + qmit.next() + QLatin1String(".qm");
                qLog(I18n) << "Loading" << tfn;
                if ( trans->load( tfn )) {
                    qApp->installTranslator( trans );
                    qmit.remove();
                }
                else {
                    qLog(I18n) << "Cannot load " << tfn;
                    delete trans;
                }
            }
        }
    }
}
#endif

/*!
  Returns a pointer to the application's QtopiaApplication instance.
 */
QtopiaApplication *QtopiaApplication::instance()
{
    static bool check = false;
    static QtopiaApplication *instance = 0;

    if(!check) {
        if(qApp) {
            instance = qobject_cast<QtopiaApplication *>(qApp);
            check = (bool)instance;
        }
    }

    return instance;
}

/*!
  Constructs a QtopiaApplication just as you would construct
  a QApplication, passing \a argc and \a argv.
*/
QtopiaApplication::QtopiaApplication( int& argc, char **argv)
: QApplication( argc, argv, GuiClient), d(0)
{
    init(argc, argv, GuiClient);
}

/*! \internal */
QtopiaApplication::QtopiaApplication( int& argc, char **argv, Type t )
: QApplication( argc, argv, t ), d(0)
{
    init(argc, argv, t);
}

void QtopiaApplication::init(int argc, char **argv, Type t)
{
#ifdef Q_WS_X11
    // Start up the QCop server under X11.
    if ( t == GuiServer )
        new QCopServer(this);
#else
    Q_UNUSED(t);
#endif

#ifdef QTOPIA_KEYPAD_NAVIGATION
    //NOTE: need to figure out keypad navigation before calling applyStyle
    QSettings config(Qtopia::defaultButtonsFile(), QSettings::IniFormat);
    config.beginGroup( QLatin1String("Device") );
    QString pi = config.value( QLatin1String("PrimaryInput"), QLatin1String("Keypad") ).toString().toLower();
    // anything other than touchscreen means keypad modal editing gets enabled
    bool keypadNavigation = pi != QLatin1String("touchscreen");
    QApplication::setKeypadNavigationEnabled(keypadNavigation);
    mousePreferred = !keypadNavigation;
#endif

    d = new QtopiaApplicationData;
#ifdef QTOPIA_USE_TEST_SLAVE
    /* Instantiate a test slave */
    testSlave();
#endif

#ifndef QT_NO_SXE
    if ( type() == GuiServer )
        setSxeAuthorizerRole( SxeAuthorizerServerOnly );
#endif

    setFont( QFont( QLatin1String("helvetica"), 9, QFont::Normal ) );

    d->fileengine = new QFileResourceFileEngineHandler();

    applyStyle();

    connect(desktop(), SIGNAL(workAreaResized(int)), this, SLOT(updateDialogGeometry()));

    QApplication::setQuitOnLastWindowClosed(false);

    QString dataDir(Qtopia::tempDir());
    if ( mkdir( dataDir.toLatin1(), 0700 ) ) {
        if ( errno != EEXIST ) {
            qFatal( QString("Cannot create Qtopia data directory with permissions 0700: %1")
                    .arg( dataDir ).toLatin1().constData() );
        }
    }

    struct stat buf;
    if ( lstat( dataDir.toLatin1(), &buf ) )
        qFatal( QString( "stat failed for Qtopia data directory: %1" )
                .arg( dataDir ).toLatin1().constData() );

    if ( !S_ISDIR( buf.st_mode ) )
        qFatal( QString( "%1 is not a directory" ).arg( dataDir ).toLatin1().constData() );

    if ( buf.st_uid != getuid() )
        qFatal( QString( "Qtopia data directory is not owned by user %1: %2" )
                .arg( getuid() ).arg( dataDir ).toLatin1().constData() );

    if ( (buf.st_mode & 0677) != 0600 )
        qFatal( QString( "Qtopia data directory has incorrect permissions (expecting 0700): %1" )
                .arg( dataDir ).toLatin1().constData() );

#ifdef QTOPIA_DBUS_IPC
    // Connect to the session bus as early as possible
    QDBusConnection dbc = QDBus::sessionBus();
    if (!dbc.isConnected()) {
        qFatal( QString( "Unable to connect to D-BUS: %1")
                .arg( dbc.lastError().message()).toLatin1().constData() );
    }
#endif

    QPixmapCache::setCacheLimit(256);  // sensible default for smaller devices.

    d->sysChannel = new QtopiaChannel( QLatin1String("QPE/System"), this );
    connect( d->sysChannel, SIGNAL(received(QString,QByteArray)),
             this, SLOT(systemMessage(QString,QByteArray)) );

    initApp( argc, argv );

    d->qpe_system_locale = new QtopiaSystemLocale();

#ifndef QT_NO_TRANSLATION
    QStringList qms;
    qms << QLatin1String("qt");
    qms << QLatin1String("libqtopiabase");
    qms << QLatin1String("libqtopia");
    qms << QLatin1String("libqtopiacomm");

    loadTranslations(qms);
#endif

#ifndef QTOPIA_HOST
    QtopiaSql::instance()->connectDiskChannel();
#endif

    QContentUpdateManager::instance();

    if ( type() == GuiServer ) {
        setVolume();
    }

    installEventFilter( this );
}

/*!
    \internal
*/
void QtopiaApplication::initApp( int argc, char **argv )
{
    QString channel = QString(argv[0]);
    d->appFullPath = channel;
    int slashPos = channel.lastIndexOf('/');
    if (slashPos >= 0)
        channel = channel.mid(slashPos+1);
    d->appName = channel;
    qApp->setApplicationName(d->appName);

#if defined(QTOPIA_DBUS_IPC)
    // Take care of quicklauncher re-init case
    if (d->m_appChannel)
        delete d->m_appChannel;

    d->m_appChannel = new DBUSQtopiaApplicationChannel(d->appName, this);
    connect(d->m_appChannel, SIGNAL(received(QString,QByteArray)),
            this, SLOT(pidMessage(QString,QByteArray)));
#endif

    if(type() != GuiServer) {
        if(d->lifeCycle) {
            d->lifeCycle->reinit();
        } else {
            d->lifeCycle = new QtopiaApplicationLifeCycle(this);
            QObject::connect(d->lifeCycle, SIGNAL(quit()), this, SLOT(quit()));
        }
    }

    delete d->pidChannel;
    d->preloaded = false;
    if(type() != GuiServer)
        d->lifeCycle->unregisterRunningTask("QtopiaPreload");

    //enforce update of image and sound dirs when started by quicklauncher
    d->fileengine->setIconPath( QStringList() );

    loadTranslations(QStringList()<<channel);

#ifdef Q_WS_QWS
    qt_fbdpy->setIdentity( channel ); // In E 2.3.6
#endif

#ifndef QTOPIA_DBUS_IPC
    channel = QLatin1String("QPE/pid/") + QString::number(getpid());
    d->pidChannel = new QtopiaChannel( channel, this);
    connect( d->pidChannel, SIGNAL(received(QString,QByteArray)),
            this, SLOT(dotpidMessage(QString,QByteArray)));
#endif

    {
        QtopiaIpcEnvelope env("QPE/QtopiaApplication",
                              "available(QString,int)");
        env << QtopiaApplication::applicationName() << ::getpid();
    }

#ifndef QTOPIA_DBUS_IPC
    processQCopFile();
#endif

    int a = 0;
    while (a < argc) {
        if(qstrcmp(argv[a], "-noshow") == 0) {
            argc-=1;
            for (int j = a; j < argc; j++)
                argv[j] = argv[j+1];
            d->noshow = true;
        } else {
            a++;
        }
    }

#ifdef Q_WS_QWS
    /* overide stored arguments */
    setArgs(argc, argv);
#endif
}

/*!
    \enum QtopiaApplication::SxeAuthorizerRole

    Specifies whether to create SXE Authorizer resources in this Qtopia Application.

    \value SxeAuthorizerClientOnly  No SXE Authorizer resources required, only acts as a client
    \value SxeAuthorizerServerAndClient  Authorizer resources required, also acts as a client of other authorizers
    \value SxeAuthorizerServerOnly  Authorizer resources required.
*/

#if !defined(QT_NO_SXE) || defined(Q_QDOC)
/*!
  Returns this applications QtopiaApplication::SxeAuthorizerRole.  In general
  this will be SxeAuthorizerClientOnly for all but a few applications such as the
  qpe server, mediaserver, and other applications which must authorize requests
  using SXE.

  \sa QtopiaApplication::SxeAuthorizerRole
*/
QtopiaApplication::SxeAuthorizerRole QtopiaApplication::sxeAuthorizerRole() const
{
    return d->sxeAuthorizerRole;
}

/*!
  Sets this applications QtopiaApplication::SxeAuthorizerRole to \a role.  In general
  this should not be required for all but a few applications such as the
  mediaserver, and other applications which must authorize requests
  using SXE; since the default value of SxeAuthorizerClientOnly will be correct.

  The qpe server (actually any app for which type() is GuiServer) will
  default to SxeAuthorizerServerOnly.

  \sa QtopiaApplication::SxeAuthorizerRole
*/
void QtopiaApplication::setSxeAuthorizerRole( SxeAuthorizerRole role )
{
#if QT_VERSION < 0x040400
    if ( !d->authorizerInitialized.testAndSet( 0, 1 ))
#else
    if ( !d->authorizerInitialized.testAndSetRelaxed( 0, 1 ))
#endif
    {
        qWarning( "cannot set authorizer status after initialization!!" );
        return;
    }
    d->sxeAuthorizerRole = role;
    if ( role != SxeAuthorizerClientOnly )
    {
        QTransportAuth *a = QTransportAuth::getInstance();
        SXEPolicyManager *p = SXEPolicyManager::getInstance();
        QString confPath = QPackageRegistry::getInstance()->sxeConfPath();
        a->registerPolicyReceiver( p );
        a->setKeyFilePath( confPath );  // this is now the dir, not the file
        a->setLogFilePath( Qtopia::tempDir() + QLatin1String("sxe_discovery.log") );
        a->setPackageRegistry( QPackageRegistry::getInstance() );
    }
}
#endif

struct InputMethodHintRec {
    InputMethodHintRec(QtopiaApplication::InputMethodHint h, const QString& p) :
        hint(h), param(p) {}
    QtopiaApplication::InputMethodHint hint;
    QString param;
};
static QMap<QWidget*,InputMethodHintRec*>* inputMethodDict=0;
static void deleteInputMethodDict()
{
    if ( inputMethodDict )
        delete inputMethodDict;
    inputMethodDict = 0;
}

static void createInputMethodDict()
{
    if ( !inputMethodDict ) {
        inputMethodDict = new QMap<QWidget*,InputMethodHintRec*>;
        qAddPostRoutine(deleteInputMethodDict);
    }
}

/*!
  Returns the currently set Input Method hint for \a widget.

  Returns QtopiaApplication::Normal if no hint has been set.

  \sa setInputMethodHint(), InputMethodHint
*/
QtopiaApplication::InputMethodHint QtopiaApplication::inputMethodHint( QWidget* widget )
{
    if (inputMethodDict && widget && inputMethodDict->contains(widget)) {
        InputMethodHintRec *r = inputMethodDict->value(widget);
        return r->hint;
    }
    return Normal;
}

/*!
  Returns the currently set hint parameter for \a widget with Named
  input method hint.

  \sa setInputMethodHint(), InputMethodHint
*/
QString QtopiaApplication::inputMethodHintParam( QWidget* widget )
{
    if ( inputMethodDict && widget && inputMethodDict->contains(widget)) {
        InputMethodHintRec* r = inputMethodDict->value(widget);
        return r->param;
    }
    return QString();
}

/*!
    \enum QtopiaApplication::InputMethodHint

    \value Normal the widget sometimes needs text input.
    \value AlwaysOff the widget never needs text input.
    \value AlwaysOn the widget always needs text input.
    \value Number the widget needs numeric input.
    \value PhoneNumber the widget needs phone-style numeric input.
    \value Words the widget needs word input.
    \value ProperNouns the widget needs proper names of peoples or places.
    \value Text the widget needs non-word input.
    \value Named the widget needs special input, defined by \c param.
        Each input method may support a different range of special
        input types, but will default to Text if they do not know the
        type.

    By default, QLineEdit and QTextEdit have the Words hint
    unless they have a QIntValidator, in which case they have the Number hint.
    This is appropriate for most cases, including the input of names (new
    names are added to the user's dictionary by switching to text mode and 
    entering them as usual).
    All other widgets default to Normal mode.

    \sa inputMethodHint(), setInputMethodHint()
*/

/*!
    \enum QtopiaApplication::PowerConstraint

    \value Disable all power saving functions are disabled.
    \value DisableLightOff the screen's backlight will not be turned off
        (dimming remains active).
    \value DisableSuspend the device will not suspend
    \value Enable all power saving functions are enabled.

    Less severe PowerConstraints always imply more severe constraints
    (e.g DisableLightOff implies DisableSuspend). The exact implications are
    determined by the integer value that each constraint is assigned to (enumeration value).
    A low valued constraint will always imply a high valued constraint.

    \sa setPowerConstraint()
*/

/*!
  Provides a hint to the system that \a widget has use for the text input method
  specified by \a named. Such methods are input-method-specific and
  are defined by the files in [qt_prefix]/etc/im/ for each input method.

  For example, the phone-keys input method includes support for the
  named input methods:

\list
  \o email
  \o netmask
  \o url
\endlist

  The effect in the phone key input method is to modify the binding of
  phone keys to characters (such as making "\c{@}" easier to input), and to
  add additional \i words to the recognition word lists, such as: \i www.

  \bold{Note:} If the current input method doesn't understand the hint, 
  it will be ignored.

  In addition, Qtopia supports auxiliary hints.  These modify the hint 
  without overriding it, and are appended to the hint, separated by 
  whitespace.

  For example, the phone-keys input method includes support for the 
  following auxiliary hints:

  \table
    \header \o name \o Description
    \row \o autocapitalization \o The first word of the field, and the first
    word of a sentence have their first letter capitalized.
    \row \o noautocapitalization \o All words not capitalized by the user are
    in lower-case
    \row \o propernouns \o The first letter of every word is capitalized (propernouns can
    be used as either an auxiliary hint, e.g. "text autocapitalization propernouns"
    or as a hint on it's own, in which case autocapitalization and words are
    implied).
  \endtable

  Specific input methods may ignore these auxiliary hints, or only handle
  some of them. Like all hints, if the input method does not understand the
  hint, it will ignore it.

  \sa inputMethodHint(), InputMethodHint
*/
void QtopiaApplication::setInputMethodHint( QWidget *widget, const QString& named )
{
    setInputMethodHint(widget,Named,named);
}

/*!
  Hints to the system that \a widget has use for text input methods
  as specified by \a mode.  If \a mode is \c Named, then \a param
  specifies the name.

  \sa inputMethodHint(), InputMethodHint
*/
void QtopiaApplication::setInputMethodHint( QWidget* widget, InputMethodHint mode, const QString& param )
{
    createInputMethodDict();
    if ( mode == Normal ) {
        if (inputMethodDict->contains(widget))
            delete inputMethodDict->take(widget);
    } else if (inputMethodDict->contains(widget)) {
        InputMethodHintRec *r = (*inputMethodDict)[widget];
        r->hint = mode;
        r->param = param;
    } else {
        inputMethodDict->insert(widget, new InputMethodHintRec(mode,param));
        connect(widget, SIGNAL(destroyed(QObject*)), qApp, SLOT(removeSenderFromIMDict()));
    }
    if ( widget->hasFocus() )
        sendInputHintFor(widget,QEvent::None);
}

/*!
  Explicitly show the current input method.

  Input methods are indicated in the title by a small icon. If the
  input method is activated (shown) and it has a visible input window
  (such as a virtual keyboard) then it takes up some proportion
  of the bottom of the screen, to allow the user to interact.

  \sa hideInputMethod()
*/
void QtopiaApplication::showInputMethod()
{
    QtopiaChannel::send( QLatin1String("QPE/InputMethod"), QLatin1String("showInputMethod()") );
}

/*!
  Explicitly hide the current input method.

  The current input method is still indicated in the title, but no
  longer takes up screen space, and can no longer be interacted with.

  This function only applies to input methods that have a visible
  input window, such as a virtual keyboard.

  \sa showInputMethod()
*/
void QtopiaApplication::hideInputMethod()
{
    QtopiaChannel::send( QLatin1String("QPE/InputMethod"), QLatin1String("hideInputMethod()") );
}

static bool isSingleFocusWidget(QWidget *focus)
{
    bool singleFocusWidget = false;
    if (focus) {
        QWidget *w = focus;
        singleFocusWidget = true;
        while ((w = w->nextInFocusChain()) != focus) {
            if (w->isVisible() && focus != w->focusProxy() && w->focusPolicy() & Qt::TabFocus) {
                if ( singleFocusWidget ) {
                    qLog(UI) << "Multi Focus:";
                    qLog(UI) << "  Current Focus:" << focus;
                }
                qLog(UI) << "  Next Focus: " << w;
                singleFocusWidget = false;
            }
        }
        if ( singleFocusWidget )
            qLog(UI) << "Single Focus: " << w;
    }

    return singleFocusWidget;
}

#ifdef Q_WS_QWS

void QtopiaApplication::mapToDefaultAction( QWSKeyEvent *ke, int key )
{
    // specialised actions for certain widgets. May want to
    // add more stuff here.
    if ( activePopupWidget() && activePopupWidget()->inherits( "QListBox" )
         && activePopupWidget()->parentWidget()
         && activePopupWidget()->parentWidget()->inherits( "QComboBox" ) )
        key = Qt::Key_Return;

    if ( activePopupWidget() && activePopupWidget()->inherits( "QMenu" ) )
        key = Qt::Key_Return;

    ke->simpleData.keycode = key;
}

/*!
  \internal
  Filters Qt event \a e to implement Qtopia-specific functionality.
*/
bool QtopiaApplication::qwsEventFilter( QWSEvent *e )
{
#ifdef QTOPIA_USE_TEST_SLAVE
    if (testSlave()) {
        testSlave()->qwsEventFilter(e);
    }
#endif
    if ( type() == GuiServer ) {
        switch ( e->type ) {
            case QWSEvent::Mouse:
                if ( e->asMouse()->simpleData.state && !QWidget::find(e->window()) )
                    emit clientMoused();
        }
    }
    if ( e->type == QWSEvent::Key ) {
#ifdef QT_QWS_SL5XXX
        QWSKeyEvent *ke = (QWSKeyEvent *)e;
        if ( ke->simpleData.keycode == Qt::Key_F33 )
            ke->simpleData.keycode = Qt::Key_Back;
        else if (ke->simpleData.keycode == Qt::Key_F30)
            ke->simpleData.keycode = Qt::Key_Select;
        else if (ke->simpleData.keycode == Qt::Key_Escape)
            ke->simpleData.keycode = Qt::Key_Back;
#endif
    } else if ( e->type == QWSEvent::Focus ) {
        if ( !d->notbusysent ) {
            if ( qApp->type() != QApplication::GuiServer ) {
                QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("notBusy(QString)") );
                e << d->appName;
            }
            d->notbusysent=true;
        }

        QWSFocusEvent *fe = (QWSFocusEvent*)e;
        if ( !fe->simpleData.get_focus ) {
            QWidget *active = activeWindow();
            while ( active && active->windowType() == Qt::Popup ) {
                active->close();
                active = activeWindow();
            }
        } else {
            // make sure our modal widget is ALWAYS on top
            QWidget *topm = activeModalWidget();
            if ( topm && (int)topm->winId() != fe->simpleData.window) {
                topm->raise();
                topm->activateWindow();
            }
        }
        if ( fe->simpleData.get_focus && inputMethodDict ) {
            InputMethodHint m = inputMethodHint( QWidget::find(e->window()) );
            if ( m == AlwaysOff )
                QtopiaApplication::hideInputMethod();
            if ( m == AlwaysOn )
                QtopiaApplication::showInputMethod();
        }
    }

    return QApplication::qwsEventFilter( e );
}

#endif // Q_WS_QWS

#ifdef Q_WS_X11

/*!
  \class QtopiaApplication::X11EventFilter
  \ingroup environment
  \brief The X11EventFilter class provides an interface for filtering Qt Window System events.
 */

/*!
  \fn QtopiaApplication::X11EventFilter::~X11EventFilter()
  \internal
 */

/*!
  \fn bool QtopiaApplication::X11EventFilter::x11EventFilter(XEvent *event)

  Called when an X11 \a event is received.  Return true to filter the event, or
  false to allow the event to continue propagation.
  */

/*!
  Install the \a filter for X11 events.  Installing an event filter is
  equivalent to deriving from QtopiaApplication directly and overriding the
  QtopiaApplication::x11EventFilter() method.

  Multiple X11 event filters may be installed simultaneously.  In this case,
  each event filter is queried sequentially in the order it was installed.  If
  any filter filters the event, subsequent filters will not be called.
 */
void QtopiaApplication::installX11EventFilter(X11EventFilter *filter)
{
    if (filter && d)
        d->x11Filters.append(filter);
}

/*!
  Remove all instances of \a filter from the list of event filters.  This method
  should be called when an event filter is destroyed.  No automatic cleanup is
  performed.
 */
void QtopiaApplication::removeX11EventFilter(X11EventFilter *filter)
{
    if (!filter || !d)
        return;
    QList<QtopiaApplication::X11EventFilter *>::Iterator it;
    it = d->x11Filters.begin();
    while (it != d->x11Filters.end()) {
        if (*it == filter)
            it = d->x11Filters.erase(it);
        else
            ++it;
    }
}

/*!
    \reimp
*/
bool QtopiaApplication::x11EventFilter(XEvent *e)
{
    if (d && !d->x11Filters.isEmpty()) {
        foreach (QtopiaApplication::X11EventFilter *filter, d->x11Filters) {
            if (filter->x11EventFilter(e))
                return true;
        }
    }
    return false;
}

#endif // Q_WS_X11

/*!
  Destroys the QtopiaApplication.
*/
QtopiaApplication::~QtopiaApplication()
{
    QContentUpdateManager::instance()->sendUpdate();

    if ( !d->notbusysent ) {
        // maybe we didn't map a window - still tell the server we're not
        // busy anymore.
        if ( qApp->type() != QApplication::GuiServer ) {
            QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("notBusy(QString)") );
            e << d->appName;
        }
    }
    // Need to delete QtopiaChannels early, since the display will
    // be gone by the time we get to ~QObject().
    delete d->sysChannel;
    delete d->pidChannel;

    delete d->editMenu;
    delete d->calendarMenu;
    delete d;
    d = 0;
}

#define setPaletteEntry(pal, cfg, role, defaultVal, defaultAlpha) \
    setPalEntry(pal, cfg, #role, QPalette::role, defaultVal, defaultAlpha)
static void setPalEntry( QPalette &pal, const QSettings &config, const QString &entry,
                                QPalette::ColorRole role, const QString &defaultVal, int defaultAlpha )
{
    QString value = config.value( entry, defaultVal ).toString();
    if ( value[0] == '#' ) {
        QColor col(value);
        if (role != QPalette::Window) {
            // Setting alpha for Window color is very bad.
            // We deal with this in the exported backgorund code.
            int alpha = config.value(entry + "_alpha", QString::number(defaultAlpha)).toInt();
            col.setAlpha(alpha);
        }
        pal.setColor(role, col);
    } else {
        QPixmap pix;
        pix = QPixmap(QLatin1String(":image/")+value);
        pal.setBrush( role, QBrush(QColor(defaultVal), pix) );
    }
}

/*!
  \internal
*/
void QtopiaApplication::applyStyle()
{
    QSettings config(QLatin1String("Trolltech"),QLatin1String("qpe"));
    config.beginGroup( QLatin1String("Appearance") );

    QString themeFile = Qtopia::qtopiaDir() + QLatin1String("etc/themes/qtopia.conf"); // default
    QString theme = config.value("Theme").toString(); // The server ensures this value is present and correct

    // Search for theme config in install paths.
    QStringList instPaths = Qtopia::installPaths();
    foreach (QString path, instPaths) {
        QString themeDataPath( path + QLatin1String("etc/themes/") + theme );
        if (QFile::exists(themeDataPath)) {
            themeFile = themeDataPath;
            break;
        }
    }

    QSettings themeCfg(themeFile, QSettings::IniFormat);
    themeCfg.beginGroup(QLatin1String("Theme"));

    // Update the icon path
    QStringList ip;
    QString iconPathText = themeCfg.value(QLatin1String("IconPath")).toString();
    if ( !iconPathText.isEmpty() )
        ip = iconPathText.split( ';', QString::SkipEmptyParts );
    d->fileengine->setIconPath( ip );

    QString styleName = config.value( QLatin1String("Style"), QLatin1String("phonestyle") ).toString();

    // Widget style; 0 if not a QtopiaStyle subclass
    QtopiaStyle *style = internalSetStyle( styleName );
    Q_UNUSED(style);

    // Colors
    // This first bit is same as settings/appearence/appearence.cpp, readColorScheme()
    QPalette tempPal;
    setPaletteEntry( tempPal, config, Button, QLatin1String("#F0F0F0"), 176 );
    setPaletteEntry( tempPal, config, Background, QLatin1String("#EEEEEE"), 64 );
    QPalette pal( tempPal.button().color(), tempPal.background().color() );
    setPaletteEntry( pal, config, Button, QLatin1String("#F0F0F0"), 176 );
    setPaletteEntry( pal, config, Background, QLatin1String("#EEEEEE"), 64 );
    setPaletteEntry( pal, config, Base, QLatin1String("#FFFFFF"), 176 );
    setPaletteEntry( pal, config, AlternateBase, QLatin1String("#FFFFFF"), 176 );
    setPaletteEntry( pal, config, Highlight, QLatin1String("#779e1f"), 176 );
    setPaletteEntry( pal, config, Foreground, QLatin1String("#000000"), 255 );
    QString color = config.value( QLatin1String("HighlightedText"), QLatin1String("#FFFFFF") ).toString();
    pal.setColor( QPalette::HighlightedText, QColor(color) );
    color = config.value( QLatin1String("Text"), QLatin1String("#000000") ).toString();
    pal.setColor( QPalette::Text, QColor(color) );
    color = config.value( QLatin1String("ButtonText"), QLatin1String("#000000") ).toString();
    pal.setColor( QPalette::ButtonText, QColor(color) );
    color = config.value( QLatin1String("Link"), QLatin1String("#0000FF") ).toString();
    pal.setColor( QPalette::Link, QColor(color) );
    color = config.value( QLatin1String("LinkVisited"), QLatin1String("#FF00FF") ).toString();
    pal.setColor( QPalette::LinkVisited, QColor(color) );

    QString val = config.value( QLatin1String("Shadow") ).toString();
    if (!val.isEmpty()) {
        pal.setColor( QPalette::Shadow, QColor(val));
    } else {
        pal.setColor( QPalette::Shadow,
            pal.color(QPalette::Normal, QPalette::Button).dark(400) );
    }

    val = config.value( QLatin1String("Text_disabled") ).toString();
    if (!val.isEmpty()) {
        QColor col(val);
        int alpha = config.value( QLatin1String("Text_disabled_alpha") , 255).toInt();
        col.setAlpha(alpha);
        pal.setColor( QPalette::Disabled, QPalette::Text, col);
    } else {
        pal.setColor( QPalette::Disabled, QPalette::Text,
            pal.color(QPalette::Active, QPalette::Base).dark() );
    }

    val = config.value( QLatin1String("Foreground_disabled") ).toString();
    if (!val.isEmpty()) {
        QColor col(val);
        int alpha = config.value( QLatin1String("Foreground_disabled_alpha") , 255).toInt();
        col.setAlpha(alpha);
        pal.setColor( QPalette::Disabled, QPalette::Foreground, col);
    } else {
        pal.setColor( QPalette::Disabled, QPalette::Foreground,
            pal.color(QPalette::Active, QPalette::Background).dark() );
    }

    val = config.value( QLatin1String("ButtonText_disabled") ).toString();
    if (!val.isEmpty()) {
        QColor col(val);
        int alpha = config.value( QLatin1String("ButtonText_disabled_alpha") , 255).toInt();
        col.setAlpha(alpha);
        pal.setColor( QPalette::Disabled, QPalette::ButtonText, col);
    } else {
        pal.setColor( QPalette::Disabled, QPalette::ButtonText,
            pal.color(QPalette::Active, QPalette::Button).darker() );
    }

    setPalette( pal );

#ifdef Q_WS_QWS
    // Window Decoration
    QString dec = config.value( QLatin1String("Decoration"), QLatin1String("Qtopia") ).toString();
    QString decTheme = config.value( QLatin1String("DecorationTheme"), QLatin1String("") ).toString();
    if ( dec != d->decorationName || !decTheme.isEmpty()) {
        qwsSetDecoration( new QtopiaDecoration( dec ) );
        d->decorationName = dec;
        d->decorationTheme = decTheme;
    }
#endif
    config.endGroup();

    // Font
    QTranslatableSettings trconfig(QLatin1String("Trolltech"),QLatin1String("qpe"));
    trconfig.beginGroup( QLatin1String("Font") );
    QString ff = trconfig.value( QLatin1String("FontFamily"), font().family() ).toString();
    double fs = trconfig.value( QLatin1String("FontSize"), font().pointSizeF() ).toDouble();
    qLog(I18n) << "Using font/size/language" << ff << fs << Qtopia::languageList();
    QFont fn(ff);
    fn.setPointSizeF(fs);

    setFont( fn );

    // text display of context menubar
    config.beginGroup( QLatin1String("ContextMenu") );
    ContextKeyManager::instance()->setLabelType( (QSoftMenuBar::LabelType)config.value( QLatin1String("LabelType"), QSoftMenuBar::TextLabel ).toInt() );
}

void QtopiaApplication::systemMessage( const QString &msg, const QByteArray &data)
{
    QDataStream stream( data );
    if ( msg == QLatin1String("applyStyle()") ) {
        applyStyle();
    } else if ( msg == QLatin1String("shutdown()") ) {
        if ( type() == GuiServer )
            shutdown();
    } else if ( msg == QLatin1String("quit()") ) {
        if ( type() != GuiServer && d->appName != QLatin1String("quicklauncher") )
            tryQuit();
    } else if ( msg == QLatin1String("close()") ) {
        if ( type() != GuiServer )
            hideOrQuit();
    } else if ( msg == QLatin1String("forceQuit()") ) {
        if ( type() != GuiServer )
            quit();
    } else if ( msg == QLatin1String("restart()") ) {
        if ( type() == GuiServer )
            restart();
    } else if ( msg == QLatin1String("language(QString)") ) {
        if ( type() == GuiServer ) {
            QString l;
            stream >> l;
            l += QLatin1String(".UTF-8");
            QString cl = getenv("LANG");
            if ( cl != l ) {
                if ( l.isNull() )
                    unsetenv( "LANG" );
                else
                    setenv( "LANG", l.toLatin1(), 1 );
                restart();
            }
        }
    } else if ( msg == QLatin1String("timeChange(QString)") ) {
        QString t;
        stream >> t;
        if ( t.isNull() )
            unsetenv( "TZ" );
        else {
            setenv( "TZ", t.toLatin1(), 1 );
            tzset(); // ensure TZ value is used by subsequent localtime() calls
        }
        // emit the signal so everyone else knows...
        emit timeChanged();
    } else if ( msg == QLatin1String("categoriesChanged()") ) {
        emit categoriesChanged();
    } else if ( msg == QLatin1String("clockChange(bool)") ) {
        QTimeString::updateFormats();
        int tmp;
        stream >> tmp;
        emit clockChanged( tmp );
    } else if ( msg == QLatin1String("weekChange(bool)") ) {
        int tmp;
        stream >> tmp;
        emit weekChanged( tmp );
    } else if ( msg == QLatin1String("setDateFormat()") ) {
        QTimeString::updateFormats();
        d->qpe_system_locale->readSettings();
        emit dateFormatChanged();
    } else if ( msg == QLatin1String("setVolume(int,int)") ) {
        if ( type() == GuiServer ) {
            int t,v;
            stream >> t >> v;
            setVolume(t,v);
        }
        emit volumeChanged( qtopia_muted );
    } else if ( msg == QLatin1String("volumeChange(bool)") ) {
        stream >> qtopia_muted;
        if ( type() == GuiServer ) {
            {
            QSettings cfg(QLatin1String("Trolltech"),QLatin1String("Sound"));
            cfg.beginGroup(QLatin1String("System"));
            cfg.setValue(QLatin1String("Muted"),qtopia_muted);
            }
            setVolume();
        }
        emit volumeChanged( qtopia_muted );
    } else if ( msg == QLatin1String("flush()") ) {
        emit flush();
        // we need to tell the desktop
        QtopiaIpcEnvelope e( QLatin1String("QPE/Desktop"), QLatin1String("flushDone(QString)") );
        e << d->appName;
    } else if ( msg == QLatin1String("reload()") ) {
        // Reload anything stored in files...
        applyStyle();
        if ( type() == GuiServer ) {
            setVolume();
            QtopiaServiceRequest e( "QtopiaPowerManager", "setBacklight(int)" );
            e << -1;
            e.send();
        }
        // App-specifics...
        emit reload();
#ifdef Q_WS_QWS
    } else if ( msg == QLatin1String("getMarkedText()") ) {
        if ( type() == GuiServer ) {
            const ushort unicode = 'C'-'@';
            const int scan = Qt::Key_C;
            qwsServer->processKeyEvent( unicode, scan, Qt::ControlModifier, true, false );
            qwsServer->processKeyEvent( unicode, scan, Qt::ControlModifier, false, false );
        }
#endif
    } else if ( msg == QLatin1String("wordsChanged(QString,int)") ) {
        QString dictname;
        int pid;
        stream >> dictname >> pid;
        if ( pid != getpid() ) {
            Qtopia::qtopiaReloadWords(dictname);
        }
    } else if ( msg == QLatin1String("RecoverMemory()") ) {
        if (qApp->type() != GuiServer) {
            QPixmapCache::clear();
        }
    }
    else if ( msg == QLatin1String("updateContextLabels()") ) {
        QSettings config("Trolltech","qpe");
        config.beginGroup( "ContextMenu" );
        ContextKeyManager::instance()->setLabelType((QSoftMenuBar::LabelType)config.value( "LabelType", QSoftMenuBar::TextLabel).toInt());
        if (activeWindow() && focusWidget()) {
            ContextKeyManager::instance()->updateContextLabels();
        }
    }
    else if ( msg == QLatin1String("resetContent()") ) {
        emit resetContent();
    }
}

/*!
  \internal
*/
bool QtopiaApplication::raiseAppropriateWindow()
{
    bool r=false;

    // 1. Raise the main widget
    QWidget *top = d->qpe_main_widget;

    // XXX now can be multiple top level widgets.
    // currently only deal with one (first)
    if ( !top && topLevelWidgets().count() > 0)
        top = topLevelWidgets()[0];

    if ( top ) {
        if ( top->isVisible() )
            r = true;
        else if (d->preloaded) {
            // We are preloaded and not visible.. pretend we just started..
            QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("fastAppShowing(QString)"));
            e << d->appName;
        }

        QDialog *dialog = qobject_cast<QDialog*>(top);
        if ( !top->isVisible() ) {
            if (dialog && top != d->qpe_main_widget) {
                d->qpe_show_dialog(dialog,d->nomaximize);
            } else {
                if ( !d->nomaximize )
                    top->showMaximized();
                else
                    top->show();
            }
        }
        top->raise();
        top->activateWindow();
    }

    QWidget *topm = activeModalWidget();

    // 2. Raise any parentless widgets (except top and topm, as they
    //     are raised before and after this loop).  Order from most
    //     recently raised as deepest to least recently as top, so
    //     that repeated calls cycle through widgets.
    QWidgetList list = topLevelWidgets();
    bool foundlast = false;
    QWidget* topsub = 0;
    if ( d->lastraised ) {
        foreach (QWidget* w, list) {
            if ( !w->parentWidget() && w != top && w != topm && w->isVisible() && w->windowType() != Qt::Desktop ) {
                if ( w == d->lastraised )
                    foundlast = true;
                if ( foundlast ) {
                    w->raise();
                    w->activateWindow();
                    topsub = w;
                }
            }
        }
    }
    foreach (QWidget* w, list) {
        if ( !w->parentWidget() && w != top && w != topm && w->isVisible() && w->windowType() != Qt::Desktop) {
            if ( w == d->lastraised )
                break;
            w->raise();
            w->activateWindow();
            topsub = w;
        }
    }
    d->lastraised = topsub;

    // 3. Raise the active modal widget.
    if ( topm && topm != top ) {
        topm->show();
        topm->raise();
        topm->activateWindow();
        // If we haven't already handled the fastAppShowing message
        if (!top && d->preloaded) {
            QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("fastAppShowing(QString)"));
            e << d->appName;
        }
        r = false;
    }

    return r;
}

void QtopiaApplication::pidMessage( const QString &msg, const QByteArray & data)
{
    if ( msg == QLatin1String("quit()") ) {
        tryQuit();
    } else if ( msg == QLatin1String("quitIfInvisible()") ) {
        if ( d->qpe_main_widget && !d->qpe_main_widget->isVisible() )
            quit();
    } else if ( msg == QLatin1String("close()") ) {
        hideOrQuit();
    } else if ( msg == QLatin1String("disablePreload()") ) {
        d->preloaded = false;
        if(type() != GuiServer)
            d->lifeCycle->unregisterRunningTask("QtopiaPreload");
        /* so that quit will quit */
    } else if ( msg == QLatin1String("enablePreload()") ) {
        if (d->qpe_main_widget) {
            if(type() != GuiServer)
                d->lifeCycle->registerRunningTask("QtopiaPreload", 0);
            d->preloaded = true;
        }
        /* so next quit won't quit */
    } else if ( msg == QLatin1String("raise()") ) {
        d->notbusysent = false;
        raiseAppropriateWindow();
        // Tell the system we're still chugging along...
        QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("appRaised(QString)"));
        e << d->appName;
    } else if ( msg == QLatin1String("flush()") ) {
        emit flush();
        // we need to tell the desktop
        QtopiaIpcEnvelope e( QLatin1String("QPE/Desktop"), QLatin1String("flushDone(QString)") );
        e << d->appName;
    } else if ( msg == QLatin1String("reload()") ) {
        emit reload();
    } else if ( msg == QLatin1String("setDocument(QString)") ) {
        QDataStream stream( data );
        QString doc;
        stream >> doc;
        QWidget *mw;

        mw = d->qpe_main_widget;
        // can be multiple top level widgets
        if( !mw && topLevelWidgets().count() > 0 ) {
            mw = topLevelWidgets()[0];
        }

        if ( mw ) {
            QMetaObject::invokeMethod(mw, "setDocument", Q_ARG(QString, doc));
        }
        raiseAppropriateWindow();
        // Tell the system we're still chugging along...
        QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("appRaised(QString)"));
        e << d->appName;
    } else if ( msg == QLatin1String("TimeMonitor::timeChange(QString)") ) {
        QDataStream stream( data );
        QString t;
        stream >> t;
        if ( t.isNull() )
            unsetenv( "TZ" );
        else {
            setenv( "TZ", t.toLatin1(), 1 );
            tzset(); // ensure TZ value is used by subsequent localtime() calls
        }
        // emit the signal so everyone else knows...
        emit timeChanged();
    } else {
        emit appMessage( msg, data);
    }
}

#ifndef QTOPIA_DBUS_IPC
// Handle messages on "QPE/pid/<mypid>".  The only message on
// this channel should be "QPEProcessQCop()", which the server uses
// to prod the application to read its qcop-msg file.
void QtopiaApplication::dotpidMessage( const QString &msg, const QByteArray & )
{
    if ( msg == QLatin1String("QPEProcessQCop()")) {
        processQCopFile();
        d->sendQCopQ();
    }
}
#endif

/*!
    Returns the main widget of this application.
*/
QWidget* QtopiaApplication::mainWidget() const
{
    return d->qpe_main_widget;
}

/*!
    Set the main widget for this application to \a widget.  If \a noMaximize is
    true, then the widget will not be maximized.
*/
void QtopiaApplication::setMainWidget(QWidget *widget, bool noMaximize)
{
    Q_ASSERT(widget->isTopLevel());
    d->qpe_main_widget = widget;
    d->nomaximize = noMaximize;
}

#ifdef Q_WS_X11

// Mark Qtopia windows so that the soft menu bar can detect
// non-Qtopia applications and handle the back button correctly.
static void markQtopiaWindow(QWidget *w)
{
    Display *dpy = QX11Info::display();
    Window wId = (w ? w->winId() : 0);
    if (dpy && wId) {
        Atom atom = XInternAtom(dpy, "_QTOPIA_SOFT_MENUS", False);
        unsigned long flag = 1;
        XChangeProperty(dpy, wId, atom, XA_CARDINAL, 32, PropModeReplace,
                        (unsigned char *)&flag, 1);
    }
}

#else

static void markQtopiaWindow(QWidget *)
{
    // Nothing to do here for other platforms.
}

#endif

/*!
    Show the main widget for this application.

    \sa showMainDocumentWidget()
*/
void QtopiaApplication::showMainWidget()
{
    if(!d->qpe_main_widget)
        return;

    markQtopiaWindow(d->qpe_main_widget);

    if(d->noshow)
        d->noshow = false;
    else
        d->show(d->qpe_main_widget, d->nomaximize);
}

/*!
    Show the main widget for this application and ask it to load
    the document specified on the command-line.

    This call assumes that the application is
    a \l {Qtopia - Main Document Widget}{document-oriented} application.
    The main widget must implement a \c{setDocument(const QString &filename)}
    slot in order to receive the filename of the document to load.

    \sa showMainWidget()
*/
void QtopiaApplication::showMainDocumentWidget()
{
    QWidget *mw = mainWidget();
    if(mw && argc() == 2)
        QMetaObject::invokeMethod(mw, "setDocument",
                                  Q_ARG(QString, arguments().at(1)));

    showMainWidget();
}

/*!
    Sets \a widget as the mainWidget() and shows it.

    If \a noMaximize is true then the main widget may not be maximized.  This
    is usually unnecessary and it is recommended that the system is allowed to
    decide how the widget is displayed by passing false for \a noMaximize.

    \sa showMainDocumentWidget()
*/
void QtopiaApplication::showMainWidget( QWidget* widget, bool noMaximize )
{
    setMainWidget(widget, noMaximize);
    showMainWidget();
}

/*!
    Sets widget \a widget as the mainWidget() and shows it.

    If \a noMaximize is true then the main widget may not be maximized.  This
    is usually unnecessary and it is recommended that the system is allowed to
    decide how the widget is displayed by passing false for \a noMaximize.

    This call assumes that the application is
    a \l {Qtopia - Main Document Widget}{document-oriented} application.
    The main widget must implement a \c{setDocument(const QString &filename)}
    slot in order to receive the filename of the document to load.

    \sa showMainWidget()
*/
void QtopiaApplication::showMainDocumentWidget( QWidget* widget, bool noMaximize )
{
    setMainWidget(widget, noMaximize);
    showMainDocumentWidget();
}

/*!
  \internal
*/
QtopiaStyle *QtopiaApplication::internalSetStyle( const QString &styleName )
{
    if (styleName != QLatin1String("themedstyle")){
        if ( styleName == d->styleName)
            return qobject_cast<QtopiaStyle*>(style());
    }else{
        QSettings config(QLatin1String("Trolltech"),QLatin1String("qpe"));
        config.beginGroup( QLatin1String("Appearance") );
        // For the Pixmap style we must remove the existing style here for theme changes to take effect
        setStyle( QLatin1String("windows") );
    }

    QtopiaStyle *newStyle = 0;

    if ( styleName == QLatin1String("QPE")  || styleName == QLatin1String("Qtopia") ) {
#ifdef EZX_A780
        newStyle = new EzXPhoneStyle;
#else
        newStyle = new QPhoneStyle;
#endif
    } else if (styleName == QLatin1String("QThumbStyle")) {
        newStyle = new QThumbStyle;
    } else {
        newStyle = qobject_cast<QtopiaStyle*>(QStyleFactory::create(styleName));
    }

    if ( !newStyle ) {
#ifdef EZX_A780
        newStyle = new EzXPhoneStyle;
#else
        //newStyle = new QPhoneStyle;
#endif
        d->styleName = QLatin1String("QPE");
    } else {
        d->styleName = styleName;
    }

    if (qApp->style() != newStyle)
        setStyle( newStyle );

    return newStyle;
}

/*!
  \internal
*/
void QtopiaApplication::shutdown()
{
    // Implement in server's QtopiaApplication subclass
}

/*!
  \internal
*/
void QtopiaApplication::restart()
{
    // Implement in server's QtopiaApplication subclass
}

static QMap<QWidget*,QtopiaApplication::StylusMode>* stylusDict=0;
static void createDict()
{
    if ( !stylusDict )
        stylusDict = new QMap<QWidget*,QtopiaApplication::StylusMode>;
}

/*!
  Returns the current StylusMode for widget \a w.

  \sa setStylusOperation(), StylusMode
*/
QtopiaApplication::StylusMode QtopiaApplication::stylusOperation( QWidget* w )
{
    if (stylusDict && stylusDict->contains(w))
        return stylusDict->value(w);
    return LeftOnly;
}

/*!
    \enum QtopiaApplication::StylusMode

    \value LeftOnly the stylus only generates LeftButton
                        events (the default).
    \value RightOnHold the stylus generates RightButton events
                        if the user uses the press-and-hold gesture.

    \sa setStylusOperation(), stylusOperation()
*/

/*!
  Allows \a widget to receive mouse events according to the stylus
  \a mode.

  Setting the stylus mode to RightOnHold causes right mouse button events
  to be generated when the stylus is held pressed for 500ms.
  The default mode is LeftOnly.

  \sa stylusOperation(), StylusMode
*/
void QtopiaApplication::setStylusOperation( QWidget* widget, StylusMode mode )
{
    createDict();
    if ( mode == LeftOnly ) {
        stylusDict->remove(widget);
    } else {
        stylusDict->insert(widget,mode);
        connect(widget,SIGNAL(destroyed()),qApp,SLOT(removeSenderFromStylusDict()));
    }
}

void QtopiaApplication::removeSenderFromIMDict()
{
    delete inputMethodDict->take( qobject_cast<QWidget*>( sender() ));
}

class WeekStartsOnMondayUpdater : public QObject {
    Q_OBJECT
public:
    WeekStartsOnMondayUpdater(QCalendarWidget* cw) : QObject(cw)
    {
        updateCalendarWidget(Qtopia::weekStartsOnMonday());
        connect(qApp,SIGNAL(weekChanged(bool)),this,SLOT(updateCalendarWidget(bool)));
    }
private slots:
    void updateCalendarWidget(bool);
};

void WeekStartsOnMondayUpdater::updateCalendarWidget(bool monday)
{
    QCalendarWidget *cw = qobject_cast<QCalendarWidget*>(parent());
    if ( cw )
        cw->setFirstDayOfWeek(monday ? Qt::Monday : Qt::Sunday);
}

/*!
  \reimp
*/
bool QtopiaApplication::eventFilter( QObject *o, QEvent *e )
{
    if ( !o->isWidgetType() )
        return false;

#ifdef QT_NO_QWS_CURSOR
    if ( e->type() == QEvent::ToolTip )
        // if we have no cursor, probably don't want tooltips
        return true;
#endif

    if ( stylusDict && e->type() >= QEvent::MouseButtonPress && e->type() <= QEvent::MouseMove ) {
        QMouseEvent* me = (QMouseEvent*)e;
        StylusMode mode = stylusOperation(qobject_cast<QWidget*>(o));
        switch (mode) {
          case RightOnHold:
            switch ( me->type() ) {
              case QEvent::MouseButtonPress:
                if (me->button() == Qt::LeftButton) {
                    if (!d->pressHandler)
                        d->pressHandler = new PressTickWidget();
                    d->pressHandler->startPress((QWidget*)o, me->pos());
                }
                break;
              case QEvent::MouseMove:
                if (d->pressHandler && d->pressHandler->active()
                    && (me->pos()-d->pressHandler->pos()).manhattanLength() > 8) {
                    d->pressHandler->cancelPress();
                    delete d->pressHandler;
                    d->pressHandler = 0;
                }
                break;
              case QEvent::MouseButtonRelease:
                if (d->pressHandler && d->pressHandler->active()
                    && me->button() == Qt::LeftButton) {
                    int rv = d->pressHandler->endPress(qobject_cast<QWidget*>(o), me->pos());
                    delete d->pressHandler;
                    d->pressHandler = 0;
                    return rv;
                }
                break;
              default:
                break;
            }
            break;
          default:
            if (me->type() == QEvent::MouseButtonRelease
                && d->pressHandler && d->pressHandler->active()
                && me->button() == Qt::LeftButton) {
                int rv = d->pressHandler->endPress(qobject_cast<QWidget*>(o), me->pos());
                delete d->pressHandler;
                d->pressHandler = 0;
                return rv;
            }
        }
    } else if ( e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease ) {
        QKeyEvent *ke = (QKeyEvent *)e;
        if ( ke->key() == Qt::Key_Enter )
        {
            if ( o->inherits( "QRadioButton" ) || o->inherits( "QCheckBox" ) )
            {
                postEvent( o, new QKeyEvent( e->type(), Qt::Key_Space,
                    ke->modifiers(), " ", ke->isAutoRepeat(), ke->count() ) );
                return true;
            }
        } else if ( ke->key() == Qt::Key_Hangup && e->type() == QEvent::KeyPress ) {
            /* XXX QComboBox does not ignore key events that it does not handle (qt 4.2.2) */
            if ( qobject_cast<QComboBox*>(o) != NULL ) {
                e->ignore();
                return true;
            }
            /* Ignore the hangup key events so the server can handle. */
            if ( qobject_cast<QDialog*>(o) != NULL ) {
                e->ignore();
                return true;
            }
        }
        else if ( !mousePreferred && ((QWidget*)o)->hasEditFocus()
            && (ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right
                || ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down)) {
            QtopiaApplicationData::updateContext((QWidget*)o);
        } // end else if cursor dir.
        else if (ke->key() == QSoftMenuBar::menuKey()
                && ke->type() == QEvent::KeyPress) {
            QWidget *w = (QWidget *)o;
            qLog(UI) << "Menu key for:" << w;
            QWidget *cw = ContextKeyManager::instance()->findTargetWidget(w,
                        ke->key(), mousePreferred || w->hasEditFocus());
            if (cw && cw != w)
            {
                sendEvent(cw,e);
                return true;
            }
        } // end elsif menu key
    } // end else if key event.
    else if ( e->type() == QEvent::FocusIn
              || (!mousePreferred && (e->type() == QEvent::EnterEditFocus ||
              e->type() == QEvent::LeaveEditFocus)) ) {

        QWidget *w = (QWidget *)o;

        d->focusOutTimer.stop();
        d->focusOutWidget = 0;

        if (e->type() == QEvent::FocusIn && !mousePreferred
                && w->focusPolicy() != Qt::NoFocus) {
            if (isSingleFocusWidget(w))
                w->setEditFocus(true);
        }

        if (!activeWindow())
            return false;

        if (e->type() != QEvent::FocusIn || ((QFocusEvent*)e)->reason() != Qt::PopupFocusReason)
            sendInputHintFor(w, e->type());
        if (qApp->focusWidget() == w && qApp->activeWindow())
            ContextKeyManager::instance()->updateContextLabels();
        if (w->inherits("QDateEdit")) {
            if(!QSoftMenuBar::hasMenu(w, QSoftMenuBar::AnyFocus)) {
                if (!d->calendarMenu)
                    d->calendarMenu = new CalendarMenu();
                /* some how needs to hook up to original widget when select happens? */
                QSoftMenuBar::addMenuTo(w, d->calendarMenu, QSoftMenuBar::AnyFocus);
            }
            d->calendarMenu->setTargetWidget(qobject_cast<QDateEdit*>(w));
        }
#ifndef QT_NO_CLIPBOARD
        if ((mousePreferred || w->hasEditFocus()) &&
                (w->inherits("QLineEdit")
                 || (w->inherits("QTextEdit") && !w->inherits("QTextBrowser")))) {
            if(!QSoftMenuBar::hasMenu(w, QSoftMenuBar::EditFocus)) {
                if (!d->editMenu)
                    d->editMenu = QSoftMenuBar::createEditMenu();
                QSoftMenuBar::addMenuTo(w, d->editMenu, QSoftMenuBar::EditFocus);
            }
        }
#endif

        QAbstractButton *b = qobject_cast<QAbstractButton*>(w);
        if ( b && b->isCheckable() && !mousePreferred ) {
            connect(b, SIGNAL(toggled(bool)),
                qApp, SLOT(buttonChange(bool)));
        }

        QTextBrowser *tb = qobject_cast<QTextBrowser*>(w);
        if ( tb && !mousePreferred ) {
            connect(tb, SIGNAL(highlighted(QString)),
                    qApp, SLOT(textBrowserHighlightChange(QString)));
        }
    }
    else if ( e->type() == QEvent::FocusOut ) {
        QWidget *w = (QWidget *)o;
        QFocusEvent *fe = (QFocusEvent*)e;
        if ((fe->reason() == Qt::PopupFocusReason)
                || (!focusWidget() && w->topLevelWidget()->isVisible() && activeWindow())) {
            ContextKeyManager::instance()->updateContextLabels();
        }
#ifndef QT_NO_CLIPBOARD
        if (d->editMenu && !d->editMenu->isActiveWindow() && (w->inherits("QLineEdit")
                || (w->inherits("QTextEdit") && !w->inherits("QTextBrowser")))) {
            QSoftMenuBar::removeMenuFrom(w, d->editMenu);
            if ( d->editMenu ) {
                d->editMenu->deleteLater();
                d->editMenu= 0;
            }
        }
#endif

        QAbstractButton *b = qobject_cast<QAbstractButton*>(w);
        if ( b && b->isCheckable() && !mousePreferred ) {
            disconnect(b, SIGNAL(toggled(bool)),
                qApp, SLOT(buttonChange(bool)));
        }
        QTextBrowser *tb = qobject_cast<QTextBrowser*>(w);
        if ( tb && !mousePreferred ) {
            disconnect(tb, SIGNAL(highlighted(QString)),
                    qApp, SLOT(textBrowserHighlightChange(QString)));
        }

        if (fe->reason() != Qt::PopupFocusReason) {
            // If we don't have a FocusIn on its way then we need
            // to deal with the FocusOut.
            d->focusOutWidget = w;
            d->focusOutTimer.start(0, this);
        }
    } else if (e->type() == QEvent::Show) {
#if defined(GREENPHONE_EFFECTS) && defined(QT_QWS_GREENPHONE)
        QMenu *menu = qobject_cast<QMenu*>(o);
        if(menu && !d->greenphoneEffects()->inProgress()) {
            d->greenphoneEffects()->menu(menu);
        } else {
            QTabWidget *tab = qobject_cast<QTabWidget *>(o);
            if(tab) 
                d->greenphoneEffects()->addTab(tab);
        }
#endif
        QMessageBox *mb = 0;
        if (((QWidget*)o)->testAttribute(Qt::WA_ShowModal)) {
            mb = qobject_cast<QMessageBox*>(o);
            if (mb) {
                // Avoid QDialog::showEvent() moving messagebox.
                // We'll do it soon anyway.
                mb->setAttribute(Qt::WA_Moved, true);
            }
#ifdef QTOPIA_USE_TEST_SLAVE
            if (testSlave()) {
                if (mb) {
                    testSlave()->showMessageBox(mb, mb->windowTitle(), mb->text());
                } else {
                    QDialog *dlg = qobject_cast<QDialog*>(o);
                    if (dlg)
                        testSlave()->showDialog(dlg, dlg->windowTitle());
                }
            }
#endif
        }
        QDialog *dlg = qobject_cast<QDialog*>(o);
        if (!mb && dlg && !Qtopia::hasKey(Qt::Key_No)) { // no context menu for QMessageBox
            if (!isMenuLike(dlg)) {
                if ( (dlg->windowFlags()&Qt::WindowSystemMenuHint) ) {
                    bool foundco = false;
                    QList<QObject*> childObjects = o->children();
                    if (childObjects.count()) {
                        foreach(QObject *co, childObjects) {
                            if (co->isWidgetType() && co->metaObject()->className() == QString("QMenu")) {
                                foundco = true;
                                break;
                            }
                        }
                    }
                    if (!foundco) {
                        // There is no context menu defined
                        if (!ContextKeyManager::instance()->haveLabelForWidget(dlg, QSoftMenuBar::menuKey(), QSoftMenuBar::AnyFocus))
                            (void)QSoftMenuBar::menuFor((QWidget*)o);
                    }
                }
                if (!ContextKeyManager::instance()->haveLabelForWidget(dlg, Qt::Key_Back, QSoftMenuBar::AnyFocus))
                    QSoftMenuBar::setLabel(dlg, Qt::Key_Back, QSoftMenuBar::Back);
            } else {
                QSoftMenuBar::setLabel(dlg, Qt::Key_Back, QSoftMenuBar::Cancel);
            }
        }
        QWidget *w = qobject_cast<QWidget *>(o);
#ifdef POPUP_SHADOWS
        if (w && w->isWindow()
            && style()->styleHint((QStyle::StyleHint)QPhoneStyle::SH_PopupShadows)
            && (w->windowFlags() & Qt::WindowType_Mask) == Qt::Popup) {
            ShadowWidget *shadow = 0;
            if (d->shadowMap.contains(w)) {
                shadow = d->shadowMap[w];
            } else {
                QDesktopWidget *desktop = QApplication::desktop();
                QRect desktopRect(desktop->screenGeometry(desktop->primaryScreen()));
                shadow = new ShadowWidget(desktopRect.width() < 240 ? 3 : 5);
                d->shadowMap.insert(w, shadow);
                shadow->setTarget(w);
            }
            shadow->show();
        }
#endif
#ifdef QTOPIA_ENABLE_FADE_IN_WINDOW
        if (w && w->isWindow()) {
            if (d->fadeInWidget && d->fadeInOpacity < 1.0)
                d->fadeInWidget->setWindowOpacity(1.0);
            w->setWindowOpacity(0.25);
            d->fadeInOpacity = 0.25;
            d->fadeInWidget = w;
            d->fadeInTimer.start(100, this);
        }
#endif
        if (w && w->inherits("QCalendarPopup")) {
            w->showMaximized();
        }
#if defined(POPUP_SHADOWS) || defined(GREENPHONE_EFFECTS)
    } else if (e->type() == QEvent::Hide) {
#if defined(GREENPHONE_EFFECTS) && defined(QT_QWS_GREENPHONE)
        QTabWidget *tab = qobject_cast<QTabWidget *>(o);
        if(tab) 
            d->greenphoneEffects()->removeTab(tab);
#endif
#ifdef POPUP_SHADOWS
        QWidget *w = qobject_cast<QWidget *>(o);
        if (w && w->isWindow() && (w->windowFlags() & Qt::WindowType_Mask) == Qt::Popup) {
            if (d->shadowMap.contains(w)) {
                ShadowWidget *shadow = d->shadowMap[w];
                d->shadowMap.remove(w);
                shadow->hide();
                shadow->deleteLater();
            }
        }
#endif
#endif
    } else if (e->type() == QEvent::WindowDeactivate ) {

        // ensure popup widgets (menus, calendar widget, combobox lists, etc.)
        // hide when switching to home screen, running apps switcher etc.
        QWidget *w = qApp->activePopupWidget();
        if (w)
            w->hide();

    } else if (e->type() == QEvent::LayoutRequest && qobject_cast<QMessageBox*>(o)) {
        return true;    //stop QMessageBox from resizing our messageboxes (we have already taken care of it)
    }

    return false;
}

void QtopiaApplication::hideMessageBoxButtons( QMessageBox *mb )
{
    QList<QPushButton*> pbList = mb->findChildren<QPushButton*>();
    if (pbList.count() == 1 || pbList.count() == 2) {
        // We only handle MBs with 1 or 2 buttons.
        // Currently we assume that the affirmative is pb1 and
        // the negative is pb2.
        QPushButton *pb1 = pbList[0];
        QPushButton *pb2 = pbList.count() == 2 ? pbList[1] : 0;

        // First hide buttons
        QDialogButtonBox *bbox = mb->findChild<QDialogButtonBox*>();
        if (bbox)
            bbox->hide();

        // Use a simpler layout.
        QLabel *icon = mb->findChild<QLabel*>(QLatin1String("qt_msgboxex_icon_label"));
        QLabel *label = mb->findChild<QLabel*>(QLatin1String("qt_msgbox_label"));
        if (icon && label && mb->layout()) {
            delete mb->layout();
            QHBoxLayout *hb = new QHBoxLayout;
            hb->setSizeConstraint(QLayout::SetNoConstraint);
            hb->setMargin(style()->pixelMetric(QStyle::PM_DefaultChildMargin));
            hb->addWidget(icon);
            hb->addWidget(label);
            mb->setLayout(hb);
        }

        // let us decide the size of the dialog.
        mb->setMaximumSize(desktop()->availableGeometry().size());

        // Setup accels for buttons and remove focus from buttons.
        pb1->setFocusPolicy(Qt::NoFocus);
        pb1->setDefault(false);
        int accel = 0;
        if (pbList.count() == 1) {
            if (Qtopia::hasKey(Qt::Key_Back))
                accel = Qt::Key_Back;
            else if (Qtopia::hasKey(Qt::Key_Yes))
                accel = Qt::Key_Yes;
            pb1->setShortcut(accel);
            QString txt = pb1->text();
            txt = txt.replace("&", "");
            QSoftMenuBar::setLabel(mb, accel, QString(), txt);
        } else if (pbList.count() == 2 ) {
            if (Qtopia::hasKey(Qt::Key_Yes)) {
                accel = Qt::Key_Yes;
            } else {
                const QList<int> &cbtns = QSoftMenuBar::keys();
                if (cbtns.count()) {
                    if (cbtns[0] != Qt::Key_Back)
                        accel = cbtns[0];
                    else if (cbtns.count() > 1)
                        accel = cbtns[cbtns.count()-1];
                }
            }
            if (accel) {
                pb1->setShortcut(accel);
                QString txt = pb1->text();
                txt = txt.replace("&", "");
                QSoftMenuBar::setLabel(mb, accel, QString(), txt);
            }
            pb2->setFocusPolicy(Qt::NoFocus);
            pb2->setDefault(false);
            accel = Qtopia::hasKey(Qt::Key_No) ? Qt::Key_No : Qt::Key_Back;
            pb2->setShortcut(accel);
            QString txt = pb2->text();
            txt = txt.replace("&", "");
            QSoftMenuBar::setLabel(mb, accel, QString(), txt);
        }

        // show the dialog using our standard dialog geometry calcuation
        showDialog(mb);
        if (mb->layout())
            mb->layout()->activate();
        mb->setFocus();
    }
}

void QtopiaApplication::sendInputHintFor(QWidget *w, QEvent::Type etype)
{
    qint32 windowId = 0;
    if (w) {
        // popups don't send window active events... so need
        // to find the window id for the actual active window,
        // rather than that of the popup.
        QWidget *p = w->topLevelWidget();;
        while(p->windowType() == Qt::Popup && p->parentWidget())
            p = p->parentWidget();
        windowId = p->topLevelWidget()->winId();
    }

    if (etype == QEvent::FocusOut) {
        QtopiaIpcEnvelope env(QLatin1String("QPE/InputMethod"), QLatin1String("inputMethodHint(QString,int)") );
        env << QString();
        env << windowId;
        return;
    }

    const QValidator* v = 0;
    InputMethodHintRec *hr=0;
    bool passwordFlag = false ;
    if ( inputMethodDict ) {
        if (!inputMethodDict->contains(w)) {
            QWidget* p = w->parentWidget();
            if ( p ) {
                if ( p->focusProxy() == w ) {
                    if (inputMethodDict->contains(p))
                        hr = inputMethodDict->value(p);
                } else {
                    p = p->parentWidget();
                    if (p && p->focusProxy() == w && inputMethodDict->contains(p))
                        hr = inputMethodDict->value(p);
                }
            }
        } else {
            hr = inputMethodDict->value(w);
        }
    }
    int n = hr ? (int)hr->hint : 0;
    QLineEdit *l = qobject_cast<QLineEdit*>(w);
    if (!l && w->inherits("QSpinBox")) {
        l = ((QSpinBoxLineEditAccessor*)w)->getLineEdit();
        if (!hr)
            n = (int)Number;
    }
    if (!l && w->inherits("QComboBox"))
        l = ((QComboBox*)w)->lineEdit();
    if (l) {
        if ( !n && !l->isReadOnly()) {
            if( l->echoMode()==QLineEdit::Normal){
                n = (int)Words;
            } else {
                n = (int)Text;
                passwordFlag = true;
            };
            v = l->validator();
        }
        if( n && !mousePreferred ) {
            if (etype == QEvent::EnterEditFocus) {
                connect(l, SIGNAL(textChanged(QString)),
                        qApp, SLOT(lineEditTextChange(QString)));
            } else if (etype == QEvent::LeaveEditFocus) {
                disconnect(l, SIGNAL(textChanged(QString)),
                        qApp, SLOT(lineEditTextChange(QString)));
            }
        }
    } else if (w->inherits("QTextEdit") && !w->inherits("QTextBrowser")) {
        QTextEdit* l = (QTextEdit*)w;
        if ( !n && !l->isReadOnly()) {
            n = (int)Words;
        }
        if( n && !mousePreferred ) {
            if (etype == QEvent::EnterEditFocus) {
                connect(l, SIGNAL(textChanged()),
                        qApp, SLOT(multiLineEditTextChange()));
            } else if (etype == QEvent::LeaveEditFocus) {
                disconnect(l, SIGNAL(textChanged()),
                        qApp, SLOT(multiLineEditTextChange()));
            }
        }
    }
    if ( !hr && v && v->inherits("QIntValidator") )
        n = (int)Number;

    // find ancestor.. top ancestor, then get its' window id
    if (etype == QEvent::FocusIn || etype == QEvent::None) {
        if ( n == Named ) {
            QtopiaIpcEnvelope env(QLatin1String("QPE/InputMethod"), QLatin1String("inputMethodHint(QString,int)") );
            env << (hr ? hr->param : QString());
            env << windowId;
        } else {
            QtopiaIpcEnvelope env(QLatin1String("QPE/InputMethod"), QLatin1String("inputMethodHint(int,int)") );
            env << n;
            env << windowId;
        }
        QtopiaIpcEnvelope passwordenv(QLatin1String("QPE/InputMethod"), QLatin1String("inputMethodPasswordHint(bool,int)") );
        passwordenv << passwordFlag;
        passwordenv << windowId;


    }
}

/*!
  \internal
*/
bool QtopiaApplication::notify(QObject* o, QEvent* e)
{
    QEvent *remapped = 0;
    if (d && d->remapKeys &&
        (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease)) {
        QKeyEvent *ke = (QKeyEvent*)e;
        if (d->keyRemapper.contains(ke->key())) {
            remapped = new QKeyEvent(ke->type(), d->keyRemapper[ke->key()],
                                     ke->modifiers(), ke->text(),
                                     ke->isAutoRepeat(), ke->count());
            e = remapped;
        }
    }
    bool r = QApplication::notify(o,e);
    if ((e->type() == QEvent::Show || e->type() == QEvent::Resize)
        && o->isWidgetType() && ((QWidget*)o)->testAttribute(Qt::WA_ShowModal)) {
        QMessageBox *mb = qobject_cast<QMessageBox*>(o);
        if (mb) {
            if (e->type() == QEvent::Show)
                hideMessageBoxButtons( mb );
            else if (mb->isVisible())
                showDialog(mb);
        }
    }
    if ( e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease ) {
        QKeyEvent *ke = (QKeyEvent*)e;
        QWidget *w = qobject_cast<QWidget*>(o);
        QMessageBox *mb = qobject_cast<QMessageBox *>(o);

        if (!ke->isAccepted())
        {
            if ( ke->key() == Qt::Key_Hangup || ke->key() == Qt::Key_Call || ke->key() == Qt::Key_Flip )
            {
                // Send some unaccepted keys back to the server for processing.
                QtopiaIpcEnvelope e(QLatin1String("QPE/System"),QLatin1String("serverKey(int,int)"));
                e << ke->key() << int(ke->type() == QEvent::KeyPress);
            }
        }

        if (!ke->isAccepted()
                || (w && !w->hasEditFocus() && isSingleFocusWidget(w)
                    && !Qtopia::mousePreferred())) {

            // We already have special handling for Message Boxes.  Don't do so here
            if (mb || o->inherits("QAbstractMessageBox")) {
                r = true;
            } else if (w && ke->key() == Qt::Key_Back && e->type() == QEvent::KeyPress) {
                w = w->window();
                qLog(UI) << "Handling Back for" << w;
                if (QDialog *dlg = qobject_cast<QDialog*>(w)) {
                    if (isMenuLike(dlg)) {
                        qLog(UI) << "Reject dialog" << w;
                        dlg->reject();
                    } else {
                        qLog(UI) << "Accept dialog" << w;
                        dlg->accept();
                    }
                } else {
                    qLog(UI) << "Closing" << w;
                    w->close();
                }
                r = true;
            }
        }
    } 
#ifndef QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING
    else if (e->type() == QEvent::InputMethod) {
        // Try to keep the back button sensible when IM is composing.
        bool setBack = false;
        QSoftMenuBar::StandardLabel label = QSoftMenuBar::NoLabel;
        if (QLineEdit *le = qobject_cast<QLineEdit*>(o)) {
            if (!ContextKeyManager::instance()->haveLabelForWidget(le, Qt::Key_Back, le->hasEditFocus())) {
                setBack = true;
                QInputMethodEvent *ime = (QInputMethodEvent*)e;
                if (mousePreferred) {
                    label = QSoftMenuBar::Back;
                } else {
                    if (ime->commitString().length() + ime->preeditString().length() > 0
                            || le->cursorPosition() > 0)
                        label = QSoftMenuBar::BackSpace;
                    else if (le->text().length() == 0)
                        label = QSoftMenuBar::RevertEdit;
                }
            }
        } else if (QTextEdit *te = qobject_cast<QTextEdit*>(o)) {
            if (!ContextKeyManager::instance()->haveLabelForWidget(te, Qt::Key_Back, te->hasEditFocus())) {
                setBack = true;
                QInputMethodEvent *ime = (QInputMethodEvent*)e;
                if (mousePreferred) {
                    label = QSoftMenuBar::Back;
                } else {
                    if (ime->commitString().length() + ime->preeditString().length() > 0
                            || te->textCursor().position() > 0)
                        label = QSoftMenuBar::BackSpace;
                    else if (te->document()->isEmpty())
                        label = QSoftMenuBar::Cancel;
                }
            }
        }
        if (setBack && !ContextKeyManager::instance()->findHelper(qobject_cast<QWidget*>(o))) {
            ContextKeyManager::instance()->setStandard(qobject_cast<QWidget*>(o), Qt::Key_Back, label);
        }
    }
#endif // QTOPIA_TURN_OFF_OLD_SOFTKEY_HANDLING

    /* Work around a QComboBox bug and Key_Hangup */
    if (e->type() == QEvent::ChildAdded) {
        QComboBox *w = qobject_cast<QComboBox*>(o);
        if ( w ) {
            w->installEventFilter(this);
        }
    } else if (e->type() == QEvent::PolishRequest) {
        QCalendarWidget* cw = qobject_cast<QCalendarWidget*>(o);
        if ( cw )
            new WeekStartsOnMondayUpdater(cw);
    }

    if (remapped)
        delete remapped;

    return r;
}


/*!
  \reimp
*/
void QtopiaApplication::timerEvent( QTimerEvent *e )
{
    if (e->timerId() == d->focusOutTimer.timerId()) {
        d->focusOutTimer.stop();
        sendInputHintFor(d->focusOutWidget, QEvent::FocusOut);
    }
#ifdef QTOPIA_ENABLE_FADE_IN_WINDOW
    else if (e->timerId() == d->fadeInTimer.timerId()) {
        if (d->fadeInWidget) {
            d->fadeInOpacity += 0.25;
            if (d->fadeInOpacity >= 1.0)
                d->fadeInTimer.stop();
            d->fadeInWidget->setWindowOpacity(d->fadeInOpacity);
        } else {
            d->fadeInTimer.stop();
        }
    }
#endif
}

void QtopiaApplication::removeSenderFromStylusDict()
{
    stylusDict->remove( qobject_cast<QWidget*>( sender() ));
    if (d->pressHandler && d->pressHandler->widget() == sender())
        d->pressHandler->cancelPress();
}

/*!
    Enters the main event loop and waits until exit() is called.

    \sa QApplication::exec()
*/
int QtopiaApplication::exec()
{
#ifndef QTOPIA_DBUS_IPC
    d->qcopQok = true;
    d->sendQCopQ();
#endif

    return QApplication::exec();

    {
        QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("closing(QString)") );
        e << d->appName;
    }

    processEvents();
    return 0;
}

/*!
  \internal
  External request for application to quit.  Quits if possible without
  loosing state.
*/
void QtopiaApplication::tryQuit()
{
    if ( activeModalWidget() || strcmp( argv()[0], "embeddedkonsole") == 0 )
        return; // Inside modal loop or konsole. Too hard to save state.

    {
        QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("closing(QString)") );
        e << d->appName;
    }

    if (d->qpe_main_widget)
        d->store_widget_rect(d->qpe_main_widget, d->appName);

    processEvents();

    quit();
}

/*!
  \internal
  User initiated quit.  Makes the window 'Go Away'.  If preloaded this means
  hiding the window.  If not it means closing all the toplevel widgets.
  As this is user initiated we don't need to check state.

  XXX - should probably be renamed hideApplication() or something (as it never
  actually forces a quit).
*/
void QtopiaApplication::hideOrQuit()
{
    if (d->qpe_main_widget)
        d->store_widget_rect(d->qpe_main_widget, d->appName);

    processEvents();

    // If we are a preloaded application we don't actually quit, so emit
    // a System message indicating we're quasi-closing.
    if ( d->preloaded && d->qpe_main_widget ) {
        QtopiaIpcEnvelope e(QLatin1String("QPE/QtopiaApplication"), QLatin1String("fastAppHiding(QString)") );
        e << d->appName;
        d->qpe_main_widget->hide();
    } else if(d->qpe_main_widget) {
        // First attempt to close modal dialogs.
        // If we fail on any of them, don't try to close any other windows.
        QWidget *wid = activeModalWidget();
        while (wid) {
            if (!wid->close())
                return;
            wid = activeModalWidget();
        }
        foreach(wid, static_cast<QApplication *>(QCoreApplication::instance())->topLevelWidgets())
            wid->close();
    }
}

void QtopiaApplication::lineEditTextChange(const QString &)
{
    if (!mousePreferred)
        QtopiaApplicationData::updateContext((QWidget*)sender());
}

void QtopiaApplication::multiLineEditTextChange()
{
    if (!mousePreferred)
        QtopiaApplicationData::updateContext((QWidget*)sender());
}

void QtopiaApplication::buttonChange(bool)
{
    if (!mousePreferred)
        QtopiaApplicationData::updateButtonSoftKeys((QWidget*)sender());
}

void QtopiaApplication::textBrowserHighlightChange(const QString &)
{
    if (!mousePreferred)
        QtopiaApplicationData::updateBrowserSoftKeys((QWidget*)sender());
}

void QtopiaApplication::removeFromWidgetFlags()
{
    // removes calling object from flags.
    const QWidget *s = (const QWidget *)sender();
    if (qpeWidgetFlags.contains(s))
        qpeWidgetFlags.remove(s);
}

void QtopiaApplication::updateDialogGeometry()
{
    foreach (QWidget *widget, topLevelWidgets()) {
        QDialog *dlg = qobject_cast<QDialog*>(widget);
        if (dlg && dlg->isVisible() && !dlg->isMaximized())
            showDialog(dlg);
    }
}



/*!
    Shows \a dialog. A heuristic approach is taken to
    determine the size of the dialog and whether it is maximized.

    If \a noMaximize is true then the dialog may not be maximized.  This
    is usually unnecessary and it is recommended that the system be allowed
    to decide how the dialog is displayed by passing false for \a noMaximize.

    \sa execDialog()
*/
void QtopiaApplication::showDialog( QDialog* dialog, bool noMaximize )
{
    markQtopiaWindow(dialog);
    QtopiaApplicationData::qpe_show_dialog( dialog, noMaximize );
}


/*!
    Shows and calls \l{QDialog::exec()}{exec()} on the \a dialog. A heuristic approach is taken to
    determine the size of the dialog and whether it is maximized.

    The function returns a \l QDialog::DialogCode result.

    If \a noMaximize is true then the dialog may not be maximized.  This
    is usually unnecessary and it is recommended that the system be allowed
    to decide how the dialog is displayed by passing false for \a noMaximize.

    \sa showDialog(), QDialog::exec()
*/
int QtopiaApplication::execDialog( QDialog* dialog, bool noMaximize )
{
    // Important to set WA_ShowModal before showing to maintain the
    // same behaviour as QDialog::exec() - i.e. flag is set when show()
    // happens.
    bool wasShowModal = dialog->testAttribute(Qt::WA_ShowModal);
    dialog->setAttribute(Qt::WA_ShowModal, true);
    showDialog(dialog,noMaximize);
    int rv = dialog->exec();
    dialog->setAttribute(Qt::WA_ShowModal, wasShowModal);
    return rv;
}

/*!
  This method temporarily overrides the current global power manager with
  the PowerConstraint \a constraint, allowing applications to control power saving
  functions during their execution.

  Calling this function will access Qtopia's power management via the 
  QtopiaPowerManagerService.

  \sa PowerConstraint
*/
void QtopiaApplication::setPowerConstraint(PowerConstraint constraint)
{
    QtopiaServiceRequest e("QtopiaPowerManager", "setConstraint(int,QString)");
    e << (int)constraint << applicationName();
    e.send();
}

#if (__GNUC__ > 2)
extern "C" void __cxa_pure_virtual();

void __cxa_pure_virtual()
{
    fprintf( stderr, "Pure virtual called\n"); // No tr
    abort();

}
#endif

#if defined(QPE_USE_MALLOC_FOR_NEW)

// The libraries with the skiff package (and possibly others) have
// completely useless implementations of builtin new and delete that
// use about 50% of your CPU. Here we revert to the simple libc
// functions.

void* operator new[](size_t size)
{
    return malloc(size);
}

void* operator new(size_t size)
{
    return malloc(size);
}

void operator delete[](void* p)
{
    if (p)
        free(p);
}

void operator delete[](void* p, size_t /*size*/)
{
    if (p)
        free(p);
}

void operator delete(void* p)
{
    if (p)
        free(p);
}

void operator delete(void* p, size_t /*size*/)
{
    if (p)
        free(p);
}
#endif

/*!
  In some cases dialogs are easier to interact with if they behave in a
  similar fashion to menus.

  The default dialog behaviour is to include a cancel menu option in the context
  menu in order to reject the dialog and map Key_Back to accept the dialog.

  Setting \a menuLike to true instructs the \a dialog to have MenuLike
  behaviour when executed.

  A MenuLike dialog will map Key_Back key to
  reject the dialog and will not map any key to accept the dialog - the
  accept action must be implemented by the developer.

  MenuLike dialogs typically have a single list of options, and accept
  the dialog when the select key is pressed on the appropriate item,
  or when a mouse/stylus is used to click on an item. Key focus should
  be accepted by only one widget.

  \sa isMenuLike()
*/
void QtopiaApplication::setMenuLike( QDialog *dialog, bool menuLike )
{
    if (menuLike == isMenuLike(dialog))
        return;

    // can't do here, if QDialog is const..... but it doesn't need to be.
    if (menuLike)
        QSoftMenuBar::setLabel(dialog, Qt::Key_Back, QSoftMenuBar::Cancel);
    else
        QSoftMenuBar::setLabel(dialog, Qt::Key_Back, QSoftMenuBar::Back);

    if (qpeWidgetFlags.contains(dialog)) {
        qpeWidgetFlags[dialog] = qpeWidgetFlags[dialog] ^ MenuLikeDialog;
    } else {
        // the ternery below is actually null.  if menuLike is false, and flags
        // not present, then will never get here, hence menuLike is always
        // true at this point.  Leave in though incase that logic
        // changes.
        qpeWidgetFlags.insert(dialog, menuLike ? MenuLikeDialog : 0);
        // connect to destructed signal.
        connect(dialog, SIGNAL(destroyed()), qApp, SLOT(removeFromWidgetFlags()));
    }
}

/*!
  Returns true if the \a dialog is set to have MenuLike behaviour when
  executed; otherwise returns false.

  \sa setMenuLike()
*/
bool QtopiaApplication::isMenuLike( const QDialog *dialog)
{
    if (qpeWidgetFlags.contains(dialog))
        return (qpeWidgetFlags[dialog] & MenuLikeDialog == MenuLikeDialog);
    return false; // default.
}

/*!
  \internal
  Return the global test slave used for this application, if one exists.
  A test slave will only be created if the QTOPIA_TEST environment variable
  is set and the qtopiatest plugins are installed.

  The test slave is used to communicate with an attached system test, if one
  exists.

  Note that Qtopia only attempts to load the plugin once, i.e. you must restart
  qtopia to turn on/off system testing.
*/
TestSlaveInterface* QtopiaApplication::testSlave()
{
#ifdef QTOPIA_USE_TEST_SLAVE
    static bool init = false;
    static TestSlaveInterface* ret = 0;

    if (init) return ret;

    init = true;
    if (qgetenv("QTOPIA_TEST").isEmpty()) {
        qLog(Qtopiatest) << "Qtopiatest is disabled because the QTOPIA_TEST "
                            "environment variable is not set.";
        return ret;
    }

    QList<QString> types;
    types << "qtopiatest_application";
    QString loadedPlugin;
    if (type() == QApplication::GuiServer)
        types << "qtopiatest_server";

    QStringList pluginsToLoad;

    foreach (QString pluginType, types) {
        // Unfortunately we cannot use QPluginManager::instance, because we
        // _must_ load the app plugin with RTLD_GLOBAL.

        bool foundPlugin = false;

        QPluginManager mgr(pluginType);
        foreach (QString plugin, mgr.list()) {
            foreach (QString path, Qtopia::installPaths()) {
                QString libFile = path + "plugins/" + pluginType + "/lib" + plugin + ".so";
                if ( QFile::exists(libFile) ) {
                    foundPlugin = true;
                    pluginsToLoad << libFile;
                    break;
                }
            }
        }

        if (!foundPlugin) {
            qWarning() << "Qtopiatest: couldn't find plugin of type"
                       << pluginType;
        }
    }

    QPluginLoader pluginLoader;
    QLibrary libLoader;
    foreach (QString plugin, pluginsToLoad) {
        libLoader.setFileName(plugin);

        // enable RTLD_GLOBAL, so plugins can access each other's symbols
        // xxx workaround for Qt bug: need to explicitly call this after
        // xxx each call to setFileName
        libLoader.setLoadHints(QLibrary::ExportExternalSymbolsHint);
        libLoader.load();
        libLoader.unload();

        pluginLoader.setFileName(plugin);
        QObject *o = pluginLoader.instance();
        ret = qobject_cast<TestSlaveInterface*>(o);
        if (!ret) {
            qWarning() << "Qtopiatest: failed to load qtopiatest plugin"
                       << "\n   plugin" << plugin
                       << "\n   instance" << o
                       << "\n   error" << pluginLoader.errorString();
        }
    }

    if (ret) {
        qLog(Qtopiatest) << "Qtopiatest is enabled.";
    } else {
        qWarning() << "Qtopiatest is disabled due to errors.";
    }

    return ret;
#else
    return 0;
#endif
}


/*!
  \macro QTOPIA_ADD_APPLICATION(name,classname)
  \relates QtopiaApplication

  This macro registers the application uniquely identified by \a name with
  the main widget \a classname. This macro is used to simplify switching between quicklaunch and normal
  launching.

  In singleexec builds, this macro is used to indicate the name of the main function rather than
  specifying the class to instantiate.

  \sa Applications
*/

/*!
  \macro QTOPIA_MAIN
  \relates QtopiaApplication

  This macro inserts an appropriate main function when normal launching is used.

  \sa Applications
*/

/*!
  \macro QTOPIA_EXPORT_PLUGIN(classname)
  \relates QtopiaApplication

  This macro causes a QObject-derived \a classname to be made available from a
  plugin file (eg foo.so). This macro should be used instead of Q_EXPORT_PLUGIN because it
  works correctly in both dynamic and singleexec builds.

  Note that this function is for Qtopia plugins. For Qt plugins you should use \l QTOPIA_EXPORT_QT_PLUGIN().
*/

/*!
  \macro QTOPIA_EXPORT_QT_PLUGIN(classname)
  \relates QtopiaApplication

  This macro causes a QObject-derived \a classname to be made available from a
  plugin file (eg foo.so). This macro should be used instead of Q_EXPORT_PLUGIN because it
  works correctly in both dynamic and singleexec builds.

  Note that this function is for Qt plugins. For Qtopia plugins you should use \l QTOPIA_EXPORT_PLUGIN().
*/

/*!
  \macro QTOPIA_APP_KEY
  \relates QtopiaApplication

  This macro is used by the QTOPIA_MAIN macro to allocate space for the SXE key.
  It's set to either QSXE_APP_KEY or QSXE_QL_APP_KEY depending on if you're
  building a quicklaunch plugin or a standalone app.

  \sa Applications, QTOPIA_SET_KEY()
*/

/*!
  \macro QTOPIA_SET_KEY(name)
  \relates QtopiaApplication

  This macro is used by the QTOPIA_MAIN macro to copy the SXE key into memory.
  It's set to either QSXE_SET_APP_KEY() or QSXE_SET_QL_KEY() (passing along \a name)
  depending on if you're building a quicklaunch plugin or a standalone app.

  \sa Applications, QTOPIA_APP_KEY
*/

/*!
  \macro QTOPIA_SET_DOCUMENT_SYSTEM_CONNECTION()
  \relates QtopiaApplication

  This macro is used by the QTOPIA_MAIN macro to set the default document system connection type.
  If you are implementing a \c main() function you should call this after constructing QtopiaApplication.
  Failure to call this macro is not fatal but you will be unable to influence the default connection type.

  The QTOPIA_DIRECT_DOCUMENT_SYSTEM_CONNECTION macro affects the implementation of this function.

  \sa Applications, QContent::setDocumentSystemConnection()
*/

/*!
  \macro QTOPIA_DIRECT_DOCUMENT_SYSTEM_CONNECTION
  \relates QtopiaApplication

  This macro should be set to specify that the app needs a direct connection to the document system.
  Note that enabling this define will mean extra security priviliges are required.
  If you are not using the QTOPIA_MAIN macros you should ensure that QTOPIA_SET_DOCUMENT_SYSTEM_CONNECTION()
  is being called or this macro will have no effect.

  \sa Applications, QContent::setDocumentSystemConnection()
*/

Q_GLOBAL_STATIC(QMutex, contentChangedChannelMutex);

/*!
    \reimp
*/
void QtopiaApplication::connectNotify(const char *signal)
{
    if(QLatin1String(signal) == SIGNAL(contentChanged(QContentIdList,QContent::ChangeType)))
    {
        QMutexLocker lock(contentChangedChannelMutex());
        if(d->contentChangedChannel == NULL && receivers(SIGNAL(contentChanged(QContentIdList,QContent::ChangeType))) > 0)
            d->contentChangedChannel = new QContentChangedChannel(this);
    }
}

/*!
    \reimp
*/
void QtopiaApplication::disconnectNotify(const char *signal)
{
    if(QLatin1String(signal) == SIGNAL(contentChanged(QContentIdList,QContent::ChangeType)))
    {
        QMutexLocker lock(contentChangedChannelMutex());
        if(d->contentChangedChannel != NULL && receivers(SIGNAL(contentChanged(QContentIdList,QContent::ChangeType))) < 1)
        {
            // disconnect the channel, as we've run out of listeners
            QContentChangedChannel *oldChannel=d->contentChangedChannel;
            d->contentChangedChannel = NULL;
            delete oldChannel;
        }
    }
}

QContentChangedChannel::QContentChangedChannel(QObject *parent)
 : QtopiaIpcAdaptor("QPE/DocAPI")
{
    Q_UNUSED(parent);
    publishAll(QtopiaIpcAdaptor::Slots);
    QObject::connect(this, SIGNAL(contentChangedSignal(QContentIdList,QContent::ChangeType)),
            qApp, SIGNAL(contentChanged(QContentIdList,QContent::ChangeType)));

}

void QContentChangedChannel::contentChanged(QContentIdList idList, QContent::ChangeType ct)
{
    if (ct != QContent::Added) {
        foreach (QContentId id, idList)
            QContentCache::instance()->remove( id );
    }
    emit contentChangedSignal(idList, ct);
}

#include "qtopiaapplication.moc"
