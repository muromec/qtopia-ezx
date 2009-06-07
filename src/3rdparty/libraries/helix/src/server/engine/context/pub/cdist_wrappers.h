/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cdist_wrappers.h,v 1.5 2004/11/24 01:47:11 dcollins Exp $ 
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

#ifndef _CDIST_WRAPPERS_H_
#define _CDIST_WRAPPERS_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcache.h"

#include "servlist.h"
#include "simple_callback.h"

_INTERFACE IHXRequest;
_INTERFACE IHXContentDistributionAdvise;

class Process;
class CDistMIIStatistics;

// cdist debug shme
// ie, same as DPRINTF, but use our cdist-specific flags.
// See note at head of cdist_wrappers.cpp for flag translations

#ifdef _DEBUG

extern INT32 g_ulCoreCDistDebugFlags;
#define CDPRINTF(mask,fmt) if ((mask) & g_ulCoreCDistDebugFlags) dprintf fmt

#else //_DEBUG

#define CDPRINTF(mask,fmt)

#endif //_DEBUG


typedef enum
{
    ON_LOCAL_RESULT,
    ON_CACHE_REQUEST,
    ON_CACHE_RESULT
    , ON_SITE_CACHE_RESULT
    , ON_PURGE_CACHE_URL    
} CDW_CALL_TYPE;

typedef enum
{
    ON_LOCAL_RESULT_DONE,
    ON_CACHE_REQUEST_DONE,
    ON_CACHE_RESULT_DONE
    , ON_SITE_CACHE_RESULT_DONE
    , ON_PURGE_CACHE_URL_DONE
} CDW_RESPONSE_TYPE;



class CDistWrapper : public IHXContentDistribution
{
public:

    CDistWrapper();

    /*
     * IHXContentDistribution methods
     */
    STDMETHOD(URLExists)		(THIS_
					 const char* pPath,
					 IHXContentDistributionResponse* pResp);

    STDMETHOD(RequestBlocks)	    (THIS);
    STDMETHOD(OnFetchedBlocks)	    (THIS);
    STDMETHOD(OnCachePurge)	    (THIS);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

private:
    ~CDistWrapper();

    LONG32			m_lRefCount;

};


class CDistAdviseWrapper : public IHXContentDistributionAdvise,
			   public IHXContentDistributionAdviseResponse
{
public:
    CDistAdviseWrapper(IUnknown* pContext,
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
     * IHXContentDistributionAdvise methods
     */
    STDMETHOD(GetPriority)			(THIS_
						 REF(INT32) lPriority);

    STDMETHOD(OnLocalResult)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest,
					 BOOL bFound);

    STDMETHOD(OnCacheRequest)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest);

    STDMETHOD(OnCacheResult)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest,
					 BOOL bFound);

    STDMETHOD(OnSiteCacheResult)	(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest,
					 BOOL bFound);

    STDMETHOD(OnPurgeCacheURL)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXBuffer* pURL,
					 IHXValues* pAdditional);
    /*
     * IHXContentDistributionAdviseResponse methods
     */

    STDMETHOD(OnLocalResultDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnCacheRequestDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnCacheResultDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnSiteCacheResultDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnPurgeCacheURLDone)		(THIS_
						 HX_RESULT status);
    /*
     * Other methods
     */
    inline BOOL					IsOK()
						 { return m_bOK; };
    inline BOOL					ForceRTSPImport()
						 { return zm_bForceRTSPImport; };

private:
    ~CDistAdviseWrapper();

    HX_RESULT			NextOnLocalResult();
    HX_RESULT			NextOnCacheRequest();
    HX_RESULT			NextOnCacheResult();
    HX_RESULT			NextOnSiteCacheResult();
    HX_RESULT			NextOnPurgeCacheURL();

    // class vars

    static BOOL			zm_bLicensedSubscriberValid;
    // These are all covered by zm_bLicensedSubscriberValid
    static BOOL			zm_bLicensedSubscriber;
    static BOOL			zm_bCDistConfigPresent;
    static CDistMIIStatistics*	zm_pMIIStats;
    static BOOL			zm_bForceRTSPImport;

    LONG32			m_lRefCount;

    // List of cdist-advise plugins.  The list is assumed read-only after 
    // creation, so iterators may be used on it without worrying about list
    // changes.
    HXList*			m_pPluginList;

    HXList_iterator*		m_pIter;
    IHXContentDistributionAdviseResponse* m_pResp;
    IHXRequest*		m_pRequest;
    Process*			m_pProc;
    BOOL			m_bFound;

    IHXBuffer* 		m_pURL;
    IHXValues* 		m_pAdditional;

    BOOL			m_bOK;
};


class CDWPluginListElem : public HXListElem
{
public:
    CDWPluginListElem(IHXContentDistributionAdvise* pCDAdvise);

    virtual 				~CDWPluginListElem();

    IHXContentDistributionAdvise*		GetObj()
							{ return(m_pCDAdvise); };

    // these should've been encapsuslated.
    BOOL					m_bLoadMultiple;
    int						m_iProcnum;
    INT32					m_lPriority;

private:
    // This is an instance of a multiload plugin.
    // non-multiloads store an instance in the proc container.

    IHXContentDistributionAdvise*		m_pCDAdvise;
};


class CDWStreamerCallback : public SimpleCallback
{
public:
    CDWStreamerCallback(CDW_RESPONSE_TYPE type,
			HX_RESULT result,
			IHXContentDistributionAdviseResponse* pResp);

    virtual ~CDWStreamerCallback();

    /*
     * SimpleCallback methods
     */
    void		func(Process* proc);

private:
    CDW_RESPONSE_TYPE				m_Type;
    HX_RESULT					m_iResult;
    IHXContentDistributionAdviseResponse*	m_pResp;
};


class CDWPluginCallback : public SimpleCallback,
			  public IHXContentDistributionAdviseResponse
{
public:

    CDWPluginCallback(CDW_CALL_TYPE type,
		      IHXContentDistributionAdviseResponse* pResp,
		      IHXRequest* pRequest,
		      BOOL found,
		      int procnum);

    CDWPluginCallback(CDW_CALL_TYPE 	type,
		      IHXContentDistributionAdviseResponse* pResp,
		      IHXBuffer*	pURL,
		      IHXValues*	pAdditional,
		      int 		procnum);

    virtual ~CDWPluginCallback();

    /*
     * SimpleCallback methods
     */
    void		func(Process* proc);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXContentDistributionAdviseResponse methods
     */

    STDMETHOD(OnLocalResultDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnCacheRequestDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnCacheResultDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnSiteCacheResultDone)		(THIS_
						 HX_RESULT status);

    STDMETHOD(OnPurgeCacheURLDone)		(THIS_
						 HX_RESULT status);

private:
    LONG32			m_lRefCount;

    CDW_CALL_TYPE				m_Type;
    IHXContentDistributionAdvise*		m_pCDAdvise;
    IHXContentDistributionAdviseResponse*	m_pResp;
    IHXRequest*				m_pRequest;
    Process*					m_pProc;
    BOOL					m_bFound;

    IHXBuffer* 		m_pURL;
    IHXValues* 		m_pAdditional;

    int						m_iProcnum;
};

#endif  /* _CDIST_WRAPPERS_H_ */
