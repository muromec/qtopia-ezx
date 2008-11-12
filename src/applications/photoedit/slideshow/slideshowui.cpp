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

#include "slideshowui.h"

#include <qthumbnail.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qfontmetrics.h>
#include <QStyleOption>

#include <QKeyEvent>

SlideShowUI::SlideShowUI( QWidget* parent, Qt::WFlags f )
    : QWidget( parent, f ), display_name( false )
{ }

void SlideShowUI::setImage( const QContent& lnk )
{
    // Clear buffer, update image, update fitted name and update display
    image_buffer = QPixmap();
    image = lnk;
    update();
}

void SlideShowUI::paintEvent( QPaintEvent* )
{
#define NAME_POSX 2
#define NAME_POSY 2
#define NAME_COLOR QColor( 162, 190, 0 )
#define SHADOW_OFFSET 1

    // If image not loaded, load image into buffer
    if( image_buffer.isNull() && !image.fileName().isNull() ) {
        // Scale image to fit within the current widget dimensions
        // while keeping the original width:height ratio
        // Load scaled image to buffer
        QThumbnail thumbnail( image.fileName() );
        image_buffer = thumbnail.pixmap( size() );
        // Update image position
        image_position.setX( ( width() - image_buffer.width() ) / 2 );
        image_position.setY( ( height() - image_buffer.height() ) / 2 );
    }

    QPainter painter( this );

    // If image has transparency, fill background and draw image
    // Otherwise, draw image and fill remaining background
    if( image_buffer.hasAlpha() ) {
        painter.fillRect( rect(), Qt::black );
        painter.drawPixmap( image_position, image_buffer );
    } else {
        painter.drawPixmap( image_position, image_buffer );
        QRect region( image_buffer.rect() );
        region.translate( image_position.x(), image_position.y() );
        painter.setClipRegion( QRegion( rect() ).subtract( region ) );
        painter.setClipping( true );
        painter.fillRect( rect(), Qt::black );
        painter.setClipping( false );
    }

    // If display name, draw fitted name onto widget
    if( display_name ) {
        QStyleOption style;
        style.initFrom( this );

        QString name = image.name();

        int x = style.direction == Qt::LeftToRight
                ? NAME_POSX + SHADOW_OFFSET
                : 0;
        int y = NAME_POSY + SHADOW_OFFSET;
        int w = width() - NAME_POSX - SHADOW_OFFSET;
        int h = height() - y;

        QRect rect( x, y, w, h );

        QString elidedName = style.fontMetrics.elidedText( name, Qt::ElideRight, w );

        // Draw shadow
        painter.setPen( Qt::black );
        painter.drawText( rect, elidedName );
        // Draw fitted name
        painter.setPen( NAME_COLOR );
        painter.drawText( rect.translated( -SHADOW_OFFSET, -SHADOW_OFFSET ), elidedName );
    }
}

void SlideShowUI::resizeEvent( QResizeEvent* )
{
    // Update image position
    image_position.setX( ( width() - image_buffer.width() ) / 2 );
    image_position.setY( ( height() - image_buffer.height() ) / 2 );
}

void SlideShowUI::keyPressEvent( QKeyEvent* e )
{
    if( e->key() == Qt::Key_Back ) {
        emit pressed();
        e->accept();
    }
}

void SlideShowUI::mousePressEvent( QMouseEvent* )
{
    emit pressed();
}
