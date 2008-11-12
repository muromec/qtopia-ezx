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

#include "qspeeddial.h"

#include <qtopiaapplication.h>
#include <qtranslatablesettings.h>
#include <qexpressionevaluator.h>
#include <qsoftmenubar.h>

#include <QPainter>
#include <QLineEdit>
#include <QLayout>
#include <QPushButton>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QDialog>
#include <QHash>
#include <QListWidgetItem>
#include <QAction>
#include <QMenu>
#include <QSettings>

#include <time.h>



static QHash<QString, QtopiaServiceDescription*>* recs=0;
static QString recs_filename;
static QDateTime recs_ts;

/*!
  \internal
  Removes all input and service mapping.
*/
static void clearReqs()
{
    if(recs)
    {
        delete recs;
        recs = 0;
    }
}

/*!
  \internal
  Reads the configuration file and populates recs.
*/
static void updateReqs()
{
    // recs_filename may not exist (never written), in which case, must
    // at least read once, even though recs_ts remains null.
    bool forceread=false;

    if ( recs_filename.isEmpty() ) {
        recs_filename = QTranslatableSettings(QLatin1String("Trolltech"),QLatin1String("SpeedDial")).fileName();
        forceread = true; // once
    }
    QFileInfo fi(recs_filename);
    QDateTime ts = fi.lastModified();

    if( forceread || ts != recs_ts )
    {
        clearReqs();

        recs = new QHash<QString, QtopiaServiceDescription*>;
        recs_ts = ts;

        QTranslatableSettings cfg(QLatin1String("Trolltech"),QLatin1String("SpeedDial"));
        cfg.beginGroup(QLatin1String("Dial"));

        QStringList d = cfg.value(QLatin1String("Defined")).toString().split( ',');
        for(QStringList::ConstIterator it = d.begin(); it != d.end(); ++it)
        {
            cfg.endGroup();
            cfg.beginGroup(QLatin1String("Dial") + *it);

            QString s = cfg.value(QLatin1String("Service")).toString();

            if (s.isEmpty()
                || cfg.value(QLatin1String("RemoveIfUnavailable")).toBool()
                  && QtopiaService::app(s).isEmpty()
            )
                continue;

            QByteArray r = cfg.value(QLatin1String("Requires")).toByteArray();
            QExpressionEvaluator expr(r);
            if ( !r.isEmpty() && !(expr.isValid() && expr.evaluate() && expr.result().toBool()) )
                continue;

            QString m = cfg.value(QLatin1String("Message")).toString();
            QByteArray a = cfg.value(QLatin1String("Args")).toByteArray();
            QString l = cfg.value(QLatin1String("Label")).toString();
            QString ic = cfg.value(QLatin1String("Icon")).toString();
            QMap<QString, QVariant> p = cfg.value(QLatin1String("OptionalProperties")).toMap();

            QtopiaServiceRequest req(s, m.toLatin1());

            if(!a.isEmpty())
                QtopiaServiceRequest::deserializeArguments(req, a);

            QtopiaServiceDescription* t = new QtopiaServiceDescription(req, l, ic, p);
            recs->insert((*it), t);
        }
    }
}

/*!
  \internal
  Writes the current recs to the configuration file.
*/
static void writeReqs(const QString& changed)
{
    QSettings cfg(QLatin1String("Trolltech"),QLatin1String("SpeedDial"));
    QStringList strList;
    QHashIterator<QString, QtopiaServiceDescription*> it(*recs);
    QtopiaServiceDescription* rec;
    bool found = false;

    while( it.hasNext() )
    {
        it.next();

        rec = it.value();
        strList.append(it.key());
        if( changed.isNull() || changed == it.key())
        {
            cfg.beginGroup(QLatin1String("Dial")+it.key());
            cfg.setValue(QLatin1String("Service"),rec->request().service());
            cfg.setValue(QLatin1String("Message"),QString(rec->request().message()));
            cfg.setValue(QLatin1String("Args"),
                         QtopiaServiceRequest::serializeArguments(rec->request()));
            cfg.setValue(QLatin1String("Label"),rec->label());
            cfg.setValue(QLatin1String("Icon"),rec->iconName());
            cfg.setValue(QLatin1String("OptionalProperties"), rec->optionalProperties());
            found = true;
            cfg.endGroup();
        }
    }

    if( !found )
    {
        // removed it
        cfg.remove(QLatin1String("Dial") + changed);
    }

    cfg.beginGroup(QLatin1String("Dial"));
    cfg.setValue(QLatin1String("Defined"), strList.join(QString(',')));
    cfg.endGroup();

    if ( !recs_filename.isEmpty() )
        recs_ts = QFileInfo(recs_filename).lastModified();
}

/*!
  \internal
  Finds the first available input slot to highlight.
*/
int firstAvailableSlot()
{
    // possible slots - 1 ~ 99
    QSettings cfg(QLatin1String("Trolltech"),QLatin1String("SpeedDial"));
    cfg.beginGroup(QLatin1String("Dial"));
    QStringList d = cfg.value(QLatin1String("Defined")).toString().split( ',');
    QList<int> defined;
    bool ok;
    int num;
    // convert to int
    foreach (QString str, d) {
        num = str.toInt(&ok);
        if (ok)
            defined << num;
    }
    qSort(defined);
    num = 1;
    // search for the first empty slot
    foreach (int i, defined) {
        if (num != i)
            break;
        else
            num++;
    }
    // all slots are used.
    if (num == 100)
        num = 0;
    return num;
}


class QSpeedDialDialog : public QDialog
{
    Q_OBJECT
public:
    QSpeedDialDialog(const QString& l, const QString& ic, const QtopiaServiceRequest& a,
        QWidget* parent);
    QSpeedDialDialog(QWidget* parent);
    QString choice();
    void setBlankSetEnabled(bool on) { list->setBlankSetEnabled(on); }
private slots:
    void store(const int row);

private:
    void init();
    QtopiaServiceRequest action;
    QString label, icon;
    QLineEdit *inputle; // non-integer speeddials no supported yet
    QWidget *currinfo;
    QLabel *curricon;
    QLabel *currlabel;
    QPushButton *ok;
    QSpeedDialList *list;

    QString userChoice;
};

class QSpeedDialItem
{
public:
    QSpeedDialItem(const QString& i, const QtopiaServiceDescription& rec, QListView* parent);

    void changeRecord(QtopiaServiceDescription* src);

    QString input() const { return _input; }
    QtopiaServiceDescription description() const { return _record; }
    void clear()
    {
        _record = QtopiaServiceDescription();
    }

private:
    QString _input;
    QtopiaServiceDescription _record;
};


class QSpeedDialModel : public QAbstractListModel
{
    friend class QSpeedDialItem;

public:
    QSpeedDialModel(QObject* parent = 0);
    ~QSpeedDialModel();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QSpeedDialItem* item(const QModelIndex & index);
    QSpeedDialItem* item(int index);
    QSpeedDialItem* takeItem(int index);

private:
    QList<QSpeedDialItem*> mItems;

    void addItem(QSpeedDialItem* item);
};

class QSpeedDialItemDelegate : public QAbstractItemDelegate
{
public:
    QSpeedDialItemDelegate(QListView* parent);
    ~QSpeedDialItemDelegate();

    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;

    void setSelectionDetails(QString label, QString icon);
    void setBlankSetEnabled(bool on) { showSet = on; }
    bool isBlankSetEnabled() const { return showSet; }

private:
    QListView* parentList;
    int iconSize;
    int textHeight;
    int itemHeight;

    QIcon selIcon;
    QString selText;
    bool showInput;
    bool showSet;
};




class QSpeedDialListPrivate : QObject
{
    Q_OBJECT
public:
    QSpeedDialListPrivate(QSpeedDialList* parent) :
        delegate(parent),
        model(parent),
        parentList(parent),
        a_edit(0),
        a_del(0),
	recentChange(-1)
    {
        parent->setItemDelegate(&delegate);
        parent->setModel(&model);
        actionChooser = 0;
        allowActionChooser = false;
        connect(parentList,SIGNAL(currentRowChanged(int)),
                this,SLOT(updateActions()));
    }



    void setSelectionDetails(QString label, QString icon)
    {
        delegate.setSelectionDetails(label, icon);
    }

    QSpeedDialItem* item(int row)
    {
        return model.item(row);
    }

    QSpeedDialItemDelegate delegate;
    QSpeedDialModel model;
    QSpeedDialList* parentList;
    QtopiaServiceSelector* actionChooser;
    bool allowActionChooser;

    QAction *a_edit;
    QAction *a_del;

    QString seltext, selicon;
    int sel;
    int sel_tid;
    int recentChange;

private slots:
    void updateActions()
    {
        QSpeedDialItem* i = item(parentList->currentRow());
        if (a_del)
            a_del->setVisible(i && !i->description().request().isNull());
    }
};




/*!
  Constructs a QSpeedDialItem object.
*/
QSpeedDialItem::QSpeedDialItem(const QString& i, const QtopiaServiceDescription& rec, QListView* parent) :
    _input(i),
    _record(rec)
{
    ((QSpeedDialModel*)parent->model())->addItem(this);
}

/*!
  Updates service description for this item.
*/
void QSpeedDialItem::changeRecord(QtopiaServiceDescription* src)
{
    _record = *src;
}


/*!
  Constructs a QSpeedDialDialog object for storing the given action.
*/
QSpeedDialDialog::QSpeedDialDialog(const QString& l, const QString& ic,
    const QtopiaServiceRequest& a, QWidget* parent) :
    QDialog(parent),
    action(a),
    label(l),
    icon(ic)
{
    init();
}

/*!
  Constructs a QSpeedDialDialog object for selecting a speed dial action.
*/
QSpeedDialDialog::QSpeedDialDialog(QWidget* parent) :
    QDialog(parent)
{
    init();
}

void QSpeedDialDialog::init()
{
    setModal(true);
    setWindowState(windowState() | Qt::WindowMaximized);

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setMargin(0);

    if ( action.isNull() ) {
        list = new QSpeedDialList(this);
        list->setActionChooserEnabled(false);
    } else {
        list = new QSpeedDialList(label, icon, this);
    }
    list->setFrameStyle(QFrame::NoFrame);
    if (list->count() > 0) {
        int currentRow = action.isNull() ? 0 : firstAvailableSlot() - 1;
        list->setCurrentRow(currentRow);
        // scroll to the row above the current row to indicate this row is the next available slot.
        list->scrollTo(list->model()->index( currentRow > 0 ? currentRow - 1 : currentRow, 0, QModelIndex()));
    } else {
        list->setCurrentRow(-1);
    }
    vb->addWidget(list);

    setWindowTitle(list->windowTitle());

    connect(list, SIGNAL(rowClicked(int)),
        this, SLOT(store(int)));

    QtopiaApplication::setMenuLike(this, true);
}

/*!
  Returns the selected input.
*/
QString QSpeedDialDialog::choice()
{
    return userChoice;
}

/*!
  Saves the service description at the input slot selected by the user.
*/
void QSpeedDialDialog::store(const int row)
{
    if(list->d->recentChange==row){
	    list->d->recentChange=-1;
	    return;
    }
    list->d->recentChange=-1;

    userChoice = QString("%1").arg(row+1);//Row 0 has speed dial 1
    if ( !action.isNull() ){
        QSpeedDial::set(userChoice, QtopiaServiceDescription(action, label, icon));
    }else if ( list->isBlankSetEnabled() ) {
        QtopiaServiceDescription* desc = QSpeedDial::find(userChoice);
        if ( !desc || desc->request().isNull() ) {
            //list->editItem(row);
            return; // no accept yet
        }
    }
    accept();
}



/*!
  \class QSpeedDialList
  \mainclass
  \brief The QSpeedDialList class provides a list widget for editing Speed Dial entries.

  If you need a dialog that allows the user to select a spot to insert an already selected
  action (for example, adding a QContact's phone number to Speed Dial list), use
  QSpeedDial::addWithDialog().

  Use editItem() to edit selected entry.
  This will open QtopiaServiceSelector which provides a list of predefined services.

  Use clearItem() to remove the entry.

  \image qspeeddiallist.png "Editing Speed Dial Entries"

  \sa QSpeedDial, QtopiaServiceSelector

  \ingroup userinput
*/

/*!
  \fn QSpeedDialList::currentRowChanged(int row)

  This signal is emitted whenever the user selects
  a different \a row (either with the keypad or the mouse).
*/

/*!
  \fn QSpeedDialList::rowClicked(int row)

  This signal is emitted whenever the user
  either clicks on a different \a row with the mouse, or presses the keypad
  Select key while a row is selected.
*/

/*!
  \fn QSpeedDialList::itemSelected(QString input)

  This signal is emitted whenever the user selects the item with
  the specified \a input string.
*/

/*!
  Constructs a QSpeedDialList object with the given \a parent.
*/
QSpeedDialList::QSpeedDialList(QWidget* parent) :
    QListView(parent)
{
    updateReqs();
    init(QString());

    setActionChooserEnabled(true);

    connect(this, SIGNAL(rowClicked(int)), this, SLOT(editItem(int)));
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(sendRowChanged()));

    setWindowTitle( tr( "Speed Dial" ) );
}

/*!
    If \a on, makes blank items have a "Set..." option, even if
    setActionChooserEnabled() has been called with the inverse
    of \a on as its parameter.

    \sa isBlankSetEnabled()
*/
void QSpeedDialList::setBlankSetEnabled(bool on)
{
    d->delegate.setBlankSetEnabled(on);
}

/*!
    Returns true if blank items have a "Set..." option, even if
    setActionChooserEnabled() has been called.

    \sa setBlankSetEnabled()
*/
bool QSpeedDialList::isBlankSetEnabled() const
{
    return d->delegate.isBlankSetEnabled();
}

void QSpeedDialList::setActionChooserEnabled(bool on)
{
    d->allowActionChooser = on;
    if ( on ) {
        QMenu *contextMenu = QSoftMenuBar::menuFor(this);

        d->a_edit = new QAction( QIcon( ":icon/edit" ), tr("Set...", "set action"), this);
        connect( d->a_edit, SIGNAL(triggered()), this, SLOT(editItem()) );
        contextMenu->addAction(d->a_edit);

        d->a_del = new QAction( QIcon( ":icon/trash" ), tr("Delete"), this);
        connect( d->a_del, SIGNAL(triggered()), this, SLOT(clearItem()) );
        contextMenu->addAction(d->a_del);
    } else {
        delete d->a_edit;
        d->a_edit = 0;
        delete d->a_del;
        d->a_del = 0;
    }
    d->delegate.setBlankSetEnabled(on);
}

/*!
  Destroys the QSpeedDialList object.
*/
QSpeedDialList::~QSpeedDialList()
{
    delete d;
}

/*!
  \internal

  Used by QSpeedDialDialog to create a special single-action insertion version of the list.
*/
QSpeedDialList::QSpeedDialList(const QString& label, const QString& icon, QWidget* parent) :
    QListView(parent)
{
    updateReqs();
    init(QString());

    setWindowTitle( tr( "Speed Dial" ) );

    d->setSelectionDetails(label, icon);
}

/*!
  Removes the Speed Dial entry in the list at \a row.
*/
void QSpeedDialList::clearItem(int row)
{
    QSpeedDialItem* item = d->item(row);
    if( item  ) {
        item->clear();
        QSpeedDial::remove(item->input());
    }
}

/*!
  \overload

  Removes the Speed Dial entry at the currently selected row.
*/
void QSpeedDialList::clearItem()
{
    QSpeedDialItem* item = d->item(currentRow());
    if( item  ) {
        item->clear();
        QSpeedDial::remove(item->input());
    }
}

/*!
  Edits the Speed Dial entry at \a row.

  Presents the a QtopiaServiceSelector for the user
  to select an service performed when the input at \a row is triggered.

  \sa QtopiaServiceSelector
*/
void QSpeedDialList::editItem(int row)
{
    if ( (d->allowActionChooser || isBlankSetEnabled()) && !d->actionChooser ) {
        d->actionChooser = new QtopiaServiceSelector(this);
        d->actionChooser->addApplications();
        QtopiaApplication::setMenuLike( d->actionChooser, true );
    }
    QSpeedDialItem* item = d->item(row);
    if( item && (d->allowActionChooser || isBlankSetEnabled()) ) {
        QtopiaServiceDescription desc = item->description();
        if ( d->allowActionChooser || desc.request().isNull() ) {
            d->recentChange=row;
            if ( d->actionChooser->edit(item->input(),desc) ) {
                if ( desc.request().isNull() ) {
                    QSpeedDial::remove(item->input());
                } else {
                    QSpeedDial::set(item->input(), desc);
                }
                reload(item->input());
                // force a reload of the context menu
                //emit activated(currentIndex());
                return;
            }
        }
    }
    emit itemSelected(rowInput(row));
}

/*!
  \overload

  Edits the Speed Dial entry at the currently selected row.
*/
void QSpeedDialList::editItem()
{
    editItem(currentRow());
}

/*!
  \internal

  Used to initialize the list and populate it with entries.
*/
void QSpeedDialList::init(const QString& f)
{
    d = new QSpeedDialListPrivate(this);
    d->sel = 0;
    d->sel_tid = 0;
    int fn = 0;

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //
    //  Create entries for each possible speed dial number
    //

    for( int i = 1; i < 100; i++ )
    {
        QString inp = QString::number(i);
        QHash<QString, QtopiaServiceDescription*>::iterator it = recs->find(inp);
        QtopiaServiceDescription* rec = 0;

        if(it != recs->end())
            rec = recs->find(inp).value();

        new QSpeedDialItem(QString::number(i), rec ? *rec : QtopiaServiceDescription(), this);
        if( f == inp || i == fn )
            setCurrentRow(i - 1);
    }

    if(f.isEmpty() && model()->rowCount() > 0)
        setCurrentRow(0);

    //
    //  Deal with speed dials above 100 ?
    //

    QHashIterator<QString, QtopiaServiceDescription*> it(*recs);
    QtopiaServiceDescription* rec;
    while( it.hasNext() )
    {
        it.next();
        rec = it.value();
        QString inp = it.key();
        bool ok;
        int num = inp.toInt(&ok);

        if( !ok || num > 99 ) // skip numbers 0..99, done above
        {
            new QSpeedDialItem(inp, *rec, this);
            if( f == inp )
                setCurrentRow(count() - 1);
        }
    }

    connect(this,SIGNAL(activated(QModelIndex)),
            this,SLOT(select(QModelIndex)));
    connect(this,SIGNAL(clicked(QModelIndex)),
            this,SLOT(click(QModelIndex)));
}

void QSpeedDialList::setCurrentRow(int row)
{
    setCurrentIndex(model()->index(row, 0));
}

/*!
  \fn void QSpeedDialList::setCurrentInput(const QString& input)

  Selects the row from the list that corresponds to the Speed Dial \a input.
*/
void QSpeedDialList::setCurrentInput(const QString& sd)
{
    for(int i = 0; i < (int)count(); ++i)
    {
        if(d->item(i)->input() == sd)
        {
            setCurrentRow(i);
            return;
        }
    }
}


/*!
  \fn void QSpeedDialList::reload(const QString& input)

  Forces the entry for Speed Dial at \a input to be refreshed from the source.
*/
void QSpeedDialList::reload(const QString& sd)
{
    QHash<QString, QtopiaServiceDescription*>::iterator it = recs->find(sd);
    QtopiaServiceDescription* r = 0;

    if(it != recs->end())
        r = it.value();

    for(int i = 0; i < (int)count(); ++i)
    {
        QSpeedDialItem *it = (QSpeedDialItem*)d->item(i);
        if( it && it->input() == sd )
        {
            if( r )
                it->changeRecord(r);
            else
                it->clear();

            update();
            return;
        }
    }

    if( r )
    {
        new QSpeedDialItem(sd, *r, this);
        setCurrentRow(count() - 1);
    }
}

/*!
  \internal

  Catches selection events and emits an event more meaningful to outside this class
  (emits a row instead of a QModelIndex)
*/
void QSpeedDialList::sendRowChanged()
{
    emit currentRowChanged(currentRow());
}

/*!
  \internal

  Catches selection events and emits an event more meaningful to outside this class
  (emits a row instead of a QModelIndex)
*/
void QSpeedDialList::select(const QModelIndex& index)
{
    emit currentRowChanged(index.row());
}

/*!
  \internal

  Catches click events and emits an event more meaningful to outside this class
  (emits a row instead of a QModelIndex)
*/
void QSpeedDialList::click(const QModelIndex& index)
{
    emit currentRowChanged(index.row());
    emit rowClicked(index.row());
}

/*!
  Returns the input required to trigger the Speed Dial entry at \a row.
*/
QString QSpeedDialList::rowInput(int row) const
{
    if(row >= 0 && row < count())
        return d->item(row)->input();
    else
        return QString();
}

/*!
  \property QSpeedDialList::currentInput
  \brief the input required for the currently selected Speed Dial entry if exists;
  otherwise returns an empty string
*/
QString QSpeedDialList::currentInput() const
{
    QSpeedDialItem *it = d->item(currentRow());
    return it ? it->input() : QString();
}

/*!
  \internal

  Detects button presses and uses them to navigate the list and make selections
*/
void QSpeedDialList::keyPressEvent(QKeyEvent* e)
{
    int k = e->key();
    if( k >= Qt::Key_0 && k <= Qt::Key_9 )
    {
        d->sel = d->sel * 10 + k - Qt::Key_0;
        if ( d->sel_tid )
        {
            killTimer(d->sel_tid);
            d->sel_tid = 0;
        }

        if ( d->sel )
        {
            setCurrentInput(QString::number(d->sel));
            if ( d->sel < 10 )
                d->sel_tid = startTimer(800);
            else
                d->sel = 0;
        }
    }
    else if( k == Qt::Key_Select )
    {
        if(currentRow() > -1)
            emit rowClicked(currentRow());
    }
    else
    {
        QListView::keyPressEvent(e);
    }
}

/*!
  \internal

  Used as a timer to help detect Speed Dial style user input (holding down number keys)
*/
void QSpeedDialList::timerEvent(QTimerEvent* e)
{
    if ( e->timerId() == d->sel_tid ) {
        killTimer(d->sel_tid);
        d->sel = 0;
        d->sel_tid = 0;
    } else {
        QListView::timerEvent( e );
    }
}

/*!
  \internal

  Make sure the list gets redisplayed on scroll
*/
void QSpeedDialList::scrollContentsBy(int dx, int dy)
{
    QListView::scrollContentsBy(dx, dy);
    update();
}

/*!
  \property QSpeedDialList::count
  \brief the number of rows in the list
*/
int QSpeedDialList::count() const
{
    return ((QSpeedDialModel*)model())->rowCount();
}

/*!
  \property QSpeedDialList::currentRow
  \brief the currently selected row number
*/
int QSpeedDialList::currentRow() const
{
    return currentIndex().row();
}




/*!
  \class QSpeedDial
  \mainclass
  \brief The QSpeedDial class provides access to the Speed Dial settings.

  The QSpeedDial class includes a set of static functions that give access to the
  Speed Dial settings. This class should not be instantiated.

  The input range is from 1 to 99. To allow the user to select
  an input for a given action, use addWithDialog().
  \image qspeeddial.png "Add with Dialog"

  To directly modify the Speed Dial settings, use remove() and set().
  Use find() to retrieve the assigned action for a given input.

  \sa QSpeedDialList

  \ingroup userinput
*/

/*!
  Provides a dialog that allows the user to select an input for action \a action,
  using \a label and \a icon as the display label and icon respectively.
  The dialog has the given \a parent.

  Returns the input that the user selected, and assigns the input to the action. 

  If the user cancels the dialog, a null string is returned.

  \sa set()
*/
QString QSpeedDial::addWithDialog(const QString& label, const QString& icon,
    const QtopiaServiceRequest& action, QWidget* parent)
{
    QSpeedDialDialog dlg(label, icon, action, parent);
    dlg.setObjectName("speeddial");
    if ( QtopiaApplication::execDialog(&dlg) )
        return dlg.choice();
    else
        return QString();
}

/*!
  Provides a dialog that allows the user to select an existing speed dial item.
  The dialog has the given \a parent.

  This dialog may be useful for example to provide a list of "Favourites".

  Returns the speed dial selected.

  If the user cancels the dialog, a null pointer is returned.
*/
QString QSpeedDial::selectWithDialog(QWidget* parent)
{
    QSpeedDialDialog dlg(parent);
    dlg.setObjectName("speeddial");
    dlg.setBlankSetEnabled(true);
    if ( QtopiaApplication::execDialog(&dlg) )
        return dlg.choice();
    else
        return 0;
}


/*!
  Returns a QtopiaServiceDescription for the given Speed Dial \a input.
*/
QtopiaServiceDescription* QSpeedDial::find(const QString& input)
{
    QHash<QString, QtopiaServiceDescription*>::iterator it;
    QtopiaServiceDescription* rec;

    updateReqs();

    it = recs->find(input);
    if(it != recs->end())
    {
        rec = it.value();
        return rec;
    }

    return 0;
}

/*!
  Removes the action currently associated with the given Speed Dial \a input.
*/
void QSpeedDial::remove(const QString& input)
{
    updateReqs();
    recs->remove(input);
    writeReqs(input); // NB. must do this otherwise won't work
}

/*!
  Assigns the given QtopiaServiceDescription, \a r, as the action to perform when the given
  Speed Dial \a input, is detected.
*/
void QSpeedDial::set(const QString& input, const QtopiaServiceDescription& r)
{
    updateReqs();
    recs->insert(input,new QtopiaServiceDescription(r));
    writeReqs(input);
}

/*!
  Returns a list of the currently assigned Speed Dial inputs.
  \since 4.3
  \sa possibleInputs()
*/
QList<QString> QSpeedDial::assignedInputs()
{
    updateReqs();
    return recs->uniqueKeys();
}

/*!
  Returns a list of possible Speed Dial inputs, some of which
  may be assigned already.

  \since 4.3
  \sa assignedInputs()
*/
QList<QString> QSpeedDial::possibleInputs()
{
    QList<QString> ret;

    for (int i = 1; i < 100; i++) {
        ret << QString::number(i);
    }
    return ret;
}

QSpeedDialItemDelegate::QSpeedDialItemDelegate(QListView* parent)
    : QAbstractItemDelegate(parent)
{
    parentList = parent;

    iconSize = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    showInput = !QApplication::style()->inherits("QThumbStyle");
    showSet = false;

    QFontMetrics fm(parent->font());
    textHeight = fm.height();

    if(iconSize > textHeight)
        itemHeight = iconSize;
    else
        itemHeight = textHeight;
}

QSpeedDialItemDelegate::~QSpeedDialItemDelegate()
{
}

void QSpeedDialItemDelegate::setSelectionDetails(QString label, QString icon)
{
    selText = label;
    selIcon = QIcon(QLatin1String(":icon/")+icon);
    if (selIcon.isNull())
        selIcon = QIcon(QLatin1String(":image/")+icon);
}

void QSpeedDialItemDelegate::paint(QPainter * painter,
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QSpeedDialModel* model = (QSpeedDialModel*)index.model();

    if(model)
    {
        QFontMetrics fm(option.font);
        int x = option.rect.x();
        int y = option.rect.y();
        int width = option.rect.width();
        int height = option.rect.height()-1;
        bool selected = option.state & QStyle::State_Selected;

        if(selected)
        {
            painter->setPen(option.palette.highlightedText().color());
            painter->fillRect(option.rect, option.palette.highlight());
        }
        else
        {
            painter->setPen(option.palette.text().color());
            painter->fillRect(option.rect, option.palette.base());
        }

        QSpeedDialItem* item = model->item(index);
        QString input = item->input();
        QPixmap pixmap;
        QString label;

        if(selected && !selText.isEmpty())
        {
            pixmap = selIcon.pixmap(option.decorationSize);
            label = selText;
        }
        else
        {
            if (!item->description().iconName().isEmpty())
                pixmap = QIcon(QLatin1String(":icon/")+item->description().iconName()).pixmap(option.decorationSize);
            label = item->description().label();
        }

        QTextOption to;
        to.setAlignment( QStyle::visualAlignment(qApp->layoutDirection(),
                    Qt::AlignLeft) | Qt::AlignVCenter);
        bool rtl = qApp->layoutDirection() == Qt::RightToLeft;

        if ( showInput ) {
            painter->drawText(QRect(x, y+1, width, height), input, to);
            if ( rtl )
                width -= fm.width(input);
            else
                x += fm.width(input);

            painter->drawText(QRect(x,y+1,width,height), QLatin1String(":"), to);
            if ( rtl )
                width -= fm.width(QLatin1String(": "));
            else
                x += fm.width(QLatin1String(": "));
        }

        if ( showSet && pixmap.isNull() && label.isEmpty() ) {
            pixmap = QIcon(":icon/reset").pixmap(option.decorationSize);
            label = QSpeedDialList::tr("Set...", "set action");
        }

        if(!pixmap.isNull())
        {
            if ( rtl ) {
                painter->drawPixmap(x+width-pixmap.width(), y+1, pixmap);
                width -= pixmap.width() + fm.width(QLatin1String(" "));
            } else {
                painter->drawPixmap(x, y+1, pixmap);
                x += pixmap.width() + fm.width(QLatin1String(" "));
            }
        }
        label = elidedText( fm, width, Qt::ElideRight, label );
        painter->drawText(QRect(x, y+1, width, height), label, to);
    }
}

QSize QSpeedDialItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(parentList->viewport()->width(), itemHeight+2);
}



QSpeedDialModel::QSpeedDialModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QSpeedDialModel::~QSpeedDialModel()
{
    for(int i = 0; i < mItems.size(); i++)
        delete mItems[i];
}

int QSpeedDialModel::rowCount(const QModelIndex & parent) const
{
    if(parent.parent() == QModelIndex())
        return mItems.count();
    else
        return 0;
}

QVariant QSpeedDialModel::data(const QModelIndex &, int) const
{
    return QVariant();
}

QSpeedDialItem* QSpeedDialModel::item(const QModelIndex & index)
{
    int row = index.row();
    int rowcount = rowCount();

    if(row >= 0 && row < rowcount)
        return mItems.at(index.row());
    else if(rowcount)
        return mItems.at(rowcount - 1);
    else
        return 0;
}

QSpeedDialItem* QSpeedDialModel::item(int index)
{
    if(index >= 0 && index < mItems.count())
        return mItems.at(index);
    else
        return 0;
}

void QSpeedDialModel::addItem(QSpeedDialItem* item)
{
    mItems.append(item);
}



#include "qspeeddial.moc"
