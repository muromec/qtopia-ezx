/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxelst.cpp,v 1.10 2007/07/06 21:58:11 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"

#include "chxeven.h"
#include "chxelst.h"
#include "srcinfo.h"
#include "hxstrm.h"
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


HX_RESULT CHXEventList::InsertEvent(CHXEvent*	pEvent)
{
    HX_RESULT theErr = HXR_OK;

    // enqueue the event into event queue
    CHXEvent*		event = NULL;
    HXBOOL		earlierPacketFound = FALSE;
    UINT32		currentPos = 0;
    UINT32		startPos = pEvent->GetTimeStartPos();
    HXBOOL                bIsImmediate = pEvent->IsImmediateEvent();

    // Loop we are done or we find an event that is after this
    // time stamp...
    LISTPOSITION	position = GetTailPosition();
    while (position != NULL && !earlierPacketFound && theErr == HXR_OK )
    {
	event = GetPrev(position);

	// This event must be valid or else it shouldn't be
	// in this list!
	HX_ASSERT_VALID_PTR(event);

	currentPos = event->GetTimeStartPos();

	// We are looking for the first packet that matches
        // the immediate state of the packet we are inserting
        // and has a lower event time. If this condition fails
        // we also check to see if the new event is not
        // immediate and the current event is immediate. This
        // check makes sure that normal events are always inserted
        // after immediate events in the case where no other normal
        // events are present. These conditionals rely on the fact
        // that we are going from the tail of the list to the head.
        // If the iteration direction is changed then this will need
        // to be updated.
	if (((bIsImmediate == event->IsImmediateEvent()) &&
             isEarlier(currentPos, startPos)) ||
            (event->IsImmediateEvent() && !bIsImmediate))
	{
	    // Remember that we found an earlier packet...
	    earlierPacketFound = TRUE;

	    // If the position is null, then event was the first
	    // item in the list, and we need to do some fancy footwork...
	    if (!position)
	    {
		POSITION theHead = GetHeadPosition();
		LISTPOSITION listRet = InsertAfter(theHead,pEvent);
                if( listRet == NULL )
                {
                    theErr = HXR_OUTOFMEMORY;
                }
	    }
	    // otherwise, roll ahead one...
	    else
	    {
		GetNext(position);
		// Now if the position is null, then event was the last
		// item in the list, and we need to do some more fancy footwork...
		if (!position)
		{
		    AddTail(pEvent);
		}
		else
		// otherwise, we have a normal case and we want to insert
		// right after the position of event
		{
		    LISTPOSITION listRet = InsertAfter(position,pEvent);
                    if( listRet == NULL )
                    {
                        theErr = HXR_OUTOFMEMORY;
		    }
		}
	    }

	    // We don't need to search any more...
	    break; // while
	}
    } // end while...

    // If we didn't find an earlier packet, then we should insert at
    // the head of the event list...
    if (!theErr && !earlierPacketFound)
    {
	AddHead(pEvent);
    }

    return theErr;
}

HX_RESULT 
CHXEventList::MakeSourceEventsImmediate(HXSource* pSource)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pSource)
    {
        LISTPOSITION listpos = GetHeadPosition();
        CHXSimpleList tmpList;

        // Remove all the events associated with pSource
        // and put them in tmpList.

        res = HXR_OK;
        while (listpos && (GetNumEvents() != 0) &&
               (res == HXR_OK))
        {
            CHXEvent*  pEvent = GetAt(listpos);
            
            if (pEvent && pEvent->m_pRendererInfo &&
                pEvent->m_pRendererInfo->m_pStreamInfo &&
                pEvent->m_pRendererInfo->m_pStreamInfo->m_pStream)
            {
                HXSource* pEventSource = 
                    pEvent->m_pRendererInfo->m_pStreamInfo->m_pStream->GetHXSource();

                if (pSource == pEventSource)
                {
                    listpos = RemoveAt(listpos);
                    if (!tmpList.AddTail(pEvent))
                    {
                        // We are out of memory. Just delete the event
                        // since we can't do anything else that would be
                        // valid. 
                        HX_ASSERT(pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued > 0);
                        pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued--;
                        delete pEvent;

                        res = HXR_OUTOFMEMORY;
                    }
                }
                else
                {
                    // /Adding this prevents endless looping when above if()
                    // fails otherwise the while() condition stays the same.
                    // This apparently only happens when multiple RVs are
                    // playing in parallel: Fixes PR 135134:
                    GetNext(listpos);
                }
                HX_RELEASE(pEventSource);
            }
            else
            {
                res = HXR_UNEXPECTED;
            }
        }

        // Iterate over all the events in tmpList
        listpos = tmpList.GetHeadPosition();
        while(listpos)
        {
            CHXEvent*  pEvent = (CHXEvent*)tmpList.GetNext(listpos);
            
            if (HXR_OK == res)
            {
                // Mark the event as an immediate event and insert it
                // back into the list. This has the effect of 
                // getting the events to dispatch ASAP.
                pEvent->SetImmediateEvent();
                InsertEvent(pEvent);
            }
            else
            {
                // Something unexpected happened. Delete the event
                HX_ASSERT(pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued > 0);
                pEvent->m_pRendererInfo->m_ulNumberOfPacketsQueued--;
                delete pEvent;
            }
        }
    }

    return res;
}

void CHXEventList::SetVelocity(INT32 lVel)
{
    m_lPlaybackVelocity = lVel;
}

HXBOOL 
CHXEventList::isEarlier(UINT32 currentPos, UINT32 startPos) const
{
    // either   a) 0xFA 0xFB 0xFC (0xFD)
    // or	    b) 0xFA .. 0xFF [roll over] (0x01)
    return HX_EARLIER_OR_SAME_USING_VELOCITY(currentPos, startPos, m_lPlaybackVelocity);
}

void CHXEventList::Dump(const char* label) const
{
#if defined(HELIX_FEATURE_LOGLEVEL_4)
    const char* pszStr = label;
    if (!pszStr)
    {
        pszStr = "CHXEventList";
    }
    HXLOGL4(HXLOG_TRIK, "%s Dump (Head-to-Tail)", pszStr);
    LISTPOSITION pos = GetHeadPosition();
    while (pos)
    {
        CHXEvent* pEvent = (CHXEvent*) GetNext(pos);
        if (pEvent)
        {
            IHXPacket* pPacket = pEvent->GetPacket();
            HXLOGL4(HXLOG_TRIK, "\t(pkt=(strm=%u,pts=%lu,rule=%u,flags=0x%02x),tsp=%lu,preseek=%lu,immed=%lu)",
                    (pPacket ? pPacket->GetStreamNumber()  : 0),
                    (pPacket ? pPacket->GetTime()          : 0),
                    (pPacket ? pPacket->GetASMRuleNumber() : 0),
                    (pPacket ? pPacket->GetASMFlags()      : 0),
                    pEvent->GetTimeStartPos(),
                    pEvent->IsPreSeekEvent(),
                    pEvent->IsImmediateEvent());
        }
    }
#endif /* #if defined(HELIX_FEATURE_LOGLEVEL_4) */
}

