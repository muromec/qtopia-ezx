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

#include "documentview.h"
#include <QtopiaSendVia>
#include <QDialog>
#include <QSoftMenuBar>
#include <QDocumentPropertiesWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QDrmContent>
#include <QtopiaApplication>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QCategoryDialog>
#include <QProgressBar>
#include <QValueSpaceItem>
#include "qabstractmessagebox.h"
#include <qtopiaservices.h>
#include <QContentFilterDialog>
#include <QTextEntryProxy>

////////////////////////////////////////////////////////////////
//
// DocumentLauncherView implementationrescan


class QLabeledProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    QLabeledProgressBar(QWidget *parent=0) : QProgressBar(parent) {};
    virtual ~QLabeledProgressBar() {};
    virtual QString text () const {return labelText;};
    virtual void setText(const QString &label) {labelText=label;};
private:
    QString labelText;
};

////////////////////////////////////////////////////////////////
//
// DocumentLauncherView implementation

DocumentLauncherView::DocumentLauncherView(QWidget* parent, Qt::WFlags fl)
    : LauncherView(parent, fl), typeLbl(0), actionDelete(0), actionProps(0),
    deleteMsg(0), propDlg(0), rightMenu(0), actionBeam(0), actionRightsIssuer(0),
    typeDlg(0), categoryLbl(0), categoryDlg(0)
{
    init();
}

void DocumentLauncherView::init() {
    softMenu = new QMenu(this);
    
    QSoftMenuBar::addMenuTo(this, softMenu);
    
    rightMenu = new QMenu(this);

    actionProps = new QAction( QIcon(":icon/info"), tr("Properties..."), this );
    actionProps->setEnabled(false);
    QObject::connect(actionProps, SIGNAL(triggered()),
                     this, SLOT(propertiesDoc()));
    softMenu->addAction(actionProps);
    rightMenu->addAction(actionProps);

    if (QtopiaSendVia::isFileSupported()) {
        actionBeam = new QAction( QIcon(":icon/beam"), tr("Send"), this );
        actionBeam->setEnabled( false );
        QObject::connect(actionBeam, SIGNAL(triggered()), this, SLOT(beamDoc()));
        softMenu->addAction(actionBeam);
        rightMenu->addAction(actionBeam);
    }

    actionDelete = new QAction( QIcon(":icon/trash"), tr("Delete..."), this );
    actionDelete->setEnabled(false);
    connect(actionDelete, SIGNAL(triggered()), this, SLOT(deleteDoc()));
    softMenu->addAction(actionDelete);
    rightMenu->addAction(actionDelete);

    actionRightsIssuer = new QAction( QIcon( ":image/drm/Drm" ), "Get license", this );
    QObject::connect(actionRightsIssuer, SIGNAL(triggered()),
                     this, SLOT(openRightsIssuerURL()) );
    actionRightsIssuer->setVisible( false );
    softMenu->addAction(actionRightsIssuer);
    rightMenu->addAction(actionRightsIssuer);

    separatorAction = softMenu->addSeparator();
    separatorAction->setEnabled(false);

    QAction *a = new QAction( tr("View Type..."), this );
    connect(a, SIGNAL(triggered()), this, SLOT(selectDocsType()));
    softMenu->addAction(a);

    a = new QAction( QIcon(":icon/viewcategory"), tr("View Category..."), this );
    connect(a, SIGNAL(triggered()), this, SLOT(selectDocsCategory()));
    softMenu->addAction(a);

    // due to string freeze, I cant' actually add this until 4.4
    // The code is kept here in case any willing adventurer sees fit to turn it on
    /*a = new QAction( tr("Rescan System"), this );
    connect(a, SIGNAL(triggered()), this, SLOT(rescan()));
    softMenu->addAction(a);*/

    typeLbl = new QLabel(this);
    layout()->addWidget(typeLbl);
    typeLbl->hide();

    categoryLbl = new QLabel(this);
    layout()->addWidget(categoryLbl);
    categoryLbl->hide();// TODO: ifdef

    scanningBar = new QLabeledProgressBar(this);
    layout()->addWidget(scanningBar);
    scanningBar->setText(tr("Scanning", "Scanner is searching for documents"));
    scanningBar->setMinimum(0);
    scanningBar->setMaximum(10);
    scanningBarUpdateTimer = new QTimer(this);
    scanningBarUpdateTimer->setInterval(1500);
    scanningBarUpdateTimer->setSingleShot(false);
    scanningVSItem=new QValueSpaceItem("/Documents/Scanning", this);
    updatingVSItem=new QValueSpaceItem("/Documents/Updating", this);
    installingVSItem=new QValueSpaceItem("/Documents/Installing", this);
    connect(scanningVSItem, SIGNAL(contentsChanged()), this, SLOT(updateScanningStatus()));
    connect(updatingVSItem, SIGNAL(contentsChanged()), this, SLOT(updateScanningStatus()));
    connect(installingVSItem, SIGNAL(contentsChanged()), this, SLOT(updateScanningStatus()));
    connect(scanningBarUpdateTimer, SIGNAL(timeout()), this, SLOT(updateScanningStatus()));
    updateScanningStatus();

    connect(this, SIGNAL(rightPressed(QContent)),
            this, SLOT(launcherRightPressed(QContent)));

    connect( icons, SIGNAL(currentIndexChanged(QModelIndex,QModelIndex)),
             this, SLOT(currentChanged(QModelIndex,QModelIndex)) );

    QContentFilter filter( QContent::Document );

    setFilter(filter);
    setViewMode(QListView::ListMode);
    if (!style()->inherits("QThumbStyle")) {
        textEntry = new QTextEntryProxy(this, icons);

        int mFindHeight = textEntry->sizeHint().height();
        findIcon = new QLabel;
        findIcon->setPixmap(QIcon(":icon/find").pixmap(mFindHeight-2, mFindHeight-2));
        findIcon->setMargin(2);
        findIcon->setFocusPolicy(Qt::NoFocus);

        findLayout = new QHBoxLayout;
        findLayout->addWidget(findIcon);
        findLayout->addWidget(textEntry);
        qobject_cast<QVBoxLayout*>(layout())->addLayout(findLayout);
        textEntry->hide();
        findIcon->hide();

        connect(textEntry, SIGNAL(textChanged(QString)), this, SLOT(textEntrytextChanged(QString)));
        QtopiaApplication::setInputMethodHint(icons, "text");
        icons->setAttribute(Qt::WA_InputMethodEnabled);
    }
    QSoftMenuBar::setLabel(this, Qt::Key_Select, QSoftMenuBar::NoLabel);
}

DocumentLauncherView::~DocumentLauncherView()
{
}

void DocumentLauncherView::setFilter( const QContentFilter &filter )
{
    LauncherView::setFilter( filter );

    if( typeDlg )
        typeDlg->setFilter( filter );

    if( categoryDlg )
        categoryDlg->setFilter( filter );
}

void DocumentLauncherView::launcherRightPressed(QContent lnk)
{
    if(lnk.id() != QContent::InvalidId && lnk.isValid())
        rightMenu->popup(QCursor::pos());
}

void DocumentLauncherView::beamDoc()
{
    const QContent &doc = currentItem();
    if (doc.id() != QContent::InvalidId && doc.isValid()) {
        QtopiaSendVia::sendFile(0, doc);
    }
}

void DocumentLauncherView::deleteDocWorker()
{
    deleteLnk.removeFiles();
    if (QFile::exists(deleteLnk.fileName())) {
        if(deleteMsg)
            delete deleteMsg;
        deleteMsg = QAbstractMessageBox::messageBox( this, tr("Delete"),
                "<qt>" + tr("File deletion failed.") + "</qt>",
                QAbstractMessageBox::Warning, QAbstractMessageBox::Ok );
        QtopiaApplication::showDialog(deleteMsg);
    }
}

void DocumentLauncherView::deleteDoc(int r)
{
    if (r == QAbstractMessageBox::Yes) {
        // We can't delete the deleteMsg object directly in the deleteDoc(int) function
        // because it is in response to the done() signal emitted by the deleteMsg object
        // which is still in use. This happens when trying to delete a read only file.
        QTimer::singleShot(10,this,SLOT(deleteDocWorker()));
    }
}

void DocumentLauncherView::deleteDoc()
{
    const QContent &doc = currentItem();
    if (doc.id() != QContent::InvalidId && doc.isValid()) {
        deleteLnk = doc;
        if(deleteMsg)
            delete deleteMsg;
        deleteMsg = QAbstractMessageBox::messageBox( this, tr("Delete"),
                "<qt>" + tr("Are you sure you want to delete %1?").arg(deleteLnk.name()) + "</qt>",
                QAbstractMessageBox::Warning, QAbstractMessageBox::Yes, QAbstractMessageBox::No );
        connect(deleteMsg, SIGNAL(finished(int)), this, SLOT(deleteDoc(int)));
        QtopiaApplication::showDialog(deleteMsg);
    }
}

void DocumentLauncherView::propertiesDoc()
{
    const QContent &doc = currentItem();
    if (doc.id() != QContent::InvalidId && doc.isValid()) {
        propLnk = doc;
        if (propDlg)
            delete propDlg;
        propDlg = new QDocumentPropertiesDialog(propLnk, this);
        propDlg->setObjectName("document-properties");
        propDlg->showMaximized();
    }
}

void DocumentLauncherView::openRightsIssuerURL()
{
    const QContent &doc = currentItem();
    if (doc.id() != QContent::InvalidId && doc.isValid())
        QDrmContent::activate( doc );
}

void DocumentLauncherView::currentChanged( const QModelIndex &current, const QModelIndex &previous )
{
    if( current.isValid() && !previous.isValid() )
    {
        actionProps->setEnabled(true);
        actionDelete->setEnabled(true);
        separatorAction->setEnabled(true);

        QContent content = model->content( current );

        if( actionBeam && content.permissions() & QDrmRights::Distribute )
            actionBeam->setEnabled( true );
        if( actionRightsIssuer && QDrmContent::canActivate( content ) )
            actionRightsIssuer->setVisible( true );

        QSoftMenuBar::setLabel(this, Qt::Key_Select, QSoftMenuBar::Select);
    }
    else if( !current.isValid() && previous.isValid() )
    {
        actionProps->setEnabled(false);
        actionDelete->setEnabled(false);
        separatorAction->setEnabled(false);

        if( actionBeam )
            actionBeam->setEnabled(false);
        if( actionRightsIssuer )
            actionRightsIssuer->setVisible( false );

        QSoftMenuBar::setLabel(this, Qt::Key_Select, QSoftMenuBar::NoLabel);
    }
    else
    {
        QContent content = model->content( current );

        bool distribute = content.permissions() & QDrmRights::Distribute;
        bool activate = QDrmContent::canActivate( content );

        if( actionBeam && actionBeam->isEnabled() != distribute )
            actionBeam->setEnabled( distribute );
        if( actionRightsIssuer && actionRightsIssuer->isVisible() != activate )
            actionRightsIssuer->setVisible( activate );
    }
}

void DocumentLauncherView::selectDocsType()
{
    if( !typeDlg )
    {
        QContentFilterModel::Template subTypePage(
                QContentFilter::MimeType,
                QString(),
                QContentFilterModel::CheckList );

        QContentFilterModel::Template typePage;

        typePage.setOptions( QContentFilterModel::CheckList | QContentFilterModel::SelectAll );

        typePage.addLabel( subTypePage, tr( "Audio" ), QContentFilter( QContentFilter::MimeType, QLatin1String( "audio/*" ) ) );
        typePage.addLabel( subTypePage, tr( "Image" ), QContentFilter( QContentFilter::MimeType, QLatin1String( "image/*" ) ) );
        typePage.addLabel( subTypePage, tr( "Text"  ), QContentFilter( QContentFilter::MimeType, QLatin1String( "text/*"  ) ) );
        typePage.addLabel( subTypePage, tr( "Video" ), QContentFilter( QContentFilter::MimeType, QLatin1String( "video/*" ) ) );
        typePage.addList( ~( QContentFilter( QContentFilter::MimeType, QLatin1String( "audio/*" ) )
                           | QContentFilter( QContentFilter::MimeType, QLatin1String( "image/*" ) )
                           | QContentFilter( QContentFilter::MimeType, QLatin1String( "text/*"  ) )
                           | QContentFilter( QContentFilter::MimeType, QLatin1String( "video/*" ) ) ),
                          QContentFilter::MimeType );

        typeDlg = new QContentFilterDialog( typePage, this );

        typeDlg->setWindowTitle( tr( "View Type" ) );
        typeDlg->setFilter( mainFilter );
        typeDlg->setObjectName( QLatin1String( "documents-type" ) );
    }

    QtopiaApplication::execDialog( typeDlg );

    showType( typeDlg->checkedFilter() );

    QString label = typeDlg->checkedLabel();

    if( !typeFilter.isValid() || label.isEmpty() )
        typeLbl->hide();
    else
    {
        typeLbl->setText( tr("Type: %1").arg( label ) );
        typeLbl->show();
    }
}

void DocumentLauncherView::selectDocsCategory()
{
    if( !categoryDlg )
    {
        QContentFilterModel::Template categoryPage;

        categoryPage.setOptions( QContentFilterModel::CheckList | QContentFilterModel::SelectAll );

        categoryPage.addList( QContentFilter::Category );
        categoryPage.addList( QContentFilter::Category, QLatin1String( "Documents" ) );

        categoryDlg = new QContentFilterDialog( categoryPage, this );

        categoryDlg->setWindowTitle( tr( "View Category" ) );
        categoryDlg->setFilter( mainFilter );
        categoryDlg->setObjectName( QLatin1String( "documents-category" ) );
    }

    QtopiaApplication::execDialog( categoryDlg );

    showCategory( categoryDlg->checkedFilter() );

    QString label = categoryDlg->checkedLabel();

    if( !categoryFilter.isValid() || label.isEmpty() )
        categoryLbl->hide();
    else
    {
        categoryLbl->setText( tr("Category: %1").arg( label ) );
        categoryLbl->show();
    }
}

void DocumentLauncherView::updateScanningStatus()
{
    if(sender() == scanningBarUpdateTimer)
    {
        if((scanningVSItem && scanningVSItem->value().toBool() == true)
        || (updatingVSItem && updatingVSItem->value().toBool() == true)
        || (installingVSItem && installingVSItem->value().toBool() == true))
        {
            scanningBar->show();
            if(!scanningBarUpdateTimer->isActive())
                QMetaObject::invokeMethod(scanningBarUpdateTimer, "start", Qt::AutoConnection);
            // update the progress. todo: when doing invert, also change the colours over too.
            if(scanningBar->value() == scanningBar->maximum())
            {
                scanningBar->setInvertedAppearance(!scanningBar->invertedAppearance());
                scanningBar->setValue(scanningBar->minimum());
            }
            else
                scanningBar->setValue(scanningBar->value()+1);
        }
        else
        {
            if(scanningBarUpdateTimer->isActive())
                QMetaObject::invokeMethod(scanningBarUpdateTimer, "stop", Qt::AutoConnection);
            scanningBar->hide();
        }
    }
    else
    {
        if(!scanningBarUpdateTimer->isActive())
            QMetaObject::invokeMethod(scanningBarUpdateTimer, "start", Qt::AutoConnection);
    }
}

void DocumentLauncherView::textEntrytextChanged(const QString &text)
{
    if(text.length() == 0)
    {
        setAuxiliaryFilter( QContentFilter() );
        if (!style()->inherits("QThumbStyle")) {
            textEntry->hide();
            findIcon->hide();
        }
    }
    else
    {
        setAuxiliaryFilter( QContentFilter(QContentFilter::Name, '*'+text+'*') );
        if (!style()->inherits("QThumbStyle")) {
            findIcon->show();
            textEntry->show();
        }
    }
}

void DocumentLauncherView::rescan()
{
    QContentSet::scan("all");
}

#include "documentview.moc"
