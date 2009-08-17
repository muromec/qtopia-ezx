/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxpropw.h,v 1.4 2004/05/03 19:02:49 tmarshall Exp $ 
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

#ifndef _HXPROPW_H_
#define _HXPROPW_H_

#include "hxtypes.h"
#include "hxcom.h"

#include "simple_callback.h"

#include "regdb_misc.h"
#include "intref.h"
#include "hxmon.h"
#include "watchlst.h"
#include "regprop.h"
#include "servreg.h"

class ServerRegistry;
class Process;
class SimpleCallback;
class HXRegistry;

class HXPropWatch : public IHXPropWatch
{
private:
			virtual ~HXPropWatch();


    LONG32			m_lRefCount;
    IHXPropWatchResponse*	m_response;
    ServerRegistry*		m_registry;
    Process*			m_proc;
    int				m_procnum;

public:
				HXPropWatch(ServerRegistry* registry, 
					    Process* proc);
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
     *      IHXPropWatch::SetWatchByXYZ
     *  Purpose:
     *      Sets a watch-point on the Property whose name or id is
     *  specified. In case the mentioned Property gets modified or deleted
     *  a notification of that will be sent to the object which set the
     *  watch-point.
     */
    STDMETHOD_(UINT32, SetWatchByName)	(THIS_
					const char*	pcName);

    STDMETHOD_(UINT32, SetWatchById)	(THIS_
					const UINT32	id);

    /************************************************************************
     *  Method:
     *      IHXPropWatch::ClearWatchOnRoot
     *  Purpose:
     *      It clears the watch on the root of the registry.
     */
    STDMETHOD(ClearWatchOnRoot)		(THIS);

    /************************************************************************
     *  Method:
     *      IHXPropWatch::ClearWatchByXYZ
     *  Purpose:
     *      Clears a watch-point based on the Property's name or id.
     */
    STDMETHOD(ClearWatchByName)		(THIS_
					const char*	pcName);

    STDMETHOD(ClearWatchById)		(THIS_
					const UINT32	id);
#ifdef TEST_MON
    void traverse();
#endif

};

class ServerPropWatch : public PropWatch
{
public:
				ServerPropWatch() : proc(0), procnum(0) {}
				~ServerPropWatch() {}
    Process*			proc;
    int				procnum;
};

class PropWatchCallback : public SimpleCallback
{
public:
				PropWatchCallback();
    void 			func(Process* p);

    IHXPropWatchResponse*	m_pPlugin;
    IHXDeletedPropResponse*    m_pDeletedPropCB;
    Process*			m_proc;
    int				m_procnum;
    UINT32			m_hash;
    HXPropType			m_type;
    ServRegDB_Event		m_event;
    UINT32			m_parentHash;
    BOOL                        m_bParentBeingNotified;
    ServerRegistry*		m_registry;
    IHXBuffer*                 m_pKey;

private:
    				~PropWatchCallback();

};

#endif	// _HXPROPW_H_
