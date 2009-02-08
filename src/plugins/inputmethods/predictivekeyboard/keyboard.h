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

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <QWidget>
#include <QTimeLine>
#include <QHash>
class PopupWindow;
class OptionsWindow;
class Board;
class WordPredict;
class AcceptWindow;

class KeyboardWidget : public QWidget
{
Q_OBJECT
public:
    enum BoardType { NonAlphabet, Numeric, UpperCase, LowerCase };
    static void instantiatePopupScreen();

    struct Config
    {
        int minimumStrokeMotionPerPeriod;
        int strokeMotionPeriod;
        int maximumClickStutter;
        int maximumClickTime;
        qreal minimumStrokeLength;
        qreal minimumStrokeDirectionRatio;

        QSize keyAreaSize;

        int selectCircleDiameter;
        int selectCircleOffset;
        qreal popupScaleFactor;

        int boardChangeTime;

        int leftSquishPoint;
        qreal leftSquishScale;
        int rightSquishPoint;
        qreal rightSquishScale;

        QSize keySize;
        int keyMargin;
        int bottomMargin;

        int maxGuesses;

        int optionWordSpacing;
        int optionsWindowHeight;

        int reallyNoMoveSensitivity;
        int moveSensitivity;
        int excludeDistance;
    };

    KeyboardWidget(const Config &, QWidget *parent = 0);
    virtual ~KeyboardWidget();

    void setSelectionHeight(int);

    void addBoard(const QString &, const QStringList &, BoardType);
    void setDefaultLayout(const QString &l_name);

    void autoCapitalizeNextWord(bool);

    virtual QSize sizeHint() const;
    bool hasText();
    void reset();

    void setAcceptDest(const QPoint &);
    virtual bool filter(int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat);

signals:
    void backspace();
    void preedit(const QString &);
    void commit(const QString &);
    void pressedAndHeld();

protected:
    virtual void paintEvent(QPaintEvent *);

    virtual void resizeEvent(QResizeEvent *);
    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);
    virtual void moveEvent(QMoveEvent *);

    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);

    virtual void timerEvent(QTimerEvent *);

public slots:
    void acceptWord();
    virtual void setHint(const QString& hint);
    void doBackspace();
    QRect rectForCharacter(const QChar &) const;
    QRect rectForWord(const QString &);
    QStringList words() const;

private slots:
    void positionTimeOut();

private:
    Config m_config;

    void clear();
    void resetToHistory();
    QStringList m_words;

    void dumpPossibleMotion();

    void startMouseTimer();
    void speedMouseTimer();
    void stopMouseTimer();
    int m_mouseTimer;
    bool m_speedMouseTimer;

    void mouseClick(const QPoint &);
    enum Stroke { NoStroke, StrokeLeft, StrokeRight, StrokeUp, StrokeDown };
    void stroke(Stroke);
    void pressAndHold();
    void pressAndHoldChar(const QChar &);

    QChar closestCharacter(const QPoint &, Board * = 0) const;

    QList<Board *> m_boards;
    int m_currentBoard;
    QSize m_boardSize;
    QRect m_boardRect;
    QPoint toBoardPoint(const QPoint &) const;
    void setBoard(int newBoard);

    enum Motion { Left = 0x01, Right = 0x02, Up = 0x04, Down = 0x08 };
    Motion m_possibleMotion;
    QPoint m_mouseMovePoint;
    QPoint m_lastSamplePoint;
    QPoint m_mousePressPoint;
    bool m_mouseClick;
    bool m_pressAndHold;
    QChar m_pressAndHoldChar;
    bool m_animate_accept;

    QPoint windowPosForChar() const;
    PopupWindow *m_charWindow;

    void setBoardByType(BoardType newBoard);
    QTimeLine m_boardChangeTimeline;
    int m_oldBoard;
    bool m_boardUp;
    QString m_defaultLayout;

    bool m_specialDelete;

    bool m_ignoreMouse;
    bool m_ignore;

    void positionOptionsWindow();
    OptionsWindow *m_options;
    QTimer* optionsWindowTimer;

    bool m_notWord;
    WordPredict *m_predict;
    QHash<QString, WordPredict *> m_predictors;
    void updateWords();
    QString closestWord();
    bool m_autoCap;
    bool m_autoCapitaliseEveryWord;
    bool m_preeditSpace;
    bool m_dontAddPreeditSpace;

    QStringList fixupCase(const QStringList &) const;

    struct KeyOccurance {
        enum Type { MousePress, CharSelect };
        Type type;
        QPoint widPoint;
        QChar explicitChar;
        int board;

        QString freezeWord;
    };
    QList<KeyOccurance> m_occuranceHistory;
};

#endif // _KEYBOARD_H_

