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

#ifndef __FINDWIDGET_H__
#define __FINDWIDGET_H__

#include "ui_findwidgetbase_p.h"

#include <qdatetime.h>

class QtopiaCalendarWidget;

class FindWidget : public QWidget, public Ui::FindWidgetBase
{
    Q_OBJECT
public:
    FindWidget( const QString &appName, QWidget *parent = 0 );
    ~FindWidget();

    QString findText() const;
    void setUseDate( bool show );

public slots:
    void slotNotFound();
    void slotWrapAround();
    void setDate( const QDate &dt );

signals:
    void signalFindClicked( const QString &txt, bool caseSensitive,
                            bool backwards, const QCategoryFilter & category );
    void signalFindClicked( const QString &txt, const QDate &dt,
                            bool caseSensitive, bool backwards, const QCategoryFilter & category );

private slots:
    void slotFindClicked();

private:
    QString mStrApp;
    QtopiaCalendarWidget *dtPicker;
    QDate mDate;
};

#endif
