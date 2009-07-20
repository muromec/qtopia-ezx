/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcache.h,v 1.4 2005/03/14 19:27:09 bobclark Exp $
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

#ifndef _HXCACHE_H_
#define _HXCACHE_H_

/*
 * Forward declarations of some interfaces defined herein.
 */
typedef _INTERFACE	IHXCache			IHXCache;
typedef _INTERFACE	IHXCacheFile			IHXCacheFile;
typedef _INTERFACE	IHXCacheFileResponse		IHXCacheFileResponse;
typedef _INTERFACE	IHXMIIFetch			IHXMIIFetch;
typedef _INTERFACE	IHXFIFOCache			IHXFIFOCache;
// $Private:
// cdist interfaces private for development, expect to make public when finalized.
typedef _INTERFACE	IHXContentDistribution		IHXContentDistribution;
typedef _INTERFACE	IHXContentDistributionResponse	IHXContentDistributionResponse;
typedef _INTERFACE	IHXContentDistributionAdvise	IHXContentDistributionAdvise;
typedef _INTERFACE	IHXContentDistributionAdviseResponse IHXContentDistributionAdviseResponse;
typedef _INTERFACE	IHXMIIPullEntire		IHXMIIPullEntire;
typedef _INTERFACE	IHXMIIPullEntireResponse	IHXMIIPullEntireResponse;
typedef _INTERFACE	IHXCacheFileSetVersion		IHXCacheFileSetVersion;
typedef _INTERFACE	IHXMIIReadStatCollection	IHXMIIReadStatCollection;
// $EndPrivate.
typedef _INTERFACE	IHXValues			IHXValues;
typedef _INTERFACE	IHXBuffer			IHXBuffer;
typedef _INTERFACE	IHXRequest			IHXRequest;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCache
 * 
 *  Purpose:
 * 
 *  IID_IHXCache:
 * 
 *	{00002E00-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXCache, 0x00002E00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCache

DECLARE_INTERFACE_(IHXCache, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXCache methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCache::Init
     *	Purpose:
     */
    STDMETHOD(Init)	(THIS_ IHXValues* pOptions) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCache::CreateCacheFile
     *	Purpose:
     */
    STDMETHOD(CreateCacheFile)	(THIS_
				IHXCacheFile**    /*OUT*/	ppObject) PURE;

    /************************************************************************
     *  Method:
     *	    IHXCache:GetCacheInfo
     */
    STDMETHOD(GetCacheInfo) (THIS_
			    REF(const char*) /*OUT*/ pShortName) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCacheFile
 * 
 *  Purpose:
 * 
 *	Object that exports persistent cache storage API
 * 
 *  IID_IHXCacheFile:
 * 
 *	{00002E02-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXCacheFile, 0x00002E02, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCacheFile

DECLARE_INTERFACE_(IHXCacheFile, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXCacheFile methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::Init
     *	Purpose:
     *	    Associates a cache file object with the file response object
     *	    it should notify of operation completeness.
     */
    STDMETHOD(Init)	(THIS_
			IHXCacheFileResponse*   /*IN*/  pFileResponse) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::Open
     *	Purpose:
     *	    Opens the file for reading and writing.
     */
    STDMETHOD(Open)	(THIS_
			 const char* 		/*IN*/	pServerName,
			 const char*		/*IN*/	pFileName) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::Close
     *	Purpose:
     *	    Closes the file resource and releases all resources associated
     *	    with the object.
     */
    STDMETHOD(Close)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::AddBlock
     *	Purpose:
     *	    Adds a block of data to the cache
     */
    STDMETHOD(AddBlock)	(THIS_
			 UINT32			/*IN*/	ulBlockOffset,
			 IHXBuffer*		/*IN*/	pBlock) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::VerifyBlock
     *	Purpose:
     *	    Verify that a block of data is in the cache.
     */
    STDMETHOD(VerifyBlock)	(THIS_
				 UINT32		/*IN*/	ulBlockOffset,
				 UINT32		/*IN*/	ulBlockLength) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::ReadBlock
     *	Purpose:
     *	    Read a block out of the cache.
     */
    STDMETHOD(ReadBlock)	(THIS_
				 UINT32		/*IN*/	ulBlockOffset,
				 UINT32		/*IN*/	ulBlockLength) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::Stat
     *	Purpose:
     *	    Obtain size and timestamp information about a cached file.
     */
    STDMETHOD(Stat)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::SetStat
     *	Purpose:
     *	    Store size and timestamp information for a cached file.
     */
    STDMETHOD(SetStat)	(THIS_
			 UINT32		/*IN*/	ulSize,
			 UINT32		/*IN*/	ulCreationTime,
			 UINT32		/*IN*/	ulAccessTime,
			 UINT32		/*IN*/	ulModificationTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFile::Expire
     *	Purpose:
     *	    Marks a cached file as expired; all blocks stored for the
     *	    file should be discarded.
     */
    STDMETHOD(Expire)	(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCacheFileResponse
 * 
 *  Purpose:
 * 
 *	Response interface for IHXCacheFile operations
 * 
 *  IID_IHXCacheFileResponse:
 * 
 *	{00002E03-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXCacheFileResponse, 0x00002E03, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCacheFileResponse

DECLARE_INTERFACE_(IHXCacheFileResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXCacheFileResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::InitDone
     *	Purpose:
     *	    Notification that IHXCacheFile::Init call has completed.
     */
    STDMETHOD(InitDone)		(THIS_
				 HX_RESULT	/*IN*/	status) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::OpenDone
     *	Purpose:
     *	    Notification that IHXCacheFile::Open operation has completed.
     */
    STDMETHOD(OpenDone)		(THIS_
				HX_RESULT	/*IN*/	status) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::CloseDone
     *	Purpose:
     *	    Notification that IHXCacheFile::Close operation has completed.
     */
    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT	/*IN*/	status) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::AddBlockDone
     *	Purpose:
     *	    Notification that IHXCacheFile::AddBlock operation has completed.
     */
    STDMETHOD(AddBlockDone)	(THIS_
				HX_RESULT	/*IN*/	status) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::VerifyBlockDone
     *	Purpose:
     *	    Notification that IHXCacheFile::VerifyBlock operation has
     *	    completed.
     */
    STDMETHOD(VerifyBlockDone)	(THIS_
				HXBOOL		/*IN*/	bExist) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::ReadBlockDone
     *	Purpose:
     *	    Notification that IHXCacheFile::ReadBlock operation has
     *	    completed.
     */
    STDMETHOD(ReadBlockDone)	(THIS_
				HX_RESULT	/*IN*/	status,
				IHXBuffer*	/*IN*/	pBuffer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::StatDone
     *	Purpose:
     *	    Return size and timestamp information about a cached file.
     */
    STDMETHOD(StatDone)		(THIS_
				 HX_RESULT	/*IN*/	status,
				 UINT32		/*IN*/ ulSize,
				 UINT32		/*IN*/ ulCreationTime,
				 UINT32		/*IN*/ ulAccessTime,
				 UINT32		/*IN*/ ulModificationTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::SetStatDone
     *	Purpose:
     *	    Store size and timestamp information for a cached file.
     *	    Notification that IHXCacheFile::SetStat operation has completed.
     */
    STDMETHOD(SetStatDone)	(THIS_
				 HX_RESULT	/*IN*/ status) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheFileResponse::ExpireDone
     *	Purpose:
     *	    Notification that IHXCacheFile::Expire operation has completed.
     */
    STDMETHOD(ExpireDone)	(THIS_	
				 HX_RESULT	/*IN*/ status) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMIIFetch
 * 
 *  Purpose:
 * 
 *	For IHXCacheFile implementations to be able to request data
 *	independantly of the mii.
 *
 *	You do not have to use this interface.  In fact, most people will not.
 *	This is only to be used if for some reason your IHXCacheFileObject
 *	Needs to request more data than it is already getting from mii.
 * 
 *  IID_IHXMIIFetch:
 * 
 *	{00002E04-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMIIFetch, 0x00002E04, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXMIIFetch

DECLARE_INTERFACE_(IHXMIIFetch, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXMIIFetch Methods.
     */
    STDMETHOD(RequestRemoteBlock)   (THIS_
				    UINT32 ulBlockOffset,
				    UINT32 ulBlockLen) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFIFOCache
 * 
 *  Purpose:
 *	To cache objects in a temporary storage for later retrieval
 *	Can be created off of IHXCommonClassFactory.
 *	Currently supported only on the client side.
 * 
 *  IID_IHXFIFOCache:
 * 
 *	{00002E05-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXFIFOCache, 0x00002E05, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXFIFOCache IID_IHXFIFOCache

#undef  INTERFACE
#define INTERFACE   IHXFIFOCache

DECLARE_INTERFACE_(IHXFIFOCache, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXFIFOCache methods
     */

    /************************************************************************
     *	Method:
     *		IHXFIFOCache::Cache
     *	Purpose:
     *	    To cache objects in a temporary storage for later retrieval
     *
     *	    Currently supported objects:
     *		IHXBuffer
     *		IHXValues
     *		IHXPacket
// $Private:
     *		IHXTimeStampedBuffer
// $EndPrivate.
     *
     */
    STDMETHOD(Cache)	(THIS_
			 IUnknown*	    pObject) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFIFOCache::Retrieve
     *	Purpose:
     *
     *
     */
    STDMETHOD(Retrieve)	    (THIS_
			    REF(IUnknown*)  pObject) PURE;

    /************************************************************************
     *	Method:
     *	    IHXFIFOCache::Flush
     *	Purpose:
     *
     *
     */
    STDMETHOD(Flush)	    (THIS) PURE;
};
// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXContentDistribution
 * 
 *  Purpose:
 *	Content distribution services, usually coordinated across all servers
 *	at a site.
 * 
 *  IID_IHXContentDistribution:
 * 
 *	{00002E06-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXContentDistribution, 0x00002E06, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXContentDistribution IID_IHXContentDistribution

#undef  INTERFACE
#define INTERFACE   IHXContentDistribution

DECLARE_INTERFACE_(IHXContentDistribution, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXContentDistribution methods
     */

    /************************************************************************
     *	Method:
     *		IHXContentDistribution::
     *	Purpose:
     *
     *
     */
    STDMETHOD(URLExists)		(THIS_
					 const char* pPath,
					 IHXContentDistributionResponse* pResp) PURE;

    STDMETHOD(RequestBlocks)	    (THIS) PURE;
    STDMETHOD(OnFetchedBlocks)	    (THIS) PURE;
    STDMETHOD(OnCachePurge)	    (THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXContentDistributionResponse
 * 
 *  Purpose:
 *	Content distribution service replies.
 * 
 *  IID_IHXContentDistributionResponse:
 * 
 *	{00002E07-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXContentDistributionResponse, 0x00002E07, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXContentDistributionResponse IID_IHXContentDistributionResponse

#undef  INTERFACE
#define INTERFACE   IHXContentDistributionResponse

DECLARE_INTERFACE_(IHXContentDistributionResponse, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXContentDistributionResponse methods
     */

    /************************************************************************
     *	Method:
     *		IHXContentDistribution::
     *	Purpose:
     *
     *
     */
    STDMETHOD(URLExistsDone)		(THIS_
					 HXBOOL bExists) PURE;

    STDMETHOD(RequestBlocksDone)	(THIS) PURE;
    STDMETHOD(OnFetchedBlocksDone)	(THIS) PURE;
    STDMETHOD(OnCachePurgeDone)	    	(THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXContentDistributionAdvise
 * 
 *  Purpose:
 *	Allowance-like advise sink, for content-distribution cache
 *	requests.
 * 
 *  IID_IHXContentDistributionAdvise:
 * 
 *	{00002E08-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXContentDistributionAdvise, 0x00002E08, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXContentDistributionAdvise IID_IHXContentDistributionAdvise

#undef  INTERFACE
#define INTERFACE   IHXContentDistributionAdvise

DECLARE_INTERFACE_(IHXContentDistributionAdvise, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXContentDistributionAdvise methods
     */

    /************************************************************************
     *	Method:
     *		IHXContentDistributionAdvise::
     *	Purpose:
     *
     *
     */
    STDMETHOD(GetPriority)		(THIS_
					 REF(INT32) lPriority) PURE;

    STDMETHOD(OnLocalResult)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest,
					 HXBOOL bFound) PURE;

    STDMETHOD(OnCacheRequest)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest) PURE;

    STDMETHOD(OnCacheResult)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest,
					 HXBOOL bFound) PURE;

    STDMETHOD(OnSiteCacheResult)	(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXRequest* pRequest,
					 HXBOOL bFound) PURE;

    STDMETHOD(OnPurgeCacheURL)		(THIS_
					 IHXContentDistributionAdviseResponse* pResp,
					 IHXBuffer* pURL,
					 IHXValues* pAdditional) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXContentDistributionAdviseResponse
 * 
 *  Purpose:
 *	Responses for IHXContentDistributionAdvise
 * 
 *  IID_IHXContentDistributionAdviseResponse:
 * 
 *	{00002E09-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXContentDistributionAdviseResponse, 0x00002E09, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXContentDistributionAdviseResponse IID_IHXContentDistributionAdviseResponse

#undef  INTERFACE
#define INTERFACE   IHXContentDistributionAdviseResponse

DECLARE_INTERFACE_(IHXContentDistributionAdviseResponse, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXContentDistributionAdviseResponse methods
     */

    /************************************************************************
     *	Method:
     *		IHXContentDistributionAdviseResponse::
     *	Purpose:
     *
     *
     */
    STDMETHOD(OnLocalResultDone)		(THIS_
						 HX_RESULT status) PURE;

    STDMETHOD(OnCacheRequestDone)		(THIS_
						 HX_RESULT status) PURE;

    STDMETHOD(OnCacheResultDone)		(THIS_
						 HX_RESULT status) PURE;

    STDMETHOD(OnSiteCacheResultDone)		(THIS_
						 HX_RESULT status) PURE;

    STDMETHOD(OnPurgeCacheURLDone)		(THIS_
						 HX_RESULT status) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMIIPullEntire
 * 
 *  Purpose:
 *	Interface to instruct MII to pull down entire file
 * 
 *  IID_IHXMIIPullEntire
 * 
 *	{00002E0A-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMIIPullEntire, 0x00002E0A, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXMIIPullEntire IID_IHXMIIPullEntire

#undef  INTERFACE
#define INTERFACE   IHXMIIPullEntire

DECLARE_INTERFACE_(IHXMIIPullEntire, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXMIIPullEntire methods
     */

    /************************************************************************
     *	Method:
     *		IHXMIIPullEntire::
     *	Purpose:
     *
     *
     */
    STDMETHOD(PullEntireFile)		(THIS_
					 IHXMIIPullEntireResponse* pResp) PURE;

    STDMETHOD(PullEntireFileCancel)	(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMIIPullEntireResponse
 * 
 *  Purpose:
 *	Callback when file is completely pulled down.
 * 
 *  IID_IHXMIIPullEntireResponse
 * 
 *	{00002E0B-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMIIPullEntireResponse, 0x00002E0B, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXMIIPullEntireResponse IID_IHXMIIPullEntireResponse

#undef  INTERFACE
#define INTERFACE   IHXMIIPullEntireResponse

DECLARE_INTERFACE_(IHXMIIPullEntireResponse, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXMIIPullEntireResponse methods
     */

    /************************************************************************
     *	Method:
     *		IHXMIIPullEntireResponse::
     *	Purpose:
     *
     *
     */
    STDMETHOD(PullEntireFileDone)		(THIS_
						 HX_RESULT status) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCacheFileSetVersion
 * 
 *  Purpose:
 *	Interface to request a specific version of a cache file
 * 
 *  IID_IHXCacheFileSetVersion
 * 
 *	{00002E0C-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXCacheFileSetVersion, 0x00002E0C, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXCacheFileSetVersion IID_IHXCacheFileSetVersion

#undef  INTERFACE
#define INTERFACE   IHXCacheFileSetVersion

DECLARE_INTERFACE_(IHXCacheFileSetVersion, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXCacheFileSetVersion methods
     */

    /************************************************************************
     *	Method:
     *		CacheFileSetVersion::
     *	Purpose:
     *
     *
     */
    STDMETHOD(CacheFileSetVersion)		(THIS_
						 UINT32 ulVersion) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMIIReadStatCollection
 * 
 *  Purpose:
 *	Interface to request a file object start collecting read stats for 
 *	CDist/MII
 * 
 *  IHXMIIReadStatCollection
 * 
 *	{00002E0D-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMIIReadStatCollection, 0x00002E0D, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXMIIReadStatCollection IID_IHXMIIReadStatCollection

#undef  INTERFACE
#define INTERFACE   IHXMIIReadStatCollection

DECLARE_INTERFACE_(IHXMIIReadStatCollection, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXMIIReadStatCollection methods
     */

    /************************************************************************
     *	Method:
     *		SetMIIReadStatsEnabled()
     *	Purpose:
     *
     */
    STDMETHOD(SetMIIReadStatsEnabled)		(THIS_
						 HXBOOL bEnabled,
						 HXBOOL* bOldValue) PURE;
    STDMETHOD(GetMIIReadStatsEnabled)		(THIS_
						 REF(HXBOOL) bEnabled) PURE;
};

// $EndPrivate.


#endif  /* _HXCACHE_H_ */
