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

#include "phoneheader.h"
#include "inputmethods.h"
#include <QTimer>
#include "windowmanagement.h"
#include "qtopiaserverapplication.h"
#include <qscreen_qws.h>

/*!
  \class PhoneHeader
  \ingroup QtopiaServer::PhoneUI
  \brief The PhoneHeader class provides a dockable, themeable phone header.

  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*!
  Create a new phone header with the specified \a parent.
 */
PhoneHeader::PhoneHeader(QWidget *parent)
    : PhoneThemedView(parent, Qt::FramelessWindowHint | Qt::Tool |
                              Qt::WindowStaysOnTopHint)
{
    setWindowTitle("_decoration_");
    inputMethods = new InputMethods(this, InputMethods::Any);
}

/*! \internal */
void PhoneHeader::themeLoaded(const QString &)
{
    QTimer::singleShot(0, this, SLOT(updateIM()));
}

/*! \internal */
void PhoneHeader::updateIM()
{
    QList<ThemeItem*> items = findItems("inputmethod", ThemedView::Widget);
    QList<ThemeItem*>::ConstIterator it;
    for (it = items.begin(); it != items.end(); ++it) {
        ThemeWidgetItem *item = (ThemeWidgetItem *)(*it);
        item->setAutoDelete(false);
        item->setWidget( inputMethods );
    }
}

/*! \internal */
QSize PhoneHeader::reservedSize() const
{
    int rh = -1;
    ThemeItem *reserved = ((ThemedView *)this)->findItem("reserved", Item);
    if (reserved)
        rh = reserved->rect().height();

    return QSize(sizeHint().width(), rh);
}

