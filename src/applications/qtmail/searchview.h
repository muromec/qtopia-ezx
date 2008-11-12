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



#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QLayout>
#include <QTextEdit>
#include <QCheckBox>
#include <QDateTime>
#include <QGroupBox>
#include <QToolButton>
#include <QScrollArea>

#include "ui_searchviewbasephone.h"
#include "search.h"
#include "account.h"
#include "emaillistitem.h"

class SearchView : public QDialog, public Ui::SearchViewBase
{
    Q_OBJECT
public:
    SearchView(bool query, QWidget* parent = 0, Qt::WFlags = 0);
    ~SearchView();
    Search *getSearch();
    void setSearch(Search *in);
    void setQueryBox(QString box);

public slots:
    void dateAfterChanged( const QDate & );
    void dateBeforeChanged( const QDate & );

private:
    void init();

private:
    QDate dateBefore, dateAfter;

    QScrollArea *sv;

    bool queryType;
};

#endif
