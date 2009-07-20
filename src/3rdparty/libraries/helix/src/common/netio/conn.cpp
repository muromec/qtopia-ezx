/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: conn.cpp,v 1.16 2007/07/06 20:43:53 jfinnecy Exp $
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

#include "hlxclib/time.h"
#include <string.h>
#if defined(_UNIX)
#include <sys/types.h>
#endif
#include "hlxclib/sys/socket.h"
#include "hlxclib/netdb.h"

#include "hxcom.h"
#include "conn.h"
#include "pckunpck.h"
#include "hxengin.h"
#include "hxcom.h"
#include "hxtick.h"
#include "hxresult.h"
#include "hxthread.h"
#include "hxbuffer.h"
#include "hxstrutl.h"
#include "netbyte.h"

#if defined( THREADS_SUPPORTED ) || defined(_UNIX_THREADED_NETWORK_IO)
#include "thrdconn.h"
#endif

//#include "../dcondev/dcon.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

#define MAX_CONN_STARVINGTIME	1000

// DNR cache
DNR_cache	conn::mCache[MAX_CACHE];
UINT16		conn::mCacheCount = 0;

// conn object list
// XXXJR Really sorry this, but as an actual instance it doesn't get
// initialized when linked into a shared library under unix.  Yuck.
CHXMapPtrToPtr*	conn::mConnectionList	    = NULL;
HXThread*	conn::m_pNetworkThread	    = NULL;
UINT32  	conn::m_ulMaxBandwidth	    = MAX_UINT32;   // in bytes per sec
UINT32		conn::m_ulTCPTotalBytesRead = 0;
UINT32		conn::m_ulTCPReadTimeStamp  = 0;
CHXSimpleList*	conn::m_pTCPStarvingList    = NULL;
HXBOOL            conn::m_bNetworkThreading   = FALSE;
HXBOOL            conn::m_bThreadedDNS        = FALSE;

static IHXMutex* z_pConnectionListMutex = NULL;

DestructConnGlobals selfConnGlobalDestructor;

void		
conn::DestructGlobals()
{
    HX_DELETE(z_pConnectionListMutex);
}

// new_socket() creates the correct version of conn depending on the
// defined platform and requested socket type
conn* conn::new_socket (IUnknown* pContext, UINT16 type)
{
    conn *c = 0;

#if defined(_UNIX_THREADED_NETWORK_IO)
    if( conn::GetNetworkThreadingPref() )
    {
        c = ThreadedConn::new_socket(pContext, type);
    }
    else
    {
        c = actual_new_socket(pContext, type);
    }
#elif defined(THREADS_SUPPORTED)
    c = ThreadedConn::new_socket(pContext, type);
#else
    c = actual_new_socket(pContext, type);
#endif

    if ( c != NULL )
    {
	conn::add_connection_to_list ( pContext, c );
	c->AddRef();
    }

    return(c);
}

conn::~conn (void)
{
//    dprintf("**Destructor** conn::conn(%X)\n", this);
    if (m_pUDPBuffer)
    {
	delete [] m_pUDPBuffer;
	m_pUDPBuffer = 0;
    }

    if ( this != NULL )
	    conn::remove_connection_from_list ( this );

    HX_RELEASE(m_pContext);
}


// platform specific constructor should set the socket to a value  
// indicating the connection is not open
conn::conn (IUnknown* pContext)
    : m_bNoAsyncDNS( FALSE )
    , m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
    mSock = HX_INVALID_SOCKET;
    mConnectionOpen = 0;
    mLastError = HXR_OK;
    mCallBack = 0;
    mHostIPAddr = 0;
    mHostIPValid = FALSE;
    mDNSDone	 = FALSE;
    m_pUDPBuffer	 = 0;
    m_ulLastStarvingTime = 0; 
}


/* 	conn::is_cached searches the cached DNR for host and returns its inet address 
	in  addr and 1 if it was found, or 0 if it was not found */

UINT16
conn::is_cached(char *host,ULONG32 *addr)
{
	UINT16 found = 0;
	
	for (UINT16 i=0; i < mCacheCount && !found; i++) 
	{
		if (mCache[i].domainName && (::strcmp(host, mCache[i].domainName) == 0)) 
		{
			*addr = mCache[i].addr;
			found = 1;
		}
	}
	return found;
}

/* 	conn::add_to_cache adds the host and its inet addr to the cache. If the cache is
	full, the oldest entry is replaced with the new entry */

void		
conn::add_to_cache(char *host,ULONG32 addr)
{
	UINT16 found = 0;
	
	// search the cache to see if entry has already been entered
	for (UINT16 i=0; i < mCacheCount && !found; i++) 
	{
		DNR_cache_ptr p = &mCache[i];

		if(addr == p->addr && p->domainName && (::strcmp(host, p->domainName) == 0)) 
		{
			::time(&p->cachetime);	// refresh cache time
			found = 1;
		}
	}

	if(found) return;  			// our work here is done
	
	if(mCacheCount < MAX_CACHE) // add the new entry to the end of the cache array
	{
		DNR_cache_ptr p = &mCache[mCacheCount];
		p->addr = addr;
		::time(&p->cachetime);
		if (p->domainName != host)
		{
		    HX_VECTOR_DELETE(p->domainName);
		    p->domainName = ::new_string(host);
		}
		mCacheCount++;
	}
	else // search cache for oldest entry and replace it with the new data
	{
		time_t minTime = mCache[0].cachetime;
		UINT16 minPos = 0;
		for(UINT16 i = 1; i < mCacheCount; i++)
		{
			if(mCache[i].cachetime < minTime)
			{
				minTime = mCache[i].cachetime;
				minPos = i;
			}
		}
		
		DNR_cache_ptr p = &mCache[minPos];
		p->addr = addr;
		::time(&p->cachetime);		// time of caching
		if (p->domainName != host)
		{
		    HX_VECTOR_DELETE(p->domainName);
		    p->domainName = ::new_string(host);
		}
	}
}

void		
conn::remove_from_cache(const char *host)
{
	UINT16 found = 0;
	// search the cache to see if entry has been entered
	for (UINT16 i = 0; i < mCacheCount && !found; i++) 
	{
		DNR_cache_ptr p = &mCache[i];
		if(p->domainName && ::strcmp(host, p->domainName) == 0) 
		{
			HX_VECTOR_DELETE(p->domainName);
			p->addr = 0;
			p->cachetime = 0;	// reset time to 0
			found = 1;
		}
	}
}

void			
conn::clear_cache()
{
    mCacheCount = 0;
}


POSITION
conn::get_first_connection_position ()
{
    if(!conn::mConnectionList)
    {
	return NULL;
    }

    return conn::mConnectionList->GetStartPosition();
}

void
conn::get_next_connection(POSITION& nextPos, conn*& rpConn)
{
    if(!conn::mConnectionList)
    {
	nextPos	= NULL;
	rpConn	= NULL;
	return;
    }

    void* pVoid = NULL;
    conn::mConnectionList->GetNextAssoc ( nextPos, (void *&)rpConn, pVoid );
}

void
conn::add_connection_to_list ( IUnknown* pContext, conn *pConn )
{
    if (!z_pConnectionListMutex)
    {
#if !defined(THREADS_SUPPORTED) && defined(_UNIX_THREADED_NETWORK_IO)
	// THREAD_SUPPORTED should be defined whenever _UNIX_THREADED_NETWORK_IO
	// is defined, please check the config file of the platform,
	// Otherwise, the mutex created below would be a stub mutex.
	HX_ASSERT(FALSE);
#endif
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&z_pConnectionListMutex, pContext);
    }
    
    HX_ASSERT( z_pConnectionListMutex);
    z_pConnectionListMutex->Lock();

    //fprintf(stderr, "==AddToList== conn::add_connection_to_list(%X)\n", pConn);
    if(!conn::mConnectionList)
    {
	conn::mConnectionList = new CHXMapPtrToPtr;
    }
        
    conn::mConnectionList->SetAt ( pConn, NULL );
    z_pConnectionListMutex->Unlock();
}

void
conn::remove_connection_from_list ( conn *pConn )
{
    LISTPOSITION    listpos = NULL;
    CHXSimpleList::Iterator  i;

    if(!conn::mConnectionList)
    {
	return;
    }
    
    HX_ASSERT(z_pConnectionListMutex);
    if (z_pConnectionListMutex)
    {
	z_pConnectionListMutex->Lock();
    }

    HX_VERIFY (conn::mConnectionList->RemoveKey ( pConn ) );
    if (conn::m_pTCPStarvingList)
    {
	listpos = conn::m_pTCPStarvingList->Find(pConn);

	if (listpos)
	{
	    conn::m_pTCPStarvingList->RemoveAt(listpos);
	    pConn->m_ulLastStarvingTime = 0;
	}
    }
    
    if (conn::mConnectionList->IsEmpty())
    {
	delete conn::mConnectionList;
	conn::mConnectionList = NULL;

	HX_ASSERT(!conn::m_pTCPStarvingList || conn::m_pTCPStarvingList->IsEmpty());
    }

    if (conn::m_pTCPStarvingList && conn::m_pTCPStarvingList->IsEmpty())
    {
	HX_DELETE(conn::m_pTCPStarvingList);
    }

    if (z_pConnectionListMutex)
    {
	z_pConnectionListMutex->Unlock();
    }
}

UINT32
conn::bytes_to_preparetcpread(conn* pConn)
{
    // no op. if EnforceMaxBandwidth is FALSE
    if (conn::m_ulMaxBandwidth == MAX_UINT32)
    {
	return MAX_UINT32;
    }

    UINT32	    ulCurrentTime = HX_GET_TICKCOUNT();
    UINT32	    ulBytesToRead = 0;
    LISTPOSITION    listpos = NULL;
    conn*	    pStarvingConn = NULL;

    UINT32	    ulTimeElapsed = CALCULATE_ELAPSED_TICKS(conn::m_ulTCPReadTimeStamp, ulCurrentTime);

    // return 0 bytes if the current bandwidth >= max. bandwidth
    if (ulTimeElapsed < 1000 && conn::m_ulTCPTotalBytesRead >= conn::m_ulMaxBandwidth)
    {
	goto cleanup;
    }
    // reset the attributes every 1 sec
    else if (ulTimeElapsed >= 1000)
    {
	conn::m_ulTCPReadTimeStamp = ulCurrentTime;
	conn::m_ulTCPTotalBytesRead = 0;
    }

    // remove the conn from the starving list if it has been starving to death :)
    // so other conn has chance to get the its share of bytes
    while (conn::m_pTCPStarvingList && !conn::m_pTCPStarvingList->IsEmpty())
    {
	pStarvingConn = (conn*)conn::m_pTCPStarvingList->GetHead();

	if (pStarvingConn != pConn &&
	    (CALCULATE_ELAPSED_TICKS(pStarvingConn->m_ulLastStarvingTime, ulCurrentTime) >=
	    MAX_CONN_STARVINGTIME))
	{	    
	    conn::m_pTCPStarvingList->RemoveHead();
	    pStarvingConn->m_ulLastStarvingTime = 0;
	}
	else
	{
	    break;
	}
    }

    // return 0 bytes if there is starving conn AND pConn is not the starving conn
    // at the top of the list
    if (!pStarvingConn || pStarvingConn == pConn)
    {
	// either there is no starving conn OR
	// pConn is the starving conn
	ulBytesToRead = conn::m_ulMaxBandwidth - conn::m_ulTCPTotalBytesRead;
	if (pStarvingConn)
	{
	    pStarvingConn->m_ulLastStarvingTime = 0;
	    conn::m_pTCPStarvingList->RemoveHead();
	}
    }

cleanup:

    HX_ASSERT(conn::mConnectionList);

    // if there is no bytes to read and more than 1 source
    if (ulBytesToRead == 0 && conn::mConnectionList && conn::mConnectionList->GetCount() > 1)
    {	
	if (!conn::m_pTCPStarvingList)			
	{
	    conn::m_pTCPStarvingList = new CHXSimpleList();
	}

	if (!pConn->m_ulLastStarvingTime)
	{
	    pConn->m_ulLastStarvingTime = ulCurrentTime;
	    conn::m_pTCPStarvingList->AddTail(pConn);
	}
    }

    return ulBytesToRead;
}

void
conn::bytes_to_actualtcpread(conn* pConn, UINT32 ulBytesRead)
{
    if (conn::m_ulMaxBandwidth != MAX_UINT32)
    {
	conn::m_ulTCPTotalBytesRead += ulBytesRead;
    }
}

HX_RESULT
conn::EnumerateInterfaces
    (REF(UINT32*) pulInterfaces, REF(UINT32) ulNumInterfaces)
{
    HX_RESULT theErr = HXR_OK;

    const UINT32 ulHostNameLen = 256;
 
    char pHostname[ulHostNameLen]; /* Flawfinder: ignore */
    
#ifndef _MACINTOSH
    char **pptr;
#endif

#ifdef _MACINTOSH
    return HXR_UNEXPECTED;
#endif

    theErr = init_drivers(NULL);
    if (FAILED(theErr))
    {
	// just retrun, no need to close_driver()
	return theErr;
    }

    theErr = get_host_name(pHostname, ulHostNameLen);
    if (FAILED(theErr))
    {
	goto bail;
    }    

    // get local host name
    struct hostent* hptr;    
    theErr = get_host_by_name(pHostname, hptr);
    if (FAILED(theErr))
    { 
	goto bail;
    }

    HX_ASSERT(hptr);

#ifndef _MACINTOSH
    if (hptr->h_addrtype == AF_INET) 
    {
	// get # of interfaces
	UINT32 ulIFCount = 0;
    	for (pptr = hptr->h_addr_list; *pptr != NULL; pptr++) 
    	{
	    ulIFCount++;
	}

	if (ulIFCount > ulNumInterfaces)
	{
	    // tell the user how many it needs.
	    ulNumInterfaces = ulIFCount;
	    theErr = HXR_BUFFERTOOSMALL;
	    goto bail;
	}
	
	ulNumInterfaces= 0;
	for (pptr = hptr->h_addr_list; *pptr != NULL; pptr++)
	{
	    pulInterfaces[ulNumInterfaces++] = DwToHost(*(UINT32*)*pptr);
	}

	HX_ASSERT(ulNumInterfaces == ulIFCount);
	theErr = HXR_OK;
    }
    else
    {
	theErr = HXR_UNEXPECTED;
    }
#endif

bail:    
    return theErr;    
}


#if defined(HELIX_FEATURE_SECURECONN)
CHXMapPtrToPtr secureconnhelper::zm_ConnMap;

// xxxbobclark yeesh, why this is required I'm still working on.
// Yuck nonetheless.

void WasteTime()
{
#ifdef _MACINTOSH
	ULONG32 startTix = HX_GET_TICKCOUNT();

	while (CALCULATE_ELAPSED_TICKS(startTix, HX_GET_TICKCOUNT()) < 200)
	{
//	    EventRecord er;
//	    ::EventAvail(everyEvent, &er);
	}
#endif
}

conn*
secureconnhelper::GetConn(LONG32 fakeFD)
{
    conn* pConn = (conn*)zm_ConnMap[(void*)fakeFD];
    
    HX_ASSERT(pConn != NULL);
    
    return pConn;
}

void
secureconnhelper::SetConn(LONG32 fakeFD, conn* pConn)
{
    HX_ASSERT(zm_ConnMap[(void*)fakeFD] == NULL);
    
    zm_ConnMap[(void*)fakeFD] = pConn;
}

long secureconnhelper::readCallback(LONG32 fakeFD, void* buff, LONG32 len)
{
    conn* pConn = GetConn(fakeFD);
    
    HX_RESULT result = HXR_FAIL;
    
    if (pConn)
    {
	UINT16 bytes = (UINT16) len;
	result = pConn->read(buff, &bytes);
	
	WasteTime();
	
	if (result == HXR_OK)
	{
	    return bytes;
	}
    }
    return -1;
}

long secureconnhelper::writeCallback(LONG32 fakeFD, void* buff, LONG32 len)
{

    conn* pConn = GetConn(fakeFD);
    
    HX_RESULT result = HXR_FAIL;
    
    if (pConn)
    {
	UINT16 bytes = (UINT16) len;
	result = pConn->write(buff, &bytes);
	
	WasteTime();
	
	if (result == HXR_OK)
	{
	    return bytes;
	}
    }
    return -1;
}

void secureconnhelper::closeCallback(LONG32 fakeFD)
{
    conn* pConn = GetConn(fakeFD);
    
    if (pConn)
    {
	pConn->done();
    }
}


LONG32 secureconn::zm_Count = 0;

secureconn::secureconn(IUnknown* pContext, IHXSSL* pHXSSL)
  : conn(pContext)
  , m_lRefCount(0)
  , m_pActualConn(0)
  , m_FakeFD(-1)
  , m_pHXSSL(NULL)
{
    m_pActualConn = conn::new_socket(pContext, HX_TCP_SOCKET);

    m_FakeFD = ++zm_Count;
    secureconnhelper::SetConn(m_FakeFD, m_pActualConn);
    m_pHXSSL = pHXSSL;
    HX_ASSERT(m_pHXSSL != NULL);
    m_pHXSSL->AddRef();
    
    m_pHXSSL->SetCallbacks((void*)secureconnhelper::readCallback,
                           (void*)secureconnhelper::writeCallback,
                           (void*)secureconnhelper::closeCallback);
    
    m_pHXSSL->Initialize();
}

secureconn::~secureconn()
{
    HX_ASSERT(m_pActualConn != NULL);
    m_pHXSSL->Shutdown();
    
    HX_RELEASE(m_pHXSSL);
    if (m_pActualConn)
    {
        m_pActualConn->Release();
        m_pActualConn = NULL;
    }
}


ULONG32	secureconn::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 secureconn::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


HX_RESULT
secureconn::connect(const char* host, UINT16 port, UINT16 blocking, ULONG32 ulPlatform)
{
    HX_ASSERT(m_pActualConn != NULL);
    HX_ASSERT(m_pHXSSL != NULL);
    
    HX_RESULT result = m_pActualConn->connect(host, port, blocking, ulPlatform);

    WasteTime();
    
    if (result == HXR_OK)
    {
	result = m_pHXSSL->PostConnect(m_FakeFD);
    }
    
    return result;
}

HX_RESULT
secureconn::read(void* buf, UINT16* size)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    HX_ASSERT(m_pHXSSL != NULL);
    
    LONG32 len = *size;
    
    len = m_pHXSSL->Read(m_FakeFD, buf, len);
    
    if (len == -1) return HXR_FAIL;
    
    *size = (UINT16) len;
    return HXR_OK;
}

HX_RESULT
secureconn::write(void* buf, UINT16* size)
{
    HX_ASSERT(m_pActualConn != NULL);
    HX_ASSERT(m_pHXSSL != NULL);
    
    LONG32 len = *size;
    
    len = m_pHXSSL->Write(m_FakeFD, buf, len);
    
    if (len == -1) return HXR_FAIL;
    
    *size = (UINT16) len;
    return HXR_OK;
}

HX_RESULT secureconn::blocking()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->blocking();
}

HX_RESULT secureconn::nonblocking()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->nonblocking();
}

HX_RESULT secureconn::readfrom(REF(IHXBuffer*)   pBuffer,
					 REF(UINT32)	    ulAddress,
					 REF(UINT16)	    ulPort)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->readfrom(pBuffer, ulAddress, ulPort);
}

HX_RESULT secureconn::writeto(void 		*buf,
					 UINT16 	*len, 
					 ULONG32 	addr,
					 UINT16 	port)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->writeto(buf, len, addr, port);
}

ULONG32 secureconn::get_addr()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->get_addr();
}

UINT16 secureconn::get_local_port()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->get_local_port();
}

HX_RESULT
secureconn::dns_find_ip_addr(const char * host, UINT16 blocking)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->dns_find_ip_addr(host, blocking);
}

HXBOOL
secureconn::dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->dns_ip_addr_found(valid, addr);
}

void
secureconn::done()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    m_pActualConn->done();
}

HX_RESULT
secureconn::init(UINT32		local_addr,
		UINT16 		port, 
		 UINT16 	blocking)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->init(local_addr, port, blocking);
}

HX_RESULT
secureconn::listen(ULONG32	ulLocalAddr,
		 UINT16		port,
		 UINT16 	backlog,
		 UINT16		blocking,
		 ULONG32	ulPlatform)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->listen(ulLocalAddr, port, backlog, blocking, ulPlatform);
}

HX_RESULT
secureconn::join_multicast_group(ULONG32 addr, ULONG32 if_addr)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->join_multicast_group(addr, if_addr);
}

HX_RESULT
secureconn::leave_multicast_group(ULONG32 addr, ULONG32 if_addr)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->leave_multicast_group(addr, if_addr);
}

HX_RESULT
secureconn::set_broadcast(HXBOOL enable)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->set_broadcast(enable);
}

HX_RESULT
secureconn::set_multicast_if(UINT32 ulInterface)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->set_multicast_if(ulInterface);
}






HX_RESULT
secureconn::last_error()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->last_error();
}

#ifdef _MACINTOSH
HX_RESULT
secureconn::GetEndpoint(REF(void*) pRef)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->GetEndpoint(pRef);
}

HX_RESULT
secureconn::SetupEndpoint(HXBOOL bWait)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->SetupEndpoint(bWait);
}
#endif

void
secureconn::set_callback(HXAsyncNetCallback* pCallback)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    m_pActualConn->set_callback(pCallback);
}

UINT16
secureconn::connection_open()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->connection_open();
}

int
secureconn::get_sock()
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->get_sock();
}

void
secureconn::set_sock(int theSock)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    m_pActualConn->set_sock(theSock);
}

HXBOOL
secureconn::set_receive_buf_size(int DesiredSize)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->set_receive_buf_size(DesiredSize);
}

HX_RESULT
secureconn::reuse_addr(HXBOOL enable)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->reuse_addr(enable);
}

HX_RESULT
secureconn::reuse_port(HXBOOL enable)
{
    HX_ASSERT(m_pActualConn != NULL);
    
    return m_pActualConn->reuse_port(enable);
}
#endif /* HELIX_FEATURE_SECURECONN */

#ifdef TESTING

#include "conn.h"

int opt_debug = 0xff;

int main( int argc, char **argv )
{
	conn *m_conn = 0;
	HX_RESULT theErr = HXR_OK;

	m_conn = conn::new_socket( HX_TCP_SOCKET );
	if ( m_conn )
	{
		// XXXAAK -- local addr binding stuff
		theErr = m_conn->init( INADDR_ANY, argc > 2 ? atoi(argv[2]) : 7071, 1);

		if ( !theErr )
		{
			theErr = m_conn->connect( argc > 1 ? argv[1] : "localhost",
					argc > 2 ? atoi(argv[2]) : 7071,
					1);

			printf("connect(): %s, port: %d, theErr: %d, errno: %d\n", 
				argc > 1 ? argv[1] : "localhost",
				argc > 2 ? atoi(argv[2]) : 7071,
				theErr, errno );
		}
		else
			printf("Couldn't init port: %d, theErr: %d\n",
					argc > 2 ? atoi(argv[2]) : 7071,
					theErr);
	}
	else
		printf("Couldn't create a new TCP socket!\n");

	exit(0);
}

#endif
