/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sapclass.cpp,v 1.6 2007/08/18 00:21:16 dcollins Exp $ 
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

/*****************************************************************************
** - This class represents deals with SAP.
** - multicasting of SAP is scheduled in each PUMP (each pump needs to send it's 
**   own SAP with it's own SDP.)
** - interval of SAP is calculated in this class
** - processing of received SAP is done in this class.
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#ifdef _UNIX
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif //_UNIX

#include "hxtypes.h"
#include "sockio.h"
#include "hxcom.h"
#include "hxerror.h"
#include "chxpckts.h"	// CHXHeader, CHXValues, CHXBuffer
#include "hxtick.h"	// GET_TICKCOUNT();
#include "hxresult.h"	// HX_RESULT
#include "hxengin.h"
#include "hxfiles.h"
#include "hxcomm.h"
#include "hxmap.h"
#include "hxsdesc.h"
#include "zlib.h"	// for compression 
#include "hxstrutl.h"	// strcasecmp

#include "carray.h"	
#include "hxsrc.h"
#include "hxplugn.h"	// IHXPluginEnumerator
#include "hxmon.h"	// IHXRegistry, IHXPropWatchResponse
#include "addrpool.h"	// IHXMulticastAddressPool
#include "sappkt.h"
#include "hxslist.h"
#include "sapmgr.h"

#include "sdptypes.h"	// INTEROP_SDP

#include "sapclass.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


//#define XXXGo_DEBUG

CSapManager::CSapManager(void)
    : m_lRefCount(0)
    , m_pSAPSock(NULL)
    , m_pSAPResp(NULL)
    , m_ulNumAds(0)
    , m_hResetCBHandle(0)
    , m_pMulticast(NULL)
    , m_pStreamDesc(NULL)
    , m_pAddrPool(NULL)
    , m_pScheduler(NULL)
    , m_pContext(NULL)
    , m_pClassFactory(NULL)
    , m_ulOrigSrc(0)
    , m_bDirectoryStarted(FALSE)
    , m_bListenEnabled(TRUE)
    , m_bSendEnabled(TRUE)
    , m_bNoCompression(FALSE)	// compress by default if SDP is too big
    , m_ulCompressionSize(1024)
    , m_sdpFileType(NEW_SDP)
{
    m_SourceIdMap.InitHashTable(100);
}

CSapManager::~CSapManager()
{    
    if (m_pSAPSock && m_bListenEnabled)
    {
	m_pSAPSock->LeaveMulticastGroup(SDP_MULTICAST_ADDR, HXR_INADDR_ANY);
    }

    if (0 != m_hResetCBHandle)
    {
     	m_pScheduler->Remove(m_hResetCBHandle);
     	m_hResetCBHandle = 0;
    }

    // clean up the map
    CHXMapLongToObj::Iterator i;
    for (i  = m_SourceIdMap.Begin();
	 i != m_SourceIdMap.End();
	 ++i)
    {
	CHXMapLongToObj::Iterator j;
	CHXMapLongToObj* pMap = (CHXMapLongToObj*)(*i);
	for (j  = pMap->Begin();
	     j != pMap->End();
	     ++j)
	{
	    m_ulNumAds--;

	    CHXPtrArray* prgAddrs = (CHXPtrArray*)(*j);

	    ReleaseAddrs(prgAddrs);
	    
	    HX_DELETE(prgAddrs);
	    // no need to remove a key.  delete pMap will take care of it
	}

	delete pMap;
    }

    HX_ASSERT(0 == m_ulNumAds);

    HX_RELEASE(m_pStreamDesc);
    HX_RELEASE(m_pMulticast);
    HX_RELEASE(m_pSAPSock);
    HX_RELEASE(m_pSAPResp);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pAddrPool);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pClassFactory);
}

STDMETHODIMP CSapManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXCallback))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}
STDMETHODIMP_(ULONG32) CSapManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}
STDMETHODIMP_(ULONG32) CSapManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


HX_RESULT
CSapManager::Init(IUnknown* pContext)
{
    HX_ASSERT(!m_pContext);
    m_pContext = pContext;
    m_pContext->AddRef();
    
    HX_RESULT theErr = HXR_OK;
    INT32 lInt = 0;
    IHXBuffer* pBuf = NULL;
    IHXRegistry* pRegistry = NULL;
    
    theErr = m_pContext->QueryInterface(IID_IHXMulticastAddressPool,
					(void**)&m_pAddrPool);
    if (FAILED(theErr))
    {
	goto bail;
    }
    theErr = m_pContext->QueryInterface(IID_IHXScheduler,
					(void**)&m_pScheduler);
    if (FAILED(theErr))
    {
	goto bail;
    }
    theErr = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
					(void**)&m_pClassFactory);
    if (FAILED(theErr))
    {
	goto bail;
    }
    theErr = m_pContext->QueryInterface(IID_IHXRegistry,
					(void**)&pRegistry);
    if (FAILED(theErr))
    {
	HX_ASSERT(!"WHAT!!!");
	goto bail;
    }

    theErr = pRegistry->GetIntByName("config.SAP.ListenAnnouncement", lInt);
    if (SUCCEEDED(theErr))
    {
	m_bListenEnabled = (BOOL)lInt;
    }
#ifdef _DEBUG
    else
    {
	// default to TRUE
	HX_ASSERT(m_bListenEnabled);
    }	
#endif	

    theErr = pRegistry->GetIntByName("config.SAP.SendAnnouncementEnabled", lInt);
    if (SUCCEEDED(theErr))
    {
	m_bSendEnabled = (BOOL)lInt;
    }
#ifdef _DEBUG
    else
    {
	// default to TRUE
	HX_ASSERT(m_bSendEnabled);
    }	
#endif	

    theErr = pRegistry->GetStrByName("config.SAP.HostAddress", pBuf);
    if (SUCCEEDED(theErr))
    {
	m_ulOrigSrc = ntohl(inet_addr((const char*)pBuf->GetBuffer()));	
    }
#ifdef _DEBUG    
    else
    {
	HX_ASSERT(!m_ulOrigSrc);
    }
#endif    
    HX_RELEASE(pBuf);    

    theErr = pRegistry->GetStrByName("config.SAP.SdpFileType", pBuf);
    if (SUCCEEDED(theErr))
    {
	// default is NEW_SDP
	const char* pc = (const char*)pBuf->GetBuffer();
	if (strcasecmp(pc, "old") == 0)
	{
	    m_sdpFileType = OLD_SDP;
	}
	else if (strcasecmp(pc, "both") == 0)
	{	
	    m_sdpFileType = BOTH_SDP;
	}
#ifdef _DEBUG    
	else
	{
	    HX_ASSERT(NEW_SDP == m_sdpFileType);
	}
#endif	
    }
#ifdef _DEBUG    
    else
    {
	HX_ASSERT(NEW_SDP == m_sdpFileType);
    }
#endif    
    HX_RELEASE(pBuf);    


    theErr = pRegistry->GetIntByName("config.SAP.NoSDPCompression", lInt);
    if (SUCCEEDED(theErr))
    {
	m_bNoCompression = (BOOL)lInt;
    }
    else
    {
	// default to FALSE
	HX_ASSERT(!m_bNoCompression);
    }	

    theErr = pRegistry->GetIntByName("config.SAP.CompressionSize", lInt);
    if (SUCCEEDED(theErr))
    {
        m_ulCompressionSize = (UINT32)lInt;
    }
    else
    {
        m_ulCompressionSize = 1024;
    }


    HX_RELEASE(pRegistry);

    /*
     * Enumerate through the plugins to try and find one that supports
     * IHXStreamDescription and save that off for the file object
     * to use when creating the SDP file
     */

    IHXPluginEnumerator*   pEnum;
    IUnknown*		    pPlugin;
    INT32		    iPlugin;
    IHXPlugin*		    pHXPlugin;

    theErr = m_pContext->QueryInterface(IID_IHXPluginEnumerator,
	(void**)&pEnum);
    if (FAILED(theErr))
    {
	goto bail;
    }

    for (iPlugin = pEnum->GetNumOfPlugins() - 1;
	iPlugin >= 0 && m_pStreamDesc == NULL;
	iPlugin--)
    {
	pEnum->GetPlugin(iPlugin, pPlugin);

	pPlugin->QueryInterface(IID_IHXStreamDescription,
	    (void**)&m_pStreamDesc);
	if (m_pStreamDesc)
	{
	    // QI IHXPlugin
	    pPlugin->QueryInterface(IID_IHXPlugin, 
    				(void**)&pHXPlugin);
	    if (pHXPlugin)    				
	    {
		pHXPlugin->InitPlugin(m_pContext);
		HX_RELEASE(pHXPlugin);
	    }
	    else
	    {
		HX_RELEASE(m_pStreamDesc);
	    }
	}

	pPlugin->Release();
    }
    pEnum->Release();
    
    if (NULL == m_pStreamDesc)
    {	
	theErr = HXR_FAIL;
	goto bail;
    }

    // consider it fail...
    if (!m_bListenEnabled && !m_bSendEnabled)
    {
	theErr = HXR_FAIL;
	goto bail;
    }
    
    return HXR_OK;
bail:
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pAddrPool);
    HX_RELEASE(pRegistry);
    return theErr;
}

// called to send SAP packet
HX_RESULT
CSapManager::SendSAP(CSapInfo* pSapInfo)
{
    HX_ASSERT(m_bSendEnabled);
    HX_ASSERT(m_pSAPSock);
    HX_ASSERT(pSapInfo && pSapInfo->m_pSapPkt);    
    
    HX_RESULT theErr = HXR_OK;        
    if (!m_bDirectoryStarted)
    {
	HX_ASSERT(!m_pSAPSock);
	return HXR_FAIL;
    }

    // write to it
    if (m_pMulticast)
    {
	m_pMulticast->InitMulticast(pSapInfo->m_uchTTL);
    }	

#ifdef XXXGo_DEBUG
    printf("sending SAP:  handle %u\n", pSapInfo->m_hSap);
#endif
//    theErr = m_pSAPSock->WriteTo(SDP_MULTICAST_ADDR, SDP_MULTICAST_PORT, pSapInfo->m_pSapPkt);
    theErr = m_pSAPSock->Write(pSapInfo->m_pSapPkt);

    
    return theErr;
}

UINT16
CSapManager::GetBWLimit(UINT8 uchTTL)
{
    // need to get bandwidth limit for this ttl
    // Don't change the order!!!
    if (uchTTL <= 15)
    {
	return SAP_BW_LIMIT_1;
    }
    else if (uchTTL <= 63)
    {
	return SAP_BW_LIMIT_2;
    }
    else if (uchTTL <= 127)
    {
	return SAP_BW_LIMIT_3;
    }
    else if (uchTTL <= 255)
    {
	return SAP_BW_LIMIT_4;
    }
    else
    {
	// this shouldn't really happen, but
	return SAP_BW_LIMIT_4;
    }
}

// calculate SAP interval based on the number of SAP pkt and TTL
UINT32
CSapManager::GetSAPInterval(UINT8 uchTTL, UINT32 ulPktSize)
{
//    HX_ASSERT(uchTTL >= 0);   XXXJMEV this is always true.

    UINT16 unLimit = GetBWLimit(uchTTL);
    
    // Here is the formula taken from draft-ietf-mmusic-sap-00.txt
    // in seconds
    UINT32 ulInterval;
    ulInterval = max(300, (8 * m_ulNumAds * ulPktSize) / unLimit);

    // from here, in msec
    ulInterval *= 1000;

#ifdef XXXGo_DEBUG
    printf("num ads: %u, pkt size: %u, limit: %u\n", m_ulNumAds, ulPktSize, 
	    unLimit);
    printf("%u\n", (8 * m_ulNumAds * ulPktSize) / unLimit);
    UINT32 ulDbgInterval = ulInterval;
#endif    
    
    // add random value (+/- 1/3 of the base interval) 
    // to prevent synchronization
    srand(HX_GET_TICKCOUNT());
    UINT32 ulRand = rand();
    int lSign = ulRand % 2 ? 1 : -1;
    double fThird = (double)ulRand / (double)(RAND_MAX * 3) * lSign;

    ulInterval += (UINT32)((double)ulInterval * fThird);

#ifdef XXXGo_DEBUG
    // ulInterval has to be in b/n +/- 1/3 of the base interval
    HX_ASSERT(ulInterval <= (ulInterval + (ulDbgInterval / 3)));
    HX_ASSERT(ulInterval >= (ulInterval - (ulDbgInterval / 3)));
    printf("numAds: %u, base interval %u, actual interval %u\n",
	    m_ulNumAds, ulDbgInterval, ulInterval);
#endif    
    

    return ulInterval;
}

HX_RESULT
CSapManager::CreateSocket
(
    UINT32 ulAddr, 
    UINT16 uPort,
    UINT8  chTTL,
    REF(IHXUDPSocket*) pSock,
    IHXUDPResponse* pResp
)
{
    HX_ASSERT(pResp);
    HX_RESULT		    theErr = HXR_OK;
    IHXUDPMulticastInit*   pMulticast	 = NULL;
    IHXNetworkServices*    pNetServices = NULL;
    IHXSetSocketOption*    pSockOpt	 = NULL;
    /*
     * Setup our UDP/multicast socket and initialize ourselves with the
     * raw source
     */
    theErr = m_pContext->QueryInterface(IID_IHXNetworkServices, 
    					(void**)&pNetServices);
    if (FAILED(theErr))
    {
	goto bail;
    }

    theErr = pNetServices->CreateUDPSocket(&pSock);
    if (theErr != HXR_OK) goto bail;

    theErr = pSock->Init(ulAddr, uPort, pResp);
    if (theErr != HXR_OK) goto bail;

    // set option before it binds
    theErr = pSock->QueryInterface(IID_IHXSetSocketOption, 
	(void**)&pSockOpt);
    if (SUCCEEDED(theErr))
    {
    	pSockOpt->SetOption(HX_SOCKOPT_REUSE_ADDR, TRUE);
    	pSockOpt->SetOption(HX_SOCKOPT_REUSE_PORT, TRUE);
       	HX_RELEASE(pSockOpt);
    }
    HX_ASSERT(!pSockOpt);
    
    theErr = pSock->Bind(HXR_INADDR_ANY, uPort);
    if (theErr != HXR_OK) goto bail;

    

    if (m_bListenEnabled)
    {
    	theErr = pSock->JoinMulticastGroup(ulAddr, HXR_INADDR_ANY);
    	if (theErr != HXR_OK) goto bail;
    }    	

    theErr = pSock->QueryInterface(IID_IHXUDPMulticastInit, 
	(void**)&pMulticast);
    if (theErr != HXR_OK) goto bail;

    theErr = pMulticast->InitMulticast(chTTL);
    if (theErr != HXR_OK) goto bail;

bail:
    HX_RELEASE(pMulticast);
    HX_RELEASE(pNetServices);

    if (theErr != HXR_OK) 
    {
	HX_RELEASE(pSock);

	CHXString str;
	str.Format("%d.%d.%d.%d/%d", 
	    (ulAddr >> 24) & 0xFF,
	    (ulAddr >> 16) & 0xFF,
	    (ulAddr >>  8) & 0xFF,
	    (ulAddr      ) & 0xFF,
	     uPort);
	/* error */	     
    }

    return theErr;
}

HX_RESULT 
CSapManager::GetSAPSock()
{
    HX_ASSERT(NULL == m_pSAPResp);
    
    HX_RESULT theErr = HXR_OK;
    
    // need to create socket
    // this reasponse handler will get readdone();
    m_pSAPResp = new CSDPResponseHandler(this);
    m_pSAPResp->AddRef();

    theErr = CreateSocket(SDP_MULTICAST_ADDR, 
			  SDP_MULTICAST_PORT, 
			  0, // ttl = 0 to begin with
			  m_pSAPSock,
			  m_pSAPResp);

    if (HXR_OK != theErr)
    {
	HX_ASSERT(NULL == m_pSAPSock);
	HX_RELEASE(m_pSAPResp);
	HX_ASSERT(NULL == m_pSAPResp);
	/* error */	
    }
    else
    {
	HX_ASSERT(m_pSAPSock);
	if (m_pSAPSock)
	{
	    // get a multicastinit
    	    theErr = m_pSAPSock->QueryInterface(IID_IHXUDPMulticastInit, 
	    	(void**)&m_pMulticast);
    	    if (theErr != HXR_OK)
    	    {
    		HX_ASSERT(!m_pMulticast);
		// all SAP pkt will have TTL = 0...Log it.    		
		/* error */
    	    }

	    if (m_bListenEnabled)
	    {
	    	// read from it...
	       	m_pSAPSock->Read(2048);
	    	// start the callback.  Every 30 min, we will reset about a
	    	// half of entries in the map because we are not handling 
	    	// time out of entries.  
	    	HandleResetCallback();
	    }
	    else
	    {
		HX_RELEASE(m_pSAPResp);
	    }
	}
    }
    return theErr;					
}

HX_RESULT
CSapManager::CompressSDP(IHXBuffer* pData, REF(IHXBuffer*) pCompBuf)
{
    HX_ASSERT(pData);
    HX_ASSERT(!pCompBuf);

    // don't change this to UINT32
    unsigned long ulDataSize = pData->GetSize();
    
    m_pClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pCompBuf);	
    
    // it can't be bigger than this
    pCompBuf->SetSize(ulDataSize + SAP_HEADER_SIZE);
    
    ulDataSize = pCompBuf->GetSize();
    int err = compress(pCompBuf->GetBuffer(),
		       &ulDataSize,
		       pData->GetBuffer(),
		       pData->GetSize());
    
    if ((Z_MEM_ERROR == err) || (Z_BUF_ERROR == err))
    {
    #ifdef XXXGo_DEBUG
	    if (Z_BUF_ERROR == err)
	    {
	    printf("Compression failed\n");
	    }
    #endif	    
	    HX_RELEASE(pCompBuf);
	    return HXR_UNEXPECTED;
    }
    
    HX_ASSERT(Z_OK == err);
    
    // reset the size
    pCompBuf->SetSize(ulDataSize);

    return HXR_OK;
}

HX_RESULT 
CSapManager::MakeSap(IHXBuffer*   pTextPayload,
	      CSapInfo*	    pSapInfo,
	      SapType	    msgType)
{
    HX_ASSERT(pTextPayload);
    HX_ASSERT(pSapInfo);
    
    UINT32  ulSize = pTextPayload->GetSize();
    IHXBuffer*     pBuf = NULL;
    BOOL    bCompressed = FALSE;
    HX_RESULT	theErr = HXR_OK;

    if (m_bNoCompression || (ulSize + SAP_HEADER_SIZE) <= m_ulCompressionSize)
    {
	pBuf = pTextPayload;
	pBuf->AddRef();
    }    
    else
    {
	IHXBuffer* pCompBuf = NULL;
	theErr = CompressSDP(pTextPayload, pCompBuf);
	if (FAILED(theErr))
	{
	    return theErr;
	}

	// pCompBuf has been AddRefed in CompressSDP()
	// pBuf->AddRef();
	// HX_RELEASE(pCompBuf);
	pBuf = pCompBuf;
	bCompressed = TRUE;	
    }
    
    ulSize = pBuf->GetSize();

    
    SapPacket pkt;
    
    pkt.msg_type	= msgType;
    pkt.compressed_bit	= bCompressed;
    pkt.msg_id_hash	= pSapInfo->m_unMsgIdHash;    
    pkt.orig_src	= m_ulOrigSrc;

    // they are all the same for all SAP packets
    pkt.version			    = SAP_VERSION;
    pkt.encryption_bit		    = 0;
    pkt.authentication_len	    = 0;
    pkt.op_authentication_header    = NULL;
    pkt.op_key_id		    = 0;
    pkt.op_timeout		    = 0;
    pkt.op_encryption_padding	    = 0;
    pkt.op_random		    = 0;

    
    pkt.text_payload.len = ulSize;
    pkt.text_payload.data = (INT8*)pBuf->GetBuffer();


    // now pack them up!
    IHXBuffer* pSapPkt = NULL;
    theErr = m_pClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pSapPkt);	    
    if (FAILED(theErr)) 
    {
	HX_ASSERT(!pSapPkt);
	HX_RELEASE(pBuf);
	return theErr;
    }
    
    // 8 byets for SAP header
    if (FAILED(pSapPkt->SetSize(SAP_HEADER_SIZE + ulSize)))
    {
	HX_RELEASE(pSapPkt);
	HX_RELEASE(pBuf);
	return HXR_OUTOFMEMORY;
    }
  
    unsigned char*   puchBuf;
    puchBuf = pSapPkt->GetBuffer();
    UINT32  ulLenSP = 0;

    puchBuf = pkt.pack(puchBuf, ulLenSP);
    HX_RELEASE(pBuf);

    HX_ASSERT((UINT32)(SAP_HEADER_SIZE + ulSize) == ulLenSP);

    if (pSapPkt->GetSize() == ulLenSP)
    {    
	pSapInfo->m_pSapPkt = pSapPkt;
	return HXR_OK;	
    }
    else
    {
	HX_RELEASE(pSapPkt);
	HX_ASSERT(!pSapInfo->m_pSapPkt);
	HX_ASSERT(!"sap pkt size is not what it is supposed to be...");
	return HXR_FAIL;
    }    
}

void
CSapManager::SendDeletionPkt(CSapInfo* pSapInfo)
{
    HX_RESULT theErr = HXR_OK;
    IHXBuffer* pBuf = NULL;
    
    theErr = m_pClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pBuf);	
    if (FAILED(theErr))
    {
	return;
    }

    pBuf->SetSize(100);
    char* pc = (char*)pBuf->GetBuffer();    
    sprintf(pc, "%u", m_ulOrigSrc);    
    pBuf->SetSize(strlen(pc));        

    // we don't need Announcement anymore...
    HX_RELEASE(pSapInfo->m_pSapPkt);
    if (SUCCEEDED(MakeSap(pBuf, pSapInfo, SAP_DELETION)))
    {
	SendSAP(pSapInfo);    
    }

    HX_RELEASE(pBuf);
}



// returns 0 in case of error.  Otherwise, it returns multicast addr in this 
// sap pkt
BOOL
CSapManager::GetAndMarkMultiAddr(REF(SapPacket) pkt, REF(CHXPtrArray*) prgAddrs)
{
    HX_ASSERT(m_bListenEnabled);
    HX_ASSERT(m_pStreamDesc);
    HX_ASSERT(prgAddrs);

    UINT32 ul = pkt.orig_src;

#ifdef XXXGo_DEBUG
    printf("New SAP from %d.%d.%d.%d\n",
		    	    	    	(ul >> 24) & 0xFF,
	    	    	    	    	(ul >> 16) & 0xFF,
	    	    	    	    	(ul >>  8) & 0xFF,
	    	    	    	    	(ul)	   & 0xFF);
    fflush(stdout);	    	    	    	    	
#endif    
    
    
    // try to decode if 
    if (pkt.encryption_bit)
    {
	// it's encrypted.  forget it.
	return 0;
    }


    IHXBuffer* pSDP = NULL;
    if (pkt.compressed_bit)
    {
        // need to decompress it.
        IHXBuffer* pDecompressedBuf = new CHXBuffer();
        pDecompressedBuf->AddRef();
        unsigned long ulTestSize = pkt.text_payload.len * 5;
        int lRet;
        do
        {
            // grow the size of buffer
            ulTestSize *= 2;
            pDecompressedBuf->SetSize(ulTestSize);

            // uncompress now.
            lRet = uncompress(pDecompressedBuf->GetBuffer(),
                                &ulTestSize,
                                (BYTE*)pkt.text_payload.data,
                                pkt.text_payload.len);

        } while (lRet == Z_BUF_ERROR);

        if (lRet == Z_OK)
        {
            /* NULL terminate the description buffer */
            // SetSize will memcpy
            pDecompressedBuf->SetSize((UINT32)ulTestSize + 1);
            pSDP = pDecompressedBuf;
            pSDP->GetBuffer()[ulTestSize] = '\0';
        }
        else
        {
            // error
            HX_RELEASE(pDecompressedBuf);
            return 0;
        }
    }
    else
    {
    	/* NULL terminate the description buffer */
    	pSDP = new CHXBuffer();
    	pSDP->AddRef();
    	pSDP->SetSize(pkt.text_payload.len + 1);
    	::memcpy(pSDP->GetBuffer(), 
	     	pkt.text_payload.data, 
	     	pkt.text_payload.len);
    	pSDP->GetBuffer()[pkt.text_payload.len] = '\0';	
    }
    
    UINT16	    cHeaders	= 0;
    IHXValues**    ppHeaders	= NULL;  // all the headers

    /*
     * Now that we have a file, create the stream and file headers
     */
    HX_RESULT theErr = m_pStreamDesc->GetValues(pSDP, cHeaders, ppHeaders);
    HX_RELEASE(pSDP);
    
    if (theErr != HXR_OK || cHeaders == 0 || ppHeaders == NULL)
    {
	return 0;
    }    

#if 0
    HX_ASSERT(cHeaders < 256);
    UINT32 addrs[256];
    memset(addrs, 0, sizeof(UINT32) * 256);
#endif

    int i;
    // now mark addr as used.
    for (i = 0; i < cHeaders; i++)
    {    
    	IHXValues* pHeader = ppHeaders[i];
    	IHXBuffer* pAddr = NULL;
	HX_RESULT theErr = HXR_OK;
	
    	theErr = pHeader->GetPropertyCString(PROP_MULTI_ADDRESS, pAddr);
    
    	if (HXR_OK == theErr && pAddr)
    	{
    	    UINT32 ulAddr = 0;
    	    ulAddr = ntohl(inet_addr((const char*)pAddr->GetBuffer()));

    	    if (SUCCEEDED(m_pAddrPool->UseAddress(ulAddr)))
    	    {
#if 0
		addrs[i] = ulAddr;
 #ifdef XXXGo_DEBUG		
		printf("\t\tNew Addr: %s\n", pAddr->GetBuffer());					    
 #endif		
#endif
    		UINT32* pulAddr = new UINT32;
    		*pulAddr = ulAddr;
    		prgAddrs->Add(pulAddr);
    	    }    		
#if 0
	    else
    	    {
    		BOOL b = FALSE;
    		for (int j = 0; j < cHeaders; j++)
    		{
    		    if (addrs[j] == ulAddr)
    		    {
    			b = TRUE;
    		    }
    		}
    		if (!b)
    		{
    		    // this should never happen...
    		    HX_ASSERT(!"What?");
    		}
    	    }
#endif    	    
    	}	
    	
    	HX_RELEASE(pAddr);
    }    

    for (i = 0; i < cHeaders; i++)
    {
	HX_ASSERT(ppHeaders[i]);
	HX_RELEASE(ppHeaders[i]);
    }
    HX_VECTOR_DELETE(ppHeaders);
    
    return TRUE;
}

// all we have to do here is to keep track a number of announcers so 
// the interval can be calculated
HX_RESULT
CSapManager::HandleSAP(REF(SapPacket) pkt)
{
    HX_ASSERT(m_bListenEnabled);
    UINT32  ulOrigSrc	= pkt.orig_src;
    UINT16  unMsgIdHash = pkt.msg_id_hash;
    SapType type	= (SapType)pkt.msg_type;

    CHXMapLongToObj* pHashMap	= NULL;
    CHXPtrArray*     prgAddrs	= NULL;

    if (m_SourceIdMap.Lookup(ulOrigSrc, (void*&)pHashMap))
    {
	// we have seen this source
	// look to see if this is a new announcement
	if (pHashMap->Lookup(unMsgIdHash, (void*&)prgAddrs))
	{
	    HX_ASSERT(prgAddrs);
	    if (SAP_ANNOUNCEMENT == type)
	    {
		// ok, we have seen this one.  don't do anything
		return HXR_OK;
	    }
	    else
	    {
		// one less ads
		m_ulNumAds--;

		ReleaseAddrs(prgAddrs);
		HX_DELETE(prgAddrs);
		
		pHashMap->RemoveKey(unMsgIdHash);

		// if there is no enty in hash map, delete hash map
		// from the srouce map
		if (pHashMap->IsEmpty())
		{
		    HX_DELETE(pHashMap);
		    m_SourceIdMap.RemoveKey(ulOrigSrc);
#ifdef XXXGo_DEBUG
		    if (m_ulNumAds == 0)
		    {
			// source map better be empty
			HX_ASSERT(m_SourceIdMap.IsEmpty());
		    }
#endif		    
		}

		return HXR_OK;
	    }
	}
	else
	{
	    // one more ads
	    m_ulNumAds++;
	    
	    // we have never seen this hash id.  Add it
	    prgAddrs = new CHXPtrArray();
	    if (!prgAddrs)
	    {
		return HXR_OUTOFMEMORY;
	    }

	    // now, try to find IP addr this ad claims to use for multicast
	    GetAndMarkMultiAddr(pkt, prgAddrs);

	    pHashMap->SetAt(unMsgIdHash, prgAddrs);	    

	    return HXR_OK;
	}
    }
    else
    {
	if (SAP_DELETION == type)
	{
	    // we have to have this entry to be able to delete it
	    return HXR_FAIL;
	}
	
	// we haven't even seen this source.  Add it
	pHashMap = new CHXMapLongToObj();
	if (!pHashMap)
	{
	    return HXR_OUTOFMEMORY;
	}
	else
	{
	    // one more ads
	    m_ulNumAds++;

	    prgAddrs = new CHXPtrArray();
	    if (!prgAddrs)
	    {
		return HXR_OUTOFMEMORY;
	    }

	    // now, try to find IP addr this ad claims to use for multicast
	    GetAndMarkMultiAddr(pkt, prgAddrs);

	    pHashMap->SetAt(unMsgIdHash, prgAddrs);

	    // now, I have to map this source
	    m_SourceIdMap.SetAt(ulOrigSrc, pHashMap);

	    
	    return HXR_OK;
	}
    
    }
    
}

void
CSapManager::ReleaseAddrs(CHXPtrArray* prgAddrs)
{
    HX_ASSERT(prgAddrs);

    for (int i = 0; i < prgAddrs->GetSize(); i++)
    {
	UINT32* pulAddr = (UINT32*)prgAddrs->GetAt(i);
	HX_ASSERT(pulAddr);
#ifdef XXXGo_DEBUG	
	printf("SAP: Releasing Addr: %d.%d.%d.%d\n",
		    	    	    	    	(*pulAddr >> 24) & 0xFF,
	    	    	    	    	    	(*pulAddr >> 16) & 0xFF,
	    	    	    	    	    	(*pulAddr >>  8) & 0xFF,
	    	    	    	    	    	(*pulAddr      ) & 0xFF);		    
#endif		            
	// release this addr
	m_pAddrPool->ReleaseAddress(*pulAddr);
	HX_DELETE(pulAddr);
    }
}

// This func. reset m_ulNumAds and maps to deal with time out issue.
// About half of entries will be deleted.
void
CSapManager::Reset()
{
#ifdef XXXGo_DEBUG
    UINT32 ulDbgNumAds = m_ulNumAds;
#endif    

    // clean up the map
    CHXMapLongToObj::Iterator i;
    for (i  = m_SourceIdMap.Begin();
	 i != m_SourceIdMap.End();
	 ++i)
    {
	CHXMapLongToObj::Iterator j;
	CHXMapLongToObj* pMap = (CHXMapLongToObj*)(*i);
	for (j  = pMap->Begin();
	     j != pMap->End();
	     ++j)
	{
	    // take random number and decide if we want to delete it
	    if ((rand() % 2))
	    {
		// delete
		pMap->RemoveKey(j.get_key());

		CHXPtrArray* prgAddrs = (CHXPtrArray*)(*j);

		ReleaseAddrs(prgAddrs);
		
		HX_DELETE(prgAddrs);
				
		m_ulNumAds--;		
	    }	    
	}

	if (pMap->IsEmpty())
	{
	    m_SourceIdMap.RemoveKey(i.get_key());
	    delete pMap;	    
	}
    }

#ifdef XXXGo_DEBUG
    if (m_SourceIdMap.IsEmpty())
    {
	HX_ASSERT(0 == m_ulNumAds);	
    }

    printf("NumAds: %u -> %u\n", ulDbgNumAds, m_ulNumAds);
#endif    
}

void
CSapManager::HandleResetCallback()
{
    Reset();

    // schedule the next one
    CResetCallback* pCB= new CResetCallback(this);
    pCB->AddRef();

    m_hResetCBHandle = m_pScheduler->RelativeEnter(pCB, SAP_RESET_TIME_INTERVAL);
    HX_RELEASE(pCB);	
}

CallbackHandle
CSapManager::ScheduleAnnouncement(CSapInfo* pSapInfo)
{
    HX_ASSERT(pSapInfo);
    
    UINT32 ulNextTime = 0;
    IHXCallback* pCB = (IHXCallback*)new CSapCallback(this, pSapInfo->m_hSap);
    pCB->AddRef();

    ulNextTime = GetSAPInterval(pSapInfo->m_uchTTL, pSapInfo->m_pSapPkt->GetSize());    
    pSapInfo->m_hCallbackID = m_pScheduler->RelativeEnter(pCB, ulNextTime);

    pCB->Release();
    return pSapInfo->m_hCallbackID;
}

void
CSapManager::RemoveAnnouncement(REF(CallbackHandle) hHandle)
{
    HX_ASSERT(m_pScheduler);
    
    if (hHandle != 0)
    {
	m_pScheduler->Remove(hHandle);
	hHandle = 0;
    }
}

STDMETHODIMP
CSapManager::ChangeTTL(SapHandle hSap, UINT8 uchTTL)
{
    CSapInfo* pSapInfo = (CSapInfo*)m_sapHandles.GetAt(hSap);
    if (pSapInfo)
    {
	if (pSapInfo->m_phSap)
	{
	    // this is dammy...that's RealSapInfo :)
	    CSapInfo* pRSI = NULL;
	    for (UINT8 i = 0; i < 2; ++i)
	    {
		pRSI = (CSapInfo*)m_sapHandles.GetAt(pSapInfo->m_phSap[i]);
		pRSI->m_uchTTL = uchTTL;
	    }		
	}
	else
	{
	    pSapInfo->m_uchTTL = uchTTL;
	}	    
	return HXR_OK;
    }

    return HXR_FAIL;
}

HX_RESULT
CSapManager::StartDirectory(UINT32 ulAddr, UINT16 unPort)
{
    if (!m_bDirectoryStarted)
    {
	HX_RESULT theErr = HXR_OK;
    	theErr = GetSAPSock();
    	if (SUCCEEDED(theErr))
    	{
    	    m_bDirectoryStarted = TRUE;
    	}

    	return theErr;
    }    	
    
    return HXR_OK;
}

/*
 *  This func assumes sdpplin we are using is capable of parsing old and new
 *  sdp files.  This is true for any sdpplin after server 8 preview.
 */
HX_RESULT
CSapManager::DealWithSDPType(IHXBuffer* pTextPayload, 
    REF(IHXBuffer*) pNewType, REF(IHXBuffer*) pOldType)
{    
    HX_ASSERT(pTextPayload);
    HX_ASSERT(!pNewType);
    HX_ASSERT(!pOldType);
    
    UINT16	    cHeaders	= 0;
    IHXValues**    ppHeaders	= NULL;  // all the headers
    HX_RESULT	    theErr	= HXR_OK;
    BOOL	    bOnceMoreWithOld = FALSE;
    IHXValues**    ppTempHeaders = NULL;
    UINT32	    i;

    theErr = m_pStreamDesc->GetValues(pTextPayload, cHeaders, ppHeaders);

    if (HXR_OK != theErr) 
    {
	// well, nothing we can do...
	HX_ASSERT(0 == cHeaders && NULL == ppHeaders);
	return theErr;
    }
    else if (cHeaders <= 0 || NULL == ppHeaders)
    {
    	// sanity check...something is wrong...
    	theErr = HXR_UNEXPECTED;
    	goto bail;
    }

    /*
     *	Create a correct sdp file(s)
     */

    // GetValues return file header and stream headers.
    // GetDescription expects file header, OPTION HEADER, and stream headers...
    ppTempHeaders = ppHeaders;
    
    ppHeaders = new IHXValues*[cHeaders + 1];
    if (!ppHeaders)
    {
	// too bad...
	ppHeaders = ppTempHeaders;
	theErr = HXR_OUTOFMEMORY;
	goto bail;	
    }

    /* 
     *	Good to go!  Since we have +1 headers
     */
    ++cHeaders;

    // file header
    ppHeaders[0] = ppTempHeaders[0]; 

    // option header
    // XXXGo - we need a better required fields support.  Right now, there is
    // no way to get this right.
    ppHeaders[1] = new CHXHeader();
    if (ppHeaders[1])
    {
	ppHeaders[1]->AddRef();    
	ppHeaders[1]->SetPropertyULONG32("LastModified", HX_GET_TICKCOUNT());
    }	

    // stream headers
    // if there is no stream header, it should just get out.
    for (i = 2; i < cHeaders; ++i)
    {
	ppHeaders[i] = ppTempHeaders[i - 1];	
    }

    if (NEW_SDP == m_sdpFileType)
    {
	ppHeaders[0]->SetPropertyULONG32("SdpFileType", INTEROP_SDP);
	theErr = m_pStreamDesc->GetDescription(cHeaders, ppHeaders, pNewType);
    }
    else if (OLD_SDP == m_sdpFileType)
    {
	ppHeaders[0]->SetPropertyULONG32("SdpFileType", BACKWARD_COMP_SDP);
	theErr = m_pStreamDesc->GetDescription(cHeaders, ppHeaders, pOldType);
    }
    else
    {
	HX_ASSERT(BOTH_SDP == m_sdpFileType);
	// start with new :)
	ppHeaders[0]->SetPropertyULONG32("SdpFileType", INTEROP_SDP);
	theErr = m_pStreamDesc->GetDescription(cHeaders, ppHeaders, pNewType);
	if (HXR_OK == theErr)
	{
	    ppHeaders[0]->SetPropertyULONG32("SdpFileType", BACKWARD_COMP_SDP);
	    theErr = m_pStreamDesc->GetDescription(cHeaders, ppHeaders, pOldType);
	}
    }    

    if (HXR_OK != theErr)
    {
	// something went wrong...
	HX_RELEASE(pNewType);
	HX_RELEASE(pOldType);	
    }
    /*
     *	We don't want NULL at the end!!!
     */
    else if (pNewType)
    {
	pNewType->SetSize(pNewType->GetSize() - 1);
    }
    else if (pOldType)
    {
	pOldType->SetSize(pOldType->GetSize() - 1);
    }
    
    for (i = 0; ppHeaders && i < cHeaders; i++)
    {
	HX_RELEASE(ppHeaders[i]);
    }
    HX_VECTOR_DELETE(ppTempHeaders);
    HX_VECTOR_DELETE(ppHeaders);
    
    return theErr;
                  
/*
 *  Man this is ugly, but since the number of headers is different depending on
 *  wehere we error out...
 */
bail:
    HX_ASSERT(!ppTempHeaders);
    for (i = 0; ppHeaders && i < cHeaders; i++)
    {
	HX_RELEASE(ppHeaders[i]);
    }
    HX_VECTOR_DELETE(ppHeaders);

    return theErr;
}

SapHandle
CSapManager::StartAnnouncement(IHXBuffer* pTextPayload, UINT8 uchTTL)
{
    if (!m_bSendEnabled || !pTextPayload)
    {
	return 0;
    }
    HX_ASSERT(m_bDirectoryStarted);

    /*
     * we have SdpFileType config var in SAP and we have to make sure this
     * text payload (sdp file) is the right one...ugly, but necessary
     */
    IHXBuffer* pNewType = NULL;
    IHXBuffer* pOldType = NULL;

    if (DealWithSDPType(pTextPayload, pNewType, pOldType) != HXR_OK)
    {
	HX_ASSERT(!pNewType && !pOldType);
	// hmmm...just send the original :)
	return ReallyStartAnnouncement(pTextPayload, uchTTL);
    }

    HX_ASSERT(pNewType || pOldType);
    if (pNewType && !pOldType)
    {
	SapHandle h = ReallyStartAnnouncement(pNewType, uchTTL);	
	HX_RELEASE(pNewType);
	return h;
    }
    else if (pOldType && !pNewType)
    {
	SapHandle h = ReallyStartAnnouncement(pOldType, uchTTL);	
	HX_RELEASE(pOldType);
	return h;
    }

    HX_ASSERT(pNewType && pOldType);

    // create a dammy sapinfo.  It just contains 2 sap handles.	
    CSapInfo* pSapInfo = new CSapInfo();
    pSapInfo->m_phSap = new SapHandle[2];

    // this is what we will return.
    pSapInfo->m_hSap = m_sapHandles.AddTail(pSapInfo);

    pSapInfo->m_phSap[0] = ReallyStartAnnouncement(pNewType, uchTTL);
    pSapInfo->m_phSap[1] = ReallyStartAnnouncement(pOldType, uchTTL);
    HX_RELEASE(pNewType);
    HX_RELEASE(pOldType);
    return pSapInfo->m_hSap;
}

SapHandle
CSapManager::ReallyStartAnnouncement(IHXBuffer* pTextPayload, UINT8 uchTTL)
{
    // create SAPInfo
    // create SAP pkt
    // Send it
    // Schedule callback
    
    SapHandle hSap = 0;

    CSapInfo* pSapInfo = new CSapInfo();
    pSapInfo->m_uchTTL = uchTTL;
    pSapInfo->m_unMsgIdHash = (UINT16)m_randGen.GetRandomNumber();                

    if (FAILED(MakeSap(pTextPayload, pSapInfo, SAP_ANNOUNCEMENT)))
    {
	HX_ASSERT(!pSapInfo->m_pSapPkt);
	HX_DELETE(pSapInfo);
	return 0;
    }

    m_ulNumAds++;
    pSapInfo->m_hSap = m_sapHandles.AddTail(pSapInfo);
    SendSAP(pSapInfo);   	
    ScheduleAnnouncement(pSapInfo);	
    
    return pSapInfo->m_hSap;    
}


STDMETHODIMP
CSapManager::StopAnnouncement(SapHandle hSap)
{
    if (!m_bSendEnabled)
    {
	return HXR_FAIL;
    }

    CSapInfo* pSapInfo = (CSapInfo*)m_sapHandles.GetAt(hSap);
    if (pSapInfo)	
    {
	HX_ASSERT(pSapInfo->m_hSap == hSap);

	if (pSapInfo->m_phSap)
	{
	    // this was a just dammy info...
	    CSapInfo* pRealSapInfo = 
		(CSapInfo*)m_sapHandles.GetAt(pSapInfo->m_phSap[0]);
	    if (pRealSapInfo)
	    {	    
		ReallyStopAnnouncement(pRealSapInfo);
		HX_DELETE(pRealSapInfo);
	    }
	    pRealSapInfo = 
		(CSapInfo*)m_sapHandles.GetAt(pSapInfo->m_phSap[1]);
	    if (pRealSapInfo)
	    {	    
		ReallyStopAnnouncement(pRealSapInfo);
		HX_DELETE(pRealSapInfo);
	    }

	    // clean up the dammy
	    m_sapHandles.RemoveAt(hSap);
	    HX_DELETE(pSapInfo);

	}
	else
	{
	    ReallyStopAnnouncement(pSapInfo);
	    HX_DELETE(pSapInfo);
	}	    

	return HXR_OK;
    }
    
    return HXR_FAIL;
}

void
CSapManager::ReallyStopAnnouncement(CSapInfo* pSapInfo)
{
    HX_ASSERT(pSapInfo);
    
    if (pSapInfo->m_hCallbackID)
    {
	RemoveAnnouncement(pSapInfo->m_hCallbackID);	    	    
    }
    
    HX_ASSERT(!pSapInfo->m_hCallbackID);
    
    m_ulNumAds--;
    // this will release the announcement pkt in pSapInfo and replace it
    // with deletion pkt.
    SendDeletionPkt(pSapInfo);
    
    m_sapHandles.RemoveAt(pSapInfo->m_hSap);    
}

BOOL
CSapManager::IsAnnouncerEnabled()
{
    return m_bSendEnabled;
}

void
CSapManager::HandleSapCallback(SapHandle hSap)
{
    CSapInfo* pSapInfo = (CSapInfo*)m_sapHandles.GetAt(hSap);
    if (pSapInfo)
    {
	// we should never be scheduling a dammy sapinfo
	HX_ASSERT(!pSapInfo->m_phSap);
	
	pSapInfo->m_hCallbackID = 0;
	SendSAP(pSapInfo);
	ScheduleAnnouncement(pSapInfo);
    }    
}

CSapManager::CSapInfo::CSapInfo() 
    : m_pSapPkt(NULL)
    , m_uchTTL(0)
    , m_unMsgIdHash(0)
    , m_hCallbackID(0)
    , m_hSap(0)
    , m_phSap(0)
{    
}

CSapManager::CSapInfo::~CSapInfo()
{ 
    HX_ASSERT(!m_hCallbackID);    
    HX_RELEASE(m_pSapPkt); 
    HX_VECTOR_DELETE(m_phSap);		
}

/////////////////////////////////////////////////////////////////////////////
// CSapManager::CSapCallback
//
STDMETHODIMP_(UINT32)
CSapManager::CSapCallback::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}
STDMETHODIMP_(UINT32)
CSapManager::CSapCallback::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    
    delete this;
    return 0;
}
STDMETHODIMP
CSapManager::CSapCallback::QueryInterface
(
    REFIID interfaceID,
    void** ppInterfaceObj
)
{
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
	AddRef();
	*ppInterfaceObj = (IUnknown*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(interfaceID, IID_IHXCallback))
    {
	AddRef();
	*ppInterfaceObj = (IHXCallback*)this;
	return HXR_OK;
    }

    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}
STDMETHODIMP
CSapManager::CSapCallback::Func()
{   
    // need actually multicast sap at m_hSap
    m_pSapMgr->HandleSapCallback(m_hSap);
    return HXR_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CSapManager::CSDPResponseHandler
//
STDMETHODIMP_(UINT32)
CSapManager::CSDPResponseHandler::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}
STDMETHODIMP_(UINT32)
CSapManager::CSDPResponseHandler::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    
    delete this;
    return 0;
}
STDMETHODIMP
CSapManager::CSDPResponseHandler::QueryInterface
(
 REFIID interfaceID,
 void** ppInterfaceObj
 )
{
    // By definition all COM objects support the IUnknown interface
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
	AddRef();
	*ppInterfaceObj = (IUnknown*)(IHXUDPResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(interfaceID, IID_IHXUDPResponse))
    {
	AddRef();
	*ppInterfaceObj = (IHXUDPResponse*)this;
	return HXR_OK;
    }
    
    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}
STDMETHODIMP
CSapManager::CSDPResponseHandler::ReadDone
(
 HX_RESULT	status,
 IHXBuffer*	pBuffer,
 ULONG32	ulAddr,
 UINT16		nPort
 )
{    
    HX_ASSERT(m_pSapMgr->m_bListenEnabled);
    BYTE*   pFirst = pBuffer->GetBuffer();
    BYTE*   pNext  = pFirst;
    UINT32  ulSize = pBuffer->GetSize();

    while (pNext && pNext < (pFirst + ulSize) &&
	   ValidSapDatagram(pNext, ulSize + pFirst - pNext))
    {
    	SapPacket pkt;
    
    	pNext = pkt.unpack(pNext, (pFirst + ulSize) - pNext);	

	if (!pNext)
	{
	    // unpack failed...
	    break;
	}

	// ignore our own
	if (m_pSapMgr->m_ulOrigSrc == pkt.orig_src)
	{
	    continue;
	}
    
	// session is uniquly determind by orig_src && msg_id_hash
	// so, m_SourceIdMap will contains another map of msg_id_hash.
	m_pSapMgr->HandleSAP(pkt);
    }
    
    m_pSapMgr->m_pSAPSock->Read(2048);
    return HXR_OK;
}

/*
 * CSapManager::CSDPResponseHandler::ValidSapDatagram()
 *
 * Make sure we don't have junk before unpacking.
 *
 * Datagram is required to refer to an unencrypted payload, ipv4 address,
 * since that's all we handle anyway.  It also gets around a problem
 * in SAPv1 versus SAPv2, in that they both use version=1 in the header, so
 * I couldn't figure out how to distinguish them if an ipv4 address was
 * specified with encryption.  (v1 includes encryption data and v2
 * doesn't; so how do I unpack in that case??)   jmevissen, 10/2001
 */
BOOL
CSapManager::CSDPResponseHandler::ValidSapDatagram(BYTE* off, UINT32 len)
{

    // must have at least this many to specify an addr, but no payload or auth

    if (len < 8) return FALSE;

    // Must be ipv4 addr (0x10) and unencrypted (0x2)

    if (*off & 0x12) return FALSE;
    off++;		// bitfield

    unsigned int authlen = *off;
    authlen *= 4;	// convert from 32-bit words to bytes

    if (len < authlen + 8) return FALSE;

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSapManager::CResetCallback
//
STDMETHODIMP_(UINT32)
CSapManager::CResetCallback::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}
STDMETHODIMP_(UINT32)
CSapManager::CResetCallback::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}
STDMETHODIMP
CSapManager::CResetCallback::QueryInterface
(
    REFIID interfaceID,
    void** ppInterfaceObj
)
{
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
	AddRef();
	*ppInterfaceObj = (IUnknown*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(interfaceID, IID_IHXCallback))
    {
	AddRef();
	*ppInterfaceObj = (IHXCallback*)this;
	return HXR_OK;
    }

    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}
STDMETHODIMP
CSapManager::CResetCallback::Func()
{   
    m_pSapMgr->HandleResetCallback();   
    return HXR_OK;
}


