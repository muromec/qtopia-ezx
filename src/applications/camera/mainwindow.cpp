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

#include "mainwindow.h"
#include "videocaptureview.h"
#include "ui_camerasettings.h"
#include "thumbbutton.h"
#include "phototimer.h"

#include <qthumbnail.h>
#include <qtopiaapplication.h>
#include <qsoftmenubar.h>
#include <qtopiaipcenvelope.h>
#include <qtopiaservices.h>
#include <qcategorymanager.h>
#include <qstorage.h>

#include <qcontent.h>
#include <qcontentset.h>
#include <qvaluespace.h>

#include <QAction>
#include <QToolButton>
#include <QPushButton>
#include <QSignalMapper>
#include <QImage>
#include <QTimer>
#include <QSettings>
#include <QMessageBox>
#include <QKeyEvent>
#include <QPainter>
#include <QDSAction>
#include <QDSData>
#include <QDSServices>
#include <QDataStream>
#include <QByteArray>
#include <QDesktopWidget>
#include <QMenu>
#include <QWaitWidget>

#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

static const int thmarg=2;
static const bool video_supported = false;

CameraMainWindow::CameraMainWindow(QWidget *parent, Qt::WFlags f):
    QMainWindow(parent, f),
    settings( NULL ),
    settingsDialog( NULL ),
    psize( -1 ),
    vsize( -1 ),
    snapRequest( 0 ),
    videoOnSecondary(false),
    m_photoContentSet( QContentSet::Asynchronous ),
    m_photoModel( 0 ),
    m_contextMenuActive(false)
{
    setWindowTitle(tr("Camera"));

    QtopiaApplication::setPowerConstraint(QtopiaApplication::DisableLightOff);

    picfile = Qtopia::tempDir() + "image.jpg";
    QWidget *cameraWidget = new QWidget(this);
    camera = new Ui::CameraBase();
    camera->setupUi(cameraWidget);
    setCentralWidget(cameraWidget);

    camcat = QLatin1String("Camera");
    // Ensure the Camera system categoy exists
    QCategoryManager catman("Documents");
    // For new code a more unique id should be used instead of using the untranslated text
    // eg. ensureSystemCategory("com.mycompany.myapp.mycategory", "My Category");
    catman.ensureSystemCategory(camcat, camcat);

    new CameraService(this);

    camera->photo->setFocus();

    connect(camera->photo, SIGNAL(clicked()), this, SLOT(takePhoto()));
    connect(camera->video, SIGNAL(clicked()), this, SLOT(toggleVideo()));

    refocusTimer = new QTimer(this);
    refocusTimer->setSingleShot(true);
    connect(refocusTimer, SIGNAL(timeout()), this, SLOT(takePhotoNow()));

    thumb[0] = camera->thumb1;
    thumb[1] = camera->thumb2;
    thumb[2] = camera->thumb3;
    thumb[3] = camera->thumb4;
    thumb[4] = camera->thumb5;
    cur_thumb = -1;

    // Load the allowable sizes from the camera hardware.
    photo_size = camera->videocaptureview->photoSizes();
    video_size = camera->videocaptureview->videoSizes();

    namehint=0;
    recording = false;


    if ( !video_supported ) {
        // Room for longer text
        camera->photo->setText(tr("Take Photo"));
        camera->video->setEnabled(false);
        camera->video->hide();
    }

    if (QApplication::desktop()->numScreens() > 1) {
        // We have a secondary display - watch for the clamshell open/close
        clamshellVsi = new QValueSpaceItem("/Hardware/Devices/ClamshellOpen", this);
        connect(clamshellVsi, SIGNAL(contentsChanged()), this, SLOT(clamshellChanged()));
        if (!clamshellVsi->value().toBool()) {
            videoToScreen(1);
        }
    }

    m_photoContentSet.setCriteria( QContentFilter( QContent::Document )
            & QContentFilter::category( QLatin1String( "Camera" ) )
            & QContentFilter::mimeType( QLatin1String( "image/jpeg" ) ) );

    m_photoContentSet.setSortCriteria( QContentSortCriteria( QContentSortCriteria::LastUpdated, Qt::DescendingOrder ) );

    m_photoModel = new QContentSetModel( &m_photoContentSet, this );
    connect( m_photoModel, SIGNAL(updateFinished()), this, SLOT(loadThumbs()) );

    m_wait = new QWaitWidget(camera->videocaptureview);
    m_iswaiting = false;
    QTimer::singleShot(1, this, SLOT(delayedInit()));
    m_currzoom = 0;
    zoomActive = false;

    zoomTimer.setSingleShot(true);
    zoomTimer.setInterval(5000);
    connect(&zoomTimer, SIGNAL(timeout()), this, SLOT(hideZoom()));
}

CameraMainWindow::~CameraMainWindow()
{
    QtopiaApplication::setPowerConstraint(QtopiaApplication::Enable);
    delete m_wait;
}

void CameraMainWindow::delayedInit()
{
    settingsDialog = new QDialog( this );
    settingsDialog->setModal( true );
    settings = new Ui::CameraSettings();
    settings->setupUi( settingsDialog );
    settingsDialog->setObjectName( "settings" );    // To pick up correct help.
    connect( settings->photo_quality, SIGNAL(valueChanged(int)),
             settings->photo_quality_n, SLOT(setNum(int)) );
    QFileSystemFilter *fsf = new QFileSystemFilter;
    fsf->documents = QFileSystemFilter::Set;
    settings->location->setFilter(fsf);
    // load settings from config
    QSettings cfg("Trolltech","Camera");
    cfg.beginGroup("General");
    QString l = cfg.value("location").toString();
    if ( !l.isEmpty() )
        settings->location->setLocation(l);
    else
        settings->location->setLocation(QFileSystem::documentsFileSystem().documentsPath());
    storagepath = settings->location->documentPath();
    cfg.endGroup();
    cfg.beginGroup("Photo");
    int w;
    w = cfg.value("width",camera->videocaptureview->recommendedPhotoSize().width()).toInt();
    for (psize=0; psize<(int)photo_size.count()-1 && photo_size[psize].width() > w;)
        psize++;
    pquality = cfg.value("quality",settings->photo_quality->value()).toInt();
    cfg.endGroup();
    cfg.beginGroup("Video");
    w = cfg.value("width",camera->videocaptureview->recommendedVideoSize().width()).toInt();
    for (vsize=0; vsize<(int)video_size.count()-1 && video_size[vsize].width() > w;)
        vsize++;
    vquality = cfg.value("quality",settings->video_quality->value()).toInt();
    vframerate = cfg.value("framerate",settings->video_framerate->value()).toInt();

    for (int i=0; i<(int)photo_size.count(); i++) {
        settings->photo_size->addItem(tr("%1 x %2","picture size e.g. 640 x 480").arg(photo_size[i].width()).arg(photo_size[i].height()));
    }
    for (int i=0; i<(int)video_size.count(); i++) {
        settings->video_size->addItem(tr("%1 x %2","picture size e.g. 640 x 480").arg(video_size[i].width()).arg(video_size[i].height()));
    }

    QMimeType m( QLatin1String( "image/jpeg" ));
    QContent a = m.application();
    QIcon picViewIcon = a.icon();
    if ( picViewIcon.isNull() )
        picViewIcon = QIcon( QLatin1String( ":icon/view" ));

    if ( video_supported ) {
        // If video_supported is ever not a constant false then ensure
        // an application is available to handle it
        if ( a.isValid() )
        {
            a_vview = new QAction( QIcon(":image/"+a.iconName()), QString("%1...").arg(a.name()), this );
            connect( a_vview, SIGNAL(triggered()), this, SLOT(viewVideos()) );
        }
    } else {
        a_vview = 0;
    }

    a_pview = new QAction( QIcon(), tr( "View pictures" ), this );
    a_pview->setIcon(picViewIcon);
    connect( a_pview, SIGNAL(triggered()), this, SLOT(viewPictures()) );

    a_timer = new QAction( QIcon( ":icon/wait" ) , tr("Timer"), this );
    connect( a_timer, SIGNAL(triggered()), this, SLOT(takePhotoTimer()) );

    if(camera->videocaptureview->available())
    {
        a_settings = new QAction( QIcon( ":icon/settings" ) , tr("Settings..."), this );
        connect( a_settings, SIGNAL(triggered()), this, SLOT(doSettings()) );
    }

    a_th_edit = new QAction( QIcon(":icon/edit"), tr("Edit"), this );
    connect( a_th_edit, SIGNAL(triggered()), this, SLOT(editThumb()) );
    a_th_del = new QAction( QIcon(":icon/trash"), tr("Delete"), this );
    connect( a_th_del, SIGNAL(triggered()), this, SLOT(delThumb()) );
    a_th_add = new QAction( QIcon(":image/addressbook/AddressBook"), tr("Save to Contact..."), this );
    connect( a_th_add, SIGNAL(triggered()), this, SLOT(moveToContact()) );
    a_send = new QAction( QIcon( ":icon/beam" ), tr("Send to Contact..."), this );
    connect( a_send, SIGNAL(triggered()), this, SLOT(sendFile()) );

    a_zoom = new QAction( QIcon( ), tr("Zoom"), this);
    connect(a_zoom, SIGNAL(triggered()), this, SLOT(showZoom()));
    m_zoom = new QSlider(this);
    m_zoom->setSliderPosition(50);
    m_zoom->setOrientation(Qt::Horizontal);

    m_zoom->setRange(camera->videocaptureview->minZoom(),camera->videocaptureview->maxZoom());
    m_zoom->setValue(0);
    m_zoom->setGeometry(width()*1/5, height()*2/3, 60*width()/100, 40*height()/100);
    m_zoom->hide();

    connect(m_zoom, SIGNAL(valueChanged(int)), this, SLOT(zoomChanged(int)));

    QMenu *contextMenu = QSoftMenuBar::menuFor(this);
    if(camera->videocaptureview->available())
    {
        contextMenu->addAction( a_pview );
        if ( video_supported )
            contextMenu->addAction( a_vview );
        contextMenu->addAction( a_th_edit );
        contextMenu->addAction( a_th_del );
        contextMenu->addAction( a_th_add );
        contextMenu->addAction( a_timer );
        contextMenu->addAction( a_send );
        contextMenu->addAction( a_settings );
        contextMenu->addAction( a_zoom );
    }

    connect(contextMenu, SIGNAL(aboutToHide()),
            this, SLOT(contextMenuAboutToHide()));
    connect(contextMenu, SIGNAL(aboutToShow()),
            this, SLOT(contextMenuAboutToShow()));

    QSignalMapper* sm = new QSignalMapper(this);
    for (int i=0; i<nthumb; i++) {
        sm->setMapping(thumb[i],i);
        connect(thumb[i],SIGNAL(clicked()),sm,SLOT(map()));
        thumb[i]->installEventFilter(this);
    }
    connect(sm,SIGNAL(mapped(int)),this,SLOT(thumbClicked(int)));

    installEventFilter(camera->photo);
    installEventFilter(camera->video);
    camera->photo->installEventFilter(this);
    camera->video->installEventFilter(this);

    loadThumbs();
    preview();
}

void CameraMainWindow::resizeEvent(QResizeEvent*)
{
    thumbw = width()/5-4;
    thumbh = thumbw*3/4;
    camera->thumbs->setFixedHeight(thumbh+thmarg*2);

    loadThumbs( true );
}

void CameraMainWindow::zoomChanged(int val)
{
    if( val > m_currzoom)
        camera->videocaptureview->zoomIn();
    else if(val < m_currzoom)
        camera->videocaptureview->zoomOut();
    m_currzoom = val;
    showZoom();
}

bool CameraMainWindow::event(QEvent* e)
{
    if ( e->type() == QEvent::WindowActivate ) {
        if ( cur_thumb < 0 )
            camera->videocaptureview->setLive();
    } else if ( e->type() == QEvent::WindowDeactivate ) {
        camera->videocaptureview->setLive(-1);
    }
    return QMainWindow::event(e);
}

void CameraMainWindow::showZoom()
{
    if(zoomActive) { zoomTimer.stop(); zoomTimer.start(); return;}
    zoomActive = true;
    m_zoom->show();
    m_zoom->setEditFocus(true);
    zoomTimer.start();
}

void CameraMainWindow::hideZoom()
{
    if (m_zoom->isSliderDown()) { showZoom(); return; }
    m_zoom->hide();
    m_zoom->setEditFocus(false);
    zoomActive = false;
}

bool CameraMainWindow::eventFilter(QObject* o, QEvent* e)
{
    if ( e->type() == QEvent::KeyPress ) {
        QKeyEvent *ke = (QKeyEvent*)e;

        if (!ke->isAutoRepeat()) {
            if (ke->key() == Qt::Key_1) {
                takePhoto();
                return true;
            } else if (ke->key() == Qt::Key_2) {
                toggleVideo();
                return true;
            }
        }
        if (ke->key() == Qt::Key_4)
        {
            if(camera->videocaptureview->minZoom() < m_currzoom)
            {
                m_currzoom--;
                m_zoom->setValue(m_currzoom);
                showZoom();
            }
            camera->videocaptureview->zoomOut();

        }
        if (ke->key() == Qt::Key_6)
        {
            if(camera->videocaptureview->maxZoom() > m_currzoom)
            {
                m_currzoom++;
                m_zoom->setValue(m_currzoom);
                showZoom();
            }
            camera->videocaptureview->zoomIn();

        }
        if ( ke->key() == Qt::Key_Select )
            for(int i = 0; i <nthumb-1; ++i)
                if( o == thumb[i])
                    return true;

        if ( ke->key() == Qt::Key_Up ) {
            camera->photo->setFocus();
            return true;
        } else if ( ke->key() == Qt::Key_Down ) {
            thumb[0]->setFocus();
            return true;
        } else if ( ke->key() == (QApplication::isLeftToRight() ? Qt::Key_Left : Qt::Key_Right) ) {
            if ( o == camera->video ) {
                camera->photo->setFocus();
                return true;
            } else {
                if ( o == thumb[0] )
                    return true;
                for (int i=1; i<nthumb; ++i) {
                    if ( o == thumb[i] ) {
                        thumb[i-1]->setFocus();
                        return true;
                    }
                }
            }
        } else if ( ke->key() == (QApplication::isLeftToRight() ? Qt::Key_Right : Qt::Key_Left) ) {
            if ( o == camera->photo ) {
                camera->video->setFocus();
                return true;
            } else {
                if ( o == thumb[nthumb-1] )
                    return true;
                for (int i=0; i<nthumb-1; ++i) {
                    if ( o == thumb[i] ) {
                        thumb[i+1]->setFocus();
                        return true;
                    }
                }
            }
        }
    }
    else if (!m_contextMenuActive)
    {
        if (e->type() == QEvent::FocusIn)
        {
            if (o == camera->photo) {
                QtopiaApplication::setPowerConstraint( QtopiaApplication::Disable );
            }
            updateActions();
        }
        else if (e->type() == QEvent::FocusOut)
        {
            if (o == camera->photo) {
                QtopiaApplication::setPowerConstraint( QtopiaApplication::Enable );
            }
        }
    }

    return QWidget::eventFilter(o,e);
}

void CameraMainWindow::updateActions()
{
    bool p=false,v=false;
    QWidget *foc = focusWidget();
    if ( foc == camera->photo ) {
        p = true; v = false;
    } else if ( foc == camera->video ) {
        v = true; p = false;
    }
    if(a_pview)
        a_pview->setVisible(p);
    if(a_timer)
        a_timer->setVisible(p);
    if ( video_supported && a_vview )
        a_vview->setVisible(v);
    //a_settings->setVisible(p || v);
    bool th=!p && !v;
    if ( th ) {

        int i;
        for (i=0; i<nthumb; i++) {
            if ( thumb[i] == foc ) {
                selectThumb(i);
                break;
            }
        }
        if ( i==nthumb || thumb[i]->icon().isNull() )
            selectThumb(-1);
    } else {

        selectThumb(-1);
    }
}

void CameraMainWindow::viewPictures()
{
    QtopiaServiceRequest req("PhotoEdit","showCategory(QString)");
    req << camcat;
    req.send();
}

void CameraMainWindow::viewVideos()
{
    QMimeType m( QLatin1String( "video/mpeg" ));
    QContent a = m.application();
    if ( a.isValid() )
        a.execute();
}

void CameraMainWindow::doSettings()
{
    bool v = a_vview != NULL;
    bool p = a_pview != NULL;
    if (p)
        settings->photo->show();
    else
        settings->photo->hide();
    if (v)
        settings->video->show();
    else
        settings->video->hide();
    settings->photo_size->setCurrentIndex(psize);
    settings->video_size->setCurrentIndex(vsize);
    settings->photo_quality->setValue(pquality);
    settings->video_quality->setValue(vquality);
    settings->video_framerate->setValue(vframerate);
    settings->video_quality_n->setFixedWidth(fontMetrics().width("100"));
    settings->photo_quality_n->setFixedWidth(fontMetrics().width("100"));

    if ( QtopiaApplication::execDialog(settingsDialog) ) {
        confirmSettings();
    } else {
        settings->location->setLocation(storagepath);
    }
}

void CameraMainWindow::confirmSettings()
{
    storagepath = settings->location->documentPath();
    psize = settings->photo_size->currentIndex();
    vsize = settings->video_size->currentIndex();
    pquality = settings->photo_quality->value();
    vquality = settings->video_quality->value();
    vframerate = settings->video_framerate->value();

    // save settings
    QSettings cfg("Trolltech","Camera");
    cfg.beginGroup("General");
    cfg.setValue("location",storagepath);
    cfg.endGroup();
    cfg.beginGroup("Photo");
    if(psize != -1)
        cfg.setValue("width",photo_size[psize].width());
    cfg.setValue("quality",pquality);
    cfg.endGroup();
    cfg.beginGroup("Video");
    if(vsize != -1)
        cfg.setValue("width",video_size[vsize].width());
    cfg.setValue("quality",vquality);
    cfg.setValue("framerate",vframerate);

    loadThumbs();

    preview();
}

void CameraMainWindow::loadThumbs( bool resized )
{
    int i = 0;
    showWaitScreen();
    for (; i < nthumb && i < m_photoModel->rowCount(); ++i)
    {
        QContent content = m_photoModel->content(i);
        if (resized || picturefile[i].id() != content.id() || picturefile[i].lastUpdated() != content.lastUpdated()) {
            picturefile[i] = content;

            thumb[i]->setIcon(QThumbnail(picturefile[i].fileName()).pixmap(QSize(thumbw, thumbh)));
            thumb[i]->setEnabled(true);
        }
    }

    for (; i < nthumb; ++i) {
        picturefile[i] = QContent();
        thumb[i]->setIcon(QIcon());
        thumb[i]->setEnabled(false);
    }

    if ( cur_thumb >= 0 )
        selectThumb(cur_thumb);

    if ( !camera->videocaptureview->available() ) {
        if(settings)
            a_timer->setVisible(false);
        camera->photo->setEnabled(false);
        camera->video->setEnabled(false);
        if (m_photoModel->rowCount() == 0) {
            thumb[0]->setEnabled(false);
        } else {
            thumb[0]->setFocus();
            thumb[0]->setEnabled(true);
        }
    }

    if(settings)
        updateActions();
    hideWaitScreen();
}

bool CameraMainWindow::delThumb(int th)
{
    switch(QMessageBox::warning(0, tr("Confirmation"),
            tr("<qt>Delete '%1'?</qt>", "%1 = file name").arg(picturefile[th].name()),
            QMessageBox::Yes,
            QMessageBox::No))
    {
        case QMessageBox::Yes:
            picturefile[th].removeFiles();

            for (int i = th; i < nthumb - 1; ++i ) {
                picturefile[i] = picturefile[i+1];
                thumb[i]->setIcon(thumb[i+1]->icon());
                thumb[i]->setEnabled(thumb[i+1]->isEnabled());
            }

            if (m_photoModel->rowCount() > nthumb) {
                picturefile[nthumb-1] = m_photoModel->content(nthumb);
                thumb[nthumb-1]->setIcon(QThumbnail(picturefile[nthumb-1].fileName()).pixmap(QSize(thumbw, thumbh)));
                thumb[nthumb-1]->setEnabled(true);
            } else {
                picturefile[nthumb-1] = QContent();
                thumb[nthumb-1]->setIcon(QIcon());
                thumb[nthumb-1]->setEnabled(false);
            }

            // Rhys Hack - if we have just deleted the last image and there
            // is no camera connected, then exit the application.  This
            // avoids a focus problem where it is impossible to exit with
            // the back button due to the focus being in a stupid place.
            if ( !camera->videocaptureview->available() &&
                 !(thumb[0]->isEnabled()) ) {
                close();
            }
            return true;
            break;
        default:
            //nothing
            break;
    }
    return false;
}

void CameraMainWindow::pushThumb(const QContent& f, const QImage& img)
{
    for (int i=nthumb; --i; ) {
        bool en = thumb[i-1]->isEnabled();
        thumb[i]->setEnabled(en);
        picturefile[i] = picturefile[i-1];
        QIcon icon = thumb[i-1]->icon();
        if ( en && !icon.isNull() ) {
            thumb[i]->setIcon(icon);
        } else {
            thumb[i]->setText("");
        }
    }
    thumb[0]->setIcon(QPixmap::fromImage(img.scaled(
            QSize( thumbw, thumbh ), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    thumb[0]->setEnabled(true);
    picturefile[0]=f;
}

void CameraMainWindow::takePhotoTimer()
{
    PhotoTimerDialog* dialog = new PhotoTimerDialog( this );
    if ( dialog->exec() == QDialog::Accepted ) {
        PhotoTimer* timer = new PhotoTimer( dialog->timeout(),
                                            dialog->number(),
                                            dialog->interval(),
                                            this );
        connect( timer, SIGNAL(takePhoto()), this, SLOT(takePhotoNow()) );
        timer->show();
    }

    delete dialog;
}

void CameraMainWindow::takePhoto()
{
    if (camera->photo != focusWidget())
    {
        camera->photo->setFocus();
        return;
    }
    QSize size = photo_size[psize];
    showWaitScreen();
    if ( size == camera->videocaptureview->captureSize() ||
        camera->videocaptureview->refocusDelay() == 0 )
    {

        // We can grab the current image immediately.
        takePhotoNow();

    } else {

        // Change the camera size and then wait for the camera to refocus.
        camera->videocaptureview->setCaptureSize( size );
        refocusTimer->start( camera->videocaptureview->refocusDelay() );

    }
}

void CameraMainWindow::takePhotoNow()
{
    QImage img = camera->videocaptureview->image();
    if ( snapRequest != 0 ) {
        // Rescale the image and pop it into a QDSData object
        QImage scaledimg = img.scaled( snap_max,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
        QByteArray savedImageData;
        {
            QDataStream stream( &savedImageData, QIODevice::WriteOnly );
            stream << QPixmap::fromImage( scaledimg );
        }
        QDSData snappedImage( savedImageData, QMimeType( "image/x-qpixmap" ) );

        // Send response with the data
        snapRequest->respond( snappedImage );

        // Reset snap mode
        setSnapMode( false );
        delete snapRequest;
        snapRequest = 0;

        // Finished serving QDS request so close the application
        close();
        hideWaitScreen();
    } else {
        showWaitScreen();
        QContent f;
        QList<QString> c;

        f.setType("image/jpeg");
        f.setName(tr("Photo, %1","date")
                .arg(QTimeString::localYMDHMS(QDateTime::currentDateTime(),QTimeString::Short)));
        f.setMedia( settings->location->documentPath() );

        c.append(camcat);
        f.setCategories(c);

        QIODevice*  contentDevice = f.open(QIODevice::WriteOnly);

        if (contentDevice != 0)
        {
            QImage  temp = img.convertToFormat(QImage::Format_RGB32);

            temp.save(contentDevice, "JPEG", pquality);

            contentDevice->close();

            f.commit();

            pushThumb(f, img);

            hideWaitScreen();

        }
        else
        {
            QString errorText = f.errorString();
            if (errorText.isEmpty())
                errorText = tr("Unknown error");

            QMessageBox::warning(0,
                                 tr("Error saving photo"),
                                 tr("Could not save photo: %1").arg(errorText));
        }
    }

    preview();
}

void CameraMainWindow::setSnapMode(bool snapMode)
{
    if (snapMode) {

        camera->thumbs->hide();
        camera->video->hide();

        // in snapshot mode, change back to "cancel taking a photo"
        QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Cancel);
    } else {

        camera->thumbs->show();

        if (video_supported)
            camera->video->show();
        else
            camera->video->hide();

        // normal back button
        QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Back);
    }
}

void CameraMainWindow::toggleVideo()
{
    if ( recording )
        stopVideo();
    else
        startVideo();
    recording = !recording;
    camera->video->setText(recording ? tr("Stop") : tr("Video"));
    camera->photo->setEnabled(!recording);
    for (int i=0; i<nthumb; i++)
        thumb[i]->setEnabled(!recording && !thumb[i]->icon().isNull());
}

void CameraMainWindow::startVideo()
{
    // XXX eg. MJPG
}
void CameraMainWindow::stopVideo()
{
}

void CameraMainWindow::thumbClicked(int i)
{
    selectThumb(i);
}

void CameraMainWindow::editThumb()
{
    picturefile[cur_thumb].execute();
}

void CameraMainWindow::selectThumb(int i)
{
    cur_thumb = i;
    if ( i >= 0 ) {
        QImage img( picturefile[i].fileName() );
        camera->videocaptureview->setStill(img);
        thumb[i]->setFocus();
        thumb[i]->setDown(false);
        QSoftMenuBar::setLabel(thumb[i], Qt::Key_Select, QString(""), QString(""));
    } else {
        camera->videocaptureview->setLive();
        QSoftMenuBar::setLabel(camera->thumbs, Qt::Key_Select, QSoftMenuBar::Select);
    }
    a_th_edit->setVisible(i>=0);
    a_th_del->setVisible(i>=0);
    a_th_add->setVisible(i>=0);
    a_send->setVisible(i>=0);
    a_zoom->setVisible(i<0);
}

void CameraMainWindow::moveToContact()
{
    /*if ( cur_thumb >= 0 ) {
        QtopiaServiceRequest e("Contacts","setContactImage(QString)");
        e << picturefile[cur_thumb].fileName();
        e.send();
    }*/

    if ( cur_thumb >= 0 ) {
        // Find a suitable QDS service
        QDSServices services( QString( "image/jpeg" ) );

        // Select the first service to create the action (assuming there
        // is only be one address book type application on the device)
        QDSAction action( services.findFirst( "setContactImage" ) );
        if ( !action.isValid() ) {
            qWarning( "Camera found no service to set the contact image" );
            return;
        }

        QFile pixFile(picturefile[cur_thumb].fileName());
        QDSData pixData(pixFile, QMimeType( "image/jpeg" ) );

        if ( action.exec( pixData ) != QDSAction::Complete ) {
            qWarning( "Camera unable to set contact image" );
            return;
        }
    }
}

void CameraMainWindow::delThumb()
{
    if ( cur_thumb >= 0 ) {
        int d = cur_thumb;
        bool ret = delThumb(d);
        if ( cur_thumb > 0 && ret )
            selectThumb(cur_thumb-1);
    }
}

void CameraMainWindow::getImage( const QDSActionRequest& request )
{
    if ( !request.isValid() ) {
        qWarning( "Camera: received invalid QDS request" );
        return;
    }

    if ( snapRequest != 0 ) {
        qWarning( "Camera: already processing another QDS request" );
        return;
    }

    // Read snap parameters from request
    QDataStream stream( request.requestData().toIODevice() );
    stream >> snap_max;

    // Set the camera for snap mode
    snapRequest = new QDSActionRequest( request );
    setSnapMode( true );
    showMaximized();
}

void CameraMainWindow::preview()
{
    if ( camera->videocaptureview->refocusDelay() > 200 && psize >= 0) {
        camera->videocaptureview->setCaptureSize( photo_size[psize] );
    } else {
        camera->videocaptureview->setCaptureSize( camera->videocaptureview->recommendedPreviewSize() );
    }
}

void CameraMainWindow::sendFile()
{
    if ( cur_thumb >= 0) {
        //copy file
        QFile input(picturefile[cur_thumb].fileName());
        if(!input.open(QIODevice::ReadOnly)){
            return; //error
        }
        QFile output(picfile);
        if(!output.open(QIODevice::WriteOnly)){
            return;
        }

        const int BUFFER_SIZE = 1024;
        qint8 buffer[BUFFER_SIZE];

        QDataStream srcStr(&input);
        QDataStream destStr(&output);

        while(!srcStr.atEnd()) {
            int i = 0;
            while(!srcStr.atEnd() && i < BUFFER_SIZE){
                srcStr >> buffer[i];
                i++;
            }
            for(int k = 0; k < i; k++) {
                destStr << buffer[k];
            }
        }

        QtopiaServiceRequest e("Email","writeMessage(QString,QString,QStringList,QStringList)");
        e << QString() << QString() << QStringList() << QStringList( QString( picfile ) );
        e.send();
    }
}

void CameraMainWindow::videoToScreen(int screen)
{
    QDesktopWidget *desktop = QApplication::desktop();
    if (desktop->screenNumber(camera->videocaptureview) == screen)
        return;

    if (screen == desktop->primaryScreen()) {
        videoOnSecondary = false;
        camera->videocaptureview->hide();
        camera->videocaptureview->setParent(camera->videocapturecontainer);
        camera->videocapturecontainer->layout()->addWidget(camera->videocaptureview);
        camera->videocaptureview->show();
    } else {
        videoOnSecondary = true;
        camera->videocaptureview->setFocusPolicy(Qt::NoFocus);
        camera->videocaptureview->setParent(0, Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        camera->videocaptureview->setGeometry(desktop->screenGeometry(screen));
        camera->videocaptureview->show();
    }
}

void CameraMainWindow::clamshellChanged()
{
    if (clamshellVsi->value().toBool()) {
        videoToScreen(QApplication::desktop()->primaryScreen());
    } else {
        videoToScreen(1);
    }
}

void CameraMainWindow::contextMenuAboutToShow()
{
    m_contextMenuActive = true;
}

void CameraMainWindow::contextMenuAboutToHide()
{
    m_contextMenuActive = false;
}

void ThumbButton::drawButtonLabel( QPainter *p )
{
    QIcon ic = icon();
    if ( !ic.isNull() ) {
        ic.paint( p, rect() );
    }
}

void CameraMainWindow::showWaitScreen(const QString &descr)
{
    m_wait->setText(descr);
    if(m_iswaiting)
    {
        return;
    }
    m_iswaiting = true;
    m_wait->show();
}

void CameraMainWindow::hideWaitScreen()
{
    m_wait->hide();
    m_iswaiting = false;
}

ThumbButton::ThumbButton( QWidget *parent ) : QToolButton(parent)
{
}

/*!
    \service CameraService Camera
    \brief Provides the Qtopia Camera service.

    The \i Camera service enables applications to access features of
    the Camera application.
*/

/*!
    \internal
*/
CameraService::~CameraService()
{
}

/*!
    Instructs the camera to take a photo of the dimensions provided in \a request.

    This slot corresponds to a QDS service with a request data type of
    "x-size/x-qsize" and a response data type of "image/x-qpixmap".

    This slot corresponds to the QCop service message
    \c{Camera::getImage(QDSActionRequest)}.
*/
void CameraService::getImage( const QDSActionRequest& request )
{
    parent->getImage( request );
}

/*!
  Instructs the camera to activate the shutter.  If the application isn't visible, this
  raise the camera and puts it into photo mode.  Otherwise it takes a photo.
*/

void CameraService::shutter()
{
    if (parent->isVisible() && parent->isActiveWindow())
        parent->takePhoto();
    else {
        parent->showMaximized();
        parent->raise();
        parent->activateWindow();
    }
}


