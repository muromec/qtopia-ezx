/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpropwclnt.h,v 1.8 2007/07/06 21:57:57 jfinnecy Exp $
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

#ifndef _HXPROPW_H_
#define _HXPROPW_H_

class HXClientRegistry;
class CHXSimpleList;

struct IHXScheduler;
struct IHXInterruptSafe;
struct IHXInterruptState;

#include "hxengin.h"
#include "hxmon.h"
#include "hxthread.h"

typedef enum 
{
    ADDEDPROP = 0,
    MODIFIEDPROP,
    DELETEDPROP
} ResponseType;

struct PropResponseValues
{
    PropResponseValues(ResponseType		uResponseType,
		       const UINT32		id,
		       const HXPropType   	propType,
		       const UINT32		ulParentID)
    {
	m_uResponseType = uResponseType;
	m_ulId		= id;
	m_propType	= propType;
	m_ulParentID	= ulParentID;
    };

    ResponseType	m_uResponseType;
    UINT32		m_ulId;
    HXPropType   	m_propType;
    UINT32		m_ulParentID;
};

class HXClientPropWatch : public IHXPropWatch
{
private:
			virtual ~HXClientPropWatch();


    LONG32			m_lRefCount;
    IHXPropWatchResponse*	m_pResponse;
    CommonRegistry*		m_pRegistry;
    IHXInterruptSafe*		m_pInterruptSafeResponse;
    IHXInterruptState*		m_pInterruptState;
    IHXScheduler*		m_pScheduler;
    IUnknown*			m_pContext;  
public:
				HXClientPropWatch(CommonRegistry* pRegistry,
						  IUnknown* pContext);
    
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXPropWatch methods
     */
    /************************************************************************
     *  Method:
     *      IHXPropWatch::Init
     *  Purpose:
     *      Initialize with the response object and the registry so that
     *  Watch notifications can be sent back to the respective plugins.
     */
    STDMETHOD(Init)		(THIS_
				IHXPropWatchResponse*	pResponse);

    /************************************************************************
     *  Method:
     *      IHXPropWatch::SetWatchOnRoot
     *  Purpose:
     *      The SetWatch method puts a watch at the highest level of
     *  the registry hierarchy. It notifies ONLY IF properties at THIS LEVEL
     *  get added/modified/deleted.
     */
    STDMETHOD_(UINT32, SetWatchOnRoot)	(THIS);

    /************************************************************************
     *  Method:
     *      IHXPropWatch::SetWatchByXXX
     *  Purpose:
     *      Sets a watch-point on the Property whose name or id is
     *  specified. In case the mentioned Property gets modified or deleted
     *  a notification of that will be sent to the object which set the
     *  watch-point.
     */
    STDMETHOD_(UINT32, SetWatchByName)	(THIS_
					const char*	pName);
    STDMETHOD_(UINT32, SetWatchById)	(THIS_
					const UINT32	id);


    /************************************************************************
     *  Method:
     *      IHXPropWatch::ClearWatchOnRoot
     *  Purpose:
     *      Clears the watch on the root of registry.
     */
    STDMETHOD(ClearWatchOnRoot)		(THIS);
 
    /************************************************************************
     *  Method:
     *      IHXPropWatch::ClearWatch
     *  Purpose:
     *      Clears a watch-point based on the Property's name or id.
     */
    STDMETHOD(ClearWatchByName)		(THIS_
					const char*	pName);
    STDMETHOD(ClearWatchById)		(THIS_
					const UINT32	id);

    class PropWatchCallback : public IHXCallback
    {
	public:
	    HXClientPropWatch*	    m_pPropWatch;
	    CallbackHandle	    m_PendingHandle;
	    HXBOOL		    m_bIsCallbackPending;

				PropWatchCallback(HXClientPropWatch* pClientPropWatch);
	    /*
	     *	IUnknown methods
	     */
	    STDMETHOD(QueryInterface)	(THIS_
					    REFIID riid,
					    void** ppvObj);

	    STDMETHOD_(ULONG32,AddRef)	(THIS);

	    STDMETHOD_(ULONG32,Release)	(THIS);

	    /*
	     *	IHXCallback methods
	     */
	    STDMETHOD(Func)		(THIS);

	protected:
			    ~PropWatchCallback();
	    LONG32	    m_lRefCount;
    };

    class PropWatchResponse : public IHXPropWatchResponse
    {
	public:
	    HXClientPropWatch*	    m_pPropWatch;

	    PropWatchResponse(HXClientPropWatch* pClientPropWatch);

	    void ProcessPendingResponses();

	    /*
	     *	IUnknown methods
	     */
	    STDMETHOD(QueryInterface)	(THIS_
					    REFIID riid,
					    void** ppvObj);

	    STDMETHOD_(ULONG32,AddRef)	(THIS);

	    STDMETHOD_(ULONG32,Release)	(THIS);

	    /*
	     *	IHXPropWatchResponse methods
	     */
	    STDMETHOD(AddedProp)	(THIS_
					const UINT32		id,
					const HXPropType   	propType,
					const UINT32		ulParentID);

	    STDMETHOD(ModifiedProp)	(THIS_
					const UINT32		id,
					const HXPropType   	propType,
					const UINT32		ulParentID);

	    STDMETHOD(DeletedProp)	(THIS_
					const UINT32		id,
					const UINT32		ulParentID);

	protected:
	    void ScheduleCallback(ResponseType		uResponseType,
			     const UINT32		id,
			     const HXPropType   	propType,
			     const UINT32		ulParentID);

			    ~PropWatchResponse();
	    LONG32	    m_lRefCount;
	    CHXSimpleList*  m_pPendingResponseList;
	    
	    IHXMutex*       m_pMutex;

    };

    friend class PropWatchResponse;
    friend class PropWatchCallback;

    PropWatchResponse*	    m_pInternalResponse;
    PropWatchCallback*	    m_pCallback;
};

#endif // _HXPROPW_H_
