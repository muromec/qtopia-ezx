/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: configwatcher.h,v 1.4 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef CONFIGWATCHER_H
#define CONFIGWATCHER_H

#include "hxcom.h"
#include "hxresult.h"
#include "hxmon.h"
#include "hxmap.h"
#include "ihxpckts.h"

class ConfigWatcher;

struct ConfigWatcherEntry : public IHXActivePropUser
{
    IHXActivePropUserResponse* m_pUserResponse;
    IHXPropWatchResponse* m_pResponse;
    HX_RESULT m_result;
    HXPropType m_type;
    char* m_pName;
    char* m_pParentName;
    INT32 m_intVal;
    IHXBuffer* m_pBufferVal;
    IHXBuffer** m_ppInfo;
    UINT32 m_ulNumInfo;
    UINT32 m_regID;
    UINT32 m_parentID;
    BOOL m_bDeleteRequest;
    
    STDMETHOD(QueryInterface)   (THIS_
	    REFIID riid,
	    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXActivePropUser 
    STDMETHOD(SetActiveInt) (THIS_
	    const char* pName,
	    UINT32 ul,
	    IHXActivePropUserResponse* pResponse);
    STDMETHOD(SetActiveStr) (THIS_
	    const char* pName,
	    IHXBuffer* pBuffer,
	    IHXActivePropUserResponse* pResponse);
    STDMETHOD(SetActiveBuf)     (THIS_
	    const char* pName,
	    IHXBuffer* pBuffer,
	    IHXActivePropUserResponse* pResponse);
    STDMETHOD(DeleteActiveProp) (THIS_
	    const char* pName,
	    IHXActivePropUserResponse* pResponse);

    ConfigWatcherEntry(IHXPropWatchResponse* pResponse, 
	    ULONG32 regID, 
	    ULONG32 parentID);
    ~ConfigWatcherEntry();
    STDMETHODIMP SendResponse();
    LONG32 m_lRefCount;
};


class ConfigWatcher : public IHXPropWatchResponse
{
public:
    ConfigWatcher(IHXRegistry* pRegistry);
    ~ConfigWatcher();

    STDMETHOD(QueryInterface)   (THIS_
	    REFIID riid,
	    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    
    STDMETHODIMP Init(IHXPropWatchResponse* pResp);
    STDMETHODIMP Manage(UINT32 regID);
    STDMETHODIMP Unmanage(UINT32 regID);
    BOOL IsPending(UINT32 regID);
    BOOL IsManaged(UINT32 regID);
    STDMETHODIMP SetResponseValue(
	    UINT32 regID, 
	    HX_RESULT result,
	    IHXBuffer* pInfo[],
	    UINT32 ulNumInfo);
    STDMETHODIMP SetResponseValueInt(
	    UINT32 regID, 
	    HX_RESULT result,
	    INT32 val, 
	    IHXBuffer* pInfo[],
	    UINT32 ulNumInfo);
    STDMETHODIMP SetResponseValueBuffer(
	    UINT32 regID, 
	    HX_RESULT result,
	    IHXBuffer* pBuf,
	    IHXBuffer* ppInfo[],
	    UINT32 ulNumInfo);
    STDMETHODIMP SetResponseValueString(
	    UINT32 regID, 
	    HX_RESULT result,
	    IHXBuffer* pBuf,
	    IHXBuffer* ppInfo[],
	    UINT32 ulNumInfo)
	{ return 
	    SetResponseValueBuffer(regID, result, pBuf, ppInfo, ulNumInfo); }
    STDMETHODIMP SendPendingResponses();
    STDMETHODIMP GetPendingValueInt(UINT32 regID, REF(INT32) val);
    STDMETHODIMP GetPendingValueBuffer(UINT32 regID, REF(IHXBuffer*) pBuf);
    STDMETHODIMP GetPendingValueString(UINT32 regID, REF(IHXBuffer*) pBuf)
	{ return GetPendingValueBuffer(regID, pBuf); }

    // IHXPropWatchResponse methods
    STDMETHODIMP AddedProp(
            const UINT32            ulId,
            const HXPropType       propType,
            const UINT32            ulParentID);
    STDMETHODIMP ModifiedProp(
            const UINT32            ulId,
            const HXPropType       propType,
            const UINT32            ulParentID);
    STDMETHODIMP DeletedProp(
            const UINT32            ulId,
            const UINT32            ulParentID);

    void Cleanup();
private:
    IHXPropWatchResponse* m_pResponse;
    IHXPropWatch* m_pPropWatch;
    IHXRegistry* m_pRegistry;
    IHXActiveRegistry* m_pActiveRegistry;
    CHXMapLongToObj m_regIDMap;
    LONG32 m_lRefCount;
};

#endif //CONFIGWATCHER_H
