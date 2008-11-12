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
#include "imageviewer.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QPainter>
#include <QImageReader>
#include <QPaintEvent>
#include <QScrollBar>
#include <QtDebug>
#include <QDrmContent>

Q_DECLARE_METATYPE(QList<QImage>);

class ImageViewerLoader : public QObject
{
    Q_OBJECT
public:
    ImageViewerLoader();

public slots:
    void setContent( const QContent &content );

signals:
    void imageLoaded(const QContent &content, const QList<QImage> &images, const QSize &size, qreal prescaling);
};

class ImageViewerPrivate : public QThread
{
    Q_OBJECT
public:

    ImageViewerPrivate( ImageViewer *viewer );

    ImageViewer *q;
    QContent content;
    QDrmContent drmContent;
    QByteArray format;
    QSize size;
    ImageViewer::ScaleMode scaleMode;
    qreal prescaling;
    qreal scaleX;
    qreal scaleY;
    qreal rotation;
    QSize scaledSize;
    QSize transformedSize;
    QRect screenRect;
    QList<QImage> images;

    QPoint lastMousePos;
    QMutex syncMutex;
    QWaitCondition syncCondition;

    void calculateScale();
    void calculateTransform();

signals:
    void setContent( const QContent &content );

protected:
    void run();

    friend class ImageViewer;
};

ImageViewerLoader::ImageViewerLoader()
{
}

void ImageViewerLoader::setContent( const QContent &content )
{
    static const int maxArea = 2304000;

    QIODevice *device = 0;
    QImageReader reader;

    int area = 0;

    QSize size;
    qreal prescaling = 1.0;
    bool canView = false;

    QDrmContent drmContent(QDrmRights::Display, QDrmContent::NoLicenseOptions);

    if(!content.isNull() && drmContent.requestLicense(content) && (device = content.open()) != 0) {
        reader.setDevice( device );

        size = reader.size();
        area = size.width() * size.height();

        canView  = reader.canRead()
                && area <= maxArea || reader.supportsOption(QImageIOHandler::ScaledSize);
    }

    while (canView && area > maxArea) {
        prescaling /= 2.0;

        area = qRound(area * prescaling * prescaling);
    }

    QImage image;

    if (canView) {
        if (prescaling < 1.0)
            reader.setScaledSize(size * prescaling);

        reader.read(&image);
    }

    if (device)
        device->close();

    delete device;

    QList<QImage> images;

    if (!image.isNull()) {
        images.append(image);

        while (qMax(image.width(), image.height()) > 120
            && qMin(image.width(), image.height()) > 10) {
            image = image.scaled(image.size() / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            if (!image.isNull()) {
                images.append(image);
            }
        }
    }

    emit imageLoaded(content, images, size, prescaling);
}

ImageViewerPrivate::ImageViewerPrivate( ImageViewer *viewer )
    : q( viewer )
    , drmContent(QDrmRights::Display, QDrmContent::Reactivate)
    , scaleMode( ImageViewer::ScaleToFit )
    , prescaling( 1.0 )
    , scaleX( 1.0 )
    , scaleY( 1.0 )
    , rotation( 0.0 )
{
}

void ImageViewerPrivate::calculateScale()
{
    if( size.isValid() )
    {
        QSize bestFitSize = size;
        bestFitSize.scale( q->size(), Qt::KeepAspectRatio );

        qreal scale = qMin( qreal(1), qreal(bestFitSize.width()) / size.width() );

        rotation = 0.0;

        if( scaleMode == ImageViewer::ScaleRotateToFit )
        {
            bestFitSize = size;
            bestFitSize.transpose();
            bestFitSize.scale( q->size(), Qt::KeepAspectRatio );

            qreal rotatedScale = qMin( qreal(1), qreal(bestFitSize.width()) / size.height() );

            if( rotatedScale > scale )
            {
                scale = rotatedScale;

                rotation = -90.0;
            }
        }

        scaleX = scale;
        scaleY = scale;
    }
    else
    {
        scaleX   = 1.0;
        scaleY   = 1.0;
        rotation = 0.0;
    }

    calculateTransform();
}

void ImageViewerPrivate::calculateTransform()
{
    QRect imageRect( QPoint( 0, 0 ), size );

    QTransform transform;

    transform.scale( scaleX, scaleY );

    scaledSize = transform.mapRect( imageRect ).size();

    transform.rotate( rotation );

    QSize oldTransformedSize = transformedSize;

    transformedSize = transform.mapRect( imageRect ).size();

    QScrollBar *hScroll = q->horizontalScrollBar();
    QScrollBar *vScroll = q->verticalScrollBar();

    int hValue = hScroll->value();
    int vValue = vScroll->value();

    QSize dSize = transformedSize - oldTransformedSize;

    hValue += dSize.width() / 2;
    vValue += dSize.height() / 2;

    dSize = transformedSize - q->size();

    hScroll->setRange( 0, dSize.width() );
    vScroll->setRange( 0, dSize.height() );
    hScroll->setValue( hValue );
    vScroll->setValue( vValue );
}

void ImageViewerPrivate::run()
{
    ImageViewerLoader loader;

    connect( this,  SIGNAL(setContent(QContent)),
             &loader, SLOT(setContent(QContent)) );

    connect( &loader, SIGNAL(imageLoaded(QContent,QList<QImage>,QSize,qreal)),
             q,         SLOT(imageLoaded(QContent,QList<QImage>,QSize,qreal)) );

    {
        QMutexLocker locker( &syncMutex );

        syncCondition.wakeAll();
    }

    exec();
}



ImageViewer::ImageViewer( QWidget *parent )
    : QAbstractScrollArea( parent )
    , d( new ImageViewerPrivate( this ) )
{
    static const int qImageListMetaId = qRegisterMetaType<QList<QImage> >();
    Q_UNUSED(qImageListMetaId);

    d->drmContent.setFocusWidget( this );

    connect( &d->drmContent, SIGNAL(rightsExpired(QDrmContent)), this, SLOT(licenseExpired()) );

    setMinimumSize( 32, 32 );
    setFrameStyle( QFrame::NoFrame );

    horizontalScrollBar()->setSingleStep( 10 );
    verticalScrollBar()->setSingleStep( 10 );

    QMutexLocker locker( &d->syncMutex );

    d->start();

    d->syncCondition.wait( &d->syncMutex );
}

ImageViewer::~ImageViewer()
{
    d->quit();

    d->wait();

    delete d;
}

void ImageViewer::setContent( const QContent &content )
{
    d->images.clear();

    QDrmContent drmContent(QDrmRights::Display, QDrmContent::Activate);

    if (!content.isNull() && drmContent.requestLicense(content))
        d->setContent(content);
    else
        d->setContent(QContent());
}

QContent ImageViewer::content() const
{
    return d->content;
}

QByteArray ImageViewer::format() const
{
    return d->format;
}

QSize ImageViewer::imageSize() const
{
    return d->size;
}

qreal ImageViewer::scaleX() const
{
    return d->scaleX;
}

qreal ImageViewer::scaleY() const
{
    return d->scaleY;
}

QSize ImageViewer::scaledSize() const
{
    return d->scaledSize;
}

QSize ImageViewer::transformedSize() const
{
    return d->transformedSize;
}

void ImageViewer::setScale( qreal sx, qreal sy )
{
    d->scaleMode = FixedScale;

    if( sx != d->scaleX && d->scaleY != sy  )
    {
        d->scaleX = sx;
        d->scaleY = sy;

        d->calculateTransform();
    }

    viewport()->update();
}

qreal ImageViewer::rotation() const
{
    return d->rotation;
}

void ImageViewer::setRotation( qreal rotation )
{
    d->scaleMode = FixedScale;

    if( rotation != d->rotation )
    {
        d->rotation = rotation;

       d->calculateTransform();

        viewport()->update();
    }
}

ImageViewer::ScaleMode ImageViewer::scaleMode() const
{
    return d->scaleMode;
}

void ImageViewer::setScaleMode( ScaleMode mode )
{
    d->scaleMode = mode;

    if( mode != FixedScale )
    {
        d->calculateScale();

        viewport()->update();
    }
}

void ImageViewer::paintEvent( QPaintEvent *event )
{
    QImage image;

    for (int i = d->images.count() - 1;
        i >= 0
        && (image.height() < d->scaledSize.height() || image.width() < d->scaledSize.width());
        --i) {
        image = d->images.at(i);
    }

    if (!image.isNull()) {
        QPainter painter( viewport() );

        painter.setClipRegion( event->region() );

        QTransform transform;

        transform.translate( -horizontalScrollBar()->value() + qMax( d->transformedSize.width(),  width() )  / 2,
                             -verticalScrollBar()->value()   + qMax( d->transformedSize.height(), height() ) / 2 );
        transform.rotate( d->rotation );
        transform.translate( -d->scaledSize.width() / 2, -d->scaledSize.height() / 2 );

        painter.setWorldTransform( transform, true );

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(QRect(QPoint(0,0), d->scaledSize), image);

        event->accept();
    } else {
        QAbstractScrollArea::paintEvent( event );
    }
}

void ImageViewer::resizeEvent( QResizeEvent *event )
{
    QScrollBar *hScroll = horizontalScrollBar();
    QScrollBar *vScroll = verticalScrollBar();

    int hValue = hScroll->value();
    int vValue = vScroll->value();

    QSize dSize = d->transformedSize - event->size();

    hScroll->setRange( 0, dSize.width() );
    vScroll->setRange( 0, dSize.height() );

    dSize = event->size() - event->oldSize();

    hScroll->setValue( hValue + dSize.width()  / 2 );
    vScroll->setValue( vValue + dSize.height() / 2 );

    hScroll->setPageStep( width()  );
    vScroll->setPageStep( height() );

    if( d->scaleMode != FixedScale )
        d->calculateScale();

    QAbstractScrollArea::resizeEvent( event );
}

void ImageViewer::mousePressEvent( QMouseEvent *event )
{ 
    QAbstractScrollArea::mousePressEvent( event );

    if( event->button() == Qt::LeftButton )
        d->lastMousePos = event->pos();
}

void ImageViewer::mouseMoveEvent( QMouseEvent *event )
{
    QAbstractScrollArea::mouseMoveEvent( event );

    if( !d->lastMousePos.isNull() )
    {
        QPoint dPos = event->pos() - d->lastMousePos;

        QScrollBar *hScroll = horizontalScrollBar();
        QScrollBar *vScroll = verticalScrollBar();

        hScroll->setValue( hScroll->value() - dPos.x() );
        vScroll->setValue( vScroll->value() - dPos.y() );

        d->lastMousePos = event->pos();
    }
}

void ImageViewer::mouseReleaseEvent( QMouseEvent *event )
{
    QAbstractScrollArea::mouseReleaseEvent( event );

    if( event->button() == Qt::LeftButton )
        d->lastMousePos = QPoint();
}

void ImageViewer::scrollContentsBy( int dx, int dy )
{
    QAbstractScrollArea::scrollContentsBy( dx, dy );
}

void ImageViewer::licenseExpired()
{
    setContent( QContent() );

    d->images.clear();

    emit imageInvalidated();
}

void ImageViewer::imageLoaded(const QContent &content, const QList<QImage> &images, const QSize &size, qreal prescaling)
{
    d->content = content;
    d->images = images;
    d->size = size;
    d->prescaling = prescaling;

    if (d->drmContent.requestLicense(content)) {
        d->drmContent.renderStarted();

        emit imageChanged();
    } else {
        licenseExpired();
    }

    viewport()->update();
}

#include "imageviewer.moc"
