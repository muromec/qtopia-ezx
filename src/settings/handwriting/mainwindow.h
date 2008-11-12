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

#ifndef QIMPENMAINWINDOW_H
#define QIMPENMAINWINDOW_H

#include <QDialog>
#include <QList>
#include <qtopia/mstroke/profile.h>

class QIMPenProfile;
class CharSetDlg;
class QDialog;
class GeneralPref;
class QTreeWidget;
class QTreeWidgetItem;

class QIMPenProfileEdit : public QDialog
{
    Q_OBJECT
public:
    QIMPenProfileEdit(QWidget *parent, Qt::WFlags f = 0);
    ~QIMPenProfileEdit();

private slots:
    void editItem(QTreeWidgetItem *);

private:
    bool loadProfiles();
    bool saveProfiles();

    QList<QIMPenProfile *> profileList;

    QTreeWidget *lv;

    CharSetDlg *cdiag;
    QDialog *gdiag;
    GeneralPref *gpb;
};

#endif
