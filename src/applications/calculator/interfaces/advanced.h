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
#ifdef ENABLE_SCIENCE

#ifndef ADVANCEDIMPL_H
#define ADVANCEDIMPL_H

#include <QPushButton>
#include <QEvent>


#include "../engine.h"
#include "stdinputwidgets.h"


class QRadioButton;


class AdvancedButton : public QPushButton {
public:
    AdvancedButton(QWidget *p = 0) : QPushButton(p), inv(false) {
        fontChange( font() );
    }
    void fontChange( const QFont &f ) {
        QFontMetrics fm( f );
        int fontHeight = fm.height();
        setMinimumHeight( fontHeight * 2 );
        fontSize10 = f.pixelSize();
        if (fontSize10 == -1 )
            fontSize10 = f.pointSize();
        fontSize05 = fontSize10 / 2;
        ascent = fm.ascent();
    }

    virtual void advancedDrawLabel( QPainter *p, int x, int y ) = 0;
    void setInversed(bool i) { inv = i; update(); repaint();}

    void paintEvent(QPaintEvent *pe)
    {
        QPushButton::paintEvent(pe);
        QPainter p(this);

        int x = (width() - fontSize10) / 2;
        int y = ((height() - fontSize10) / 2) + ascent;
        if ( isDown() )
            x++, y++;
        advancedDrawLabel(&p, x, y);
    }

protected:
    bool inv;
    int fontSize05, fontSize10, ascent;
};


class FormAdvanced:public DecimalInputWidget{
    Q_OBJECT
public:
        FormAdvanced(QWidget * parent = 0);
        virtual ~FormAdvanced();

        QString interfaceName() { return QString(tr("Scientific")); };


protected:
        void init(int fromRow, int fromCol);
        void showEvent ( QShowEvent * );

private:
        QPushButton *PBDel,*PBC;
        QPushButton *PBMR,*PBMC,*PBMPlus;
        QPushButton *PBPi, *PBE;
        QPushButton *PBSin,*PBCos,*PBTan,*PBOneOverX;
        QPushButton *PBLog,*PBFactorial;
        AdvancedButton *PBLn,*PBSquare,*PBPow;
        QPushButton *PBInverse;

        QRadioButton *degree, *radians, *gradians;
        bool IsInverse;

private slots:
        void DelClicked();
        void CClicked();
        void MRClicked();
        void MPlusClicked();
        void MCClicked();
        void SinDegClicked();
        void CosDegClicked();
        void TanDegClicked();
        void SinRadClicked();
        void CosRadClicked();
        void TanRadClicked();
        void SinGraClicked();
        void CosGraClicked();
        void TanGraClicked();
        void OneOverXClicked();
        void LnClicked();
        void FactorialClicked();
        void SquareClicked();
        void PowClicked();
        void PiClicked();
        void EClicked();
        void LogClicked();
        void InverseClicked();
        void DegreeMode(bool);
        void RadiansMode(bool);
        void GradiansMode(bool);
};

#endif //ADVANCEDIMPL_H
#endif //ENABLE_SCIENCE
