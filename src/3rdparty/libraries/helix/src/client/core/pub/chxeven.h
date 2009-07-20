/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxeven.h,v 1.5 2005/03/23 23:19:23 liam_murray Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef _CHXEVENT_H_
#define _CHXEVENT_H_

#include "hxtypes.h"

struct IHXPacket;
struct RendererInfo;

class CHXEvent 
{
protected:
    UINT32		m_ulTimeStartPos;
    UINT32		m_ulSequenceNum;
    IHXPacket*		m_pPacket;
    HXBOOL		m_bIsPreSeek;
    HXBOOL                m_bIsImmediate;

public:
    // used by HXPlayer to associate packet with the renderer..
    RendererInfo*	m_pRendererInfo;
    LONG32		m_lOffset; 

public: 

    CHXEvent(IHXPacket*   packetArg,
	     UINT32	    timeStartPosArg = 0,
	     HXBOOL	    bIsPreSeek = FALSE,
	     UINT32	    sequenceNumArg = 0,
             HXBOOL           bIsImmediate = FALSE)
	     :	m_ulTimeStartPos(timeStartPosArg),
                m_ulSequenceNum(sequenceNumArg),
                m_pPacket(packetArg),
		m_bIsPreSeek(bIsPreSeek),
                m_bIsImmediate(bIsImmediate),
		m_pRendererInfo(NULL),
		m_lOffset(0)
     { 
	if (m_pPacket)
	{
	    m_pPacket->AddRef();
	}
     };

    ~CHXEvent(void)
    {
	if (m_pPacket)
	{
	    m_pPacket->Release();
	    m_pPacket = 0;
	}
    };

    inline  IHXPacket* GetPacket(void) 
			    {return m_pPacket;};
    
    inline  UINT32	GetTimeStartPos(void) 
			    {return m_ulTimeStartPos;};

    inline  void	SetTimeStartPos(UINT32 ulTimeStartPos) 
			    {m_ulTimeStartPos = ulTimeStartPos;};

    inline  UINT32	GetSequence(void) 
			    {return m_ulSequenceNum;};

    inline  int		Due(UINT32 timePosArg) 
			    {return (timePosArg >= m_ulTimeStartPos);};
		
    inline  void	SetTimeOffset(LONG32 lOffset) 
			    {m_lOffset = lOffset;};

    inline  LONG32	GetTimeOffset(void) 
			    {return m_lOffset;};

    inline  void	SetPreSeekEvent(HXBOOL bIsPreSeek = TRUE) 
			    {m_bIsPreSeek = bIsPreSeek;};

    inline  HXBOOL	IsPreSeekEvent(void) 
			    {return m_bIsPreSeek;};

    inline  void	SetImmediateEvent(HXBOOL bIsImmediate = TRUE) 
			    {m_bIsImmediate = bIsImmediate;};

    inline  HXBOOL	IsImmediateEvent(void) const
			    {return m_bIsImmediate;};
};

#endif // _CHXEVENT_H_
