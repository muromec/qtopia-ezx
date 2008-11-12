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

#ifndef _DOCUMENTVIEW_H_
#define _DOCUMENTVIEW_H_

#include "launcherview.h"

class QContentFilterDialog;
class QLabeledProgressBar;
class QValueSpaceItem;
class QLabel;
class QTextEntryProxy;
class QHBoxLayout;

class DocumentLauncherView : public LauncherView
{
    Q_OBJECT
    public:
        DocumentLauncherView(QWidget* parent = 0, Qt::WFlags fl = 0);
        virtual ~DocumentLauncherView();

        virtual void setFilter( const QContentFilter &filter );

    private slots:
        void launcherRightPressed(QContent);
        void beamDoc();
        void deleteDocWorker();
        void deleteDoc(int r);
        void deleteDoc();
        void propertiesDoc();
        void openRightsIssuerURL();
        void selectDocsType();
        void selectDocsCategory();
        void updateScanningStatus();
        void currentChanged( const QModelIndex &current, const QModelIndex &previous );
        void textEntrytextChanged(const QString &);
        void rescan();

    private:
        QLabel *typeLbl;
        QAction *actionDelete;
        QAction *actionProps;
        QContent deleteLnk;
        QAbstractMessageBox *deleteMsg;
        QDialog *propDlg;
        QContent propLnk;
        QMenu *rightMenu;
        QAction *actionBeam;
        QAction *actionRightsIssuer;
        QContentFilterDialog *typeDlg;
        QLabel *categoryLbl;
        QContentFilterDialog *categoryDlg;
        QLabeledProgressBar *scanningBar;
        QTimer *scanningBarUpdateTimer;
        QValueSpaceItem *scanningVSItem;
        QValueSpaceItem *updatingVSItem;
        QValueSpaceItem *installingVSItem;
        
    protected:
        QMenu *softMenu;
        QTextEntryProxy *textEntry;
        QLabel *findIcon;
        QHBoxLayout *findLayout;

    public:
        QAction *separatorAction;
        
    private:
        void init();
};

#endif // _DOCUMENTVIEW_H_
