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

#include "receivewindow.h"
#include "taskmanagerentry.h"

#include <qcontent.h>
#include <qmimetype.h>
#include <qtopiaipcenvelope.h>
#include <qtopiaservices.h>
#include <qtopiaipcadaptor.h>
#include <qtopiaapplication.h>
#include <qsoftmenubar.h>
#include <qtopialog.h>

#include <qtopia/pim/qcontact.h>
#include <qtopia/pim/qappointment.h>
#include <qtopia/pim/qtask.h>

#include <QHeaderView>
#include <QListView>
#include <QAbstractListModel>
#include <QtopiaItemDelegate>
#include <QMessageBox>
#include <QCloseEvent>
#include <QPainter>
#include <QTimer>
#include <QFont>
#include <QMutableListIterator>
#include <QMenu>
#include <QTabWidget>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

static QString receivewindow_prettyPrintSize(qint64 fsize)
{
    static const char *size_suffix[] = {
        QT_TRANSLATE_NOOP("CustomPushService", "B"),
        QT_TRANSLATE_NOOP("CustomPushService", "KB"),
        QT_TRANSLATE_NOOP("CustomPushService", "MB"),
        QT_TRANSLATE_NOOP("CustomPushService", "GB"),
    };

    double max = fsize;

    int i = 0;
    for (; i < 4; i++) {
        if (max > 1024.0) {
            max /= 1024.0;
        }
        else {
            break;
        }
    }

    // REALLY big file?
    if (i == 4)
        i = 0;

    if (fsize < 1024) {
        return QString::number(fsize) + qApp->translate("CustomPushService", size_suffix[i]);
    } else {
        return QString::number(max, 'f', 2)
                + qApp->translate("CustomPushService", size_suffix[i]);
    }
}

static void vcalInfo( const QString &fileName, bool *todo, bool *cal )
{
    *cal = *todo = false;

    QList<QAppointment> events = QAppointment::readVCalendar( fileName );

    if ( events.count() ) {
        *cal = true;
    }

    QList<QTask> tasks = QTask::readVCalendar( fileName );

    if ( tasks.count() ) {
        *todo = true;
    }
}


class FileTransfer
{
public:

    enum Direction { Sending, Receiving };

    FileTransfer(int id, Direction dir, const QString &fileName, const QString &mimeType, const QString &description);

    void setProgress(qint64 completed, qint64 total);
    qint64 total() const { return m_total; }
    qint64 completed() const { return m_completed; }

    const QString &fileName() const { return m_fileName; }
    const QString &mimeType() const { return m_mimeType; }
    const QString &description() const { return m_description; }
    const QIcon &icon() const { return m_icon; }

    void setFinished(bool failed) { m_finished = true; m_failed = failed; }
    bool failed() const { return m_failed; }
    bool finished() const { return m_finished; }

    Direction direction() const { return m_direction; }

    int id() const { return m_id; }

    void setContentId(const QContentId &contentId) { m_contentId = contentId; }
    QContentId contentId() const { return m_contentId; }

private:
    bool m_failed;
    bool m_finished;
    QString m_fileName;
    QString m_mimeType;
    QString m_description;
    qint64 m_completed;
    qint64 m_total;
    Direction m_direction;
    int m_id;
    QIcon m_icon;
    QContentId m_contentId;
};

FileTransfer::FileTransfer(int id, Direction dir, const QString &fileName, const QString &mimeType, const QString &description)
{
    m_id = id;
    m_fileName = fileName;
    m_mimeType = mimeType;
    m_description = description;
    m_direction = dir;
    m_failed = false;
    m_finished = false;
    m_completed = 0;
    m_total = 0;
    m_icon = QMimeType(mimeType).icon();

    if (dir == Sending)
        m_contentId = QContent(fileName, false).id();
    else
        m_contentId = QContent::InvalidId;
}

void FileTransfer::setProgress(qint64 completed, qint64 total)
{
    m_total = total;
    m_completed = completed;
}

class FileTransferListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    FileTransferListModel(QObject *parent = 0);

    virtual int rowCount(const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    void addFile(const FileTransfer &file);
    void removeFinishedFiles();

    QModelIndex indexFromId(int id) const;
    const FileTransfer &findFile(const QModelIndex &index) const;   // index must be valid
    FileTransfer &findFile(const QModelIndex &index);   // index must be valid

    void updateFileProgress(int id, qint64 completed, qint64 total);
    void setFileCompleted(int id, bool error);

    int size() const { return m_list.size(); }

private:
    QList<FileTransfer> m_list;
};

FileTransferListModel::FileTransferListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FileTransferListModel::rowCount(const QModelIndex &) const
{
    return m_list.size();
}

QVariant FileTransferListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
        case Qtopia::AdditionalDecorationRole:
            if (m_list[index.row()].finished() && !m_list[index.row()].failed())
                return QIcon(":icon/ok");
        case Qt::TextAlignmentRole:
            return QVariant(Qt::AlignBottom | Qt::AlignLeft);
        case Qt::DecorationRole:
            return m_list[index.row()].icon();
        case Qt::DisplayRole:
            const FileTransfer &file = m_list[index.row()];
            if (!file.description().isEmpty())
                return file.description();
            QString str = file.fileName();
            int pos = str.lastIndexOf( QDir::separator() );
            if ( pos != -1 )
                str = str.mid( pos + 1 );
            return str;
    }
    return QVariant();
}

void FileTransferListModel::addFile(const FileTransfer &file)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_list.insert(0, file);
    endInsertRows();
}

void FileTransferListModel::removeFinishedFiles()
{
    QMutableListIterator<FileTransfer> i(m_list);
    while (i.hasNext()) {
        if (i.next().finished())
            i.remove();
    }
}

QModelIndex FileTransferListModel::indexFromId(int id) const
{
    for (int i=0; i < m_list.size(); i++) {
        if (m_list[i].id() == id)
            return index(i);
    }
    return QModelIndex();
}

const FileTransfer &FileTransferListModel::findFile(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());
    return m_list[index.row()];
}

FileTransfer &FileTransferListModel::findFile(const QModelIndex &index)
{
    Q_ASSERT(index.isValid());
    return m_list[index.row()];
}

void FileTransferListModel::updateFileProgress(int id, qint64 completed, qint64 total)
{
    for (int i=0; i < m_list.size(); i++) {
        if (m_list[i].id() == id) {
            m_list[i].setProgress(completed, total);
            emit dataChanged(index(i, 0), index(i, 0));
            break;
        }
    }
}

void FileTransferListModel::setFileCompleted(int id, bool error)
{
    for (int i=0; i < m_list.size(); i++) {
        if (m_list[i].id() == id) {
            m_list[i].setFinished(error);
            emit dataChanged(index(i, 0), index(i, 0));
            break;
        }
    }
}

//==============================================================

class ReceiveWindowItemDelegate : public QtopiaItemDelegate
{
    Q_OBJECT
public:
    ReceiveWindowItemDelegate(FileTransferListModel *model, QObject *parent);
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index ) const;

protected:
    virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                        const QRect &rect, const QString &text) const;

private:
    static int progressHorizontalMargin();
    static int progressVerticalMargin();
    static int progressBarHeight(int totalRowHeight);
    static QFont progressFont(const QModelIndex &index);
    static void getProgressRect(QRect *rect, const QStyleOptionViewItem &option);
    static const qreal PROGRESS_HEIGHT_PROPORTION;

    void drawProgressBar(QPainter *painter, const QStyleOptionViewItem &option,
                         const QRect &rect, qint64 progress, qint64 max) const;

    FileTransferListModel *m_model;
};


// this is the proportion of the height of the progress bar within the total row height
const qreal ReceiveWindowItemDelegate::PROGRESS_HEIGHT_PROPORTION = (3/7.0);

ReceiveWindowItemDelegate::ReceiveWindowItemDelegate(FileTransferListModel *model, QObject *parent)
    : QtopiaItemDelegate(parent),
      m_model(model)
{
}

int ReceiveWindowItemDelegate::progressHorizontalMargin()
{
    return QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
}

int ReceiveWindowItemDelegate::progressVerticalMargin()
{
    return QApplication::style()->pixelMetric(QStyle::PM_FocusFrameVMargin);
}

int ReceiveWindowItemDelegate::progressBarHeight(int totalRowHeight)
{
    return int(totalRowHeight * PROGRESS_HEIGHT_PROPORTION) + progressVerticalMargin();
}

QFont ReceiveWindowItemDelegate::progressFont(const QModelIndex &index)
{
    QFont f = index.data(Qt::FontRole).value<QFont>();
    f.setPointSize(f.pointSize() - 1);
    return f;
}

void ReceiveWindowItemDelegate::getProgressRect(QRect *rect, const QStyleOptionViewItem &option)
{
    *rect = option.rect;

    // margin between progress and borders
    int progressHMargin = progressHorizontalMargin();
    int progressVMargin = progressVerticalMargin();

    rect->setHeight(progressBarHeight(option.rect.height()) - progressVMargin);
    rect->moveBottom(option.rect.bottom() - progressVMargin);

    // line up with left edge of filename, taking into account the margins
    // on both side of the icon, and the margin on the side of the text
    int hOffsetForIcon = option.decorationSize.width() +
            (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin)+1)*3;
    if (option.direction == Qt::LeftToRight)
        rect->adjust(hOffsetForIcon, 0, -progressHMargin, 0);
    else
        rect->adjust(progressHMargin, 0, -hOffsetForIcon, 0);
}

void ReceiveWindowItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
{
    QRect textRect = rect;
    textRect.setY(option.rect.y());
    textRect.setHeight(option.rect.height() -
            progressBarHeight(option.rect.height()));
    QtopiaItemDelegate::drawDisplay(painter, option, textRect, text);
}

QSize ReceiveWindowItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sz = QtopiaItemDelegate::sizeHint(option, index);
    int minProgressHeight = QFontMetrics(progressFont(index)).height();
    int height = int(minProgressHeight / PROGRESS_HEIGHT_PROPORTION) +
            progressVerticalMargin();
    sz.setHeight(height);
    return sz;
}

void ReceiveWindowItemDelegate::drawProgressBar(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, qint64 progress, qint64 max) const
{
    // set the progress bar style options
    QStyleOptionProgressBar progressOption;
    progressOption.rect = rect;
    progressOption.fontMetrics = option.fontMetrics;
    progressOption.direction = QApplication::layoutDirection();
    progressOption.minimum = 0;
    progressOption.maximum = max;
    progressOption.textAlignment = Qt::AlignCenter;
    progressOption.textVisible = true;
    progressOption.state = QStyle::State_Enabled;

    if (progress > max || max == 0) {
        progressOption.progress = 0;
        progressOption.text = QString("%1/?").arg(receivewindow_prettyPrintSize(progress));
    } else {
        progressOption.progress = progress;
        progressOption.text = QString("%1/%2").
                arg(receivewindow_prettyPrintSize(progress)).
                arg(receivewindow_prettyPrintSize(max));
    }

    // draw the progress bar
    QApplication::style()->drawControl(QStyle::CE_ProgressBar,
            &progressOption, painter);
}

void ReceiveWindowItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    // paint everything except the progress bar / result text
    QtopiaItemDelegate::paint(painter, option, index);

    QRect progressRect;
    getProgressRect(&progressRect, option);

    painter->save();
    painter->setFont(progressFont(index));
    const FileTransfer &file = m_model->findFile(index);
    if (file.finished()) {
        if (file.failed()) {
            painter->drawText(progressRect, Qt::AlignLeft,
                    tr("Transfer error."));
        } else {
            painter->drawText(progressRect, Qt::AlignLeft,
                    QString("%1").arg(receivewindow_prettyPrintSize(file.completed())));
        }
    } else {
        drawProgressBar(painter, option, progressRect, file.completed(),
                file.total());
    }
    painter->restore();
}

//===================================================================

/*
    The FilteredTransferListView class provides a filtered list view of
    file transfers.

    It will only display file transfers that match the transfer direction
    given in the constructor.
*/
class FilteredTransferListView : public QListView
{
    Q_OBJECT
public:
    FilteredTransferListView(FileTransfer::Direction direction, QWidget *parent = 0);

protected slots:
    virtual void rowsInserted(const QModelIndex &parent, int start, int end);

private:
    FileTransfer::Direction m_direction;
};

FilteredTransferListView::FilteredTransferListView(FileTransfer::Direction direction, QWidget *parent)
    : QListView(parent),
      m_direction(direction)
{
}

void FilteredTransferListView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QListView::rowsInserted(parent, start, end);

    FileTransferListModel *m = qobject_cast<FileTransferListModel*>(model());
    if (m) {
        for (int i=start; i<=end; i++) {
            const FileTransfer &transfer = m->findFile(m->index(i));
            if (transfer.direction() != m_direction)
                setRowHidden(i, true);
        }
    }
}



//===================================================================

ReceiveWindow::ReceiveWindow(QWidget *parent)
    : QMainWindow(parent),
      m_model(new FileTransferListModel(this))
{
    m_incomingView = new FilteredTransferListView(FileTransfer::Receiving);
    m_outgoingView = new FilteredTransferListView(FileTransfer::Sending);
    setUpView(m_incomingView);
    setUpView(m_outgoingView);

    m_tabs = new QTabWidget;
    m_tabs->addTab(m_incomingView, tr("Received"));
    m_tabs->addTab(m_outgoingView, tr("Sent"));

    QMenu *menu = QSoftMenuBar::menuFor(m_tabs);
    m_cancelAction = menu->addAction(QIcon(":icon/cancel"), 
            tr("Cancel transfer"), this, SLOT(stopCurrentTransfer()));
    m_cancelAction->setVisible(false);

    setObjectName(QLatin1String("filetransferwindow"));
    setWindowTitle(QObject::tr("Send/Receive Files"));
    setCentralWidget(m_tabs);

    m_taskManagerEntry = new TaskManagerEntry(windowTitle(), "sync", this);
    connect(m_taskManagerEntry, SIGNAL(activated()), SLOT(showWindow()));
}

void ReceiveWindow::setUpView(QListView *view)
{
    view->setModel(m_model);
    view->setItemDelegate(new ReceiveWindowItemDelegate(m_model, this));
    view->setAlternatingRowColors(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setFrameStyle(QFrame::NoFrame);
    view->setTextElideMode(Qt::ElideMiddle);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setSpacing(3);
    view->setUniformItemSizes(true);

    connect(view, SIGNAL(activated(QModelIndex)),
            SLOT(activated(QModelIndex)));
    connect(view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(currentChanged(QModelIndex,QModelIndex)));

    // no items in list to begin with, so remove 'select' label
    QSoftMenuBar::setLabel(view, Qt::Key_Select, QSoftMenuBar::NoLabel);
}

void ReceiveWindow::closeEvent(QCloseEvent *event)
{
    // remove all completed transfers
    m_model->removeFinishedFiles();
    if (m_model->size() == 0)
        m_taskManagerEntry->hide();

    QMainWindow::closeEvent(event);
}

void ReceiveWindow::receiveInitiated(int id, const QString &fileName, const QString &mime, const QString &description)
{
    QString mimeType = ( mime.isEmpty() ? QMimeType(fileName).id() : mime );

    qLog(Obex) << "recvInitiated: " << fileName << "(" << mime << ") ->"
            << mimeType << description;

    if ((mimeType.toLower() == "text/x-vcard") ||
            (mimeType.toLower() == "text/x-vcalendar")) {
        // don't show vCard/vCal transfers
        qLog(Obex) << "Looks like a Vcard or VCal";
        handleIncomingVObject(id, fileName, mimeType, description);
    } else {
        qLog(Obex) << "Looks like a regular file...";
        m_model->addFile(FileTransfer(id, FileTransfer::Receiving, fileName,
                mimeType, description));

        m_incomingView->setCurrentIndex(m_model->indexFromId(id));
        m_tabs->setCurrentWidget(m_incomingView);

        showWindow();
        m_taskManagerEntry->show();
    }
}

void ReceiveWindow::sendInitiated(int id, const QString &fileName, const QString &mime, const QString &description)
{
    qLog(Obex) << "sendInitiated";

    if ((mime.toLower() == "text/x-vcard") ||
            (mime.toLower() == "text/x-vcalendar")) {
        // don't show vCard/vCal transfers
        qLog(Obex) << "Looks like a Vcard or VCal";
    } else {
        qLog(Obex) << "Looks like a regular file...";
        m_model->addFile(FileTransfer(id, FileTransfer::Sending, fileName,
                mime, description));

        m_tabs->setCurrentWidget(m_outgoingView);
        m_outgoingView->setCurrentIndex(m_model->indexFromId(id));

        showWindow();
        m_taskManagerEntry->show();
    }
}

void ReceiveWindow::progress(int id, qint64 bytes, qint64 total)
{
    qLog(Obex) << "Got Progress " << id << bytes << "/" << total;
    m_model->updateFileProgress(id, bytes, total);
}

void ReceiveWindow::completed(int id, bool error)
{
    qLog(Obex) << "Got completion message " << id << "(" << error << ")";

    if (handleVObjectReceived(id, error))
        return;

    m_model->setFileCompleted(id, error);
    update();

    QModelIndex index = m_model->indexFromId(id);
    if (!index.isValid())
        return;

    FileTransfer &file = m_model->findFile(index);
    if (!error && file.direction() == FileTransfer::Receiving) {
        // save the received file
        QContentId contentId = saveFile(file);
        if (contentId != QContent::InvalidId)
            file.setContentId(contentId);
    }

    QListView *view = qobject_cast<QListView*>(m_tabs->currentWidget());
    if (view->currentIndex() == index) {
        // indicate file can now be opened
        if (file.contentId() != QContent::InvalidId) {
            QSoftMenuBar::setLabel(view, Qt::Key_Select,
                    QSoftMenuBar::Select);
        }
        m_cancelAction->setVisible(false);
    }
}

QContentId ReceiveWindow::saveFile(const FileTransfer &file)
{
    if (!file.completed() || file.direction() == FileTransfer::Sending)
        return QContent::InvalidId;

    QString saveAs = file.fileName();
    int pos = saveAs.lastIndexOf( QDir::separator() );
    if ( pos != -1 )
        saveAs = saveAs.mid( pos + 1 );

    QString tmpFile = Qtopia::tempDir() + "obex/in/" + saveAs;

    QFile f(tmpFile);
    if ( !f.open(QIODevice::ReadOnly) ) {
        qLog(Obex) << "Couldn't open file to read!!";
        return QContent::InvalidId;
    }

    QContent doc;
    doc.setType(file.mimeType());
    doc.setName(file.fileName());

    QIODevice *device = doc.open(QIODevice::WriteOnly);

    qLog(Obex) << "File for QContent is:" << doc.name()
            << "with fileName:" << doc.fileName();

    // If there's a Description value, use that as the QContent name instead
    // of using the Name value, cos it's probably a more user-friendly name
    // whereas the Name is probably a filename.
    // Must do this after QContent::open() so that the file has already been
    // created under the filename in the Name value.
    QString desc = file.description();
    if (!desc.isEmpty()) {
        qLog(Obex) << "Renaming QContent name for" << doc.name()
                << "to" << desc;
        doc.setName(desc);
    }

    QContentId contentId = QContent::InvalidId;

    if (device) {
        char buf[4096];
        int size;
        while ( (size = f.read(buf, 4096)) > 0) {
            device->write(buf, size);
        }
        device->close();
        delete device;

        if (doc.commit()) {
            contentId = doc.id();
        } else {
            qLog(Obex) << "Error saving QContent file" << doc.fileName();
        }
    } else {
        qLog(Obex) << "Failed to save file" << doc.fileName();
    }

    f.close();
    ::unlink(tmpFile.toLocal8Bit().data());

    return contentId;
}

void ReceiveWindow::handleIncomingVObject(int id, const QString &fileName, const QString &mime, const QString &/*description*/)
{
    VObjectTransfer t;
    t.id = id;
    t.fileName = fileName;
    t.mimeType = mime;
    m_vObjects.append(t);
}

void ReceiveWindow::activated(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    const FileTransfer &file = m_model->findFile(index);
    if (file.contentId() != QContent::InvalidId) {
        QContent content(file.contentId());
        qLog(Obex) << "Opening file:" << content.name();
        content.execute();
    }
}

void ReceiveWindow::currentChanged(const QModelIndex &current, const QModelIndex &)
{
    if (!current.isValid())
        return;

    // don't show 'select' label if file can't be opened (i.e. is incoming
    // file and haven't fully received it yet)
    const FileTransfer &file = m_model->findFile(current);
    QListView *view = (file.direction() == FileTransfer::Receiving ?
                m_incomingView : m_outgoingView);
    if (file.contentId() == QContent::InvalidId)
        QSoftMenuBar::setLabel(view, Qt::Key_Select, QSoftMenuBar::NoLabel);
    else
        QSoftMenuBar::setLabel(view, Qt::Key_Select, QSoftMenuBar::Select);

    // only show 'cancel transfer' in context menu if transfer is in progress
    m_cancelAction->setVisible(!file.finished());
}

void ReceiveWindow::showWindow()
{
    showMaximized();
    activateWindow();
    raise();
}

void ReceiveWindow::stopCurrentTransfer()
{
    qLog(Obex) << "User wants to abort file transfer";

    QListView *view = qobject_cast<QListView *>(m_tabs->currentWidget());
    if (!view)
        return;
    QModelIndex index = view->currentIndex();
    if (!index.isValid())
        return;
    const FileTransfer &file = m_model->findFile(index);
    emit abortTransfer(file.id());
}

bool ReceiveWindow::handleVObjectReceived(int id, bool err)
{
    int index = -1;
    for (int i = 0; i < m_vObjects.size(); i++) {
        if (m_vObjects[i].id == id) {
            index = i;
            break;
        }
    }

    if (index == -1)
        return false;

    qLog(Obex) << "Found vCard/vCalendar file...";

    VObjectTransfer transfer = m_vObjects.takeAt(index);

    if (err) {
        qLog(Obex) << "Error occurred, deleting received file...";
        QString fn = Qtopia::tempDir() + "obex/in/" + transfer.fileName;
        ::unlink(fn.toLocal8Bit().data());
        return true;
    }

    qLog(Obex) << "Figuring out mimeType and app to send to";

    QString incoming = Qtopia::tempDir() + "obex/in/" + transfer.fileName;
    qLog(Obex) << "Temp file:" << incoming << "exists?" << QFile::exists(incoming);

    QMimeType mt(transfer.mimeType);

    qLog(Obex) << "Mimetype is: " << mt.id();
    QString service = "Receive/"+mt.id();
    qLog(Obex) << "Service is: " << service;
    QString receiveChannel = QtopiaService::channel(service);
    qLog(Obex) << "receive Channel is: " << receiveChannel;

    if ( receiveChannel.isEmpty() ) {
        // Special cases...
        // ##### should split file, or some other full fix
        if ( mt.id().toLower() == "text/x-vcalendar" ) {
            bool calendar, todo;
            vcalInfo(Qtopia::tempDir() + "obex/in/" + transfer.fileName,
                     &todo, &calendar);
            if ( calendar ) {
                receiveChannel = QtopiaService::channel(service+"-Events");
            }
            else if ( todo ) {
                receiveChannel = QtopiaService::channel(service+"-Tasks");
            }
        }
    }

    if ( !receiveChannel.isEmpty() ) {
        // Send immediately
        QContent lnk(QtopiaService::app(service));
        qLog(Obex) << "Sending QtopiaIpcEnvelope";
        QtopiaIpcEnvelope e( receiveChannel, "receiveData(QString,QString)");
        e << Qtopia::tempDir() + "obex/in/" + transfer.fileName << mt.id();
    }

    return true;
}

#include "receivewindow.moc"
