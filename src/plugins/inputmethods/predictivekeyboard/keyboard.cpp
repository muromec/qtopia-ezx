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

#include <QApplication>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLabel>
#include <QDesktopWidget>
#include <QListWidget>
#include <QFont>
#include <QFontMetrics>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QDebug>
#include <QTimeLine>
#include <QPixmap>
#include <QPainter>
#include <QTextEdit>
#include <QWidget>
#include <QTimer>
#include <QCoreApplication>
#include <QInputContext>
#include "proxyscreen.h"
#include <QExportedBackground>
#include <ThemedView>
#include <qtopialog.h>
#include "keyboard.h"
#include "pred.h"

#define PREFIX_PREEDIT

#define QTOPIA_INTERNAL_QDAWG_TRIE
#include "qdawg.h"

class Board
{
public:
    Board(const QString &l_name,
          const QStringList &,
          const QSize &,
          KeyboardWidget::BoardType alphabet);

    int lines() const;
    QString characters(int line) const;
    QString characters() const;

    QRect rect(const QChar &) const;

    QSize size() const;

    QFont font() const;

    bool isAlphabet() const;
    bool isNumeric() const;

    QString layoutName() const;
    KeyboardWidget::BoardType type() const;

private:
    QSize m_size;
    QFont m_font;
    QStringList m_lines;
    QString m_characters;
    QString m_layoutName;
    QHash<QChar, QRect> m_rects;
    KeyboardWidget::BoardType m_isAlphabet;
};


Board::Board(const QString &l_name,
             const QStringList &lines,
             const QSize &boardSize,
             KeyboardWidget::BoardType isAlphabet)
: m_size(boardSize), m_font(QApplication::font()), m_lines(lines), m_layoutName(l_name.toLower()), m_isAlphabet(isAlphabet)
{
    int maxline = -1;
    int maxline_len = 0;

    for(int ii = 0; ii < m_lines.count(); ++ii) {
        m_characters.append(m_lines.at(ii));
        if(maxline == -1 || m_lines.at(ii).length() > maxline_len) {
            maxline = ii;
            maxline_len = m_lines.at(ii).length();
        }
    }

    if(maxline == -1 || !maxline_len || lines.isEmpty())
        return;

    int width = m_size.width() / maxline_len;
    int height = m_size.height() / lines.count();

    int max_fontwidth = width*7/8; // leave some inline space
    m_font.setPixelSize((height*3)/4); // leave some interline space
    while (QFontMetrics(m_font).width('W')>max_fontwidth) // fit font's width (FIXME: 'W' is considered the widest)
      m_font.setPixelSize(m_font.pixelSize()-1);


    for(int ii = 0; ii < m_lines.count(); ++ii) {
        const QString &line = m_lines.at(ii);

        int border = (m_size.width() - line.length() * width) / 2;

        for(int jj = 0; jj < line.length(); ++jj) {
            QRect r(border + jj * width, ii * height, width, height);
            m_rects.insert(line.at(jj), r);
        }
    }
}

int Board::lines() const
{
    return m_lines.count();
}

QString Board::characters(int line) const
{
    return m_lines.at(line);
}

QRect Board::rect(const QChar &c) const
{
    return m_rects[c];
}

QSize Board::size() const
{
    return m_size;
}

QFont Board::font() const
{
  return m_font;
}

bool Board::isAlphabet() const
{
    return m_isAlphabet == KeyboardWidget::UpperCase ||
           m_isAlphabet == KeyboardWidget::LowerCase;
}

bool Board::isNumeric() const
{
    return m_isAlphabet == KeyboardWidget::Numeric;
}

KeyboardWidget::BoardType Board::type() const
{
    return m_isAlphabet;
}

QString Board::layoutName() const
{
  return m_layoutName;
}

QString Board::characters() const
{
    return m_characters;
}

class AcceptWindow : public QWidget
{
Q_OBJECT
public:
    AcceptWindow(int time);

    void accept(const QString &word, const QRect &from, const QPoint &to);
    void setToPoint(const QPoint &);

protected:
    virtual void paintEvent(QPaintEvent *);

private slots:
    void valueChanged(qreal);

private:
    QTimeLine m_anim;

    QPoint m_from;
    QPoint m_to;
    QString m_word;
};

AcceptWindow::AcceptWindow(int time)
: QWidget(0, (Qt::WindowFlags)(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)), m_anim(time)
{
#ifdef Q_WS_QWS
    QPalette p = palette();
    p.setBrush(backgroundRole(), QColor(0,0,0,0));
    setPalette(p);
#endif
    setAttribute(Qt::WA_InputMethodTransparent, true);

    m_anim.setCurveShape(QTimeLine::EaseInCurve);
    QObject::connect(&m_anim, SIGNAL(valueChanged(qreal)),
                     this, SLOT(valueChanged(qreal)));

    setWindowTitle("_allow_on_top_"); // Use window manager back door.
    setWindowModality (Qt::NonModal);
    setFocusPolicy(Qt::NoFocus);
}

void AcceptWindow::valueChanged(qreal v)
{
    if(v == 1.0f) {
        hide();
        deleteLater();
    } else {
        QPoint p = m_from + (m_to - m_from) * v;
        move(p);
        update();
    }
}

void AcceptWindow::accept(const QString &word, const QRect &from, const QPoint &to)
{
    setFixedSize(from.size());
    m_from = from.topLeft();
    m_to = to - QPoint(from.width() / 2, from.height() / 2);
    move(m_from);
    m_word = word;

    show();
    m_anim.start();
}

void AcceptWindow::setToPoint(const QPoint &p)
{
    if(m_anim.state() != QTimeLine::NotRunning) {

        QPoint currentPoint = m_from + (m_to - m_from) * m_anim.currentValue();

        m_to = p - QPoint(width() / 2, height() / 2);

        QPoint newStart;
        if(m_anim.currentValue() == 1.0f) {
            newStart = m_to;
        } else {
            newStart = (currentPoint - m_anim.currentValue() * m_to) / (1.0f - m_anim.currentValue());
        }

        m_from = newStart;
    }

}

void AcceptWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QColor color = palette().text().color();
    color.setAlpha((int)(255 * (1.0f - m_anim.currentValue())));
    p.setPen(color);
    p.drawText(rect(), m_word, Qt::AlignHCenter | Qt::AlignVCenter);
}

class OptionsWindow : public QWidget
{
Q_OBJECT
public:
    OptionsWindow(int wordSpacing);

    void setWords(const QStringList &);

    enum ClearType { ClearImmediate, ClearSoon, ClearEventually };
    void clear(ClearType = ClearImmediate);

    QString acceptWord(const QPoint &, bool animate = true);
    void setAcceptDest(const QPoint &);
    QString selectedWord() const;

    QRect wordRect(int ii) const;

signals:
    void wordAccepted();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void showEvent(QShowEvent *);
    virtual void moveEvent(QMoveEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void timerEvent(QTimerEvent *);

private slots:
    void backgroundValueChanged(qreal);
    void valueChanged(qreal);
    void finished();

private:
    void layoutWords(const QStringList &);

    bool m_specialDelete;
    bool m_ignore;

    QList<QPair<QString, QRect> > m_words;
    int m_selectedWord;

    void slideTo(int word);
    int optionsOffset() const;
    QTimeLine m_slideTimeline;
    int m_slideStart;
    int m_slideEnd;

    QTimeLine m_clearTimeline;
    QTimeLine m_backgroundTimeline;
    QImage m_background;
    QPixmap m_backgroundPix;
    int m_wordSpacing;

    void startClearTimer(ClearType);
    void stopClearTimer();
    int m_clearTimer;
    ClearType m_clearType;

    QPointer<AcceptWindow> m_lastAccept;
};

// Absurd functions copied from themedview
static void yuv_to_rgb(int Y, int U, int V, int& R, int& G, int&B)
{
    R = int(Y +  ((92242 * (V - 128)) >> 16));
    G = int(Y - (((22643 * (U - 128)) >> 16)) - ((46983 * (V - 128)) >> 16));
    B = int(Y + ((116589 * (U - 128)) >> 16));
}

static void rgb_to_yuv(int R, int G, int B, int& Y, int& U, int& V)
{
    Y = int(R *  19595 + G *  38470 + B *  7471) >> 16;
    U = int(R * -11076 + G * -21758 + (B << 15) + 8388608) >> 16;
    V = int(R *  32768 + G * -27460 + B * -5328 + 8388608) >> 16;
}

static inline QRgb blendYuv(QRgb rgb, int /*sr*/, int /*sg*/, int /*sb*/, int sy, int su, int sv, int alpha, bool colorroles)
{
    int a = (rgb >> 24) & 0xff;
    int r = (rgb >> 16) & 0xff;
    int g = (rgb >> 8) & 0xff;
    int b = rgb & 0xff;
    if (colorroles) {
        int y,u,v;
        rgb_to_yuv(r,g,b,y,u,v);
        y = (y*2+sy)/3;
        u = (u+su*2)/3;
        v = (v+sv*2)/3;
        yuv_to_rgb(y,u,v,r,g,b);
        if (r>255) r = 255; if (r<0) r=0;
        if (g>255) g = 255; if (g<0) g=0;
        if (b>255) b = 255; if (b<0) b=0;
    }
    if (alpha != 255)
        a = (a*alpha)/255;
    return qRgba(r, g, b, a);
}

/*!
  Blend the color \a col and the alpha value \a alpha with the image \a img if \a blendColor is true (the default).
  If \a blendColor is false, only the alpha value \a alpha is blended.
  This function modifies \a img directly.
*/
static void colorizeImage( QImage& img, const QColor& col, int alpha, bool blendColor )
{
    QColor colour = col;
    Q_ASSERT( !img.isNull() );
    Q_ASSERT( col.isValid() );
    int count;
    int sr, sg, sb;
    colour.getRgb(&sr, &sg, &sb);
    int sy, su, sv;
    rgb_to_yuv(sr,sg,sb,sy,su,sv);

    if ( alpha != 255 &&
         img.format() != QImage::Format_ARGB32 &&
         img.format() != QImage::Format_ARGB32_Premultiplied ) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }
    if (img.depth() == 32) {
        const QImage &cimg = img; // avoid QImage::detach().
        QRgb *rgb = (QRgb*)cimg.bits();
        count = img.bytesPerLine()/sizeof(QRgb)*img.height();
        for (int i = 0; i < count; i++, rgb++)
            *rgb = blendYuv(*rgb, sr, sg, sb, sy, su, sv, alpha,
                blendColor);
    } else {
        QVector<QRgb> ctable = img.colorTable();
        for (int i = 0; i < ctable.count(); i++)
            ctable[i] = blendYuv(ctable[i], sr, sg, sb, sy, su, sv, alpha,
                    blendColor);
        img.setColorTable(ctable);
    }
}
OptionsWindow::OptionsWindow(int wordSpacing)
: QWidget(0, (Qt::WindowFlags)(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)), m_specialDelete(false), m_ignore(false),
  m_selectedWord(0), m_slideTimeline(300), m_backgroundTimeline(300),
  m_background(":image/predictivekeyboard/softbar"),
  m_wordSpacing(wordSpacing), m_clearTimer(0)
{
    setAttribute(Qt::WA_InputMethodTransparent, true);

#ifdef Q_WS_QWS
    QPalette p = palette();
    p.setBrush(backgroundRole(), QColor(0,0,0,0));
    setPalette(p);
#endif

    m_clearTimeline.setCurveShape(QTimeLine::LinearCurve);
    QObject::connect(&m_clearTimeline, SIGNAL(valueChanged(qreal)),
                     this, SLOT(valueChanged(qreal)));
    QObject::connect(&m_backgroundTimeline, SIGNAL(valueChanged(qreal)),
                     this, SLOT(backgroundValueChanged(qreal)));
    QObject::connect(&m_clearTimeline, SIGNAL(finished()),
                     this, SLOT(finished()));
    QObject::connect(&m_backgroundTimeline, SIGNAL(finished()),
                     this, SLOT(finished()));
    QObject::connect(&m_slideTimeline, SIGNAL(valueChanged(qreal)),
                     this, SLOT(update()));

    setWindowTitle("_allow_on_top_"); // Use window manager back door.
    setWindowModality (Qt::NonModal);
    setFocusPolicy(Qt::NoFocus);

    colorizeImage(m_background, Qt::black, 125, true);
}

void OptionsWindow::setWords(const QStringList &words)
{
    stopClearTimer();

    if(isHidden()) {
        show();
    } else if(m_backgroundTimeline.direction() == QTimeLine::Backward) {
        m_ignore = true;
        m_backgroundTimeline.stop();
        m_ignore = false;
        m_backgroundTimeline.setDirection(QTimeLine::Forward);
        m_backgroundTimeline.start();
    }

    if(m_clearTimeline.state() != QTimeLine::NotRunning) {
        m_ignore = true;
        m_clearTimeline.stop();
        m_clearTimeline.setCurrentTime(0);
        m_ignore = false;
    }

    if(m_slideTimeline.state() != QTimeLine::NotRunning) {
        m_ignore = true;
        m_slideTimeline.stop();
        m_slideTimeline.setCurrentTime(0);
        m_ignore = false;
    }

    m_words.clear();
    layoutWords(words);

    update();
}

void OptionsWindow::layoutWords(const QStringList &words)
{
    m_words.clear();
    m_selectedWord = 0;

    if(words.isEmpty())
        return;

    QFont font;
    font.setPointSize((font.pointSize() * 15) / 10);
    QFontMetrics fm(font);

    int leftEdge = 0;
    int rightEdge = 0;
    for(int ii = 0; ii < words.count(); ++ii) {

        QRect r;
        QString w = words.at(ii);
        int ww = fm.width(w);

        if(ii == 0) {
            r = QRect(-ww/2, 0, ww, 1);
        } else if(ii % 2) {
            r = QRect(leftEdge - ww, 0, ww, 1);
        } else {
            r = QRect(rightEdge, 0, ww, 1);
        }

        if(r.left() < leftEdge)
            leftEdge = r.left() - m_wordSpacing;

        if(r.right() > rightEdge)
            rightEdge = r.right() + m_wordSpacing;

        m_words << qMakePair(w, r);
    }
}

void OptionsWindow::finished()
{
    if(m_backgroundTimeline.direction() == QTimeLine::Backward &&
       m_backgroundTimeline.state() == QTimeLine::NotRunning &&
       m_clearTimeline.state() == QTimeLine::NotRunning) {
        hide();
    }
}

void OptionsWindow::showEvent(QShowEvent *)
{
    m_ignore = true;
    m_backgroundTimeline.stop();
    m_ignore = false;
    m_backgroundTimeline.setDirection(QTimeLine::Forward);
    m_backgroundTimeline.start();

    m_backgroundPix = QPixmap(size());
    m_backgroundPix.fill(Qt::black);
    QExportedBackground eb;
    QPainter p(&m_backgroundPix);
    p.drawPixmap(0, -pos().y(), eb.background());
    QPixmap bp = QPixmap::fromImage(m_background).scaled(width(), height());
    p.drawPixmap(0, 0, bp);
}

void OptionsWindow::moveEvent(QMoveEvent *)
{
}

void OptionsWindow::startClearTimer(ClearType type)
{
    stopClearTimer();
    if(type == ClearSoon) {
        m_clearTimer = startTimer(1000);
    } else if(type == ClearEventually) {
        m_clearTimer = startTimer(2500);
    }
}

void OptionsWindow::stopClearTimer()
{
    if(m_clearTimer) {
        killTimer(m_clearTimer);
        m_clearTimer = 0;
    }
}

void OptionsWindow::timerEvent(QTimerEvent *)
{
    stopClearTimer();
    m_ignore = true;
    m_backgroundTimeline.stop();
    m_ignore = false;
    m_backgroundTimeline.setDirection(QTimeLine::Backward);
    m_backgroundTimeline.start();
    m_clearTimeline.start();
    update();
}

void OptionsWindow::clear(ClearType type)
{
    bool special = false;
    for(int ii = 0; !special && ii < m_words.count(); ++ii) {
        if(m_words.at(ii).first.toLower() == "pacman")
            special = true;
    }

    if(special) {
        m_clearTimeline.setDuration(2000);
        m_specialDelete = true;
    } else {
        m_clearTimeline.setDuration(300);
        m_specialDelete = false;
    }

    m_clearTimeline.start();
    update();

    stopClearTimer();
    if(type == ClearImmediate) {
        m_ignore = true;
        m_backgroundTimeline.stop();
        m_ignore = false;
        m_backgroundTimeline.setDirection(QTimeLine::Backward);
        m_backgroundTimeline.start();
    } else {
        startClearTimer(type);
    }
}

void OptionsWindow::setAcceptDest(const QPoint &p)
{
    if(m_lastAccept)
        m_lastAccept->setToPoint(p);
}

QString OptionsWindow::selectedWord() const
{
    if(m_words.isEmpty())
        return QString();
    else
        return m_words.at(m_selectedWord).first;
}

QString OptionsWindow::acceptWord(const QPoint &p, bool animate)
{
    QString word = m_words.at(m_selectedWord).first;
    QRect startRect = wordRect(m_selectedWord);

    if(animate) {
        AcceptWindow *win = new AcceptWindow(500 /* XXX */);
        win->accept(word,
                QRect(mapToGlobal(startRect.topLeft()), startRect.size()), p);
        m_lastAccept = win;
    }
    m_words.clear();
    update();
    return word;
}

void OptionsWindow::backgroundValueChanged(qreal v)
{
    Q_UNUSED(v);
    update();
}

void OptionsWindow::valueChanged(qreal v)
{
    if(m_ignore)
        return;

    if(1.0f == v)
        m_words.clear();

    update();
}

void OptionsWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_words.isEmpty()) {
        // Hide now so that the user can get to the menubar.
        clear(ClearImmediate);
    } else {
        for(int ii = 0; ii < m_words.count(); ++ii) {
            QRect r = wordRect(ii);
            if(r.contains(e->pos())) {
                if(ii == m_selectedWord) {
                    emit wordAccepted();
                } else {
                    slideTo(ii);
                    update();
                }

                return;
            }
        }
    }
}

QRect OptionsWindow::wordRect(int ii) const
{
    return QRect(m_words.at(ii).second.x() + optionsOffset(), 0,
                 m_words.at(ii).second.width(), height());
}

void OptionsWindow::slideTo(int word)
{
    int offset = optionsOffset();
    m_ignore = true;
    m_slideTimeline.stop();
    m_slideTimeline.setCurrentTime(0);
    m_ignore = false;
    m_selectedWord = word;
    m_slideStart = offset;
    m_slideEnd = optionsOffset();
    m_slideTimeline.start();
}

int OptionsWindow::optionsOffset() const
{
    if(m_words.isEmpty())
        return 0;

    if(m_slideTimeline.state() == QTimeLine::NotRunning) {
        return width() / 2 - m_words.at(m_selectedWord).second.center().x();
    } else {
        return (int)(m_slideStart + (m_slideEnd - m_slideStart) * m_slideTimeline.currentValue());
    }
}

void OptionsWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);

#ifdef Q_WS_QWS
    p.setOpacity(m_backgroundTimeline.currentValue());
    p.drawPixmap(0, 0, m_backgroundPix);
    p.setOpacity(1.0f);
#endif

    QPixmap pix(size());
    pix.fill(Qt::transparent);
    QRect totalWordRect;
    {
        QPainter pixp(&pix);
        QFont font;
        QFont bigfont = font;
        bigfont.setPointSize((bigfont.pointSize() * 15) / 10);
        if(!m_words.isEmpty()) {

            for(int ii = 0; ii < m_words.count(); ++ii) {
                const QString &word = m_words.at(ii).first;
                QRect rect = wordRect(ii);
                totalWordRect = totalWordRect.united(rect);

                if(ii == m_selectedWord) {
                    pixp.setFont(bigfont);
                }
                pixp.setPen(palette().text().color());
                pixp.drawText(rect, word, Qt::AlignVCenter | Qt::AlignHCenter);
                if(ii == m_selectedWord) {
                    pixp.setFont(font);
                }
            }
        }
    }

    if(m_clearTimeline.state() != QTimeLine::NotRunning) {

        QPainter pixp(&pix);

        int fadeWidth = 60;
        int fadeStart;
        if(m_specialDelete) {
            fadeStart = (int)((1.0f - m_clearTimeline.currentValue()) * (width() + fadeWidth) - fadeWidth);
        } else {
            fadeStart = (int)((1.0f - m_clearTimeline.currentValue()) * (totalWordRect.width() + fadeWidth) - fadeWidth) + totalWordRect.x();
        }
        int fadeEnd = fadeStart + fadeWidth;

        QLinearGradient grad(QPoint(fadeStart, 0), QPoint(fadeEnd, 0));
        grad.setColorAt(0, QColor(0,0,0,255));
        grad.setColorAt(1.0f, QColor(0,0,0,0));
        pixp.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        pixp.fillRect(pix.rect(), grad);
//        pixp.setCompositionMode(QPainter::CompositionMode_Plus);
 //       pixp.fillRect(pix.rect(), QColor(255, 0, 0, 0));


        if(m_specialDelete) {
            QPainterPath pacman;
            int pac_width = (height() * 12) / 10;
            int pac_height = height();

            qreal pacmouth = m_clearTimeline.currentValue() / 0.2f;
            pacmouth = pacmouth - (int)(pacmouth);
            pacmouth *= 2.0f;
            pacmouth -= 1.0f;
            pacmouth = qAbs(pacmouth);

            pacman.moveTo(pac_width / 2, pac_height / 2);

            pacman.arcTo(0, 0, pac_width, pac_height, 180 - pacmouth * 50.0f,
                               -(360 - pacmouth * 100.0f));
            pacman.closeSubpath();
            p.setPen(Qt::black);
            p.setBrush(Qt::yellow);
            int pacStart = (int)((1.0f - m_clearTimeline.currentValue()) * (width() + pac_width) - pac_width);

            p.save();
            p.setClipRect(0, 0, (pac_width / 2) + pacStart, height());
            p.drawPixmap(0,0,pix);
            p.restore();

            p.translate(pacStart, 0);
            p.setRenderHint(QPainter::Antialiasing);
            p.drawPath(pacman);
            p.setBrush(Qt::black);
            p.drawEllipse(pac_width / 2, pac_height / 5, 2, 2);
            p.setRenderHint(QPainter::Antialiasing, false);
            p.translate(-pacStart, 0);
        } else {
            p.drawPixmap(0,0,pix);
        }
    } else {
        p.drawPixmap(0,0,pix);
    }
}

class PopupWindow : public QWidget
{
Q_OBJECT
public:
    PopupWindow(int raise, qreal scaleFactor, QWidget * = 0);

    void setChar(const QChar &, const QPoint &, Board *board);
    bool charOk() const;
    QChar getChar() const;
    void dismiss();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);

private slots:
    void valueChanged(qreal);
    void showvalueChanged(qreal);

private:
    void moveTo(const QPoint &newPos);
    bool m_ignore;
    QPoint m_startPoint;
    QPoint m_endPoint;
    QTimeLine m_timeline;
    QTimeLine m_showtimeline;
    QChar m_char;
    Board *m_board;

    int m_offset;
    qreal m_scaleFactor;
    bool m_dismissing;
    qreal m_dismissValue;
    QPoint m_point;
};

PopupWindow::PopupWindow(int raise, qreal scaleFactor, QWidget *parent)
: QWidget(parent, (Qt::WindowFlags)(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)),
  m_ignore(false), m_timeline(300), m_showtimeline(300), m_board(0),
  m_offset(raise), m_scaleFactor(scaleFactor), m_dismissing(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
#ifdef Q_WS_QWS
    QPalette p = palette();
    p.setBrush(backgroundRole(), QColor(0,0,0,0));
    setPalette(p);
#endif
    QObject::connect(&m_timeline, SIGNAL(valueChanged(qreal)), this, SLOT(valueChanged(qreal)));
    QObject::connect(&m_showtimeline, SIGNAL(valueChanged(qreal)), this, SLOT(showvalueChanged(qreal)));

    setWindowTitle("_allow_on_top_"); // Use window manager back door.
    setWindowModality (Qt::NonModal);
    setFocusPolicy(Qt::NoFocus);
}

void PopupWindow::showvalueChanged(qreal v)
{
    if(m_ignore)
        return;

    if(1.0f == v && m_dismissing) {
        hide();
    } else {
        update();
    }
}

void PopupWindow::moveTo(const QPoint &newPos)
{
    m_ignore = true;
    m_timeline.stop();
    m_startPoint = pos();
    m_endPoint = newPos;
    m_ignore = false;
    m_timeline.start();
}

void PopupWindow::valueChanged(qreal v)
{
    if(m_ignore)
        return;
    QPoint newPos = m_startPoint + (m_endPoint - m_startPoint) * v;
    update();
    move(newPos);
}

void PopupWindow::showEvent(QShowEvent *)
{
    m_showtimeline.start();
    moveTo(m_endPoint);
}

void PopupWindow::hideEvent(QHideEvent *)
{
    m_ignore = true;
    m_timeline.stop();
    m_showtimeline.stop();
    m_timeline.setCurrentTime(0);
    m_showtimeline.setCurrentTime(0);
    m_ignore = false;
    m_dismissing = false;
    deleteLater();
}

void PopupWindow::dismiss()
{
    m_ignore = true;
    if(m_showtimeline.state() == QTimeLine::Running)
        m_dismissValue = m_showtimeline.currentValue();
    else
        m_dismissValue = 1.0f;
    m_showtimeline.stop();
    m_showtimeline.setCurrentTime(0);
    m_ignore = false;

    m_dismissing = true;
    moveTo(m_point);
    m_showtimeline.start();
}

bool PopupWindow::charOk() const
{
    return (m_showtimeline.state() == QTimeLine::NotRunning ||
            m_showtimeline.currentValue() > 0.66f) &&
           (m_timeline.state() == QTimeLine::NotRunning ||
            m_timeline.currentValue() > 0.66f);
}

QChar PopupWindow::getChar() const
{
    return m_char;
}

void PopupWindow::setChar(const QChar &c, const QPoint &p, Board *board)
{
    m_char = c;
    m_board = board;

    QPoint pos = p - QPoint(width() / 2, height() / 2);
    if(isHidden()) {
        move(pos);
        m_endPoint = pos + QPoint(0, m_offset);
        show();
    } else {
        moveTo(pos + QPoint(0, m_offset));
        update();
    }
    m_point = pos;

    raise();
}

void PopupWindow::paintEvent(QPaintEvent *)
{
    if(!m_board)
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient grad(QPoint(0, 0), QPoint(0, height()));
    QLinearGradient grad2(QPoint(2, 0), QPoint(0, height() - 4));

    grad.setColorAt(0.0f, QColor(127, 127, 127));
    grad.setColorAt(0.8f, QColor(200, 200, 200));
    grad.setColorAt(1.0f, QColor(230, 230, 230));
    grad2.setColorAt(0.0f, QColor(200, 200, 200));
    grad2.setColorAt(0.2f, QColor(180, 180, 180));
    grad2.setColorAt(1.0f, QColor(50, 50, 50));

    int ewidth;
    if(!m_dismissing)
        ewidth = (int)(m_showtimeline.currentValue() * (width() - 2));
    else
        ewidth = (int)(m_dismissValue * (1.0f - m_showtimeline.currentValue()) * (width() - 2));


    if(ewidth == 0)
        return;

    p.setBrush(grad);
    p.setPen(QPen(grad2, 0));
    p.drawEllipse((width() - ewidth) / 2, (width() - ewidth) / 2, ewidth, ewidth);

    QPainterPath path;
    path.addEllipse((width() - ewidth) / 2, (width() - ewidth) / 2, ewidth, ewidth);
    p.setClipPath(path);

    p.setRenderHint(QPainter::Antialiasing, false);

    p.setPen(Qt::black);

    QString characters = m_board->characters();
    QPoint boardCenter = m_board->rect(m_char).center();
    QPoint widgetPosTrans = pos() - (m_point + QPoint(0, m_offset));
    QPoint widgetCenter = rect().center() - widgetPosTrans;

    QPointF transform = QPointF(widgetCenter)/m_scaleFactor - boardCenter;
    p.scale(m_scaleFactor, m_scaleFactor); // Scale the popup

    QFont mainFont = QApplication::font();
    QFont bigFont = mainFont;
    bigFont.setPointSize((bigFont.pointSize() * 16) / 10);

    for(int ii = 0; ii < characters.count(); ++ii) {
        const QChar &c = characters.at(ii);
        QRectF crect = QRectF(m_board->rect(c)).translated(transform.x(), transform.y());
        QRectF r = rect();
        if(r.intersects(crect)) {
            if(c == m_char) {
                r.moveCenter(crect.center());
                p.setFont(bigFont);
                p.drawText(r, c, Qt::AlignHCenter | Qt::AlignVCenter);
                p.setFont(mainFont);
            } else {
                p.drawText(crect, c, Qt::AlignHCenter | Qt::AlignVCenter);
            }
        }
    }
}

QString KeyboardWidget::closestWord()
{
    QString rv;
    for(int ii = 0; ii < m_occuranceHistory.count(); ++ii)  {
        const KeyOccurance &o = m_occuranceHistory.at(ii);
        if(!o.freezeWord.isEmpty())
            rv = o.freezeWord;
        if(o.type == KeyOccurance::CharSelect)
            rv.append(o.explicitChar);
        else if(o.type == KeyOccurance::MousePress)
            rv.append(closestCharacter(o.widPoint, m_boards.at(o.board)));
    }
    return rv;
}

void KeyboardWidget::dumpPossibleMotion()
{

    QString res;
    if(m_possibleMotion & Right) res += "Right ";
    if(m_possibleMotion & Left) res += "Left ";
    if(m_possibleMotion & Up) res += "Up ";
    if(m_possibleMotion & Down) res += "Down ";
    qWarning() << res;
}

KeyboardWidget::KeyboardWidget(const Config &config,
                               QWidget *parent)
: QWidget(parent, (Qt::WindowFlags)(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)), m_config(config), m_mouseTimer(0),
  m_currentBoard(0),
  m_pressAndHold(false),m_animate_accept(true), m_charWindow(0),
  m_boardChangeTimeline(config.boardChangeTime),
  m_ignoreMouse(false), m_ignore(false), optionsWindowTimer(0),
  m_notWord(false),
  m_predict(0), m_autoCap(false), m_autoCapitaliseEveryWord(false),
  m_preeditSpace(false), m_dontAddPreeditSpace(false)
{
    setAttribute(Qt::WA_InputMethodTransparent, true);

    /*
    WordPredict::Config wconfig;
    wconfig.reallyNoMoveSensitivity = m_config.reallyNoMoveSensitivity;
    wconfig.moveSensitivity = m_config.moveSensitivity;
    wconfig.excludeDistance = m_config.excludeDistance;

    m_predict = new WordPredict(wconfig, m_config.maxGuesses);
    */

    setFixedSize(m_config.keySize.width(), m_config.keySize.height());

    m_boardRect = QRect(m_config.keyMargin,
                        m_config.bottomMargin,
                        m_config.keySize.width() - 2 * m_config.keyMargin,
                        m_config.keySize.height() - 2 * m_config.bottomMargin);
    m_boardSize = m_boardRect.size();

    QObject::connect(&m_boardChangeTimeline, SIGNAL(valueChanged(qreal)), this, SLOT(update()));

    m_options = new OptionsWindow(m_config.optionWordSpacing);
    QObject::connect(m_options, SIGNAL(wordAccepted()),
                     this, SLOT(acceptWord()));

    setWindowTitle("_allow_on_top_"); // Use window manager back door.
    setWindowModality (Qt::NonModal);
    setFocusPolicy(Qt::NoFocus);
}

KeyboardWidget::~KeyboardWidget()
{
    qDeleteAll(m_predictors);
}

void KeyboardWidget::autoCapitalizeNextWord(bool autocap)
{
    m_autoCap = autocap;
}

void KeyboardWidget::setAcceptDest(const QPoint &p)
{
    m_options->setAcceptDest(p);
}

void KeyboardWidget::reset()
{
    if(!m_words.isEmpty()) {
        m_animate_accept = false;
        m_dontAddPreeditSpace = true;
        acceptWord();
        m_dontAddPreeditSpace = false;
        m_animate_accept = true;
    };

    clear();
    m_options->clear(OptionsWindow::ClearSoon);
}

/*void KeyboardWidget::focusOut()
{
    return;
    if(!m_words.isEmpty()) {
        m_animate_accept = false;
        acceptWord();
        m_animate_accept = true;
    } else {
        emit commit(QString());
    }
}*/

QSize KeyboardWidget::sizeHint() const
{
    return QSize(m_config.keySize.width(), m_config.keySize.height());
}

void KeyboardWidget::addBoard(const QString &l_name, const QStringList &chars, BoardType type)
{
    Board *board = new Board(l_name, chars, m_boardSize, type);
    m_boards << board;

    if(NonAlphabet != type && !m_predictors.count(l_name)) {
        WordPredict::Config wconfig;
        wconfig.reallyNoMoveSensitivity = m_config.reallyNoMoveSensitivity;
        wconfig.moveSensitivity = m_config.moveSensitivity;
        wconfig.excludeDistance = m_config.excludeDistance;

        WordPredict *wp = new WordPredict(wconfig, m_config.maxGuesses);

        QString chars = board->characters();
        for(int ii = 0; ii < chars.length(); ++ii) {
            wp->setLetter(chars.at(ii).toLower(), board->rect(chars.at(ii)).center());
        }
        m_predictors[l_name] = wp;
    }

    if(LowerCase == type)
        setBoard(m_boards.count() - 1);
}

void KeyboardWidget::setBoard(int newBoard)
{
    bool layoutChanged = m_boards.at(m_currentBoard)->layoutName() != m_boards.at(newBoard)->layoutName();
    m_currentBoard = newBoard;
    m_predict = m_predictors[m_boards.at(m_currentBoard)->layoutName()];
    if (m_predict && layoutChanged)
      m_predict->reset();
}

void KeyboardWidget::setDefaultLayout(const QString &l_name)
{
  m_defaultLayout = l_name;
}

void KeyboardWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    p.setClipRect(m_boardRect);

    if(!m_boards.isEmpty()) {
        if(m_boardChangeTimeline.state() == QTimeLine::Running) {
            if(m_boardUp) {
                p.translate(0, m_boardRect.height() * (1.0f - m_boardChangeTimeline.currentValue()));
            } else {
                p.translate(0, -m_boardRect.height() + m_boardChangeTimeline.currentValue() * m_boardRect.height());
            }
        }
        Board *board = m_boards.at(m_currentBoard);
        p.setFont(board->font());

        QString str = board->characters();
        for(int ii = 0; ii < str.length(); ++ii) {
            QChar c = str.at(ii);
            QRect r = board->rect(c);

            p.drawText(r.translated(m_boardRect.x(), m_boardRect.y()), c, Qt::AlignHCenter | Qt::AlignVCenter);
        }

        if(m_boardChangeTimeline.state() == QTimeLine::Running) {
            if(m_boardUp) {
                p.translate(0, -m_boardRect.height());
            } else {
                p.translate(0, m_boardRect.height());
            }
            Board *board = m_boards.at(m_oldBoard);
            p.setFont(board->font());
            QString str = board->characters();
            for(int ii = 0; ii < str.length(); ++ii) {
                QChar c = str.at(ii);
                QRect r = board->rect(c);

                p.drawText(r.translated(m_boardRect.x(), m_boardRect.y()), c, Qt::AlignHCenter | Qt::AlignVCenter);
            }
        }
    }
}

void KeyboardWidget::mousePressEvent(QMouseEvent *e)
{
    if(m_ignoreMouse)
        return;

    m_mousePressPoint = e->pos();
    m_lastSamplePoint = m_mousePressPoint;
    m_mouseMovePoint = m_mousePressPoint;
    m_possibleMotion = (Motion)(Left | Right | Up | Down);

    m_pressAndHold = false;
    m_mouseClick = true;
    startMouseTimer();
}

/*
   Interesting motions are:
        Click
            + Limited time
            + Limited motion

        Stroke up
        Stroke down
        Stroke left
        Stroke right
            + Minimum sampled velocity
            + Minimum single direction motion
            + Minimum motion ratio

        Hold and move
            + Everything else
*/
void KeyboardWidget::mouseReleaseEvent(QMouseEvent *e)
{
    bool m_charWindowOk = false;
    if(m_charWindow) {
        m_charWindowOk = m_charWindow->charOk();
        m_charWindow->dismiss();
        m_charWindow = 0;
    }

    if(m_ignoreMouse) {
        m_ignoreMouse = false;
        return;
    }

    //dumpPossibleMotion();
    update();

    stopMouseTimer();

    if(m_boards.isEmpty())
        return;

    QPoint rp = e->pos();

    if(m_mouseClick) {
        QPoint p((rp.x() + m_mousePressPoint.x()) / 2,
                 (rp.y() + m_mousePressPoint.y()) / 2);
        mouseClick(p);
        return;
    }

    if(m_pressAndHold) {
        if(m_boardRect.contains(e->pos()) && m_charWindowOk)
            pressAndHoldChar(m_pressAndHoldChar);
        m_pressAndHold = false;
        return;
    }


    Stroke updown = NoStroke;
    int y_delta = e->pos().y() - m_mousePressPoint.y();
    int x_delta = e->pos().x() - m_mousePressPoint.x();

    Stroke leftright = NoStroke;

    if(m_possibleMotion & Down && y_delta > (m_boardSize.height() / 3))
        updown = StrokeDown;
    else if(m_possibleMotion & Up && -y_delta > (m_boardSize.height() / 3))
        updown = StrokeUp;

    if(m_possibleMotion & Right && x_delta > (m_boardSize.width() / 3))
        leftright = StrokeRight;
    else if(m_possibleMotion & Left && -x_delta > (m_boardSize.width() / 3))
        leftright = StrokeLeft;

    if(updown != NoStroke && leftright != NoStroke) {
        if((qAbs(y_delta) / qAbs(x_delta)) > m_config.minimumStrokeDirectionRatio) {
            leftright = NoStroke;
        } else if((qAbs(x_delta) / qAbs(y_delta)) > m_config.minimumStrokeDirectionRatio) {
            updown = NoStroke;
        } else {
            leftright = NoStroke;
            updown = NoStroke;
        }
    }

    if(updown != NoStroke) {
        stroke(updown);
    } else if(leftright != NoStroke) {
        stroke(leftright);
    }

}

void KeyboardWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(m_ignoreMouse)
        return;

    m_mouseMovePoint = e->pos();

    if(qMax(qAbs(e->pos().x() - m_mousePressPoint.x()), qAbs(e->pos().y() - m_mousePressPoint.y())) > m_config.maximumClickStutter)
        m_mouseClick = false;

    if(m_pressAndHold || m_charWindow) {

        QChar c = closestCharacter(m_mouseMovePoint);
        if(c != m_pressAndHoldChar) {
            QPoint cp = m_boards.at(m_currentBoard)->rect(c).translated(m_boardRect.x(), m_boardRect.y()).center();
            QPoint cp_cur = m_boards.at(m_currentBoard)->rect(m_pressAndHoldChar).translated(m_boardRect.x(), m_boardRect.y()).center();

            int delta_x = cp.x() - m_mouseMovePoint.x();
            int delta_y = cp.y() - m_mouseMovePoint.y();
            int delta_x_cur = cp_cur.x() - m_mouseMovePoint.x();
            int delta_y_cur = cp_cur.y() - m_mouseMovePoint.y();

            int distance = delta_x * delta_x + delta_y * delta_y;
            int distance_cur = delta_x_cur * delta_x_cur + delta_y_cur * delta_y_cur;
            bool closer = distance < ((distance_cur * 4) / 9);

            if(closer)  {
                m_pressAndHoldChar = c;
                m_charWindow->setChar(c, windowPosForChar(), m_boards.at(m_currentBoard));
                update();
            }
        }
    }
}

QPoint KeyboardWidget::windowPosForChar() const
{
    QPoint c = m_boards.at(m_currentBoard)->rect(m_pressAndHoldChar).translated(m_boardRect.x(), m_boardRect.y()).center();

    return mapToGlobal(c);
}

void KeyboardWidget::timerEvent(QTimerEvent *)
{
    m_mouseClick = false;

    if(m_possibleMotion & Right) {
        if((m_mouseMovePoint.x() - m_lastSamplePoint.x()) < m_config.minimumStrokeMotionPerPeriod)
            m_possibleMotion = (Motion)(m_possibleMotion & ~Right);
    }

    if(m_possibleMotion & Left) {
        if((m_lastSamplePoint.x() - m_mouseMovePoint.x()) < m_config.minimumStrokeMotionPerPeriod)
            m_possibleMotion = (Motion)(m_possibleMotion & ~Left);
    }

    if(m_possibleMotion & Down) {
        if(m_mouseMovePoint.y() - m_lastSamplePoint.y() < m_config.minimumStrokeMotionPerPeriod)
            m_possibleMotion = (Motion)(m_possibleMotion & ~Down);
    }

    if(m_possibleMotion & Up) {
        if((m_lastSamplePoint.y() - m_mouseMovePoint.y()) < m_config.minimumStrokeMotionPerPeriod)
            m_possibleMotion = (Motion)(m_possibleMotion & ~Up);
    }

    m_lastSamplePoint = m_mouseMovePoint;

    if(m_possibleMotion == 0) {
        stopMouseTimer();
        pressAndHold();
    } else {
        speedMouseTimer();
    }

    //dumpPossibleMotion();
}

void KeyboardWidget::startMouseTimer()
{
    if(m_mouseTimer)
        return;

    if(m_notWord || m_boards.at(m_currentBoard)->isAlphabet())
        m_mouseTimer = startTimer(m_config.maximumClickTime);
    else
        m_mouseTimer = startTimer(m_config.maximumClickTime / 2);

    m_speedMouseTimer = false;
}

void KeyboardWidget::stopMouseTimer()
{
    if(m_mouseTimer) {
        killTimer(m_mouseTimer);
        m_mouseTimer = 0;
        m_speedMouseTimer = false;
    }
}

void KeyboardWidget::speedMouseTimer()
{
    if(m_mouseTimer && m_speedMouseTimer)
        return;

    if(m_mouseTimer)
        killTimer(m_mouseTimer);

    m_mouseTimer = startTimer(m_config.strokeMotionPeriod);
    m_speedMouseTimer = true;
}


QChar KeyboardWidget::closestCharacter(const QPoint &p, Board *board) const
{
    if(!board)
        board = m_boards.at(m_currentBoard);

    QString str = board->characters();
    int distance = -1;
    QChar c;

    QPoint bp = toBoardPoint(p);
    for(int ii = 0; ii < str.length(); ++ii) {
        QRect r = board->rect(str.at(ii));

        int x_delta = r.center().x() - bp.x();
        int y_delta = r.center().y() - bp.y();

        int t_distance = x_delta * x_delta + y_delta * y_delta;
        if(distance == -1 || t_distance < distance) {
            distance = t_distance;
            c = str.at(ii);
        }
    }

    return c;
}

/*
    Returns the rect in widget co-ordinates where \a c is visible, or QRect().
*/
QRect KeyboardWidget::rectForCharacter(const QChar &c) const
{
    QRect ret;

    Board *board = 0;
    if (-1 != m_currentBoard && (board = m_boards.at(m_currentBoard))) {
        ret = board->rect(c).translated(m_boardRect.x(), m_boardRect.y());
    }

    return ret;
}

/*
    Returns the rect in widget co-ordinates where \a word is visible, or QRect().
*/
QRect KeyboardWidget::rectForWord(const QString &word)
{
    QRect ret;

    for (int ii = 0; ii < m_words.count(); ++ii) {
        if (word == m_words.at(ii)) {
            QRect wr = m_options->wordRect(ii);
            ret = wr;
            ret.moveTopLeft(mapFromGlobal(m_options->mapToGlobal(wr.topLeft())));
            break;
        }
    }

    return ret;
}

/*
    Returns the words currently shown to the user.
*/
QStringList KeyboardWidget::words() const
{
    return m_words;
}

QPoint KeyboardWidget::toBoardPoint(const QPoint &p) const
{
    QPoint rv = p;
    rv.setX(rv.x() - m_boardRect.x());
    rv.setY(rv.y() - m_boardRect.y());
    return rv;
}

void KeyboardWidget::mouseClick(const QPoint &p)
{
    if(m_preeditSpace) {
        m_preeditSpace = false;

        if(m_boards.at(m_currentBoard)->isAlphabet() ||
           m_boards.at(m_currentBoard)->isNumeric()) {
            emit commit(QString(" "));
        } else {
            emit commit(QString());
        }
    }

    if(m_boards.at(m_currentBoard)->type() == Numeric) {
        pressAndHoldChar(closestCharacter(p, m_boards.at(m_currentBoard)));
        return;
    }

    if(!m_boards.at(m_currentBoard)->isAlphabet())
        m_notWord = true;

    if(m_notWord)
        return;

    QPoint point = toBoardPoint(p);

    KeyOccurance occurance;
    occurance.type = KeyOccurance::MousePress;
    occurance.widPoint = p;
    occurance.board = m_currentBoard;
    m_occuranceHistory << occurance;

    if (m_predict)
      m_predict->addTouch(point);

    updateWords();
}

void KeyboardWidget::pressAndHoldChar(const QChar &c)
{
    bool wasNotWord = m_notWord;

    if(!m_boards.at(m_currentBoard)->isAlphabet() && c != '\'')
        m_notWord = true;

    if(m_preeditSpace) {
        m_preeditSpace = false;

        if(m_boards.at(m_currentBoard)->isAlphabet() ||
           m_boards.at(m_currentBoard)->isNumeric()) {
            emit commit(QString(" "));
        } else {
            emit commit(QString());
        }
    }

    if(c == QChar(0x21b5) || c.isNumber()) {// Magic number - return key.
                                            // Numbers aren't that magic, though they don't need completion too.
        //m_preeditSpace = false;
        m_dontAddPreeditSpace = true;
        if (hasText())
          acceptWord();
        if (c==QChar(0x21b5))
          emit commit("\n");
        else
          emit commit(c);
        m_dontAddPreeditSpace = false;
        clear();
    }
    else {
        KeyOccurance occurance;
        occurance.type = KeyOccurance::CharSelect;
        // Special-case carriage returns
        occurance.board = m_currentBoard;
        occurance.explicitChar = c;
        if(!wasNotWord && m_notWord)
            occurance.freezeWord = m_options->selectedWord();

        m_occuranceHistory << occurance;

        if (m_predict)
          m_predict->addLetter(c.toLower());

        updateWords();
    }
}

/*!
    Returns true if the keyboard widget currently has input awaiting a commit, or if it has a preedit space pending, otherwise returns false.
    This is an effective check for a preedit string.
*/
bool KeyboardWidget::hasText()
{
    return !m_words.isEmpty() || m_preeditSpace;
}

void KeyboardWidget::updateWords()
{
    m_words = QStringList();

    if(!m_notWord && m_predict)
        m_words = m_predict->words();

    if(m_words.isEmpty())
        m_words << closestWord();

    m_words = fixupCase(m_words);
    m_options->setWords(m_words);

    // Change to lowercase if appropriate for autocapitalisation -
    // i.e. if this is the first letter of the word, autocapitalisation is on,
    // and the current board is still uppercase.
    // Changing back to uppercase is handled in KeyboardWidget::acceptWord()
    if(m_autoCap && m_occuranceHistory.count() == 1 && m_boards.at(m_currentBoard)->type() == UpperCase) {
        setBoardByType(LowerCase);
    }

    update();

    if(m_words.isEmpty() && m_preeditSpace) {
        emit commit(" ");
    } else if(m_words.isEmpty()) {
        emit commit(QString());
    } else {
#ifdef PREFIX_PREEDIT
        if(!m_notWord && m_predict && !m_predict->prefixedWord().isEmpty())
            emit preedit(m_predict->prefixedWord());
        else
#endif
        emit preedit(m_words.first());
    }
}

/*
  We change case if the user has any explicit characters or if the user does not
  stay on a lowercase board.
*/
QStringList KeyboardWidget::fixupCase(const QStringList &list) const
{
    bool needFixupCase = false;

    for(int ii = 0; !needFixupCase && ii < m_occuranceHistory.count(); ++ii) {
        const KeyOccurance &o = m_occuranceHistory.at(ii);
        if(o.type == KeyOccurance::CharSelect ||
           m_boards.at(o.board)->type() != LowerCase)
            needFixupCase = true;
    }

    if(!needFixupCase) {
        return list;
    } else {
        QStringList rv;

        // XXX - stupidly inefficient
        for(int ii = 0; ii < list.count(); ++ii) {
            QString str = list.at(ii);
            Q_ASSERT(str.length() == m_occuranceHistory.count());

            for(int jj = 0; jj < str.length(); ++jj) {
                if(m_boards.at(m_occuranceHistory.at(jj).board)->type()
                        == UpperCase)
                    str[jj] = str[jj].toUpper();
                else if(m_boards.at(m_occuranceHistory.at(jj).board)->type()
                        == LowerCase)
                    str[jj] = str[jj].toLower();
            }

            rv << str;
        }
        return rv;
    }
}

void KeyboardWidget::setBoardByType(BoardType newBoardType)
{
    int newBoard = -1;
    // find new board that matches type;

    for(int i = 0; i < m_boards.count(); i++) {
        if (m_boards.at(i)->type() == newBoardType) {
            if (newBoard==-1 || // Select at least any
                (newBoard!=-1 && m_boards.at(newBoard)->layoutName()!=m_boards.at(m_currentBoard)->layoutName() &&
                  m_boards.at(i)->layoutName()==m_defaultLayout) || // Prefer default layout to any
                m_boards.at(i)->layoutName()==m_boards.at(m_currentBoard)->layoutName()) // We don't need to change current layout name
              newBoard = i;
        }
    }

    if(newBoard == -1) {
        qLog(Input) << "KeyboardWidget::setBoardByType - Failed to find board of type "<<newBoardType;
        return;
    }

    if( newBoard == m_currentBoard ) {
        qLog(Input) << "KeyboardWidget::setBoardByType - already at board of type "<<newBoardType;
        return;
    }

    if(isVisible()) {
        // animate
        m_oldBoard = m_currentBoard;
        setBoard(newBoard);
        if(m_currentBoard < 0)
            m_currentBoard = m_boards.count() - 1;
        if(!m_boards.at(m_oldBoard)->isAlphabet() ||
                !m_boards.at(m_currentBoard)->isAlphabet()) {
            m_boardChangeTimeline.start();

            m_boardUp = ( m_currentBoard > m_oldBoard );
            if( abs(m_currentBoard - m_oldBoard) > m_boards.count() / 2)
                m_boardUp = !m_boardUp;
        } else {
            update();
        }
    } else {
        //not visible, so just change the board
        m_oldBoard = m_currentBoard;
        setBoard(newBoard);
        update();
    }
}


void KeyboardWidget::stroke(Stroke s)
{
    switch(s) {
        case StrokeUp:
        {
            m_oldBoard = m_currentBoard;
            int newBoard = m_currentBoard-1;
            if(newBoard < 0)
                newBoard = m_boards.count() - 1;
            setBoard(newBoard);
            if(m_boards.at(m_oldBoard)->layoutName() !=
               m_boards.at(m_currentBoard)->layoutName()) {
                m_boardChangeTimeline.start();
                m_boardUp = true;
            } else {
               update();
            }
        } break;

        case StrokeDown:
            m_oldBoard = m_currentBoard;
            setBoard((m_currentBoard + 1) % m_boards.count());
            if(m_boards.at(m_oldBoard)->layoutName() !=
               m_boards.at(m_currentBoard)->layoutName()) {
                m_boardChangeTimeline.start();
                m_boardChangeTimeline.start();
                m_boardUp = false;
            } else {
               update();
            }
            break;

        case StrokeLeft:
            doBackspace();
            break;

        case StrokeRight:
            acceptWord();
            break;

        case NoStroke:
            break;
    }
}

void KeyboardWidget::pressAndHold()
{
    m_pressAndHold = true;

    if(!m_charWindow) {
        m_pressAndHoldChar = closestCharacter(m_mouseMovePoint);

        m_charWindow = new PopupWindow(m_config.selectCircleOffset, m_config.popupScaleFactor, 0);
        m_charWindow->setFixedSize(m_config.selectCircleDiameter,
                                   m_config.selectCircleDiameter);
        m_charWindow->setChar(m_pressAndHoldChar, windowPosForChar(), m_boards.at(m_currentBoard));
    }

    update();

    emit pressedAndHeld();
}

/*
   If the last character was an explicit character, we will remove just it.
   Otherwise we remove the entire word.

   The rational for this behavior is in regards to how the user
   discovers their error.  If the user doesn't see the word they
   want on the screen as the set of choices, they have no reliable
   way of working out which character they typed wrong.  Hence
   Starting over is the most reasonable choice.  Given the goal
   is to be fast, retyping the word correctly should be faster than
   deleting half the word to correct.

   The counter argument for this rational is two fold.  The first
   is that people can sometimes realize their error without screen
   feedback.  Secondly the error might not be their typing, but missing
   the 'select a word' area, putting in an unintended letter at the end
   (happened to the author of this paragraph five times so far).
   Finally, if 'backing' is easy as a button press rather than a stroke
   then its actually pretty easy to back up to the point where the error
   is obvious.

   For now leaving as the 'remove whole word' behavior.
*/
void KeyboardWidget::doBackspace()
{
    bool wasEmpty = m_words.isEmpty();

    if(m_preeditSpace) {

        emit commit(QString());
        m_preeditSpace = false;
        return;

    } else if(m_occuranceHistory.isEmpty() || m_occuranceHistory.count() == 1 ||
       m_occuranceHistory.last().type == KeyOccurance::MousePress) {

        clear();

    } else {

        m_occuranceHistory.removeLast();
        resetToHistory();

    }

    if(m_words.isEmpty()) {
        if(wasEmpty) {
            m_options->clear(OptionsWindow::ClearImmediate);
            emit backspace();
        } else {
            m_options->clear(OptionsWindow::ClearSoon);
        }

        emit commit(QString());
    }
}

void KeyboardWidget::acceptWord()
{
    if(m_words.isEmpty()) {
        emit commit(" ");
        m_preeditSpace = false;
        return;
    }

    QString word = m_options->acceptWord(QPoint(0, 0), m_animate_accept /* XXX microfocushint */);
    m_options->clear(OptionsWindow::ClearEventually);
    clear();
    m_autoCap = m_autoCapitaliseEveryWord;

    if(m_autoCap && m_boards.at(m_currentBoard)->type() == LowerCase)
        setBoardByType(UpperCase);

    emit commit(word);
    if(!m_dontAddPreeditSpace) {
        m_preeditSpace = true;
        emit preedit(" ");
    }
}

void KeyboardWidget::resizeEvent(QResizeEvent *)
{
    positionOptionsWindow();
}

void KeyboardWidget::hideEvent(QHideEvent *)
{
    m_options->hide();
}

void KeyboardWidget::showEvent(QShowEvent *)
{
    positionOptionsWindow();
}

void KeyboardWidget::moveEvent(QMoveEvent *)
{
    positionOptionsWindow();
}

// Start a zero timer to reposition the options window (zero timer is
// needed to allow windowmanager time to finish docking keyboard widget).
void KeyboardWidget::positionOptionsWindow()
{
    if(!optionsWindowTimer)
    {
        optionsWindowTimer = new QTimer(0);
        optionsWindowTimer->setInterval(0);
        optionsWindowTimer->setSingleShot(true);
        connect(optionsWindowTimer, SIGNAL(timeout()),this, SLOT(positionTimeOut()));
    }
    optionsWindowTimer->start();
}

// Position the options window immediately.
void KeyboardWidget::positionTimeOut()
{
    int oheight = m_config.optionsWindowHeight;

    if(oheight == -1) {
        int bottom = QApplication::desktop()->availableGeometry().bottom();
        oheight = pos().y() - bottom - 1;
        if(oheight <= 0)
            oheight = 50;
    }

    m_options->setFixedSize(width(), oheight);
    m_options->move(pos() - QPoint(0, oheight));
}

void KeyboardWidget::clear()
{
    if (m_predict)
      m_predict->reset();
    m_words.clear();
    m_notWord = false;
    m_occuranceHistory.clear();
}

void KeyboardWidget::resetToHistory()
{
    if (!m_predict) // Only predictor's state is changed here
      return;

    m_predict->reset();

    bool localNotWord = false;
    for(int ii = 0; ii < m_occuranceHistory.count(); ++ii ) {
        const KeyOccurance &o = m_occuranceHistory.at(ii);

        if(!m_boards.at(o.board)->isAlphabet())
            localNotWord = true;

        if(!localNotWord) {
            if(o.type == KeyOccurance::CharSelect) {
                m_predict->addLetter(o.explicitChar.toLower());
            } else if(o.type == KeyOccurance::MousePress) {
                if(!localNotWord) {
                    m_predict->addTouch(toBoardPoint(o.widPoint));
                }
            }
        }
    }
    m_notWord = localNotWord;

    updateWords();
}

#ifdef Q_WS_QWS

#include <private/qwindowsurface_qws_p.h>
#include <private/qwindowsurface_p.h>

/*
  Trivial surface class who sole purpose is to reimplement the move
  functions and prevent blitting to the screen while moving.
*/
class PopupWindowSurface : public QWSSharedMemSurface
{
public:
    PopupWindowSurface() : QWSSharedMemSurface() {}
    PopupWindowSurface(QWidget *w) : QWSSharedMemSurface(w) {}

    QString key() const { return QLatin1String("PopupWindowSurface"); }

    bool move(const QPoint &offset);
    QRegion move(const QPoint &offset, const QRegion &newClip);

    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);

private:
    QRegion needsFlush;
};

bool PopupWindowSurface::move(const QPoint &offset)
{
    const QRegion oldGeometry = geometry();
    QWSSharedMemSurface::move(offset);
    const QPoint winOffset = window()->mapToGlobal(QPoint(0, 0));
    needsFlush += (oldGeometry - geometry()).translated(-winOffset);

    return false;
}

QRegion PopupWindowSurface::move(const QPoint &offset, const QRegion &newClip)
{
    QWSSharedMemSurface::move(offset, newClip);
    return QRegion();
}

void PopupWindowSurface::flush(QWidget *widget, const QRegion &region,
                               const QPoint &offset)
{
    QWSSharedMemSurface::flush(widget, region + needsFlush, offset);
    needsFlush = QRegion();
}

/*/
  Trival screen class who sole purpose is to instantiate PopupWindowSurfaces
  on the server side.
*/
class PopupScreen : public ProxyScreen
{
public:
    PopupScreen() : ProxyScreen(0)
    {
        ProxyScreen::setScreen(qt_screen);
        qt_screen = this; // XXX
    }

    QWSWindowSurface* createSurface(QWidget *w) const
    {
        if (qobject_cast<PopupWindow*>(w))
            return new PopupWindowSurface(w);
        return ProxyScreen::createSurface(w);
    }

    QWSWindowSurface* createSurface(const QString &key) const
    {
        if (key == QLatin1String("PopupWindowSurface"))
            return new PopupWindowSurface;
        return ProxyScreen::createSurface(key);
    }
};
#endif

void KeyboardWidget::instantiatePopupScreen()
{
#ifdef Q_WS_QWS
    // XXX
    new PopupScreen;
#endif

}

void KeyboardWidget::setHint(const QString& hint)
{
    QStringList args = hint.split(" ");
    QString h=args.first();

    //bool wasVisible = isVisible();
    bool boardHasBeenSet = false;

    //qLog(Input) << "PredictiveKeyboard : setHint("""<< hint <<""", head is "<<h<<", full args are "<<args;

    // update microfocus
    //qwsServer->sendIMQuery ( Qt::ImMicroFocus );

    if (h.contains("propernouns")) { // no tr
        qLog(Input) << "PredictiveKeyboard::setHint(" << h << ") - setting autocapitalisation";
        // TODO: Set autocapitalisation
        setBoardByType(UpperCase);
        m_autoCap = true;
        m_autoCapitaliseEveryWord = true;
        boardHasBeenSet = true;
    } else if(args.contains("noautocapitalization")) {
        m_autoCap = false;
        m_autoCapitaliseEveryWord = false;
    } else {
        m_autoCap = true;
        m_autoCapitaliseEveryWord = false;
    }

    if ((h == "phone" || h == "int")) { // no tr
        qLog(Input) << "PredictiveKeyboard::setHint(" << h << ") - changing to numbers mode";
        setBoardByType(Numeric);
        boardHasBeenSet = true;
        m_autoCap = false;
    }

    if(!boardHasBeenSet && h == "text" || h == "email" || h == "words") {
        if (!m_autoCap || h == "email")
            setBoardByType(LowerCase);
        else
            setBoardByType(UpperCase);
        boardHasBeenSet = true;
    }
}

/*!
    Filter key events.
    On Qt::Key_Select, the input method discards any preedit space, and commits any other preedit text.  The predictive keyboard never consumes Qt::Key_Select, it is always passed back to the system after processing.
    On Qt::Key_Back, the input method will consume the key press and release only if it has preedit text. If the key event is consumed, the KeyboardWidget will \l{doBackspace()}
    \sa acceptWord(), doBackspace()
*/
bool KeyboardWidget::filter ( int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat )
{
    Q_UNUSED(unicode);
    Q_UNUSED(modifiers);
    Q_UNUSED(modifiers);
    Q_UNUSED(autoRepeat);

    if (!hasText())
        return false;

    if(keycode == Qt::Key_Select) {
        // on release, commit text
        if (isPress)
            return true;
        acceptWord();
        return true;
    }

    //Handle backspace
    if(keycode == Qt::Key_Back) {
        if(isPress)
            return true;
        doBackspace();
        return true;
    }

    return false;
}

#include "keyboard.moc"
