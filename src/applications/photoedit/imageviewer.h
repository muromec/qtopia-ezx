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
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QAbstractScrollArea>

class QContent;
class ImageViewerPrivate;

class ImageViewer : public QAbstractScrollArea
{
    Q_OBJECT
public:

    enum ScaleMode
    {
        FixedScale,
        ScaleToFit,
        ScaleRotateToFit
    };

    ImageViewer( QWidget *parent = 0 );
    virtual ~ImageViewer();

    QContent content() const;
    QByteArray format() const;
    QSize imageSize() const;
    bool canView() const;

    ScaleMode scaleMode() const;
    void setScaleMode( ScaleMode mode );

    qreal rotation() const;
    qreal scaleX() const;
    qreal scaleY() const;
    QSize scaledSize() const;
    QSize transformedSize() const;

signals:
    void imageInvalidated();
    void imageChanged();

public slots:
    void setScale( qreal sx, qreal sy );
    void setRotation( qreal rotation );
    void setContent( const QContent &content );

protected:
    void paintEvent( QPaintEvent *event );
    void resizeEvent( QResizeEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    void scrollContentsBy( int dx, int dy );

private slots:
    void licenseExpired();
    void imageLoaded(const QContent &content, const QList<QImage> &images, const QSize &size, qreal prescaling);

private:
    ImageViewerPrivate *d;
};

#endif // IMAGEVIEWER_H
