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

#include "cleanupwizard.h"

#include <qsoftmenubar.h>
#include <qtopiaservices.h>
#include <qdatetimeedit.h>
#include <qtopiaipcenvelope.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlistview.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qgrid.h>
#include <QContent>
#include <QContentSet>

class FinalCleanupWidget : public QWidget
{
    Q_OBJECT
    public:
        FinalCleanupWidget(QWidget *p = 0, const char* name = 0, WFlags f = 0)
            :QWidget(p, name, f)
        {
            QVBoxLayout* vl = new QVBoxLayout(this);
            vl->setSpacing( 6 );
            vl->setMargin( 2 );

            summary = new QLabel(this, "summaryLabel");
            summary->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            summary->setTextFormat(Qt::RichText);
            vl->addWidget(summary);

            text = "";
        };

        void setResult(const QString& newResult)
        {
            text = newResult;
        };

        void appendResult(const QString& result)
        {
            if (text.isEmpty())
                text = "<ul>"; // no tr
            text = text.append(result);
        };

        void reset() { text = ""; };

    protected:
        void showEvent(QShowEvent* se)
        {
            if (text.isEmpty())
                text = tr("No actions have been taken to cleanup the device.");
            else {
                text = text.append("</ul>"); //no tr
                text = text.prepend(tr("The following items have been deleted:"));
            }
            summary->setText(text);
            setFocus();
            QWidget::showEvent(se);
        };

    private:
        QString text;
        QLabel *summary;
};

class PreselectionWidget : public QWidget
{
    Q_OBJECT
    public:
        PreselectionWidget(QWidget *p = 0, const char* name = 0, WFlags f =0)
            :QWidget(p, name, f)
        {
            QVBoxLayout* vl = new QVBoxLayout(this);
            vl->setSpacing( 6 );
            vl->setMargin( 2 );

            vl->addWidget(new QLabel("<qt>" + tr("What would you like to clean up?") + "</qt>", this));

            doc = new QCheckBox(tr("Documents"), this, "doc_checkbox");
            vl->addWidget( doc );
            mail = new QCheckBox(tr("Messages"), this, "mail_checkbox");
            vl->addWidget( mail );
            datebook = new QCheckBox(tr("Events"), this, "datebook_checkbox");
            vl->addWidget( datebook );
            init();

            QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                    QSizePolicy::Expanding);
            vl->addItem(spacer);
        };

        void init() {
            doc->setChecked( false );
            mail->setChecked( false );
            datebook->setChecked( false );
        };

    private:
        QCheckBox *doc, *mail, *datebook;

    friend class CleanupWizard;
};

class DocSummaryWidget : public QWidget
{
    Q_OBJECT
    public:
        DocSummaryWidget(QWidget* p = 0, const char* name = 0, WFlags f = 0)
            :QWidget(p, name, f)
        {
            QVBoxLayout* l = new QVBoxLayout(this);
            l->setSpacing( 6 );
            l->setMargin( 2 );

            progress = new QProgressBar(this, "Progressbar" );
            progress->setFrameStyle( QFrame::Panel | QFrame::Sunken );
            progress->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
            progress->setIndicatorFollowsStyle( true );
            progress->setCenterIndicator( true );
            l->addWidget(progress);

            desc = new QLabel("", this); //no tr
            desc->setAlignment(Qt::AlignHCenter);
            l->addWidget(desc);
            pb = new QPushButton(this);
            l->addWidget(pb);

            QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                    QSizePolicy::Expanding);
            l->addItem(spacer);
        };

        virtual ~DocSummaryWidget() {};

        void monitorFileDeletion(int fileSize) {
            if (fileSize) {
                pb->setText(tr("Cancel"));
                pb->show();
                desc->setText(tr("Starting Cleanup"));
                progress->reset();
                progress->setTotalSteps(fileSize);
                progress->show();
            } else {
                pb->hide();
                progress->hide();
                desc->setText(tr("<qt>No action was selected.</qt>"));
                desc->setFocus(); //update QSoftMenuBar
            }
        };

    public slots:
        void docDeleted(const QString& fileName, int fileSize) {
            static int docCount = 0;
            if (!deleted)
                docCount = 0;
            if (fileSize > 0) {
                docCount++;
                deleted+=fileSize;
                progress->setProgress( deleted );
                desc->setText(tr("Deleting... ") + fileName);
            } else {
                desc->setText(fileName);
                pb->hide();
                desc->setFocus(); //update QSoftMenuBar
                if (deleted)
                    emit docCleanupFinished(docCount);
            }
        };

    signals:
        void startDocCleanup();
        void docCleanupFinished(int docCount);

    protected:
        void showEvent(QShowEvent *se) {
            QTimer::singleShot(1000, this, SIGNAL(startDocCleanup()));
            deleted=0;
            pb->show();
            QWidget::showEvent(se);
        };

    private:
        QLabel * desc;
        QPushButton * pb;
        QProgressBar * progress;
        int deleted;

    friend class CleanupWizard;
};

class DocCleanWidget: public QWidget
{
    Q_OBJECT
    public:
        DocCleanWidget(QWidget* p = 0, const char* name = 0, WFlags f = 0)
            :QWidget(p, name, f)
        {
            QVBoxLayout* vl = new QVBoxLayout(this);
            vl->setSpacing( 6 );
            vl->setMargin( 2 );

            vl->addWidget(new QLabel(tr("<qt>Cleanup Documents</qt>"), this));

            QFrame *line = new QFrame( this, "Line");
            line->setFrameStyle(QFrame::HLine | QFrame::Sunken );
            vl->addWidget(line);

            QGrid* grid = new QGrid(2, this ) ;
            new QLabel(tr("bigger than"), grid);
            sizeBox = new QSpinBox(grid);
            sizeBox->setButtonSymbols(QSpinBox::UpDownArrows);
            sizeBox->setSuffix(" " + tr("kB", "KiloByte"));
            sizeBox->setMinValue( 0 );
            sizeBox->setMaxValue( 100000 );
            sizeBox->setValue( 10 );
            sizeBox->setLineStep( 10 );
            vl->addWidget(grid);

            QGroupBox* mediatypes = new QGroupBox(tr("Document types"), this);
            mediatypes->setColumnLayout( 0, Qt::Vertical );
            mediatypes->layout()->setSpacing( 1 );
            QGridLayout *mediaLayout = new QGridLayout(mediatypes->layout(), 2, 2, 1);
            audio = new QCheckBox(tr("Audio"), mediatypes);
            mediaLayout->addWidget(audio, 0, 0);
            audio->setChecked(false);

            pictures = new QCheckBox(tr("Pictures"), mediatypes);
            mediaLayout->addWidget(pictures, 1, 0);
            pictures->setChecked(false);

            text = new QCheckBox(tr("Text"), mediatypes);
            mediaLayout->addWidget(text, 1, 1);
            text->setChecked(false);

            video = new QCheckBox(tr("Video"), mediatypes);
            mediaLayout->addWidget(video, 0, 1);
            video->setChecked(false);

            vl->addWidget(mediatypes);
            QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                    QSizePolicy::Expanding);
            vl->addItem(spacer);
        };

        virtual ~DocCleanWidget() {};

    private:
        QSpinBox *sizeBox;
        QCheckBox* audio, *video, *text, *pictures;

    friend class CleanupWizard;
};

class DocCheckListItem: public QObject, public QCheckListItem {
    Q_OBJECT
    public:
        DocCheckListItem(QListView* list, const QString& text, Type tt, QContent& lnk)
            : QObject(list), QCheckListItem(list, text, tt)
        {
            content = lnk;
            setPixmap(0, content.pixmap());
        };

        QContent& docLink() {
            return content;
        };
    protected:
        void stateChange(bool b) {
            int size = QFileInfo(content.fileName()).size();
            if (b)
                emit selectionChanged(size);
            else
                emit selectionChanged(-size);
        };
    signals:
        void selectionChanged(int size);
    private:
        QContent content;
};

class DocResultWidget: public QWidget
{
    Q_OBJECT
    public:
        DocResultWidget(QWidget*p = 0, const char* name = 0, WFlags f = 0)
            : QWidget(p, name, f)
            , tooltip(0)
            , timer(0)
            , cleanupStopped( false )
        {
            QVBoxLayout * vl = new QVBoxLayout(this);
            vl->setSpacing( 6 );
            vl->setMargin( 2 );

            summary = new QLabel("", this); //no tr
            vl->addWidget(summary);
            details = new QPushButton(this);
            vl->addWidget(details);

            list = new QListView(this, "DocumentListView");
            list->addColumn(tr("Document"));
            list->addColumn(tr("Size"));
            list->header()->hide();
            list->setMinimumHeight(60);

            connect(list , SIGNAL(selectionChanged(QListViewItem*)),
                    this, SLOT(documentSelected(QListViewItem*)));

            vl->addWidget(list);

            QSpacerItem *spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                    QSizePolicy::Expanding);
            vl->addItem(spacer);

            tooltip = new QLabel(this, "TooltipLabel");
            tooltip->setFrameStyle(QFrame::Panel|QFrame::Raised);
            tooltip->setAlignment(Qt::AlignCenter | Qt::WordBreak);
            tooltip->setBackgroundMode(QWidget::PaletteHighlightedText);
            tooltip->setAutoResize(true);
            tooltip->hide();

            timer = new QTimer(this, "TooltipTimer");
        }

        virtual ~DocResultWidget() { };

        void performSearch()
        {
            list->clear();
            QContent *dl;
            QContentSet allDocs;
            selectedFileSize = 0;
            uint hits = 0;
            int width = (list->width()- list->style().scrollBarExtent().width()) /2  ;

            QFontMetrics fmt = list->fontMetrics();
            for (QStringList::ConstIterator fit=filter.begin(); fit != filter.end(); ++fit) {
                QContentSet resultDocs;
                QContentSet::findDocuments( &resultDocs, *fit );

                const QList<DocLnk> doclist = resultDocs.children();
                QListIterator<DocLnk> it(doclist);
                DocCheckListItem *item = 0;
                QString unit;
                int displayedSize;
                while ((dl=it.current())) {
                    QFileInfo fInfo(dl->file());
                    if (minFileSize*1024 < (int)fInfo.size()){
                        item = new DocCheckListItem(list,
                                calcVisibleStrg(dl->name(), fmt, width),
                                QCheckListItem::CheckBox, *dl);
                        selectedFileSize += fInfo.size(); //ignore size of link file (insignificant size)
                        item->setOn(true);
                        sizeUnit(fInfo.size(), unit, &displayedSize);
                        bool isMB = false;
                        if ( fInfo.size()/1024 > 10000 )
                            isMB = true;
                        item->setText(1, tr("%1 %2").arg(displayedSize).arg(unit));

                        list->insertItem(item);
                        connect(item, SIGNAL(selectionChanged(int)), this, SLOT(selectionChanged(int)) );
                        hits++;
                    }
                    ++it;
                }
            }
            qLog(CleanupWizxard) << hits << "documents found";

            if (!hits) {
                summary->setText(tr("<qt>There are no documents on this device"
                    " that match the given filter critieria.</qt>"));
                list->hide();
                details->hide();
                summary->setFocus();//update QSoftMenuBar
            } else {
                QString unit;
                int displayedSize = 0;
                sizeUnit(selectedFileSize, unit, &displayedSize);

                summary->setText(tr("<qt>%1 documents have been found. "
                           "Deleting these documents would free up %2 %3 of "
                           "storage.</qt>", "%1 = number, %2 = number, %3 = MB/kB/Bytes" )
                        .arg(hits)
                        .arg(displayedSize).arg(unit));

                details->setText(tr("Show details"));
                details->disconnect();
                connect(details, SIGNAL(clicked()), this, SLOT(showDetails()));
                list->hide();
                details->setFocus();
                details->show();
            }
        };

    public slots:
        void cleanup() {
            DocCheckListItem *item = (DocCheckListItem *) list->firstChild();
            if (!item) //empty list
                return;

            qLog(CleanupWizard) << "Deleting Documents...";
            while (item && !cleanupStopped) {
                if (item->isOn()) {
                    QFileInfo info(item->docLink().fileName());
                    emit docDeleted(item->docLink().name(), info.size());
                    qApp->processEvents();
                    item->docLink().removeFiles();
                    item->setOn(false); // uncheck to keep track what was done
                }
                item = (DocCheckListItem *) item->nextSibling();
            }

            if (cleanupStopped)
                emit docDeleted(tr("Aborted"), -1);
            else {
                emit docDeleted(tr("Done"), -1);
                list->clear();
            }
        };

        void stopCleanup() {
            cleanupStopped = true;
        };
    public:

        void setMinimalFileSize(int minSize) { minFileSize = minSize; };

        void setFilter(QStringList& filterList) { filter = filterList; };

        uint hasDocumentsToRemove(){ return selectedFileSize; };

    signals:
        void docDeleted(const QString & fileName, int fileSize);

    protected:
        void showEvent(QShowEvent *se) {
            cleanupStopped = false;
            QWidget::showEvent(se);
        };

        void hideEvent(QHideEvent *) {
            tooltip->hide();
            timer->stop();
        };

    private slots:
        void showDetails() {
            details->hide();
            selectionChanged(0);
            list->show();
            list->setFocus();
        };

        //fileSize in Bytes - display in unit
        void sizeUnit(int fileSize, QString& unit, int* display)
        {
            if (fileSize < 1024){
                unit = tr("bytes");
                *display=fileSize;
            } else if (fileSize / 1024 < 10240) {// < 10MB
                unit = tr("kB", "KiloByte");
                *display = fileSize/1024;
            } else {
                unit = tr("MB", "MegaByte");
                *display = fileSize/1024/1024;
            }
        };

        void selectionChanged(int size) {
            selectedFileSize += size;
            QString unit;
            int displayedSize;
            sizeUnit(selectedFileSize, unit, &displayedSize);
            summary->setText(tr("<qt>Uncheck documents you want to "
                        "keep (%1 %2 selected)</qt>")
                    .arg(displayedSize).arg(unit));
            tooltip->hide();
        };
        void documentSelected(QListViewItem * item) {
            tooltip->hide();
            if (!item)
                return;
            DocCheckListItem * dcli = (DocCheckListItem*) item;

            connect(timer, SIGNAL(timeout()), this, SLOT(showToolTip()));
            timer->start(1000, true);

            tooltip->setText(dcli->docLink().name());

            int x = (width() - tooltip->width())/2;

            tooltip->move(x, 0);
        };

        void showToolTip() {
            tooltip->raise();
            tooltip->show();
            timer->disconnect();
            connect(timer, SIGNAL(timeout()), this, SLOT(hideToolTip()));
            timer->start(2000, true);
        };

        void hideToolTip() {
            tooltip->hide();
        };

    private:
        QString calcVisibleStrg(const QString& text, QFontMetrics& fmt, int availSpace) {
            int i = 1;
            int length = text.length();
            while (i <= length && fmt.width(text, i) < availSpace)
                i++;
            if (i <= length)
                return text.left(i-3)+"..."; //no tr
            else
                return text;
        };

        QLabel * tooltip;
        QTimer * timer;
        QListView *list;
        QLabel *summary ;
        QPushButton *details;
        int minFileSize;
        QStringList filter;
        uint selectedFileSize;
        bool cleanupStopped;

        enum SizeCategory { Bytes, KBytes, MBytes };
        friend class CleanupWizard;
};


class MailCleanWidget : public QWidget {
    Q_OBJECT
    public:
        MailCleanWidget(QWidget* p = 0, const char* name = 0, WFlags f = 0)
            :QWidget(p, name, f)
        {
            QVBoxLayout* vl = new QVBoxLayout(this);
            vl->setSpacing( 6 );
            vl->setMargin( 2 );
            vl->addWidget(new QLabel(tr("<qt>Cleanup Messages</qt>"), this));

            QFrame *line = new QFrame( this, "Line");
            line->setFrameStyle(QFrame::HLine | QFrame::Sunken );
            vl->addWidget(line);

            QGridLayout *grid = new QGridLayout(vl, 2, 2, 3);
            grid->addWidget(new QLabel(tr("older than"), this), 0, 0);
            dp = new DateEdit( this, 0, false, true );
            grid->addWidget( dp, 0, 1 );

            grid->addWidget(new QLabel(tr("bigger than"), this), 1, 0);
            sizeBox = new QSpinBox( 0, 10000, 10, this );
            sizeBox->setButtonSymbols(QSpinBox::UpDownArrows);
            sizeBox->setSuffix(" " + tr("kB", "KiloByte"));
            sizeBox->setMinValue( 0 );
            sizeBox->setMaxValue( 100000 );
            sizeBox->setValue( 10 );
            sizeBox->setLineStep( 10 );
            grid->addWidget(sizeBox, 1, 1);

            QSpacerItem *spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                    QSizePolicy::Expanding);
            vl->addItem(spacer);
        };

    private:
        QSpinBox *sizeBox;
        DateEdit *dp;
    friend class CleanupWizard;
};

class DatebookCleanWidget : public QWidget {
    Q_OBJECT
    public:
        DatebookCleanWidget(QWidget* p = 0, const char* name = 0, WFlags f = 0)
            :QWidget(p, name, f)
        {
            QVBoxLayout* vl = new QVBoxLayout(this);
            vl->setSpacing( 6 );
            vl->setMargin( 2 );
            vl->addWidget(new QLabel(tr("<qt>Cleanup Events</qt>"), this));

            QFrame *line = new QFrame( this, "Line");
            line->setFrameStyle(QFrame::HLine | QFrame::Sunken );
            vl->addWidget(line);

            QGridLayout *grid = new QGridLayout(vl, 2, 2, 3);
            grid->addWidget(new QLabel(tr("older than"), this), 0, 0);

            dp = new DateEdit( this, 0, false, true );
            grid->addWidget( dp, 0, 1 );

            QSpacerItem *spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                    QSizePolicy::Expanding);
            vl->addItem(spacer);
        };

    private:
        QSpinBox *dayBox;
        DateEdit *dp;
    friend class CleanupWizard;
};

CleanupWizard::CleanupWizard(QWidget * parent, const char* name, WFlags fl)
    : QWidget(parent, name, fl ), wStack(0)
{
    QTimer::singleShot(10, this, SLOT(init()));
}

CleanupWizard::~CleanupWizard()
{
}

void CleanupWizard::init()
{
    setCaption(tr("Cleanup Wizard"));

    QVBoxLayout* vb = new QVBoxLayout(this) ;
    wStack = new QWidgetStack(this);
    vb->addWidget(wStack);

    m_PreselectionWidget = new PreselectionWidget(this);
    m_PreselectionWidget->installEventFilter(this);
    m_DocCleanWidget = new DocCleanWidget(this);
    m_DocResultWidget = new DocResultWidget(this);
    m_DocSummaryWidget = new DocSummaryWidget(this);
    m_MailCleanWidget = new MailCleanWidget(this);
    m_DatebookCleanWidget = new DatebookCleanWidget(this);
    m_FinalCleanupWidget = new FinalCleanupWidget(this);

    wStack->addWidget(m_PreselectionWidget, 0);
    wStack->addWidget(m_DocCleanWidget, 1);
    wStack->addWidget(m_DocResultWidget, 2);
    wStack->addWidget(m_DocSummaryWidget, 3);
    wStack->addWidget(m_MailCleanWidget, 4);
    wStack->addWidget(m_DatebookCleanWidget, 5);
    wStack->addWidget(m_FinalCleanupWidget, 6);

    wStack->raiseWidget(m_PreselectionWidget);

    connect(m_DocSummaryWidget, SIGNAL(startDocCleanup()),
            m_DocResultWidget, SLOT(cleanup()));
    connect(m_DocResultWidget, SIGNAL(docDeleted(QString,int)),
            m_DocSummaryWidget, SLOT(docDeleted(QString,int)));
    connect(m_DocSummaryWidget->pb, SIGNAL(clicked()),
            m_DocResultWidget, SLOT(stopCleanup()));
    connect(m_DocSummaryWidget, SIGNAL(docCleanupFinished(int)),
            this, SLOT(addToFinalSummary(int)));
}

void CleanupWizard::keyPressEvent(QKeyEvent* ke)
{
   if (ke->key() == Key_Context1) {
        setContextBar(Default);
        if (wStack->visibleWidget() == m_DocCleanWidget
                    || wStack->visibleWidget() == m_FinalCleanupWidget) {
            wStack->raiseWidget(m_PreselectionWidget);
        } else if (wStack->visibleWidget() == m_DocSummaryWidget
                    || wStack->visibleWidget() == m_DocResultWidget) {
            wStack->raiseWidget(m_DocCleanWidget);
        } else if (wStack->visibleWidget() == m_MailCleanWidget) {
            if (m_PreselectionWidget->doc->isChecked())
                wStack->raiseWidget(m_DocCleanWidget);
            else {
                wStack->raiseWidget(m_PreselectionWidget);
            }
        } else if ( wStack->visibleWidget() == m_DatebookCleanWidget ){
            if (m_PreselectionWidget->mail->isChecked())
                wStack->raiseWidget(m_MailCleanWidget);
            else if (m_PreselectionWidget->doc->isChecked())
                wStack->raiseWidget(m_DocCleanWidget);
            else {
                wStack->raiseWidget(m_PreselectionWidget);
            }
        } else if ( wStack->visibleWidget() == m_PreselectionWidget)
            setContextBar(NoBack);
        ke->accept();
        return;
    }
    if (ke->key() == Key_Back) {
            ke->accept();
            setContextBar(Default);
            if (wStack->visibleWidget() == m_PreselectionWidget) {
                if (m_PreselectionWidget->doc->isChecked())
                    wStack->raiseWidget(m_DocCleanWidget) ;
                else if (m_PreselectionWidget->mail->isChecked())
                    wStack->raiseWidget(m_MailCleanWidget);
                else if (m_PreselectionWidget->datebook->isChecked())
                    wStack->raiseWidget(m_DatebookCleanWidget);
                else {
                    setContextBar(NoForward);
                    wStack->raiseWidget(m_FinalCleanupWidget);
                }
            } else if (wStack->visibleWidget() == m_DocCleanWidget){
                m_DocResultWidget->setMinimalFileSize(m_DocCleanWidget->sizeBox->value());

                QStringList filter;
                if (m_DocCleanWidget->audio->isChecked())
                    filter.append("audio/*"); //no tr
                if (m_DocCleanWidget->video->isChecked())
                    filter.append("video/*"); //no tr
                if (m_DocCleanWidget->text->isChecked())
                    filter.append("text/*"); //no tr
                if (m_DocCleanWidget->pictures->isChecked())
                    filter.append("image/*"); //no tr
                m_DocResultWidget->setFilter(filter);

                m_DocResultWidget->performSearch();
                wStack->raiseWidget(m_DocResultWidget);
            } else if (wStack->visibleWidget() == m_DocResultWidget){
                if (m_DocResultWidget->hasDocumentsToRemove()) {
                    switch (QMessageBox::warning(this, tr("WARNING"),
                            tr("<qt>Are you sure you want to delete these files?</qt>"),
                            QMessageBox::Yes,
                            QMessageBox::No|QMessageBox::Default))
                    {
                        case QMessageBox::Yes:
                            m_DocSummaryWidget->monitorFileDeletion(
                                    m_DocResultWidget->hasDocumentsToRemove());
                            break;
                        default:
                            m_DocSummaryWidget->monitorFileDeletion(0);
                            return;
                            break;
                    }
                    wStack->raiseWidget(m_DocSummaryWidget);
                } else {
                    m_DocSummaryWidget->monitorFileDeletion(0);
                    setContextBar(Default);
                    if (m_PreselectionWidget->mail->isChecked())
                        wStack->raiseWidget(m_MailCleanWidget);
                    else if (m_PreselectionWidget->datebook->isChecked())
                        wStack->raiseWidget(m_DatebookCleanWidget);
                    else {
                        setContextBar(NoForward);
                        wStack->raiseWidget(m_FinalCleanupWidget);
                    }
               }
            } else if (wStack->visibleWidget() == m_DocSummaryWidget){
                if (m_PreselectionWidget->mail->isChecked())
                    wStack->raiseWidget(m_MailCleanWidget);
                else if (m_PreselectionWidget->datebook->isChecked())
                    wStack->raiseWidget(m_DatebookCleanWidget);
                else {
                    setContextBar(NoForward);
                    wStack->raiseWidget(m_FinalCleanupWidget);
                }
            } else if (wStack->visibleWidget() == m_MailCleanWidget) {
                QDate date = m_MailCleanWidget->dp->date();
                if (!date.isNull()) {
                    int size = m_MailCleanWidget->sizeBox->value();
                    switch (QMessageBox::warning(this, tr("WARNING"),
                            tr("<qt>Are you sure you want to delete messages older than %1?</qt>")
                                .arg(QTimeString::localYMD(date, QTimeString::Short)),
                            QMessageBox::Yes,
                            QMessageBox::No|QMessageBox::Default))
                    {
                        case QMessageBox::Yes:
                            break;
                        default:
                            return;
                            break;
                    }


                    qLog(CleanupWizard) << "Deleting events older than" << date.toString();

                    m_FinalCleanupWidget->appendResult(tr("<li>Messages (%1)</li>")
                            .arg(QTimeString::localYMD(date, QTimeString::Short)));
                    QtopiaServiceRequest req("Email", "cleanupMessages(QDate,int)");
                    req << date.addDays(-1);
                    req << size;
                    req.send();
                }
                if (m_PreselectionWidget->datebook->isChecked())
                    wStack->raiseWidget(m_DatebookCleanWidget);
                else {
                    setContextBar(NoForward);
                    wStack->raiseWidget(m_FinalCleanupWidget);
                }
            } else if (wStack->visibleWidget() == m_DatebookCleanWidget) {
                QDate date = m_DatebookCleanWidget->dp->date();
                if (!date.isNull()) {
                    switch (QMessageBox::warning(this, tr("WARNING"),
                            tr("<qt>Are you sure you want to delete events older than %1?</qt>")
                                .arg(QTimeString::localYMD(date, QTimeString::Short)),
                            QMessageBox::Yes,
                            QMessageBox::No|QMessageBox::Default))
                    {
                        case QMessageBox::Yes:
                            break;
                        default:
                            return;
                            break;
                    }

                    qLog(CleanupWizard) << "Deleting events older than" << date.toString();

                    QtopiaServiceRequest req("Calendar", "cleanByDate(QDate)");
                    req << date.addDays(-1);
                    req.send();
                    m_FinalCleanupWidget->appendResult(tr("<li>Events (%1)</li>", "e.g. %1 = 20 Aug 2004")
                            .arg(QTimeString::localYMD(date, QTimeString::Short)));
                }
                setContextBar(NoForward);
                wStack->raiseWidget(m_FinalCleanupWidget);
            } else if (wStack->visibleWidget() == m_FinalCleanupWidget)
                close();
            return;
    }
    ke->ignore();
}

void CleanupWizard::setContextBar(WizardStyle style)
{
    switch (style) {
        case Default:
            QSoftMenuBar::setLabel(this, Key_Back, QSoftMenuBar::Next);
            QSoftMenuBar::setLabel(this, Key_Context1, QSoftMenuBar::Previous);
            break;
        case NoBack:
            QSoftMenuBar::setLabel(this, Key_Back, QSoftMenuBar::Next);
            QSoftMenuBar::setLabel(this, Key_Context1, QSoftMenuBar::NoLabel);
            break;
        case NoForward:
            QSoftMenuBar::setLabel(this, Key_Back, QSoftMenuBar::Back);
            QSoftMenuBar::setLabel(this, Key_Context1, QSoftMenuBar::Previous);
            break;
    }
}

void CleanupWizard::addToFinalSummary(int docCount)
{
    if (!docCount)
        return;

    m_FinalCleanupWidget->appendResult(tr("<li>%1 document(s)</li>")
            .arg(docCount));
}

void CleanupWizard::showEvent(QShowEvent *se)
{
    m_PreselectionWidget->init();
    wStack->raiseWidget(m_PreselectionWidget);
    QWidget::showEvent(se);
}

bool CleanupWizard::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Show ) {
        if (o == m_PreselectionWidget) {
            setContextBar(NoBack);
            m_FinalCleanupWidget->reset();
        }
    }
    return false;
}

#include "cleanupwizard.moc"
