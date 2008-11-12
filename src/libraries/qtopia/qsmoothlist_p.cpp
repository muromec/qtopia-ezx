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

#include "qsmoothlist_p.h"
#include "motionpath_p.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QDebug>
#include <QKeyEvent>
#include <QPainterPath>
#include <QPixmap>
#include <QDirectPainter>
#include <math.h>
#include <QTime>
#include <QImage>
#include <QExportedBackground>
#include <QtopiaApplication>
#include <QFocusEvent>
#include <QTimer>

#ifdef QT_QWS_GREENPHONE
#define ENABLE_DIRECTPAINTER
#endif

bool direct = false;

// From qtopiaapplication.cpp
class QTcpSocket;
class SyncPainter : public QDirectPainter
{
public:
    SyncPainter(QObject *parent = 0);
    virtual ~SyncPainter();

    void syncRegion(const QRect &);

private:
    QTcpSocket *socket;
};

class FPainter : public SyncPainter
{
    Q_OBJECT
public:
    FPainter( QObject * parent = 0, SurfaceFlag flag = NonReserved ) : opacity(0x80) { buffer = 0; fBuffer = 0; }
    ~FPainter() { delete [] buffer; buffer = 0; }
    void setRect(const QRect &);
    void clear();
    void setOpacity(qreal);
    void copy(const QRect &, const QImage &);
    void blit(const QRect &, const QImage &);

    void flip(const QRect &);
    void flip();
    
signals:
    void needUpdate(bool);

private:
    QRect fRect;
    uchar *fBuffer;
    int linestep;

    int width;
    int height;
    uchar *buffer;

    uchar opacity;
};

void FPainter::setRect(const QRect &rect)
{
    bool needDelete = (rect != fRect) ? true : false;
    fRect = rect;

    width = rect.width();
    height = rect.height();

    linestep = QDirectPainter::linestep();
    fBuffer = QDirectPainter::frameBuffer();

    if (buffer && needDelete) {
        delete[] buffer;
        buffer = 0;
    }
    if (!buffer)
        buffer = new uchar[width * height * 2];

    qWarning() << "Syncing region" << rect;
    syncRegion(rect);

    emit needUpdate(true);
}

void FPainter::clear()
{
    ::memset(buffer, 0x77, width * height * 2);
}

void FPainter::setOpacity(qreal value)
{
    if(value == 1.0f) {
        opacity = 0x80;
    } else if(value == 0.0f) {
        opacity = 0x00;
    } else {
        opacity = (qreal)0x80 * value;
    }
}

void FPainter::copy(const QRect &rect, const QImage &img)
{
    QPoint screenPos(0, 0);
    QPoint imgPos(0, 0);

    if(rect.x() > 0)
        screenPos.setX(rect.x());
    else
        imgPos.setX(-1 * rect.x());

    if(rect.y() > 0)
        screenPos.setY(rect.y());
    else
        imgPos.setY(-1 * rect.y());

    imgPos = img.offset();

    QRect screenRect(0, 0, width, height);
    if(!screenRect.contains(screenPos) ||
       !img.rect().contains(imgPos))
        return; // Not on screen

    int bwidth = qMin(width - screenPos.x(), img.width() - imgPos.x());
    int bheight = qMin(height - screenPos.y(), img.height() - imgPos.y());

    const ushort *iBits = (ushort *)img.bits();
    int iStep = img.bytesPerLine() / 2;

    iBits += imgPos.y() * iStep;
    iBits += imgPos.x();

    ushort *bBits = (ushort *)buffer;
    int bStep = width;

    bBits += screenPos.y() * bStep;
    bBits += screenPos.x();


    for(int ii = 0; ii < bheight; ++ii) {
        ::memcpy(bBits, iBits, bwidth * 2);
        bBits += bStep;
        iBits += iStep;
    }
}

inline ushort qConvertRgb32To16(uint c)
{
   return (((c) >> 3) & 0x001f)
       | (((c) >> 5) & 0x07e0)
       | (((c) >> 8) & 0xf800);
}

inline uint qConvertRgb16To32(uint c)
{
    return 0xff000000
        | ((((c) << 3) & 0xf8) | (((c) >> 2) & 0x7))
        | ((((c) << 5) & 0xfc00) | (((c) >> 1) & 0x300))
        | ((((c) << 8) & 0xf80000) | (((c) << 3) & 0x70000));
}

static inline uint BYTE_MUL(uint x, uint a) {
    uint t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff00ff) * a;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}

static inline uint opacity_adjust(uint pixel, uchar opacity)
{
    if(opacity == 0) {
        return 0;
    } else if(opacity == 0x80) {
        return pixel;
    } else {
        uint blend = (((pixel & 0xFF00FF00) >> 7) * opacity) & 0xFF00FF00;
        blend |= (((pixel & 0x00FF00FF) * opacity) >> 7 ) & 0x00FF00FF;
        return blend;
    }

}

static inline ushort blend(ushort screen, uint pixel)
{
#if 1
    uchar alpha = pixel >> 26;
    if(0x3f == alpha) {
        return qConvertRgb32To16(pixel);
    } else if(0x00 == alpha) {
        return screen;
    } else {
        alpha = 0x40 - (alpha + 1);

        ushort blend = (((screen & 0xF81F) * alpha) >> 6) & 0xF81F;
        blend |= (((screen & 0x07e0) * alpha) >> 6) & 0x07e0;
        blend += qConvertRgb32To16(pixel);

        return blend;
    }



#else
    uchar alpha = (pixel & 0xFF000000) >> 24;
    if(0xFF == alpha) {
        return qConvertRgb32To16(pixel);
    } else if(0x00 == alpha) {
        return screen;
    } else {
        uint iscreen = qConvertRgb16To32(screen);

        /*
        uchar ialpha = 255 - alpha;
        iscreen = BYTE_MUL(iscreen, ialpha);
        iscreen += pixel;
        return qConvertRgb32To16(iscreen);
        */
        
        alpha = 0x80 - (alpha >> 1);

        uint blend = (((iscreen & 0x00FF00FF) * alpha) >> 7) & 0x00FF00FF;
        blend |= ((iscreen & 0x0000FF00) * alpha >> 7) & 0x0000FF00;
        blend += pixel;

        return qConvertRgb32To16(blend);
        
    }
#endif
}

void FPainter::flip(const QRect &r)
{
    QRect rect = r.intersect(QRect(0, 0, width, height));

    uchar *output = fBuffer + fRect.y() * linestep + fRect.x() * 2;
    uchar *input = buffer;


    output += rect.y() * linestep + rect.x() * 2;
    input += rect.y() * width * 2 + rect.x() * 2;

    for(int ii = 0; ii < rect.height(); ++ii) {
        ::memcpy(output, input, rect.width() * 2);
        input += width * 2;
        output += linestep;
    }
}

void FPainter::flip()
{
    uchar *output = fBuffer + fRect.y() * linestep + fRect.x() * 2;
    uchar *input = buffer;

    for(int ii = 0; ii < height; ++ii) {
        ::memcpy(output, input, width * 2);
        input += width * 2;
        output += linestep;
    }
}

#define BLEND_SPAN(dest, src, length) \
do { \
    ushort *_d = (ushort*)(dest);         \
    const uint *_s = (uint*)(src);    \
    register int n = ((length) + 7) / 8;      \
    switch ((length) & 0x07)                  \
    {                                         \
    case 0: do { *_d++ = blend(*_d, *_s++);    \
    case 7:      *_d++ = blend(*_d, *_s++);    \
    case 6:      *_d++ = blend(*_d, *_s++);    \
    case 5:      *_d++ = blend(*_d, *_s++);    \
    case 4:      *_d++ = blend(*_d, *_s++);    \
    case 3:      *_d++ = blend(*_d, *_s++);    \
    case 2:      *_d++ = blend(*_d, *_s++);    \
    case 1:      *_d++ = blend(*_d, *_s++);    \
    } while (--n > 0);                        \
    }                                         \
} while(0)

#define BLEND_SPAN_ADJUST(dest, src, length) \
do { \
    ushort *_d = (ushort*)(dest);         \
    const uint *_s = (uint*)(src);    \
    register int n = ((length) + 7) / 8;      \
    switch ((length) & 0x07)                  \
    {                                         \
    case 0: do { *_d++ = blend(*_d, opacity_adjust(*_s++, opacity));    \
    case 7:      *_d++ = blend(*_d, opacity_adjust(*_s++, opacity));    \
    case 6:      *_d++ = blend(*_d, opacity_adjust(*_s++, opacity));    \
    case 5:      *_d++ =blend(*_d, opacity_adjust(*_s++, opacity));    \
    case 4:      *_d++ =blend(*_d, opacity_adjust(*_s++, opacity));    \
    case 3:      *_d++ =blend(*_d, opacity_adjust(*_s++, opacity));    \
    case 2:      *_d++ =blend(*_d, opacity_adjust(*_s++, opacity));    \
    case 1:      *_d++ =blend(*_d, opacity_adjust(*_s++, opacity));    \
    } while (--n > 0);                        \
    }                                         \
} while(0)

void FPainter::blit(const QRect &rect, const QImage &img)
{
    if(opacity == 0)
        return;

    QPoint screenPos(0, 0);
    QPoint imgPos(0, 0);

    if(rect.x() > 0)
        screenPos.setX(rect.x());
    else
        imgPos.setX(-1 * rect.x());

    if(rect.y() > 0)
        screenPos.setY(rect.y());
    else
        imgPos.setY(-1 * rect.y());

    QRect screenRect(0, 0, width, height);
    if(!screenRect.contains(screenPos) ||
       !img.rect().contains(imgPos))
        return; // Not on screen

    int bwidth = qMin(width - screenPos.x(), img.width() - imgPos.x());
    int bheight = qMin(height - screenPos.y(), img.height() - imgPos.y());

    const uint *iBits = (uint *)img.bits();
    int iStep = img.bytesPerLine() / 4;

    iBits += imgPos.y() * iStep;
    iBits += imgPos.x();

    ushort *bBits = (ushort *)buffer;
    int bStep = width;

    bBits += screenPos.y() * bStep;
    bBits += screenPos.x();


    if(opacity == 0x80) {
        for(int ii = 0; ii < bheight; ++ii) {
    /*        for(int jj = 0; jj < bwidth; ++jj) {
                bBits[jj] = blend(bBits[jj], iBits[jj]);
            }*/
            BLEND_SPAN(bBits, iBits, bwidth);
            bBits += bStep;
            iBits += iStep;
        }
    } else {
        for(int ii = 0; ii < bheight; ++ii) {
            /*
            for(int jj = 0; jj < bwidth; ++jj) {
                uint pixel = opacity_adjust(iBits[jj], opacity);
                bBits[jj] = blend(bBits[jj], pixel);
            } */
            BLEND_SPAN_ADJUST(bBits, iBits, bwidth);
            bBits += bStep;
            iBits += iStep;
        }
    }
}

static QStyleOptionViewItemV3 defaultoption;

//initialize default view options
void defaultViewOptionsV3(QSmoothList *w)
{
    QStyleOptionViewItemV3 option;

    option.initFrom(w);

    option.state &= ~QStyle::State_MouseOver;
    option.font = w->font();

    if (!w->hasFocus())
        option.state &= ~QStyle::State_Active;

    option.state &= ~QStyle::State_HasFocus;
    option.decorationSize = w->iconSize();

    option.decorationPosition = QStyleOptionViewItem::Left;
    option.decorationAlignment = Qt::AlignCenter;
    option.displayAlignment = Qt::AlignLeft|Qt::AlignVCenter;
    option.textElideMode = Qt::ElideRight;
    option.rect = QRect();
    option.showDecorationSelected = QApplication::style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, 0);

    //if (wrapItemText)
    //    option.features = QStyleOptionViewItemV2::WrapText;
    option.locale = QLocale::system();
    option.locale.setNumberOptions(QLocale::OmitGroupSeparator);
    option.widget = w;
    option.palette = w->palette();

    option.fontMetrics = w->fontMetrics();
    option.direction = w->layoutDirection();

    defaultoption = option;
}

static /*const*/ int ItemHeight = 0;
static const int MoveThreshold = 10;

class ListItem 
{
public:
    ListItem(const QModelIndex &idx, int width = 0);

    QRect rect() const;
    void setWidth(int width);
    void setDelegate(QAbstractItemDelegate *delegate);

    IValue *listPosition();
    qreal screenPosition() const;

    void paint(FPainter *, bool);
    void paint(QPainter *, bool);
    int index() const;
    QModelIndex modelIndex() const { return m_idx; }
    void invalidateCache() { cacheValid = false; }


private:
    void checkCache(bool);
    QModelIndex m_idx;
    QAbstractItemDelegate *m_delegate;
    int m_width;

    ConcreteValue m_listPos;

    bool cacheValid;
    bool cacheActiveValid;
    QPixmap cache;
    QPixmap cacheActive;
    QImage imageCache;
    QImage imageCacheActive;

public:
    void snap();
    QRect snapRect();
private:
    QRect m_snapRect;
};

class ItemHighlight
{
public:
    ItemHighlight(int width = 0);

    QRect rect() const;
    void setWidth(int width);

    void paint(FPainter *);
    void paint(QPainter *);

    IValue *listPosition();
    IValue *highlightPosition();
    IValue *highlightAlpha();

    qreal screenPosition() const;

private:
    void checkCache();
    int m_width;

    ConcreteValue m_highlightPos;
    ConcreteValue m_listPos;
    ConcreteValue m_highlightAlpha;
    
    bool cacheValid;
    QPixmap cache;
    QImage imageCache;
public:
    void snap();
    QRect snapRect();
private:
    QRect m_snapRect;
};

class Scrollbar 
{
public:
    Scrollbar(int height = 0);

    QRect rect() const;
    void setHeight(int height);
    void setPos(int pos);
    void setListHeight(int height);

    void paint(FPainter *);
    void paint(QPainter *);

    IValue *listPosition();
    IValue *scrollAlpha();

private:
    void checkCache();
    ConcreteValue m_alpha;
    ConcreteValue m_listPos;
    int m_list;
    int m_height;
    int m_pos;
    
    bool cacheValid;
    QPixmap cache;
    QImage imageCache;
public:
    int barHeight();

    void snap();
    QRect snapRect();
private:
    QRect m_snapRect;
};

Scrollbar::Scrollbar(int height)
: m_height(height), cacheValid(false)
{
}

QRect Scrollbar::rect() const
{
    return QRect(m_pos - 4, 2, 4, m_height - 4);
}

IValue *Scrollbar::scrollAlpha()
{
    return &m_alpha;
}

IValue *Scrollbar::listPosition()
{
    return &m_listPos;
}

void Scrollbar::setListHeight(int height)
{
    m_list = height;
    cacheValid = false;
}

void Scrollbar::setHeight(int height)
{
    m_height = height;
    cacheValid = false;
}

void Scrollbar::setPos(int pos)
{
    m_pos = pos;
}

void Scrollbar::checkCache()
{
    if(!cacheValid) {
        int tbarHeight = barHeight();
        cache = QPixmap(4, tbarHeight);
        cache.fill(QColor(0,0,0,0));
        QPainter cp(&cache);
        cp.setPen(Qt::NoPen);
        cp.setBrush(QPalette().color(QPalette::Text));
        cp.setRenderHint(QPainter::Antialiasing);
        cp.drawRoundRect(cache.rect(), 600 / cache.width(), 600 / cache.height());
        imageCache = cache.toImage();
        cacheValid = true;
    }
}

void Scrollbar::paint(FPainter *p)
{
    if(m_alpha.value() == 0.0f)
        return;

    checkCache();

    int tbarHeight = barHeight();
    int barPosMax = rect().height() - tbarHeight;
    int barPos = barPosMax * (m_listPos.value() / (qreal)(m_list - rect().height()));

    p->setOpacity(m_alpha.value());
    p->blit(QRect(rect().x(), rect().y() + barPos, cache.width(), cache.height()), imageCache);
    p->setOpacity(1.0f);
}

void Scrollbar::paint(QPainter *p)
{
    if(m_alpha.value() == 0.0f)
        return;

    checkCache();

    int tbarHeight = barHeight();
    int barPosMax = rect().height() - tbarHeight;
    int barPos = barPosMax * (m_listPos.value() / (qreal)(m_list - rect().height()));

    p->setOpacity(m_alpha.value());
    p->drawPixmap(rect().x(), rect().y() + barPos, cache);
    p->setOpacity(1.0f);
}

int Scrollbar::barHeight()
{
    int barHeight = (qreal)rect().height() * (qreal)rect().height() / (qreal)m_list;

    if(barHeight < (rect().height() / 6))
        barHeight = (rect().height() / 6);
    return barHeight;
}

void Scrollbar::snap()
{
    m_snapRect = rect();
}

QRect Scrollbar::snapRect()
{
    return m_snapRect;
}

ListItem::ListItem(const QModelIndex &idx, int width)
: m_idx(idx), m_width(width), m_listPos(0.0f), cacheValid(false),
  cacheActiveValid(false)
{
}

QRect ListItem::rect() const
{
    return QRect(0, screenPosition(), m_width, ItemHeight);
}

void ListItem::setWidth(int width)
{
    m_width = width;
    cacheValid = false;
    cacheActiveValid = false;
}

IValue *ListItem::listPosition()
{
    return &m_listPos;
}

qreal ListItem::screenPosition() const
{
    return index() * ItemHeight - m_listPos.value();
}

void ListItem::setDelegate(QAbstractItemDelegate *delegate)
{
    m_delegate = delegate;
    QStyleOptionViewItemV3 opt = defaultoption;
    opt.rect = QRect(0, 0, m_width, ItemHeight);
    ItemHeight = m_delegate->sizeHint(opt, m_idx).height();
}

void ListItem::checkCache(bool active)
{
    if(!active && !cacheValid) {
        cache = QPixmap(m_width, ItemHeight);
        cache.fill(QColor(0,0,0,0));
        QPainter cp(&cache);
        QStyleOptionViewItemV3 opt = defaultoption;
        opt.rect = QRect(0, 0, m_width, ItemHeight);
        m_delegate->paint(&cp, opt, m_idx);
        imageCache = cache.toImage();
        cacheValid = true;
    }

    if(active && !cacheActiveValid) {
        cacheActive = QPixmap(m_width, ItemHeight);
        cacheActive.fill(QColor(0,0,0,0));
        QPainter cp(&cacheActive);
        QStyleOptionViewItemV3 opt = defaultoption;
        opt.state |= QStyle::State_Selected;
        m_delegate->paint(&cp, opt, m_idx);
        imageCacheActive = cacheActive.toImage();
        cacheActiveValid = true;
    }
}

void ListItem::paint(FPainter *p, bool active)
{
    checkCache(active);

    if(active)
        p->blit(rect(), imageCacheActive);
    else
        p->blit(rect(), imageCache);
}

void ListItem::paint(QPainter *p, bool active)
{
    checkCache(active);

    if(active)
        p->drawPixmap(rect(), cacheActive);
    else
        p->drawPixmap(rect(), cache);
}

int ListItem::index() const
{
    return m_idx.row();
}

void ListItem::snap()
{
    m_snapRect = rect();
}

QRect ListItem::snapRect()
{
    return m_snapRect;
}

ItemHighlight::ItemHighlight(int width)
: m_width(0), m_highlightPos(0.0f), m_listPos(0.0f), cacheValid(false)
{
}

QRect ItemHighlight::rect() const
{
    return QRect(0, screenPosition(), m_width, ItemHeight);
}

void ItemHighlight::setWidth(int width)
{
    cacheValid = false;
    m_width = width;
}

void ItemHighlight::checkCache()
{
    if(!cacheValid) {
        //ideally, we would use the item delegate to determine what the highlight should look like
        cache = QPixmap(m_width, ItemHeight);
        cache.fill(QColor(0,0,0,0));
        QPainter cp(&cache);
        cp.setRenderHint(QPainter::Antialiasing);
        QColor color = QPalette().color(QPalette::Highlight);   //TODO: colorgroup  (styleoption needed)
        cp.setPen(color);
        QLinearGradient bgg(QPoint(0,0), QPoint(0, cache.rect().height()));
        bgg.setColorAt(0.0f, color.lighter(175));
        bgg.setColorAt(0.49f, color.lighter(105));
        bgg.setColorAt(0.5f, color);
        cp.setBrush(bgg);
        cp.drawRoundRect(cache.rect(), 800/cache.rect().width(),800/cache.rect().height());
        imageCache = cache.toImage();
        cacheValid = true;
    }
}

void ItemHighlight::paint(FPainter *p)
{
    if(m_highlightAlpha.value() == 0.0f)
        return;

    checkCache();
    p->setOpacity(m_highlightAlpha.value());
    p->blit(rect(), imageCache);
    p->setOpacity(1.0f);
}

void ItemHighlight::paint(QPainter *p)
{
    if(m_highlightAlpha.value() == 0.0f)
        return;

    checkCache();
    p->setOpacity(m_highlightAlpha.value());
    p->drawPixmap(rect(), cache);
    p->setOpacity(1.0f);
}

IValue *ItemHighlight::listPosition()
{
    return &m_listPos;
}

IValue *ItemHighlight::highlightPosition()
{
    return &m_highlightPos;
}

IValue *ItemHighlight::highlightAlpha()
{
    return &m_highlightAlpha;
}

qreal ItemHighlight::screenPosition() const
{
    return m_highlightPos.value() - m_listPos.value();
}

void ItemHighlight::snap()
{
    m_snapRect = rect();
}

QRect ItemHighlight::snapRect()
{
    return m_snapRect;
}


QSmoothList::QSmoothList(QWidget *parent, Qt::WFlags flags)
: QWidget(parent, flags), m_model(0), m_concretePosition(0.0f),
  m_inFlick(false), m_focusItem(-1),
  m_scrollbar(0), m_scrollOn(false), m_needRefreshOnShow(true), m_iconSize(-1,-1)
{
    QExportedBackground bg;
    background = bg.background().toImage();
    
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    
    int pm = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
    m_iconSize = QSize(pm, pm);

    fp = new FPainter(this, QDirectPainter::NonReserved);   //QDirectPainter::ReservedSynchronous
    QObject::connect(fp, SIGNAL(needUpdate(bool)), 
                     this, SLOT(doUpdate(bool)));

    m_listPosition.addValue(&m_concretePosition);
    m_listTimeline.setValue(&m_listPosition);
    QObject::connect(&m_listTimeline, SIGNAL(updated()), 
                     this, SLOT(updateList()));
    QObject::connect(&m_listTimeline, SIGNAL(completed()), 
                     this, SLOT(updateCompleted()));
    QObject::connect(&m_listTimeline, SIGNAL(completed()), 
                     this, SLOT(tryHideScrollBar()));

    m_highlight = new ItemHighlight(0);
    m_highlight->highlightPosition()->setValue(0);
    m_highlight->listPosition()->setValue(0);
    m_listPosition.addValue(m_highlight->listPosition());
    m_highlightTimeline.setValue(m_highlight->highlightPosition());
    m_highlightAlpha.setValue(m_highlight->highlightAlpha());
    QObject::connect(&m_highlightTimeline, SIGNAL(updated()), 
                     this, SLOT(updateHighlight()));
    QObject::connect(&m_highlightAlpha, SIGNAL(updated()), 
                     this, SLOT(updateHighlight()));
    QObject::connect(&m_highlightTimeline, SIGNAL(completed()), 
                     this, SLOT(tryHideScrollBar()));

    m_scrollbar = new Scrollbar(0);
    m_scrollTime.setValue(m_scrollbar->scrollAlpha());
    
    QObject::connect(&m_pauseTimeline, SIGNAL(completed()), 
                     this, SLOT(tryHideScrollBar()));
    
    QObject::connect(&m_activatedTimeline, SIGNAL(completed()), 
                     this, SLOT(emitActivated()));
}

QSmoothList::~QSmoothList()
{
}

void QSmoothList::updateList()
{
    fill();
    trim();
}

void QSmoothList::updateCompleted()
{
    if(m_inFlick) {
        m_inFlick = false;
        fixupPosition();
    }
}

void QSmoothList::enableDirect()
{
    if(direct)
        return;

    reserve = QRect(mapToGlobal(rect().topLeft()), QSize(rect().size()));
    fp->setRect(reserve);
   
    //grabMouse();

    direct = true;

    if (isVisible())
        doUpdate(true);
}

void QSmoothList::disableDirect()
{
    if(!direct)
        return;

    reserve = QRect();
//    fp->setRect(reserve);
    //fp->setRect(reserve);   //???
    //releaseMouse();

    direct = false;

    update();

    QTimer::singleShot(0, this, SLOT(completeDisable()));
    //if (isVisible())
    //    doUpdate(true);
}

void QSmoothList::completeDisable()
{
    if(!direct) 
        fp->setRect(QRect());
}

void QSmoothList::tickStart()
{
    for(int ii = 0; ii < m_items.count(); ++ii)
        m_items[ii]->snap();
    m_highlight->snap();
    m_scrollbar->snap();
}

#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
                 
//causes update if using QPainter, otherwise paints with direct painter
void QSmoothList::doUpdate(bool force)
{
    static QTime lastTime;
    static int cleanFrames = 0;
    static int renderTime = 0;
    static int renderFrames = 0;

    QTime t = QTime::currentTime();
    int ft = lastTime.msecsTo(t);
    lastTime = t;

    QRegion changeRect;
    QRegion subRegion;
    if(force) {
        changeRect = rect();
        subRegion = rect();
    } else {
        for(int ii = 0; ii < m_items.count(); ++ii) {
            QRect snap = m_items[ii]->snapRect();
            QRect rect = m_items[ii]->rect();

            if(snap != rect) {
                changeRect = changeRect.unite(snap);
                changeRect = changeRect.unite(rect);
            }

            if(m_items.at(ii)->index() == m_focusItem)
                changeRect = changeRect.unite(rect);
        }
        changeRect = changeRect.unite(m_highlight->snapRect());
        changeRect = changeRect.unite(m_highlight->rect());
        
        subRegion = changeRect;

        changeRect = changeRect.unite(m_scrollbar->snapRect());
        changeRect = changeRect.unite(m_scrollbar->rect());
    }

    if(!direct) {
        update(changeRect);
    } else {
        if(background.isNull())
            fp->clear();
        else {
            background.setOffset(mapToGlobal(rect().topLeft()));
            fp->copy(rect(), background);
        }
        if(changeRect.intersects(m_highlight->rect()))
            m_highlight->paint(fp);

        for(int ii = 0; ii < m_items.count(); ++ii) {
            if(changeRect.intersects(m_items[ii]->rect()))
                m_items[ii]->paint(fp, false /* m_items[ii]->index() == m_focusItem */);
        }

        //TODO: better to disable timelines, etc so we don't take any performance hit?
        if (m_scrollbar->barHeight() < height())
            m_scrollbar->paint(fp);

        fp->flip(subRegion.boundingRect());
        fp->flip(m_scrollbar->rect());
    }

    int nt = lastTime.msecsTo(QTime::currentTime());

    if(ft  + nt > 30) {
        //qWarning() << "XXXXXX Time" << cleanFrames << ft << nt; 
        cleanFrames = 0;
    } else {
        cleanFrames++;
    }

    renderFrames++;
    renderTime += nt;
    if(renderFrames == 100) {
        //qWarning() << "Render frames" << renderTime / 100;
        renderTime = 0;
        renderFrames = 0;
    }
}

//hide scrollbar is if isn't needed
void QSmoothList::tryHideScrollBar()
{
    if(!m_listTimeline.isRunning() &&
       !m_highlightTimeline.isRunning() &&
       !m_pauseTimeline.isRunning())
        hideScrollBar();
}

void QSmoothList::emitActivated()
{
    for(int ii = 0; ii < m_items.count(); ++ii)
    if(m_items.at(ii)->index() == m_focusItem) {        //TODO: need better way of getting index of selected item
        emit activated(m_items.at(ii)->modelIndex());
        return;
    }
}

void QSmoothList::updateHighlight()
{
    bool isMoving = m_listTimeline.isRunning() || m_highlightTimeline.runName() == "Jump";
    QRect rect = m_highlight->rect();

    if(!isMoving && rect.top() < 0) {
        m_listPosition.setValue(m_listPosition.value() + rect.top());
        updateList();
    } else if(!isMoving && rect.bottom() > height()) {
        m_listPosition.setValue(m_listPosition.value() + rect.bottom() - height());
        updateList();
    }
}

void QSmoothList::mousePressEvent(QMouseEvent *e)
{
    bool click = m_inFlick ? false : true;
    m_inFlick = false;    
    m_listTimeline.reset();
    m_mouseClick = click;
    m_origMousePos = e->pos();
    m_prevMousePos = e->pos();
    m_velocity.reset();
    m_velocity.setPosition(e->pos().y());
    e->accept();
}

void QSmoothList::mouseMoveEvent(QMouseEvent *e)
{
    QPoint pos = e->pos();
    int delta = m_prevMousePos.y() - pos.y();
    tickStart();
    m_listPosition.setValue(m_listPosition.value() + delta);
    m_prevMousePos = pos;

    if(abs(m_prevMousePos.y() - m_origMousePos.y()) >= MoveThreshold) {
        m_mouseClick = false;
        showScrollBar();
    }

    m_velocity.setPosition(e->pos().y());
    updateList();
    doUpdate();
}

void QSmoothList::mouseReleaseEvent(QMouseEvent *e)
{
    int idx = -1;
    for(int ii = 0; -1 == idx && ii < m_items.count(); ++ii) {
        if(m_items.at(ii)->rect().contains(e->pos()))
            idx = m_items.at(ii)->index();
    }

    e->accept();

    bool fixup = true;
    if (m_mouseClick) {
        m_activatedTimeline.reset();
        if(idx != m_focusItem) {
    
            m_focusItem = idx;
    
            m_highlightAlpha.reset();
            m_highlightTimeline.reset();
    
            bool slow = true;
            if(m_highlight->highlightAlpha()->value() != 0.0f) {
                // Need to fade out
                m_highlightAlpha.move(150, 0.0f);
                m_highlightTimeline.pause(150);
                slow = false;
            }
    
            if(-1 != idx) {
                m_highlightTimeline.set(idx * ItemHeight);
                m_highlightAlpha.move(slow?300:150, 1.0f);
            }
    
            m_activatedTimeline.pause(300);
            m_highlightAlpha.start();
            m_highlightTimeline.start(0, "Jump");
    
            correctHighlight(idx * ItemHeight); // Fixup position correctly overrides this
        }
        if (m_focusItem > -1) {
            m_activatedTimeline.start();       //opt 1: wait for highlight animation to complete before activatubg
            /*for(int ii = 0; ii < m_items.count(); ++ii) //opt 2: activate immediately (don't wait for highlight)
                if(m_items.at(ii)->index() == m_focusItem) {        //TODO: need better way of getting index of selected item
                    emit activated(m_items.at(ii)->modelIndex());
                    break;
                }*/
        }
    } else {
        qreal velocity = m_velocity.velocity() * 1000.0f;
        if(fabs(velocity) > 100.0f) {
            if(flickList(velocity))
                fixup = false;
        }
    } 

    if(fixup)
        fixupPosition();

    tryHideScrollBar();
}

bool QSmoothList::flickList(qreal velocity)
{
    velocity = velocity * -1.0f;
    qreal accel = (velocity > 0.0f)?-200.0f:200.0f;

    qreal time = -1.0f * velocity / accel;
    qreal distance = velocity * time + 0.5f * accel * time * time;
    qreal endPosition = m_listPosition.value() + distance;

    if(endPosition < 0.0f) {
        endPosition = ItemHeight * -0.5f;
    } else if(endPosition > maxListPos()) {
        endPosition = maxListPos() + ItemHeight * 0.5f;
    }

    distance = endPosition - m_listPosition.value();

    if((distance < 0.0f) != (velocity < 0.0f))
        return false;

    m_listTimeline.reset();
    m_listTimeline.accelDistance(velocity, distance);
    m_listTimeline.start();
    m_inFlick = true;
    return true; 
}

qreal QSmoothList::maxListPos() const
{
    qreal rv = m_model->rowCount() * ItemHeight - height();
    if(rv < 0.0f)
        rv = 0.0f;
    return rv;
}

void QSmoothList::keyPressEvent(QKeyEvent *e)
{   
    if(e->key() == Qt::Key_Select) {
        if (m_focusItem != -1) {
            for(int ii = 0; ii < m_items.count(); ++ii)
                if(m_items.at(ii)->index() == m_focusItem) {        //TODO: need better way of getting index of selected item
                    emit activated(m_items.at(ii)->modelIndex());
                    e->accept();
                    return;
                }
        }
    }

    if(e->key() != Qt::Key_Up &&
       e->key() != Qt::Key_Down) {
        e->ignore();
        return;
    }

    e->accept();
       
    qreal newHighlightPos = m_highlight->highlightPosition()->value();
    qreal newHighlightAlpha = m_highlight->highlightAlpha()->value();

    if(e->key() == Qt::Key_Up) {
        int newFocusItem = m_focusItem - 1;

        if(newFocusItem >= 0) {
            m_focusItem--;
            newHighlightPos = newFocusItem * ItemHeight;
        } else if (newFocusItem == -1 && !e->isAutoRepeat()) {  //wrap
            newFocusItem = m_model->rowCount() - 1;
            m_focusItem = m_model->rowCount() - 1;
            newHighlightPos = newFocusItem * ItemHeight;
            
            m_listTimeline.reset();
            m_listTimeline.move(200, -height() - 20); //20 = magic number
            m_listTimeline.set(m_model->rowCount() * ItemHeight + height());
            m_listTimeline.move(200, (m_model->rowCount() * ItemHeight) - height());
            m_listTimeline.start();
            
            m_highlightAlpha.reset();
            m_highlightTimeline.reset();

            m_highlightTimeline.pause(400);
            m_highlightTimeline.set(newHighlightPos);
            m_highlightAlpha.pause(400);
            m_highlightAlpha.set(0.0f);
            m_highlightAlpha.move(300, 1.0f);
    
            m_highlightAlpha.start();
            m_highlightTimeline.start(0, "Jump");
            return;
        }

    } else if(e->key() == Qt::Key_Down) {
        int newFocusItem = m_focusItem + 1;

        if(newFocusItem < m_model->rowCount()) {
            m_focusItem++;
            newHighlightPos = newFocusItem * ItemHeight;
            if(newHighlightAlpha == 0.0f)
                newHighlightAlpha = 1.0f;
        } else if (newFocusItem == m_model->rowCount() && !e->isAutoRepeat()) { //wrap
            newFocusItem = 0;
            m_focusItem = 0;
            newHighlightPos = newFocusItem * ItemHeight;
            
            m_listTimeline.reset();
            m_listTimeline.move(200, m_listPosition.value() + height() + 20);   //20 = magic number
            m_listTimeline.set(-height());
            m_listTimeline.move(200, 0);
            m_listTimeline.start();
            
            m_highlightAlpha.reset();
            m_highlightTimeline.reset();

            m_highlightTimeline.pause(400);
            m_highlightTimeline.set(newHighlightPos);
            m_highlightAlpha.pause(400);
            m_highlightAlpha.set(0.0f);
            m_highlightAlpha.move(300, 1.0f);
    
            m_highlightAlpha.start();
            m_highlightTimeline.start(0, "Jump");
    
            return;
        }
    } 

    correctHighlight(newHighlightPos);

    if(newHighlightPos != m_highlight->highlightPosition()->value()) {
        m_highlightTimeline.reset();
        m_highlightTimeline.move(300, newHighlightPos);
        m_highlightTimeline.start();
    }

    if(newHighlightAlpha != m_highlight->highlightAlpha()->value()) {
        m_highlightAlpha.reset();
        m_highlightAlpha.move(300, newHighlightAlpha);
        m_highlightAlpha.start();
    }
        
    showScrollBar();
    m_pauseTimeline.reset();
    m_pauseTimeline.pause(1000);
    m_pauseTimeline.start();
}

void QSmoothList::paintEvent(QPaintEvent *pe)
{
    QPainter p(this);

    if(pe->rect().intersects(m_highlight->rect()))
        m_highlight->paint(&p);

    for(int ii = 0; ii < m_items.count(); ++ii)
        if (pe->rect().intersects(m_items[ii]->rect()))
            m_items[ii]->paint(&p, false /*m_items[ii]->index() == m_focusItem*/);

    //TODO: better to disable timelines, etc so we don't take any performance hit?
    if (m_scrollbar->barHeight() < height())
        m_scrollbar->paint(&p);
}

bool QSmoothList::event(QEvent *event)
{
    switch(event->type()) {
        case QEvent::PaletteChange:
            if(isVisible())
                refresh();
            else
                m_needRefreshOnShow = true;
            break;
        case QEvent::FocusIn:
#ifdef ENABLE_DIRECTPAINTER
            enableDirect();
#endif
            QObject::connect(MotionTimeLine::clock(), SIGNAL(tick()), 
                     this, SLOT(doUpdate()));
            QObject::connect(MotionTimeLine::clock(), SIGNAL(tickStart()), 
                     this, SLOT(tickStart()));
            break;
        case QEvent::FocusOut:
#ifdef ENABLE_DIRECTPAINTER
            disableDirect();
#endif
            QObject::disconnect(MotionTimeLine::clock(), SIGNAL(tick()), 
                     this, SLOT(doUpdate()));
            QObject::disconnect(MotionTimeLine::clock(), SIGNAL(tickStart()), 
                     this, SLOT(tickStart()));
            break;
        case QEvent::Resize:
#ifdef ENABLE_DIRECTPAINTER
            if (!direct)
                break;
            reserve = QRect(mapToGlobal(rect().topLeft()), QSize(((QResizeEvent*)event)->size()));
            fp->setRect(reserve);
#endif
            break;
        default:
            break;
    }
    return QWidget::event(event);
}

void QSmoothList::setModel(QAbstractItemModel *model)
{
    m_model = model;
    
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(rowsInserted(QModelIndex,int,int)));

    if(isVisible())
        refresh();
    else
        m_needRefreshOnShow = true;
}

void QSmoothList::refresh()
{
    int oldFocusItem = m_focusItem;
    init();
    fill();
    
    if (m_model->rowCount() > 0) {
        doUpdate(true);
    
        int idx = oldFocusItem > -1 ? oldFocusItem : 0; //don't lose the selection when we leave the list and come back 
        m_focusItem = idx;
    
        m_highlightAlpha.reset();
        m_highlightTimeline.reset();
    
        if(-1 != idx) {
            m_highlightTimeline.set(idx * ItemHeight);
            m_highlightAlpha.move(300, 1.0f);
        }
    
        m_highlightAlpha.start();
        m_highlightTimeline.start(0, "Jump");
    
        correctHighlight(idx * ItemHeight); // Fixup position correctly overrides this
    }
    m_needRefreshOnShow = false;
}

void QSmoothList::reset()
{
    if(isVisible())
        refresh();
    else
        m_needRefreshOnShow = true;
}

void QSmoothList::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if(isVisible())
        refresh();  //note: a full refresh is a bit drastic for dataChanged
    else
        m_needRefreshOnShow = true;
}

void QSmoothList::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if(isVisible())
        refresh();
    else
        m_needRefreshOnShow = true;
}

void QSmoothList::setItemDelegate(QAbstractItemDelegate *delegate)
{
    m_delegate = delegate;
    
    if(isVisible())
        refresh();
    else
        m_needRefreshOnShow = true;
}

void QSmoothList::setIconSize(const QSize &size)
{
    m_iconSize = size;
    defaultoption.decorationSize = m_iconSize;
    QStyleOptionViewItemV3 opt = defaultoption;
    opt.rect = QRect(0, 0, width(), ItemHeight);
    ItemHeight = m_delegate->sizeHint(opt, m_model->index(0,0)).height();
    
    //scrollbars
    m_scrollbar->setListHeight(m_model->rowCount() * ItemHeight);
    
    //items
    for(int ii = 0; ii < m_items.count(); ++ii)
        m_items[ii]->invalidateCache();
    
    //force redraw
    doUpdate(true);
}

void QSmoothList::showEvent(QShowEvent *e)
{
    if (m_focusItem == -1 || m_needRefreshOnShow)
        refresh();
    QWidget::showEvent(e);
}

void QSmoothList::init()
{
    for(int ii = 0; ii < m_items.count(); ++ii) {
        m_listPosition.remValue(m_items[ii]->listPosition());
        delete m_items[ii];
    }
    m_items.clear();
    
    defaultViewOptionsV3(this); //initialize view options

    m_highlight->setWidth(width());
    m_highlight->highlightPosition()->setValue(0.0f);
    m_highlight->highlightAlpha()->setValue(0.0f);
    m_focusItem = -1;
    m_listPosition.setValue(0.0f);

    m_scrollbar->setHeight(height());
    QStyleOptionViewItemV3 opt = defaultoption;
    opt.rect = QRect(0, 0, width(), ItemHeight);
    ItemHeight = m_delegate->sizeHint(opt, m_model->index(0,0)).height();
    m_scrollbar->setListHeight(m_model->rowCount() * ItemHeight);
    m_listPosition.addValue(m_scrollbar->listPosition());
    m_scrollbar->setPos(width() - 2);
}

QRect QSmoothList::fill()
{
    QRect changeRect;

    if(!m_model || 0 == m_model->rowCount())
        return changeRect;

    // Check if we need to fill the top or the bottom
    while(m_items.isEmpty() || 
          (m_items.first()->rect().top() > 0 && 
           m_items.first()->index())) {

        int index = m_items.isEmpty() ? 0 : m_items.first()->index() - 1;
        ListItem *item = new ListItem(m_model->index(index, 0),
                                              width());
        item->setDelegate(m_delegate);
        m_listPosition.addValue(item->listPosition());
        item->listPosition()->setValue(m_listPosition.value());
        m_items.prepend(item);

        changeRect = changeRect.united(item->rect());
    }

    while(m_items.last()->rect().bottom() < height() &&
          m_items.last()->index() < (m_model->rowCount() - 1)) {

        int index = m_items.last()->index() + 1;
        
        ListItem *item = new ListItem(m_model->index(index, 0),
                                              width());
        item->setDelegate(m_delegate);
        m_listPosition.addValue(item->listPosition());
        item->listPosition()->setValue(m_listPosition.value());
        m_items.append(item);

        changeRect = changeRect.united(item->rect());
    }

    return changeRect;
}

void QSmoothList::correctHighlight(qreal top)
{
    top = top - m_listPosition.value();
    qreal bottom = top + ItemHeight;
    qreal newListPos = m_listPosition.value();
    
    if(-1 != m_focusItem) {
        if(top < 0) 
            newListPos = m_focusItem * ItemHeight;
        else if(bottom > height())
            newListPos = m_focusItem * ItemHeight - height() + ItemHeight;
    }
    
    if(newListPos != m_listPosition.value()) {
        m_inFlick = false;
        m_listTimeline.reset();
        m_listTimeline.move(300, newListPos);
        m_listTimeline.start();
    }
}

void QSmoothList::fixupPosition()
{
    // We fixup the position if:
    //    The list is in a negative position
    //    The screen is not fully filled
    qreal newListPosition = m_listPosition.value();

    if((newListPosition + height()) > (m_model->rowCount() * ItemHeight))
        newListPosition = (m_model->rowCount() * ItemHeight) - height();

    if(newListPosition < 0.0f)
        newListPosition = 0.0f;

    if(newListPosition != m_listPosition.value()) {
        m_inFlick = false;
        m_listTimeline.reset();
        m_listTimeline.move(300, newListPosition);
        m_listTimeline.start();
    }
}

void QSmoothList::trim()
{
    while(m_items.count() > 1 && m_items.first()->rect().bottom() < 0) {
        m_listPosition.remValue(m_items.first()->listPosition());
        delete m_items.first();
        m_items.removeFirst();
    }

    while(m_items.count() > 1 && m_items.last()->rect().top() > height()) {
        m_listPosition.remValue(m_items.last()->listPosition());
        delete m_items.last();
        m_items.removeLast();
    }
}

//fade scrollbar in
void QSmoothList::showScrollBar()
{
    if(scrollBarIsVisible())
        return;
    m_scrollOn = true;
    m_scrollTime.reset();
    m_scrollTime.move(300, 1.0f);
    m_scrollTime.start();
}

//fade scrollbar out
void QSmoothList::hideScrollBar()
{
    if(!scrollBarIsVisible())
        return;
    m_scrollOn = false;
    m_scrollTime.reset();
    m_scrollTime.move(300, 0.0f);
    m_scrollTime.start();
}

//return whether scrollbar is visible
bool QSmoothList::scrollBarIsVisible() const
{
    return m_scrollOn;
}

#include "qsmoothlist_p.moc"
