/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxelst.h,v 1.7 2005/06/22 17:16:12 albertofloyd Exp $
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

#ifndef _CHXELST_H_
#define _CHXELST_H_
#include "hxslist.h"
#include "hxplayvelocity.h"

typedef LISTPOSITION POSITION;

class CHXEvent;
class HXSource;

/*--------------------------------------------------------------------------
| CHXEventList
|-------------------------------------------------------------------------*/
class CHXEventList : public CHXSimpleList
{
public:
    CHXEventList() : m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL) {};
	inline CHXEvent* GetHead(void) const {return (CHXEvent *)CHXSimpleList::GetHead();};
		inline CHXEvent* GetTail(void) const {return (CHXEvent *)CHXSimpleList::GetTail();};
		inline CHXEvent*& GetHead(void) {return (CHXEvent *&)CHXSimpleList::GetHead();};
		inline CHXEvent*& GetTail(void) {return (CHXEvent *&)CHXSimpleList::GetTail();};
		
		inline LISTPOSITION	AddHead(CHXEvent *e) {return CHXSimpleList::AddHead(e);};
		inline CHXEvent	*RemoveHead(void) {return (CHXEvent *)CHXSimpleList::RemoveHead();};
		
		inline LISTPOSITION	AddTail(CHXEvent *e) {return CHXSimpleList::AddTail(e);};
		inline CHXEvent	*RemoveTail(void) {return (CHXEvent *)CHXSimpleList::RemoveTail();};
		
	    inline CHXEvent* GetNext(LISTPOSITION& p) const {return (CHXEvent *)CHXSimpleList::GetNext(p);};
	    inline CHXEvent*& GetNext(LISTPOSITION& p) {return (CHXEvent *&)CHXSimpleList::GetNext(p);};
  		inline CHXEvent* GetPrev(LISTPOSITION& p) const {return (CHXEvent *)CHXSimpleList::GetPrev(p);};
	    inline CHXEvent*& GetPrev(LISTPOSITION& p) {return (CHXEvent *&)CHXSimpleList::GetPrev(p);};

	    inline CHXEvent* GetAtNext(LISTPOSITION& p) const {return (CHXEvent *)CHXSimpleList::GetAtNext(p);};
	    //inline CHXEvent*& GetAtNext(LISTPOSITION& p) {return (CHXEvent *&)CHXSimpleList::GetAtNext(p);};
  		inline CHXEvent* GetAtPrev(LISTPOSITION& p) const {return (CHXEvent *)CHXSimpleList::GetAtPrev(p);};
	    inline CHXEvent*& GetAtPrev(LISTPOSITION& p) {return (CHXEvent *&)CHXSimpleList::GetAtPrev(p);};

		inline CHXEvent*& GetAt( LISTPOSITION p ) {return (CHXEvent*&)CHXSimpleList::GetAt(p);};
		inline CHXEvent* GetAt( LISTPOSITION p ) const {return (CHXEvent*)CHXSimpleList::GetAt(p);};
		inline void SetAt( LISTPOSITION p, CHXEvent *e ) {CHXSimpleList::SetAt(p, e);};

//		inline LISTPOSITION InsertBefore( LISTPOSITION p, CHXEvent* e) {return CHXSimpleList::InsertBefore(p, e);};

		inline LISTPOSITION InsertBefore( LISTPOSITION p, void *vp) {return CHXSimpleList::InsertBefore(p, vp);};
		
//		inline LISTPOSITION InsertAfter( LISTPOSITION p, CHXEvent* e) {return CHXSimpleList::InsertAfter(p, e);};

		inline LISTPOSITION InsertAfter( LISTPOSITION p, void *vp) {return CHXSimpleList::InsertAfter(p, vp);};

		inline LISTPOSITION Find( CHXEvent* s, LISTPOSITION StartAfter = NULL ) {return CHXSimpleList::Find(s, StartAfter);};

		inline LISTPOSITION Find( void *vp, LISTPOSITION StartAfter = NULL ) {return CHXSimpleList::Find(vp, StartAfter);};


		inline ULONG32 GetNumEvents() { return GetCount(); }

			HX_RESULT InsertEvent(CHXEvent*	pEvent);

                HX_RESULT MakeSourceEventsImmediate(HXSource* pSource);
                void      SetVelocity(INT32 lVel);
                virtual void Dump(const char* label = "Dump: ") const;
private:
    inline HXBOOL isEarlier(UINT32 currentPos, UINT32 startPos) const;
    INT32 m_lPlaybackVelocity;
};

#endif // _CHXELST_H_
