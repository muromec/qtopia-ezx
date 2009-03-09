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

#ifndef _MSC_VER
# define private public
# define protected public
#endif
#include <qtopiaapplication.h>
#undef private
#undef protected

#include "firstuse.h"
#include "inputmethods.h"

#include "../settings/language/languagesettings.h"
#include "../settings/systemtime/settime.h"

#include <qtopianamespace.h>
#include <custom.h>

#if defined(QPE_NEED_CALIBRATION)
#include "../settings/calibrate/calibrate.h"
#endif

#include <qtopiaipcenvelope.h>
#include <qsettings.h>
#include <qtimezone.h>


#include <qfile.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtreewidget.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qheaderview.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qtranslator.h>

#include <stdlib.h>

//#define USE_INPUTMETHODS

static FirstUse *firstUse = 0;

class TzListItem : public QTreeWidgetItem
{
public:
    TzListItem(QTreeWidgetItem *li, const QString &text, const QString &id)
        : QTreeWidgetItem(li), tzId(id) {
        setText(0, text);
    }

    const QString &id() const { return tzId; }

public:
    QString tzId;
};

class TzAreaListItem : public QTreeWidgetItem
{
public:
    TzAreaListItem(QTreeWidget *li, const QString &text)
        : QTreeWidgetItem(li), childSel(false) {
        setText(0, text);
    }

    void setChildSelected(bool s) { childSel = s; /*repaint();*/ }

protected:
#ifdef QTOPIA4_TODO
    void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align) {
        QColorGroup mycg(cg);
        if (childSel) {
            mycg.setColor(QColorGroup::Text, cg.highlightedText());
            mycg.setBrush(QColorGroup::Base, cg.brush(QColorGroup::Highlight));
        }
        QTreeWidgetItem::paintCell(p, mycg, column, width, align);
    }
#endif

public:
    bool childSel;
};

class TimeZoneDialog : public QDialog
{
    Q_OBJECT
public:
    TimeZoneDialog(QWidget *parent=0);

protected:
    void accept();

protected slots:
    void tzChanged(QTreeWidgetItem *);
    void tzClicked(QTreeWidgetItem *);

private:
    QTreeWidget *tzTreeWidget;
    TzListItem *currItem;
};

TimeZoneDialog::TimeZoneDialog(QWidget *parent)
    : QDialog(parent), currItem(0)
{
    QVBoxLayout *vb = new QVBoxLayout(this);
    tzTreeWidget = new QTreeWidget(this);
    tzTreeWidget->header()->hide();
    tzTreeWidget->setRootIsDecorated(true);
    tzTreeWidget->setColumnCount(1);
    vb->addWidget(tzTreeWidget);

    QString currTz = getenv("TZ");
    QSettings config("Trolltech","locale");
    config.beginGroup("Location");
    currTz = config.value("Timezone", currTz).toString();

    QMap<QString,QTreeWidgetItem *> areaMap;

    QStringList ids = QTimeZone::ids();
    QStringList::const_iterator it;
    for (it = ids.begin(); it != ids.end(); ++it) {
        QString tzId = *it;
        if (tzId.contains(QRegExp("/RC")) || tzId.contains(QRegExp("/OK"))) {
            continue;
        }
        QTimeZone tz(tzId.toAscii().constData());
        QString area = tz.area();
        int pos = area.indexOf('/');
        if (pos > -1) {
            area.truncate(pos);
            area = qApp->translate("QTimeZone", area.toAscii().constData());
        }

        QTreeWidgetItem *rparent = 0;
        QMap<QString,QTreeWidgetItem *>::ConstIterator ait = areaMap.find(area);
        if (ait == areaMap.end()) {
            rparent = new TzAreaListItem(tzTreeWidget, area);
#ifdef QTOPIA4_TODO
            rparent->setSelectable(false);
#endif
            areaMap[area] = rparent;
        } else {
            rparent = *ait;
        }
        TzListItem *citem = new TzListItem(rparent, tz.city(), tzId);
        if (tzId == currTz.toLatin1()) {
            TzAreaListItem *ai;
            if (currItem) {
                ai = (TzAreaListItem *)currItem->parent();
                ai->setChildSelected(false);
            }
            currItem = citem;
            tzTreeWidget->setCurrentItem(currItem);
            ai = (TzAreaListItem *)currItem->parent();
            ai->setChildSelected(true);
            tzTreeWidget->setItemExpanded(citem->parent(), true);
        } else if (!currItem) {
            currItem = citem;
            tzTreeWidget->setCurrentItem(currItem);
            TzAreaListItem *ai = (TzAreaListItem *)currItem->parent();
            ai->setChildSelected(true);
        }
    }
    tzTreeWidget->sortItems(0, Qt::AscendingOrder);
//    tzTreeWidget->resizeColumnToContents(0);
    connect(tzTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(tzChanged(QTreeWidgetItem*)));
    connect(tzTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(tzClicked(QTreeWidgetItem*)));
}

void TimeZoneDialog::accept()
{
    if (currItem) {
        setenv("TZ", currItem->id().toLatin1(), 1);
        QSettings config("Trolltech","locale");
        config.beginGroup("Location");
        config.setValue("Timezone", QString(currItem->id().toLatin1()));
    }
    QDialog::accept();
}

void TimeZoneDialog::tzChanged(QTreeWidgetItem *item)
{
    TzAreaListItem *ai;
    if (item) {
        if (item->parent()) {
            tzTreeWidget->setItemSelected(currItem, false);
            ai = (TzAreaListItem *)currItem->parent();
            ai->setChildSelected(false);
            TzListItem *tzitem = (TzListItem*)item;
            currItem = tzitem;
        }
    }
    tzTreeWidget->setItemSelected(currItem, true);
    ai = (TzAreaListItem *)currItem->parent();
    ai->setChildSelected(true);
}

void TimeZoneDialog::tzClicked(QTreeWidgetItem *item)
{
    if (item && !item->parent()) {
        tzTreeWidget->setItemExpanded(item, !tzTreeWidget->isItemExpanded(item));
    }
}

static QDialog *createTimeZone(QWidget *parent) {
    return new TimeZoneDialog(parent);
}

//===========================================================================

static QDialog *createLanguage(QWidget *parent) {
    LanguageSettings *dlg = new LanguageSettings(parent);
    dlg->setConfirm(false);
    return dlg;
}

static void acceptLanguage(QDialog *dlg)
{
    dlg->accept();
    if (firstUse)
        firstUse->reloadLanguages();
}

static QDialog *createDateTime(QWidget *parent) {
    SetDateTime *dlg = new SetDateTime(parent);
    dlg->setTimezoneEditable(false);
    return dlg;
}

static void acceptDialog(QDialog *dlg)
{
    dlg->accept();
}


struct {
    bool enabled;
    QDialog *(*createFunc)(QWidget *parent);
    void (*acceptFunc)(QDialog *dlg);
    const char *trans;
    const char *desc;
    bool needIM;
}
settingsTable[] =
{
    { true, createLanguage, acceptLanguage, "language.qm", QT_TRANSLATE_NOOP("FirstUse", "Language"), false },
    { true, createTimeZone, acceptDialog, "timezone.qm", QT_TRANSLATE_NOOP("FirstUse", "Timezone"), false },
    { true, createDateTime, acceptDialog, "systemtime.qm", QT_TRANSLATE_NOOP("FirstUse", "Date/Time"), true },
    { false, 0, 0, "", "", false }
};


FirstUse::FirstUse(QWidget *parent, Qt::WFlags f)
    : QDialog(parent, f), currDlgIdx(-1), currDlg(0),
        needCalibrate(false), needRestart(true)
{
    ServerApplication::allowRestart = false;
    // we force our height beyound the maximum (which we set anyway)
    QDesktopWidget *desktop = QApplication::desktop();
    QRect desk = desktop->screenGeometry(desktop->primaryScreen());
    setGeometry(desk);

    // more hackery
    // It will be run as either the main server or as part of the main server
    QWSServer::setScreenSaverIntervals(0);
    loadPixmaps();

    setFocusPolicy(Qt::NoFocus);

    taskBar = new QWidget(0, (Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint));
    taskBar->setAttribute(Qt::WA_GroupLeader, true);

#ifdef USE_INPUTMETHODS
    inputMethods = new InputMethods(taskBar);
    connect(inputMethods, SIGNAL(inputToggled(bool)),
            this, SLOT(calcMaxWindowRect()));
#endif

    back = new QPushButton(tr("<< Back"), taskBar);
    back->setFocusPolicy(Qt::NoFocus);
    connect(back, SIGNAL(clicked()), this, SLOT(previousDialog()) );

    next = new QPushButton(tr("Next >>"), taskBar);
    next->setFocusPolicy(Qt::NoFocus);
    connect(next, SIGNAL(clicked()), this, SLOT(nextDialog()) );

    // need to set the geom to lower corner
    int x = 0;
    controlHeight = back->sizeHint().height();
    QSize sz(0,0);
#ifdef USE_INPUTMETHODS
    sz = inputMethods->sizeHint();
    inputMethods->setGeometry(0,0, sz.width(), controlHeight );
    x += sz.width();
#endif
    int buttonWidth = (width() - sz.width()) / 2;
    back->setGeometry(x, 0, buttonWidth, controlHeight);
    x += buttonWidth;
    next->setGeometry(x, 0, buttonWidth, controlHeight);

    taskBar->setGeometry( 0, height() - controlHeight, desk.width(), controlHeight);
    taskBar->hide();

    QWidget *w = new QWidget(0);
    w->showMaximized();
    int titleHeight = w->geometry().y() - w->frameGeometry().y();
    delete w;

    titleBar = new QLabel(0, (Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint));
    titleBar->setAttribute(Qt::WA_GroupLeader, true);
    QPalette pal = titleBar->palette();
    pal.setBrush(QPalette::Background, pal.brush(QPalette::Normal, QPalette::Highlight));
    pal.setColor(QPalette::Text, pal.color(QPalette::Normal, QPalette::HighlightedText));
    titleBar->setPalette(pal);
    titleBar->setAlignment(Qt::AlignCenter);
    titleBar->setGeometry(0, 0, desk.width(), titleHeight);
    titleBar->hide();

    calcMaxWindowRect();
#if defined(QPE_NEED_CALIBRATION)
    QString calFile = qgetenv("POINTERCAL_FILE");
    if(calFile.isEmpty())
      calFile = "/etc/pointercal";
    if ( !QFile::exists(calFile) ) {
        needCalibrate = true;
        grabMouse();
    }
#endif
    defaultFont = font();

    reloadLanguages();

    firstUse = this;
}


FirstUse::~FirstUse()
{
    delete taskBar;
    delete titleBar;
    ServerApplication::allowRestart = true;
    firstUse = 0;
}

void FirstUse::calcMaxWindowRect()
{
#ifdef Q_WS_QWS
    QRect wr;
    QDesktopWidget *desktop = QApplication::desktop();
    QRect displayRect = desktop->screenGeometry(desktop->primaryScreen());
    QRect ir;
# ifdef USE_INPUTMETHODS
    ir = inputMethods->inputRect();
# endif
    if ( ir.isValid() ) {
        wr.setCoords(displayRect.x(), displayRect.y(),
                    displayRect.width()-1, ir.top()-1 );
    } else {
        wr.setCoords(displayRect.x(), displayRect.y(),
                    displayRect.width()-1,
                    displayRect.height() - controlHeight-1);
    }

    QWSServer::setMaxWindowRect( wr );
#endif
}

/* accept current dialog, and bring up next */
void FirstUse::nextDialog()
{
    if (currDlg)
        settingsTable[currDlgIdx].acceptFunc(currDlg);
    currDlgIdx = findNextDialog(true);
}

/* accept current dialog and bring up previous */
void FirstUse::previousDialog()
{
    if (currDlgIdx != 0) {
        if (currDlg)
            settingsTable[currDlgIdx].acceptFunc(currDlg);
        currDlgIdx = findNextDialog(false);
    }
}

void FirstUse::switchDialog()
{
    if (currDlgIdx == -1) {
        {
            QSettings config("Trolltech","qpe");
            config.beginGroup( "Startup" );
            config.setValue( "FirstUse", false );
            config.sync();
        }
        QSettings cfg = QSettings( "Trolltech", "WorldTime" );
        cfg.beginGroup( "TimeZones" );

        // translate the existing list of QTimeZone names
        // This is usually enforced during the startup of qpe
        // (main.cpp::refreshTimeZoneConfig). However when we use
        // First use we have to do it here again to ensure a
        // translation
        int zoneIndex = 0;
        QTimeZone curZone;
        QString zoneID;

        while (cfg.contains( "Zone"+ QString::number( zoneIndex ))){
            zoneID = cfg.value( "Zone" + QString::number( zoneIndex )).toString();
            curZone = QTimeZone( zoneID.toAscii().constData());
            if ( !curZone.isValid() ){
                qWarning( "initEnvironment() Invalid QTimeZone %s", (const char *)zoneID.toLatin1() );
                break;
            }
            zoneIndex++;
    }

        QPixmap pix(":image/bigwait");
        QLabel *lblWait = new QLabel(0, (Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint));
        lblWait->setAttribute(Qt::WA_DeleteOnClose);
        lblWait->setPixmap( pix );
        lblWait->setAlignment( Qt::AlignCenter );
        lblWait->setGeometry( this->geometry() );
        lblWait->show();
        qApp->processEvents();
        QTimer::singleShot( 1000, lblWait, SLOT(close()) );
        accept();
        ServerApplication::allowRestart = true;
    } else {
        updateButtons();
        currDlg = settingsTable[currDlgIdx].createFunc(this);
        currDlg->showMaximized();
        currDlg->exec();
        delete currDlg;
        currDlg = 0;
        QTimer::singleShot(0, this, SLOT(switchDialog()));
    }
}

void FirstUse::reloadLanguages()
{
    // read language from config file.  Waiting on QCop takes too long.
    QSettings config("Trolltech","locale");
    config.beginGroup( "Language");
    QString l = config.value( "Language", "en_US").toString();
    QString cl = getenv("LANG");
    qWarning(QString("language message - %1").arg(l).toLatin1().constData());
    // setting anyway...
    if (l.isNull() )
        unsetenv( "LANG" );
    else {
        qWarning("and its not null");
        setenv( "LANG", l.toLatin1(), 1 );
    }
#ifndef QT_NO_TRANSLATION
    // clear old translators
#ifdef QTOPIA4_TODO
#ifndef _MSC_VER
  //### revise to allow removal of translators under MSVC
  if(qApp->translators) {
        qApp->translators->setAutoDelete(true);
        delete (qApp->translators);
        qApp->translators = 0;
    }
#endif
#endif

    const char *qmFiles[] = { "qt.qm", "qpe.qm", "libqpe.qm", "libqtopia.qm" , 0 };

    // qpe/library translation files.
    int i = 0;
    QTranslator *trans;
    while (qmFiles[i]) {
        trans = new QTranslator(qApp);
        QString atf = qmFiles[i];
        QString tfn = Qtopia::qtopiaDir() + "i18n/"+l+"/"+atf;
        qWarning(QString("loading %1").arg(tfn).toLatin1().constData());
        if ( trans->load(tfn) ) {
            qWarning(" installing translator");
            qApp->installTranslator( trans );
        } else  {
            delete trans;
        }
        i++;
    }

    // first use dialog translation files.
    i = 0;
    while (settingsTable[i].createFunc) {
        if (settingsTable[i].enabled && settingsTable[i].trans) {
            trans = new QTranslator(qApp);
            QString atf = settingsTable[i].trans;
            QString tfn = Qtopia::qtopiaDir() + "i18n/"+l+"/"+atf;
            qWarning(QString("loading %1").arg(tfn).toLatin1().constData());
            if ( trans->load(tfn) ) {
                qWarning(" installing translator");
                qApp->installTranslator( trans );
            } else  {
                delete trans;
            }
        }
        i++;
    }

    loadPixmaps();

    QStringList qpepaths = Qtopia::installPaths();
    for (QStringList::Iterator qit=qpepaths.begin();
            qit != qpepaths.end(); ++qit ) {
        QTranslator t(0);
        QString tfn = *qit+"i18n/";
        if (t.load(tfn+l+"/QtopiaDefaults.qm")) {
            QSettings fmcfg(QSettings::SystemScope, "Trolltech","qpe");
            fmcfg.beginGroup("Font");
            QString f = config.value( QLatin1String("FontFamily[]"), "dejavu_sans_condensed").toString());
            f = t.translate( "QPE",  f.toAscii().constData() );
            QFont font(f);
            f.setPointSize(t.translate("QPE",config.value( QLatin1String("FontSize[]"), "6.4").toAscii().constData()).toDouble() );
            qApp->setFont(font);
        } else{ //drop back to default font
            qApp->setFont(QFont("dejavu"));
        }
    }
#endif
    updateButtons();

}

void FirstUse::paintEvent( QPaintEvent * )
{
    QPainter p( this );

    p.drawPixmap(0,0, splash);

    QFont f = p.font();
    f.setPointSize(15);
    f.setItalic(false);
    f.setBold(false);
    p.setFont(f);

    if ( currDlgIdx < 0 ) {
        drawText(p, tr( "Tap anywhere on the screen to continue." ));
    } else if ( settingsTable[currDlgIdx].createFunc ) {
        drawText(p, tr("Please wait, loading %1 settings.").arg(tr(settingsTable[currDlgIdx].desc)) );
    } else {
        drawText(p, tr("Please wait..."));
    }
}

void FirstUse::loadPixmaps()
{
    /* create background, tr so can change image with language.
       images will likely contain text. */
    splash.fromImage( QImage(":image/FirstUseBackground") //No tr
            .scaled( width(), height() ) );

    QPalette pal = palette();
    pal.setBrush(QPalette::Background, splash);
    setPalette(pal);
}

void FirstUse::drawText(QPainter &p, const QString &text)
{
    QString altered = "<CENTER>" + text + "</CENTER>";

#ifdef QTOPIA4_TODO
    Q3SimpleRichText rt(altered, p.font());
    rt.setWidth(width() - 20);

    int h = (height() * 3) / 10; // start at 30%
    if (rt.height() < height() / 2)
        h += ((height() / 2) - rt.height()) / 2;
    rt.draw(&p, 10, h, QRegion(0,0, width()-20, height()), palette());
#else
    p.drawText( QRect(0, 0, width()-20, height()), 0, altered );
#endif
}

int FirstUse::findNextDialog(bool forwards)
{
    int i;
    if (forwards) {
        i = currDlgIdx+1;
        while ( settingsTable[i].createFunc && !settingsTable[i].enabled )
            i++;
        if ( !settingsTable[i].createFunc )
            i = -1;
    } else {
        i = currDlgIdx-1;
        while ( i >= 0 && !settingsTable[i].enabled )
            i--;
    }

    return i;
}

void FirstUse::updateButtons()
{
    if ( currDlgIdx >= 0 ) {
#ifdef USE_INPUTMETHODS
        inputMethods->setEnabled(settingsTable[currDlgIdx].needIM);
#endif
        taskBar->show();
        titleBar->setText("<b>"+tr(settingsTable[currDlgIdx].desc)+"</b>");
        titleBar->show();
    }

    int i = findNextDialog(false);
    back->setText(tr("<< Back"));
    back->setEnabled( i >= 0 );

    i = findNextDialog(true);
    if ( i < 0)
        next->setText(tr("Finish"));
    else
        next->setText(tr("Next >>"));
    next->setEnabled( true );
}

void FirstUse::keyPressEvent( QKeyEvent *e )
{
    // Allow cancelling at first dialog, in case display is broken.
    if ( e->key() == Qt::Key_Escape && currDlgIdx < 0 )
        QDialog::keyPressEvent(e);
}

void FirstUse::mouseReleaseEvent( QMouseEvent * )
{
    if ( currDlgIdx < 0 ) {
#if defined(QPE_NEED_CALIBRATION)
        if ( needCalibrate ) {
            releaseMouse();
            Calibrate *cal = new Calibrate;
            cal->exec();
            delete cal;
        }
#endif
        currDlgIdx = 0;
        currDlg = 0;
        QTimer::singleShot(0, this, SLOT(switchDialog()));
    }
}

#include "firstuse.moc"
