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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qpixmap.h>
#include "ui_camerabase.h"
#include "ui_camerasettings.h"
#include <qmainwindow.h>
#include <qcontent.h>
#include <QDSActionRequest>
#include <QtopiaAbstractService>
#include <QContentSet>
#include <QTimer>

class QAction;
class QValueSpaceItem;
class QWaitWidget;
class QSlider;

class CameraMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CameraMainWindow( QWidget *parent=0, Qt::WFlags fl=0 );
    ~CameraMainWindow();

public slots:
    void takePhoto();
    void toggleVideo();
    void selectThumb(int i);
    void thumbClicked(int i);
    void getImage( const QDSActionRequest& request );

private slots:
    void viewPictures();
    void viewVideos();
    void doSettings();
    void editThumb();
    void delThumb();
    void moveToContact();
    void takePhotoNow();
    void takePhotoTimer();
    void sendFile();
    void clamshellChanged();
    void contextMenuAboutToShow();
    void contextMenuAboutToHide();
    void loadThumbs( bool resized = false );
    void delayedInit();
    void zoomChanged(int);
    void showZoom();
    void hideZoom();
private:
    bool event(QEvent* e);
    void updateActions();
    void resizeEvent(QResizeEvent*);

    bool eventFilter(QObject*, QEvent*);
    QString nextFileName();

    void pushThumb(const QContent& f, const QImage& img);
    static const int nthumb = 5;
    QToolButton* thumb[nthumb];
    QContent picturefile[nthumb];
    int cur_thumb;
    bool delThumb(int th);

    // Settings
    void confirmSettings();
    Ui::CameraSettings *settings;
    QDialog *settingsDialog;
    QString storagepath;
    QString media;
    int thumbw;
    int thumbh;
    int psize;
    int vsize;
    int pquality;
    int vquality;
    int vframerate;
    int m_currzoom;
    bool zoomActive;
    QSlider* m_zoom;
    QTimer zoomTimer;
    // Snap
    QSize snap_max;
    void setSnapMode( bool snapMode );

    Ui::CameraBase *camera;

    int namehint;
    QAction *a_pview, *a_vview, *a_timer, *a_settings;
    QAction *a_th_edit, *a_th_del, *a_th_add;
    QAction *a_send;
    QAction *a_zoom;
    QList<QSize> photo_size;
    QList<QSize> video_size;

    QTimer *refocusTimer;
    QString picfile;

    QDSActionRequest* snapRequest;

    bool recording;
    void stopVideo();
    void startVideo();

    void preview();

    void videoToScreen(int screen);
    bool videoOnSecondary;
    QValueSpaceItem *clamshellVsi;

    QContentSet m_photoContentSet;
    QContentSetModel *m_photoModel;
    QString camcat;
    bool    m_contextMenuActive;
    QWaitWidget *m_wait;
    bool m_iswaiting;
    void showWaitScreen(const QString& s = "");
    void hideWaitScreen();
};

class CameraService : public QtopiaAbstractService
{
    Q_OBJECT
    friend class CameraMainWindow;
private:
    CameraService( CameraMainWindow *parent )
        : QtopiaAbstractService( "Camera", parent )
        { this->parent = parent; publishAll(); }

public:
    ~CameraService();

public slots:
    void getImage( const QDSActionRequest& request );
    void shutter();

private:
    CameraMainWindow *parent;
};


#endif

