/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 
#include "splay_cont.h"

#include <eiklabel.h>
#include <barsread.h>
#include <eikedwin.h>

#include "splay.rsg"

const TInt KBufLength = 64;

#define LABEL_POS        TPoint(10, 10)
#define CMD_LINE_WIN_POS TPoint(10, 25)
#define STATUS_POS       TPoint(10, 60)

CHXSplayContainer::CHXSplayContainer() :
    m_pLabel(0),
    m_pCmdLineWin(0),
    m_pPlaybackStatus(0)
{}

CHXSplayContainer::~CHXSplayContainer()
{
    delete m_pLabel;
    delete m_pCmdLineWin;
    delete m_pPlaybackStatus;
}

void CHXSplayContainer::ConstructL(const TRect& aRect)
{
    CreateWindowL();

    TBuf<KBufLength> text;

    iCoeEnv->ReadResource(text, R_SPLAY_LABEL);
    m_pLabel = new (ELeave) CEikLabel;
    m_pLabel->SetContainerWindowL(*this);
    m_pLabel->SetTextL(text);
    m_pLabel->SetExtent(LABEL_POS, m_pLabel->MinimumSize());

    TResourceReader reader;
    iCoeEnv->CreateResourceReaderLC(reader, R_SPLAY_EDWIN);
    m_pCmdLineWin = new (ELeave) CEikEdwin;
    m_pCmdLineWin->SetContainerWindowL(*this);
    m_pCmdLineWin->ConstructFromResourceL(reader);
    CleanupStack::PopAndDestroy();  // Resource reader
    m_pCmdLineWin->SetExtent(CMD_LINE_WIN_POS, m_pCmdLineWin->MinimumSize());

    iCoeEnv->ReadResource(text, R_BLANK_TEXT);
    m_pPlaybackStatus = new (ELeave) CEikLabel;
    m_pPlaybackStatus->SetContainerWindowL(*this);
    m_pPlaybackStatus->SetTextL(text);
    m_pPlaybackStatus->SetExtent(STATUS_POS, m_pPlaybackStatus->MinimumSize());

    m_pCmdLineWin->SetFocus(ETrue);

    SetRect(aRect);
    ActivateL();
}

void CHXSplayContainer::FocusTo(TInt aCommand)
{}

void CHXSplayContainer::ChangeState(CHXSplayAppState::PlayerState newState)
{
    TBuf<KBufLength> text;

    TBool focus = ETrue;

    switch(newState) {
    case CHXSplayAppState::ESplayPlaying :
	focus = EFalse;
	iCoeEnv->ReadResource(text, R_PLAYING_TEXT);
	break;

    case CHXSplayAppState::ESplayComplete :
	iCoeEnv->ReadResource(text, R_COMPLETE_TEXT);
	break;

    case CHXSplayAppState::ESplayFailed :
	iCoeEnv->ReadResource(text, R_FAILED_TEXT);
	break;

    default:
	iCoeEnv->ReadResource(text, R_BLANK_TEXT);
	break;
    };

    // Update control focus
    m_pCmdLineWin->SetFocus(focus);

    // Update status text
    m_pPlaybackStatus->SetTextL(text);
    m_pPlaybackStatus->SetExtent(STATUS_POS, m_pPlaybackStatus->MinimumSize());
    
    DrawDeferred();
}

HBufC* CHXSplayContainer::CommandLine() const
{
    HBufC* pRet = HBufC::New(100);
    
    if (pRet && m_pCmdLineWin)
	m_pCmdLineWin->Text()->Extract(pRet->Des());
    return pRet;
}

void CHXSplayContainer::SizeChanged()
{}

TInt CHXSplayContainer::CountComponentControls() const
{
    return 3;
}

CCoeControl* CHXSplayContainer::ComponentControl(TInt aIndex) const
{
    CCoeControl* pRet = 0;

    switch(aIndex) {
    case 0:
	pRet = m_pLabel;
	break;
    case 1:
	pRet = m_pCmdLineWin;
	break;
    case 2:
	pRet = m_pPlaybackStatus;
	break;
    };

    return pRet;
}

void CHXSplayContainer::Draw(const TRect& aRect) const
{
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushColor(KRgbGray);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(aRect);
}

TKeyResponse CHXSplayContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,
					       TEventCode aType)
{
    TKeyResponse ret = EKeyWasNotConsumed;

    if (m_pCmdLineWin && m_pCmdLineWin->IsFocused())
	ret = m_pCmdLineWin->OfferKeyEventL(aKeyEvent, aType);

    return ret;
}

void CHXSplayContainer::HandleControlEventL(CCoeControl* /*aControl*/,
					    TCoeEvent /*aEventType*/)
{}
