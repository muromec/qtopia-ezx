/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: memreap_proc.h,v 1.4 2004/05/03 19:02:48 tmarshall Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _MEMREAP_PROC_H_
#define _MEMREAP_PROC_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxengin.h"
#include "hxmon.h"
#include "simple_callback.h"

struct IUnknown;
struct IHXRegistry;
struct IHXPropWatch;
class  LoadInfo;

#define NUM_HIGH_LOAD_SKIPS	3

#define CONFIG_INTERVAL_NAME	  "config.MemoryReclaimInterval"
#define CONFIG_AGE_THRESHOLD_NAME "config.MemoryReclaimAgeThreshold"

#define CONFIG_INTERVAL_DEFAULT       86400
#define CONFIG_AGE_THRESHOLD_DEFAULT  34200

class MemReaperProcessInitCallback : public SimpleCallback
{
public:
    void			func(Process* proc);
    Process*			m_proc;			// Main's proc obj
private:
				~MemReaperProcessInitCallback() {};
};


class MemReaperCallback  : public IHXCallback,
			   public IHXPropWatchResponse
{
private:
    ~MemReaperCallback();

    LONG32			m_lRefCount;
    Process*			m_pProc;
    IHXScheduler*		m_pScheduler;
    IHXRegistry*		m_pRegistry;
    IHXPropWatch*		m_pPropWatch;

    HXTimeval			m_timeLastRun;
    UINT32			m_uiHighLoadSkips;
    INT32			m_iNextBucket;
    UINT32			m_ulNumReclaimed;
    INT32			m_iInterval;
    INT32			m_iAge;
    UINT32			m_uiIntervalPropID;
    UINT32			m_uiAgePropID;
    UINT32			m_uCallbackHandle;


public:
    MemReaperCallback(Process* proc);

    /*
     * Helper function for dividing up reclaim work
     */

    STDMETHOD(Func2)			(THIS);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXCallback methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCallback::Func
     *	Purpose:
     *	    This is the function that will be called when a callback is
     *	    to be executed.
     */
    STDMETHOD(Func)			(THIS);

    /*
     * IHXPropWatchResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::AddedProp
     *  Purpose:
     *      Gets called when a watched Property gets modified. It passes
     *  the id of the Property just modified, its datatype and the
     *  id of its immediate parent COMPOSITE property.
     */
    STDMETHOD(AddedProp)	(THIS_
				const UINT32		id,
				const HXPropType   	propType,
				const UINT32		ulParentID);

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::ModifiedProp
     *  Purpose:
     *      Gets called when a watched Property gets modified. It passes
     *  the id of the Property just modified, its datatype and the
     *  id of its immediate parent COMPOSITE property.
     */
    STDMETHOD(ModifiedProp)	(THIS_
				const UINT32		id,
				const HXPropType   	propType,
				const UINT32		ulParentID);

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::DeletedProp
     *  Purpose:
     *      Gets called when a watched Property gets deleted. As can be
     *  seen, it returns the id of the Property just deleted and
     *  its immediate parent COMPOSITE property.
     */
    STDMETHOD(DeletedProp)	(THIS_
				const UINT32		id,
				const UINT32		ulParentID);

};

#endif /* _MEMREAP_PROC_H_ */
