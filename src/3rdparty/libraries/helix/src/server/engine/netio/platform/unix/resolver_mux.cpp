/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: resolver_mux.cpp,v 1.4 2006/02/27 20:08:04 atin Exp $ 
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

#ifndef _VXWORKS

#include "sockio.h"

#include "hxcom.h"
#include "hxresult.h"
#include "hxengin.h"
#include "hxcomm.h"
#include "hxerror.h"
#include "microsleep.h"

#include "hxassert.h"
#include "proc.h"
#include "shmem.h"
#include "debug.h"

#ifdef _UNIX
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <ctype.h>

#include "resolver_mux.h"

// RCODE values
#define RC_OK       0       /* Success */
#define RC_FMT      1       /* Format error - server cannot grok */
#define RC_FAIL     2       /* Server failed */
#define RC_EXIST    3       /* No such host/domain */
#define RC_NOTIMPL  4       /* Not implemented */
#define RC_ACCESS   5       /* Access denied */

// CLASS values
#define CL_IN       1       /* Internet */
#define CL_CS       2
#define CL_CH       3
#define CL_HS       4

// RRTYPE and QTYPE values (QTYPE is a superset of RRTYPE)
#define RR_A         1      /* Address */
#define RR_NS        2
#define RR_MD        3
#define RR_MF        4
#define RR_CNAME     5      /* Canonical name (alias) */
#define RR_SOA       6
#define RR_MB        7
#define RR_MG        8
#define RR_MR        9
#define RR_NULL     10
#define RR_WKS      11
#define RR_PTR      12      /* Pointer */
#define RR_HINFO    13
#define RR_MINFO    14
#define RR_MX       15
#define RR_TXT      16

#define MAX_PKT_LEN 512

#define INADDR_NONE_U 0xffffffffUL

#if defined(_SOLARIS)
static int inet_aton_resolver( const char* cp, struct in_addr* inp )
{
    unsigned long addr = inet_addr( cp );
    if( addr == INADDR_NONE_U )
    {
        return 0;
    }
    inp->s_addr = addr;
    return 1;
}
#define inet_aton(a,b) inet_aton_resolver((a),(b))
#endif

#define ERROR_REPORT(str) (m_pErrMsg ? \
    m_pErrMsg->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, str, 0) : 0)

// Server sets this to TRUE when --dir option is used
BOOL g_bDisableInternalResolver = FALSE;

// This is the structure that is passed to the resolver
struct gethost_request
{
    int  type;                  /* query type: only RR_A is supported for now */
    char name[MAX_HOST_LEN];    /* null terminated name to resolve */
};

// This is the structure that is passed back from the resolver
struct gethost_response
{
    int            result;              /* 0 = success, otherwise error code */
    char           name[MAX_HOST_LEN];  /* name that was requested */
    struct in_addr addr;                /* address, if successful */
};

// This is a singly linked list for responses waiting on the same query
struct request_info
{
    struct request_info* pnext;
    ResolverMUXResp*     presp;
};

#if !defined _LINUX && defined PTHREADS_SUPPORTED

extern "C" void* AllocateStack();
extern "C" void AddToProcessList(int id);
extern "C" void RecordStackBottom(Process* proc);

extern "C" void*
ResolverPthreadEntryPoint(resolver_thread_args* ta)
{   
    Process* proc;

    // set up this thread'/process's signal stack
    stack_t ss;
    memset(&ss, 0, sizeof(stack_t));
    ss.ss_sp = AllocateStack();

    *((void**)ss.ss_sp - 1) = ta;
    ss.ss_size = STACKSIZE - sizeof(thread_args*) - GUARD_SIZE;

    ss.ss_sp = (char*)ss.ss_sp - STACKSIZE + GUARD_SIZE;
    sigaltstack(&ss, NULL);

    ta->process_number = Process::GetNewProcessNumber();
    proc = new Process;
    proc->AssignProcessNumber(ta->process_number);

    RecordStackBottom(proc);

    printf("Starting TID %lu, procnum %d (resolver)\n",
           pthread_self(), proc->procnum());
    fflush(0);

    AddToProcessList(pthread_self());

    // never return from inside ResolverMux::DoChild()
    ta->resolver_mux->DoChild(ta->readfd, ta->writefd);

    return 0;
}

#endif

/*
 * Resolver Info
 */
ResolverGateway::ResolverGateway(int readfd, int writefd, unsigned int reqmax,
                                IHXAsyncIOSelection* pSelect)
: m_writeto(writefd)
, m_readfrom(readfd)
, m_nRequestMax(reqmax)
, m_pSelect(NULL)
{
    m_pSelect = pSelect;
    m_pSelect->AddRef();
    int flags;
    flags = fcntl(m_readfrom, F_GETFL);
    fcntl(m_readfrom, F_SETFL, flags | O_NONBLOCK);
    m_pSelect->Add(this, m_readfrom, PNAIO_READ);
}

ResolverGateway::~ResolverGateway()
{
    m_pSelect->Remove(m_readfrom, PNAIO_READ);
    m_pSelect->Release();

    POSITION pos;

    pos = m_mapRequests.GetStartPosition();
    while( pos )
    {
        const char* phost = NULL;
        struct request_info* pinfo = NULL;
        m_mapRequests.GetNextAssoc( pos, phost, (void*&)pinfo );
        while( pinfo != NULL )
        {
            struct request_info* pnext = pinfo->pnext;
            pinfo->presp->GetHostByNameDone( HXR_FAIL, INADDR_NONE_U );
            delete pinfo;
            pinfo = pnext;
        }
    }
}

/*
 *  IUnknown methods
 */
STDMETHODIMP
ResolverGateway::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ResolverGateway::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG32)
ResolverGateway::Release()
{
    return 1;
}

/*
 *  IHXCallback methods
 */

STDMETHODIMP
ResolverGateway::Func()
{
    struct gethost_response resp;
    int n;
    HX_RESULT res = HXR_FAIL;
    UINT32 ulAddr = INADDR_NONE_U;

    n = read( m_readfrom, &resp, sizeof(resp) );
    HX_ASSERT( n == sizeof(resp) );
    if( n == sizeof(resp) )
    {
        res = HXR_OK;
        ulAddr = ntohl( resp.addr.s_addr );
    }

    struct request_info* pinfo = NULL;
    m_mapRequests.Lookup( resp.name, (void*&)pinfo );
    HX_ASSERT( pinfo != NULL );
    while( pinfo != NULL )
    {
        struct request_info* pnext = pinfo->pnext;
        pinfo->presp->GetHostByNameDone( res, ulAddr );
        delete pinfo;
        pinfo = pnext;
    }
    m_mapRequests.RemoveKey( resp.name );

    return HXR_OK;
}

UINT32
ResolverGateway::Count()
{
    return m_mapRequests.GetCount();
}

HX_RESULT
ResolverGateway::AddRequest(const char* pHostname, ResolverMUXResp* pResp)
{
    HX_ASSERT( pHostname != NULL );

    if( pHostname == NULL || m_mapRequests.GetCount() >= m_nRequestMax )
    {
        return HXR_FAIL;
    }

    struct request_info* pnew = new request_info;
    pnew->pnext = NULL;
    pnew->presp = pResp;

    // see if we are already waiting for this hostname
    struct request_info* pinfo = NULL;
    m_mapRequests.Lookup( pHostname, (void*&)pinfo );
    if( pinfo != NULL )
    {
        pnew->pnext = pinfo;
        m_mapRequests.SetAt( pHostname, (void*)pnew );
        return HXR_OK;
    }

    // nope, send a new request down the pipe
    m_mapRequests.SetAt( pHostname, pnew );

    struct gethost_request req;
    req.type = RR_A;
    strcpy( req.name, pHostname );
    write( m_writeto, &req, sizeof(req) );
    return HXR_OK;
}

/*
 * ResolverMUX
 */
ResolverMUX::ResolverMUX(int capacity, int max, IHXServerFork* pFork,
        IHXAsyncIOSelection* pSel, IHXErrorMessages* pErrMsg)
{
    m_max = max;
    m_capacity = capacity;
    m_current = 0;
    m_pResolvers = new ResolverGateway*[max];
    memset((void*)m_pResolvers, 0, max * sizeof(ResolverGateway*));
    m_pSelect = pSel;
    m_pSelect->AddRef();
    m_pFork = pFork;
    if( m_pFork != NULL )
    {
        m_pFork->AddRef();
    }
    m_pErrMsg = pErrMsg;
    if( m_pErrMsg != NULL )
    {
        m_pErrMsg->AddRef();
    }
}

ResolverMUX::~ResolverMUX()
{
    for (int i = 0; i < m_current; i++)
    {
        delete m_pResolvers[i];
    }
    delete[] m_pResolvers;
    HX_RELEASE( m_pSelect );
    HX_RELEASE( m_pFork );
    HX_RELEASE( m_pErrMsg );
}

HX_RESULT
ResolverMUX::GetHostByName(const char* hostname, ResolverMUXResp* pResp)
{
    int best = -1;
    int bestcount = 666;
    int i;
    int topipe[2];
    int frompipe[2];
    
    if (!hostname || strlen(hostname) + 1 > MAX_HOST_LEN)
    {
        pResp->GetHostByNameDone(HXR_FAIL, INADDR_NONE_U);
        return HXR_OK;
    }
    
    /*
     * find resolver with smallest queued items
     */
    for (i = 0; i < m_current; i++)
    {
        if (m_pResolvers[i]->Count() < bestcount &&
                m_pResolvers[i]->Count() < m_capacity)
        {
            best = i;
        }
    }
    /*
     * found none, have to create anew.
     */
    if (best == -1)
    {
        if (m_current == m_max)
        {
            PANIC(("at resolver capacity!"));
            ERROR_REPORT( "at resolver capacity!" );
            pResp->GetHostByNameDone(HXR_RESOURCE_NOT_FOUND, INADDR_NONE_U);
        }
        else
        {
            pipe(topipe);
            pipe(frompipe);

#ifdef _LINUX
            int forkres;
            IHXResolverExec* pResExec = NULL;
            m_pFork->QueryInterface(IID_IHXResolverExec, (void**)&pResExec);
            if( m_pFork != NULL )
            {
                forkres = m_pFork->Fork();
            }
            else
            {
                forkres = fork();
            }

            if( !forkres )
            {
                /* child */
                close(topipe[1]);
                close(frompipe[0]);
                if( pResExec )
                {
                    pResExec->ResolverExec(topipe[0], frompipe[1]);
                    /* never to return */
                    HX_ASSERT(0);
                }
                if( g_bDisableInternalResolver )
                {
                    do_hostnames( topipe[0], frompipe[1] );
                }
                else
                {
                    do_hostnames_internal( topipe[0], frompipe[1] );
                }
            }
            else
            {
		printf("Starting PID %d (resolver)\n", forkres);

                /* parent */
                close(topipe[0]);
                close(frompipe[1]);
                m_pResolvers[m_current] = new ResolverGateway(frompipe[0],
                        topipe[1], m_capacity, m_pSelect);
                m_pResolvers[m_current]->AddRequest(hostname, pResp);
                m_current++;
            }
#elif defined PTHREADS_SUPPORTED
	    resolver_thread_args* pArgs = new resolver_thread_args;
	    pArgs->readfd = topipe[0];
	    pArgs->writefd = frompipe[1];
	    pArgs->resolver_mux = this;

	    // the child thread never returns from MakeThread()
	    pthread_t child_tid = MakeThread(pArgs);

	    /* parent */
	    m_pResolvers[m_current] = new ResolverGateway(frompipe[0],
		    topipe[1], m_capacity, m_pSelect);
	    m_pResolvers[m_current]->AddRequest(hostname, pResp);
	    m_current++;
#endif
        }
    }
    else
    {
        m_pResolvers[best]->AddRequest(hostname, pResp);
    }
    return HXR_OK;
}

#if !defined _LINUX && defined PTHREADS_SUPPORTED

HX_RESULT
ResolverMUX::DoChild(int readfd, int writefd)
{
    if (g_bDisableInternalResolver) 
    {
	do_hostnames(readfd, writefd);
    }
    else
    {
	do_hostnames_internal(readfd, writefd);
    }
    return HXR_OK;
}

pthread_t
ResolverMUX::MakeThread(resolver_thread_args* pArgs)
{
    pthread_t tid = 0;
    pthread_attr_t* attr = new pthread_attr_t;

    pthread_attr_init(attr);
    void** pStack = (void**)AllocateStack();
    *(pStack - 1) = pArgs;
    size_t stacksize = STACKSIZE - GUARD_SIZE -
    sizeof(thread_args*);;
    pStack = (void**)((char*)pStack - STACKSIZE + GUARD_SIZE);

    pthread_attr_setstackaddr(attr, pStack);
    pthread_attr_setstacksize(attr, stacksize);

    pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM);

    if (pthread_create(&tid, attr, (void*(*)(void*))ResolverPthreadEntryPoint, pArgs) != 0)
    {
        char* pSysError = strerror(errno);
        printf("Creation of resolver thread failed: %s\n", pSysError);
        exit(1);
    }

    return tid;
}

#endif

/*****************************************************************************
 *
 * Supporting code for resolver
 *
 *****************************************************************************/

//#define MAX_TIME_T 0x7fffffff

// Domains to search, max 6
static char  g_szSearchDomains[6][MAX_HOST_LEN];
static int   g_nSearchDomains;

// Nameservers to query, max 3
static struct sockaddr_in   g_addrNameservers[3];
static int                  g_nNameservers;

static struct timeval g_tvLastPrune;

inline void tv_add( struct timeval& tv1, const struct timeval& tv2 )
{
    tv1.tv_sec += tv2.tv_sec;
    tv1.tv_usec += tv2.tv_usec;
    while( tv1.tv_usec >= 1000*1000 )
    {
        tv1.tv_usec -= 1000*1000;
        tv1.tv_sec++;
    }
}

inline void tv_sub( struct timeval& tv1, const struct timeval& tv2 )
{
    tv1.tv_sec -= tv2.tv_sec;
    tv1.tv_usec -= tv2.tv_usec;
    while( tv1.tv_usec < 0 )
    {
        tv1.tv_usec += 1000*1000;
        tv1.tv_sec--;
    }
}

class CHostInfo
{
public:
    CHostInfo( const char* szName )
    {
        m_tvExpire.tv_sec = m_tvExpire.tv_usec = 0;
        strcpy( m_szName, szName );
        m_addr.s_addr = INADDR_NONE_U;
    }

    struct timeval  m_tvExpire;
    char            m_szName[MAX_HOST_LEN];
    struct in_addr  m_addr;
};

// For inverse "PTR" queries -- not currently used
class CAddrInfo
{
public:

    CAddrInfo( struct in_addr addr )
    {
        m_tExpire = 0;
        m_addr = addr;
        m_szName[0] = '\0';
    }

    time_t          m_tExpire;
    struct in_addr  m_addr;
    char            m_szName[MAX_HOST_LEN];
};

// inet_aton for in-addr.arpa hack: "4.3.2.1.in-addr.arpa" -> 1.2.3.4
static int inet_aton_rev( const char* cp, struct in_addr* inp )
{
    int ret = 0;
    char host[16]; // aaa.bbb.ccc.ddd
    const char* end = cp;
    while( *end && (isdigit(*end) || '.' == *end) ) end++;
    if( end > cp && end-cp < 16 && !strcasecmp( end, "in-addr.arpa" ) )
    {
        struct in_addr addr;
        end--;
        memcpy( host, cp, end-cp );
        host[end-cp] = '\0';
        ret = inet_aton( host, &addr );
        if( ret )
        {
            unsigned n;
            BYTE* src = (BYTE*)&addr.s_addr + sizeof(struct in_addr);
            BYTE* dst = (BYTE*)&inp->s_addr;
            for( n = 0; n < sizeof(struct in_addr); n++ )
            {
                src--;
                *dst = *src;
                dst++;
            }
        }
    }
    return ret;
}

// inet_ntoa for in-addr.arpa hack: 1.2.3.4 -> "4.3.2.1.in-addr.arpa"
static char* inet_ntoa_rev( struct in_addr in )
{
    static char host[29]; // aaa.bbb.ccc.ddd.in-addr.arpa\0
    BYTE qa[4];
    memcpy( qa, &in.s_addr, 4 );
    sprintf( host, "%u.%u.%u.%u.in-addr.arpa", qa[3], qa[2], qa[1], qa[0] );
    return host;
}

CResolver::CResolver( int writefd ) :
    m_sock(-1),
    m_writefd(writefd),
    m_usQueryID(0)
{
    struct sockaddr_in sin;
    HX_SOCKLEN_T sinlen;

    m_sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
    if( m_sock != -1 )
    {
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = 0;
	sinlen = sizeof(sin);

        if( ::bind( m_sock, (struct sockaddr*)&sin, sinlen ) != 0 )
        {
            close( m_sock );
            m_sock = -1;
        }
    }
    ReadConfig();

    srand(time(NULL));
    m_usQueryID = rand();
}

CResolver::~CResolver( void )
{
    struct in_addr addr;
    addr.s_addr = INADDR_NONE_U;
    POSITION pos;

    pos = m_mapHostQueries.GetStartPosition();
    while( pos )
    {
        LONG32 lQueryID = 0;
        CHostQuery* pQuery = NULL;
        m_mapHostQueries.GetNextAssoc( pos, lQueryID, (void*&)pQuery );
        delete pQuery;
    }
    pos = m_mapHostInfo.GetStartPosition();
    while( pos )
    {
        const char* pHost = NULL;
        CHostInfo* pInfo = NULL;
        m_mapHostInfo.GetNextAssoc( pos, pHost, (void*&)pInfo );
        delete pInfo;
    }
}

BOOL CResolver::GetHost( const char* szHost )
{
    HX_ASSERT( szHost != NULL );
    if( szHost == NULL )
    {
        return FALSE;
    }

    // Determine if this is a number or name
    BOOL bIsNumeric = TRUE;
    const char* p = szHost;
    while( *p )
    {
        if( !isdigit(*p) && *p != '.' )
        {
            bIsNumeric = FALSE;
            break;
        }
        p++;
    }
    if( bIsNumeric )
    {
        int err = 0;
        in_addr addr;
        if( !inet_aton( szHost, &addr ) )
        {
            err = ENOENT;
        }
        SendResponse( err, szHost, addr );
        return TRUE;
    }

    struct timeval tvNow;
    gettimeofday( &tvNow, NULL );

    // First search for name as given
    CHostInfo* pHostInfo = NULL;
    if( m_mapHostInfo.Lookup( szHost, (void*&)pHostInfo ) )
    {
        HX_ASSERT( pHostInfo != NULL );
        if( pHostInfo->m_tvExpire.tv_sec >= tvNow.tv_sec )
        {
            SendResponse( 0, szHost, pHostInfo->m_addr );
            return TRUE;
        }
        m_mapHostInfo.RemoveKey( szHost );
    }

    // Not found - search the domain list
    char szFQDN[MAX_HOST_LEN];
    if( g_nSearchDomains <= 0 )
    {
        // Domain list is empty - game over, man
        in_addr addr;
        addr.s_addr = INADDR_NONE_U;
        SendResponse( ENOENT, szHost, addr );
        return TRUE;
    }
    for( int i = 0; i < g_nSearchDomains; i++ )
    {
        strcpy( szFQDN, szHost );
        strcat( szFQDN, "." );
        strcat( szFQDN, g_szSearchDomains[i] );
        if( m_mapHostInfo.Lookup( szFQDN, (void*&)pHostInfo ) )
        {
            if( pHostInfo->m_tvExpire.tv_sec >= tvNow.tv_sec )
            {
                SendResponse( 0, szHost, pHostInfo->m_addr );
                return TRUE;
            }
            m_mapHostInfo.RemoveKey( szFQDN );
        }
    }

    // Initial query delta in milliseconds
    UINT32 nDelta = 4000/g_nNameservers;

    // Looks like we have to send a query
    UINT16 usQueryID = NextQueryID();
    CHostQuery* pQuery = new CHostQuery( szHost );
    gettimeofday( &pQuery->m_tvExpire, NULL );
    pQuery->m_tvDelta.tv_sec = nDelta/1000;
    pQuery->m_tvDelta.tv_usec = nDelta%1000;
    tv_add( pQuery->m_tvExpire, pQuery->m_tvDelta );
    pQuery->m_nRetries = 0;
    pQuery->m_nCurDomain = -1;
    pQuery->m_nCurServer = 0;
    m_mapHostQueries.SetAt( (LONG32)usQueryID, (void*)pQuery );

    SendQuery( RR_A, usQueryID, pQuery, szHost );

    return TRUE;
}

void CResolver::OnReadReady( void )
{
    BYTE    buf[MAX_PKT_LEN+1];
    ssize_t recvlen;
    size_t  pos;
    UINT16  ustmp;
    dns_hdr hdr;

    recvlen = recv( m_sock, (char*)buf, sizeof(buf), 0 );
    if( recvlen == -1 )
    {
        //XXX: socket error, fail all pending queries and die
        return;
    }
    if( recvlen < (ssize_t)sizeof(dns_hdr) || recvlen > (ssize_t)MAX_PKT_LEN )
    {
        // Packet length is out of bounds, ignore it
        return;
    }

    pos = 0;
    memcpy( &ustmp, buf+pos, 2 ); pos += 2; hdr.id = ntohs( ustmp );
    memcpy( &ustmp, buf+pos, 2 ); pos += 2; hdr.flags = ntohs( ustmp );
    memcpy( &ustmp, buf+pos, 2 ); pos += 2; hdr.qdcnt = ntohs( ustmp );
    memcpy( &ustmp, buf+pos, 2 ); pos += 2; hdr.ancnt = ntohs( ustmp );
    memcpy( &ustmp, buf+pos, 2 ); pos += 2; hdr.nscnt = ntohs( ustmp );
    memcpy( &ustmp, buf+pos, 2 ); pos += 2; hdr.arcnt = ntohs( ustmp );

    // QR=1, OPCODE=0, AA=x, TC=0, RD=1, RA=x, Z=0, RCODE=0
    // 10000x01x000xxxx
    if( (hdr.flags & 0xFB70) != 0x8100 || hdr.qdcnt != 1 )
    {
        // Bad header flags, ignore it
        return;
    }

    struct timeval  tvnow;
    dns_qr_hdr      qrhdr;
    dns_rr_hdr      rrhdr;
    char            szHost[MAX_HOST_LEN];

    gettimeofday( &tvnow, NULL );
    while( hdr.qdcnt )
    {
        if( ! ParseQuestionHeader( &qrhdr, buf, recvlen, pos ) )
        {
            //fprintf( stderr, "CResolver: bad question" );
            return;
        }
        hdr.qdcnt--;
    }

    // Find pending query
    CHostQuery* pQuery = NULL;
    m_mapHostQueries.Lookup( (LONG32)hdr.id, (void*&)pQuery );
    if( pQuery == NULL )
    {
        // No match so ignore it
        return;
    }

    /*
     * XXXTDM: could validate further by checking:
     *         1. qrhdr.qtype == RR_A
     *         2. qrhdr.szHost is correct (in which case we would want to
     *            add a requested FQDN member to CHostQuery)
     */

    UINT rc = (hdr.flags & 0xF);
    if( rc != RC_OK )
    {
        // Does not exist - try next domain
        m_mapHostQueries.RemoveKey( (LONG32)hdr.id );

        pQuery->m_nCurDomain++;
        if( pQuery->m_nCurDomain >= g_nSearchDomains )
        {
            // We exhausted the search list
            struct in_addr addr;
            addr.s_addr = INADDR_NONE_U;
            SendResponse( ENOENT, pQuery->m_szHost, addr );
            delete pQuery;
        }
        else
        {
            UINT16 usQueryID = NextQueryID();

            UINT32 nDelta = 4000/g_nNameservers;
            pQuery->m_tvDelta.tv_sec = nDelta/1000;
            pQuery->m_tvDelta.tv_usec = nDelta%1000;
            pQuery->m_tvExpire = tvnow;
            tv_add( pQuery->m_tvExpire, pQuery->m_tvDelta );
            pQuery->m_nRetries = 0;

            strcpy( szHost, pQuery->m_szHost );
            strcat( szHost, "." );
            strcat( szHost, g_szSearchDomains[pQuery->m_nCurDomain] );
            //XXX: reset query's server iterator here?
            m_mapHostQueries.SetAt( (LONG32)usQueryID, (void*)pQuery );
            SendQuery( RR_A, usQueryID, pQuery, szHost );
        }

        // Don't bother parsing the answers
        hdr.ancnt = 0;
    }

    while( hdr.ancnt )
    {
        struct in_addr addr;
        struct timeval tvexp;
        struct timeval tvtmp;

        if( ! ParseAnswerHeader( &rrhdr, buf, recvlen, pos ) )
        {
            return;
        }
        switch( rrhdr.rtype )
        {
        case RR_A:
            if( !rrhdr.rdlen || rrhdr.rdlen%4 )
            {
                return;
            }
            //XXX: keep multiple addrs?
            memcpy( &addr, buf+pos, 4 );
            tvtmp.tv_sec = rrhdr.ttl;
            tvtmp.tv_usec = 0;
            tvexp = tvnow;
            tv_add( tvexp, tvtmp );
            AddHostEntry( tvexp, pQuery->m_szHost, addr );
            m_mapHostQueries.RemoveKey( (LONG32)hdr.id );
            SendResponse( 0, pQuery->m_szHost, addr );
            return; // Got the answer so no need to hang around
        case RR_CNAME:
            if( ! DecodeName( szHost, buf, recvlen, pos ) )
            {
                return;
            }
            break;
        case RR_PTR:
            //XXX: don't think we should be getting these
            if( ! DecodeName( szHost, buf, recvlen, pos ) )
            {
                return;
            }
            // inet_aton_rev( qrhdr.szHost, &addr );
            // AddHostEntry( ... );
            break;
        default: // Looks valid but useless
            break;
        }
        hdr.ancnt--;
    }

    // Ignore authority and additional RR's
}

void CResolver::OnTimer( void )
{
    POSITION pos;

    struct timeval tvNow;
    gettimeofday( &tvNow, NULL );

    // Check for query timeouts
    pos = m_mapHostQueries.GetStartPosition();
    while( pos )
    {
        LONG32 qid = 0;
        CHostQuery* pQuery = NULL;
        m_mapHostQueries.GetNextAssoc( pos, qid, (void*&)pQuery );
        if( pQuery->m_tvExpire.tv_sec < tvNow.tv_sec ||
            ( pQuery->m_tvExpire.tv_sec == tvNow.tv_sec &&
              pQuery->m_tvExpire.tv_usec < tvNow.tv_usec ) )
        {
            // Query timed out -- next nameserver
            pQuery->m_nCurServer++;
            if( pQuery->m_nCurServer >= g_nNameservers )
            {
                // Nameserver list exhausted, bump timeout
                pQuery->m_nCurServer = 0;
                pQuery->m_nRetries++;
                if( pQuery->m_nRetries >= 4 )
                {
                    // Absolute timeout -- fail the query
                    struct in_addr addr;
                    addr.s_addr = INADDR_NONE_U;
                    SendResponse( ENOENT, pQuery->m_szHost, addr );
                    m_mapHostQueries.RemoveKey( qid );
                    continue;
                }

                tv_add( pQuery->m_tvDelta, pQuery->m_tvDelta );

                pQuery->m_tvExpire = tvNow;
                tv_add( pQuery->m_tvExpire, pQuery->m_tvDelta );
            }

            char szHost[MAX_HOST_LEN];
            strcpy( szHost, pQuery->m_szHost );
            if( pQuery->m_nCurDomain >= 0 )
            {
                strcat( szHost, "." );
                strcat( szHost, g_szSearchDomains[pQuery->m_nCurDomain] );
            }
            SendQuery( RR_A, (UINT16)qid, pQuery, szHost );
        }
    }

    // Prune cache once per hour
    if( tvNow.tv_sec >= g_tvLastPrune.tv_sec + 3600 )
    {
        g_tvLastPrune = tvNow;
        pos = m_mapHostInfo.GetStartPosition();
        while( pos )
        {
            const char* pHost = NULL;
            CHostInfo* pInfo = NULL;
            m_mapHostInfo.GetNextAssoc( pos, pHost, (void*&)pInfo );
            if( pInfo->m_tvExpire.tv_sec <= tvNow.tv_sec )
            {
                m_mapHostInfo.RemoveKey( pHost );
                delete pInfo;
            }
        }
    }
}

static BYTE s_byQueryTmpl[] = {
    0x00, 0x00,     // Query ID (fill this in)
    0x01, 0x00,     // QR=0, OPCODE=0, AA=0, TC=0, RD=1, RA=0, Z=0, RCODE=0
    0x00, 0x01,     // QD count
    0x00, 0x00,     // AN count
    0x00, 0x00,     // NS count
    0x00, 0x00      // AR count
};

void CResolver::SendQuery( UINT16 qtype, UINT16 qid, CHostQuery* pQuery, const char* pHost )
{
    // Create the query buffer
    BYTE buf[512];
    BYTE* p = buf;
    size_t len = 512;

    // Encode the header
    memcpy( p, s_byQueryTmpl, sizeof(s_byQueryTmpl) );
    *(p+0) = (qid >> 8); *(p+1) = (qid & 0xFF);
    p += sizeof(s_byQueryTmpl);
    len -= sizeof(s_byQueryTmpl);

    // Encode the hostname
    if( !EncodeName( pHost, p, len ) )
    {
        return;
    }
    if( len < 4 )
    {
        return;
    }

    // qtype, qclass
    *(p+0) = qtype >> 8; *(p+1) = qtype & 0xFF;
    *(p+2) = 0; *(p+3) = 1;
    p += 4; len -= 4;

    struct sockaddr_in* paddr = &g_addrNameservers[ pQuery->m_nCurServer ];

    sendto( m_sock, (char*)buf, 512-len, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in) );
}

void CResolver::SendResponse( int res, const char* pHost, struct in_addr addr )
{
    struct gethost_response resp;
    resp.result = res;
    strcpy( resp.name, pHost );
    resp.addr = addr;
    write( m_writefd, &resp, sizeof(resp) );
}

void CResolver::AddHostEntry( struct timeval tvExpire, const char* szHost, struct in_addr addr )
{

    CHostInfo* pInfo = new CHostInfo( szHost );
    pInfo->m_tvExpire = tvExpire;
    pInfo->m_addr = addr;
    m_mapHostInfo.SetAt( szHost, (void*)pInfo );
}

static BOOL
parse_tokval( char* buf, const char* tok, char** ppval )
{
    BOOL rv = FALSE;
    int toklen = strlen(tok);
    if( !strncmp( buf, tok, toklen ) && isspace(buf[toklen]) )
    {
        char* pval = buf+toklen;
        while( isspace(*pval) ) pval++;
        if( *pval )
        {
            *ppval = pval;
            rv = TRUE;
        }
    }
    return rv;
}

static BOOL
parse_ip4( char* buf, struct in_addr* paddr, char** ppend )
{
    BOOL rv = FALSE;
    int len = 0;
    while( buf[len] && (isdigit(buf[len]) || buf[len] == '.') ) len++;
    if( len < 16 && isspace(buf[len]) )
    {
        if( inet_aton( buf, paddr ) != 0 )
        {
            *ppend = buf+len;
            rv = TRUE;
        }
    }
    return rv;
}

void CResolver::ReadConfig( void )
{
    FILE* fp;
    char linebuf[1024];
    char* pval;
    char* p;

    g_nSearchDomains = 0;
    g_nNameservers = 0;

    fp = fopen( "/etc/resolv.conf", "r" );
    if( fp != NULL )
    {
        while( fgets( linebuf, sizeof(linebuf), fp ) != NULL )
        {
            if( parse_tokval( linebuf, "nameserver", &pval ) )
            {
                struct sockaddr_in addr;
                p = pval;
                while( *p && !isspace(*p) ) p++;
                *p = '\0';
                if( g_nNameservers < 3 && inet_aton( pval, &addr.sin_addr ) != 0 )
                {
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(53);
                    g_addrNameservers[g_nNameservers++] = addr;
                }
            }
            else if( parse_tokval( linebuf, "domain", &pval ) )
            {
                p = pval;
                while( *p && !isspace(*p) ) p++;
                *p = '\0';
                g_nSearchDomains = 1;
                strcpy( g_szSearchDomains[0], pval );
            }
            else if( parse_tokval( linebuf, "search", &pval ) )
            {
                while( *pval )
                {
                    p = pval;
                    while( *p && !isspace(*p) )
                    {
                        p++;
                    }
                    if( g_nSearchDomains < 6 )
                    {
                        strncpy( g_szSearchDomains[g_nSearchDomains], pval, p-pval );
                        g_szSearchDomains[g_nSearchDomains][p-pval] = '\0';
                        g_nSearchDomains++;
                    }
                    pval = p;
                    while( *pval && isspace(*pval) )
                    {
                        pval++;
                    }
                }
            }
        }
        fclose( fp );
    }

    fp = fopen( "/etc/hosts", "r" );
    if( fp != NULL )
    {
        while( fgets( linebuf, sizeof(linebuf), fp ) != NULL )
        {
            struct in_addr addr;
            if( parse_ip4( linebuf, &addr, &pval ) )
            {
                while( *pval )
                {
                    char name[MAX_HOST_LEN];
                    while( isspace(*pval) ) pval++;
                    if( !*pval )
                    {
                        break;
                    }
                    p = pval;
                    while( *p && !isspace(*p) )
                    {
                        p++;
                    }
                    if( p - pval >= MAX_HOST_LEN )
                    {
                        fprintf( stderr, "Warning: host name too long in /etc/hosts: '%s'", linebuf );
                        break;
                    }
                    if( p > pval )
                    {
                        struct timeval tvinf;
                        tvinf.tv_sec = tvinf.tv_usec = 0x7FFFFFFF;

                        memcpy( name, pval, p-pval );
                        name[p-pval] = '\0';
                        AddHostEntry( tvinf, name, addr );
                        pval = p;
                    }
                }
            }
        }

        fclose( fp );
    }

    // If no nameservers are specified, use localhost
    if( g_nNameservers == 0 )
    {
        struct in_addr addr;
        inet_aton( "127.0.0.1", &addr );
        g_addrNameservers[0].sin_family = AF_INET;
        g_addrNameservers[0].sin_addr.s_addr = addr.s_addr;
        g_addrNameservers[0].sin_port = htons(53);
        g_nNameservers = 1;
    }
}

BOOL CResolver::EncodeName( const char* szName, BYTE*& rpbuf, size_t& rlen )
{
    while( *szName && rlen > 0 )
    {
        const char* pLabel = szName;
        BYTE nLabelLen = 0;
        BYTE nOverLen = min( 64, rlen-1 );
        while( *szName && '.' != *szName && nLabelLen < nOverLen )
        {
            nLabelLen++;
            szName++;
        }
        if( nLabelLen == nOverLen )
        {
            return FALSE;
        }
        *rpbuf++ = nLabelLen;
        rlen--;
        memcpy( rpbuf, pLabel, nLabelLen );
        rpbuf += nLabelLen;
        rlen -= nLabelLen;
        if( *szName == '.' )
        {
            szName++;
        }
    }
    if( rlen < 1 )
    {
        return FALSE;
    }
    *rpbuf++ = 0;
    rlen--;

    return TRUE;
}

BOOL CResolver::ParseQuestionHeader( dns_qr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos )
{
    char szHost[MAX_HOST_LEN];
    UINT16 usTmp;

    if( ! DecodeName( szHost, pbuf, buflen, rpos ) )
    {
        return FALSE;
    }
    if( rpos+4 > buflen )
    {
        return FALSE;
    }

    strcpy( phdr->szHost, szHost );
    memcpy( &usTmp, pbuf+rpos, 2 ); rpos += 2;
    phdr->qtype = ntohs( usTmp );
    memcpy( &usTmp, pbuf+rpos, 2 ); rpos += 2;
    phdr->qclass = ntohs( usTmp );
    if( phdr->qclass != CL_IN )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CResolver::ParseAnswerHeader( dns_rr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos )
{
    char szHost[MAX_HOST_LEN];
    UINT16 usTmp;
    UINT32 ulTmp;

    if( ! DecodeName( szHost, pbuf, buflen, rpos ) )
    {
        return FALSE;
    }
    if( rpos+10 > buflen )
    {
        return FALSE;
    }

    memcpy( &usTmp, pbuf+rpos, 2 ); rpos += 2;
    phdr->rtype = ntohs( usTmp );
    memcpy( &usTmp, pbuf+rpos, 2 ); rpos += 2;
    phdr->rclass = ntohs( usTmp );
    memcpy( &ulTmp, pbuf+rpos, 4 ); rpos += 4;
    phdr->ttl = ntohl( ulTmp );
    memcpy( &usTmp, pbuf+rpos, 2 ); rpos += 2;
    phdr->rdlen = ntohs( usTmp );
    if( phdr->rclass != CL_IN )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CResolver::DecodeName( char* pname, const BYTE* pbuf, size_t buflen, size_t& rpos )
{
    size_t pos = rpos;
    size_t namelen = 0;
    HX_ASSERT( buflen > 0 && buflen <= 512 && pos < buflen );

    BOOL bHasPtr = FALSE;
    while( pbuf[pos] )
    {
        UINT8 len = pbuf[pos];
        if( !(len & 0xC0) )
        {
            // Label
            pos++;
            if( len >= buflen-pos || len+namelen > MAX_HOST_LEN-2 )
            {
                return FALSE;
            }
            memcpy( pname, pbuf+pos, len );
            pos += len;
            *(pname+len) = '.';
            len++;
            pname += len;
            namelen += len;
            if( !bHasPtr )
            {
                rpos += len;
            }
        }
        else
        {
            // Pointer
            if( (len & 0xC0) != 0xC0 || pos > buflen-2 )
            {
                return FALSE;
            }
            pos = (UINT16)(pbuf[pos] & 0x3F)*256 + (UINT16)(pbuf[pos+1]);
            if( pos >= buflen-1 )
            {
                return FALSE;
            }
            if( !bHasPtr )
            {
                rpos += 2;
            }
            bHasPtr = TRUE;
        }
    }
    if( !namelen )
    {
        return FALSE; //XXX: is root domain
    }
    *(pname-1) = '\0';
    if( !bHasPtr )
    {
        rpos++;
    }
    return TRUE;
}

/*****************************************************************************
 *
 * And finally, the main loop
 *
 *****************************************************************************/

// The original resolver using gethostbyname(3)
void
ResolverMUX::do_hostnames( int readfd, int writefd )
{
    int n;
    struct gethost_request req;
    struct gethost_response resp;

    while( (n = read( readfd, &req, sizeof(req) )) == sizeof(req) )
    {
        req.name[MAX_HOST_LEN-1] = '\0'; // Make absolutely sure that name is terminated
        strcpy( resp.name, req.name );

        hostent* phe = gethostbyname( req.name );
        if( phe != NULL )
        {
            // Don't cast phe->h_addr to (struct in_addr*) due to alignment issues
            memcpy( &resp.addr, phe->h_addr, sizeof(resp.addr) );
            resp.result = 0;
        }
        else
        {
            resp.addr.s_addr = INADDR_NONE_U;
            resp.result = -1;
        }
        write( writefd, &resp, sizeof(resp) );
    }
}

// New resolver using internal code
void
ResolverMUX::do_hostnames_internal( int readfd, int writefd )
{
    int n;
    int rc;
    int sock;
    int fdmax;
    fd_set fdsread;
    fd_set fds;
    struct timeval tvnow;
    struct timeval tvnext;
    struct timeval tvtimer;
    struct gethost_request req;
    struct gethost_response resp;

    gettimeofday( &g_tvLastPrune, NULL );

    CResolver resolver( writefd );
    sock = resolver.GetSocket();
    if( sock == -1 )
    {
        fprintf( stderr, "FATAL: ResolverMUX::do_hostnames_internal: cannot "
	    "get resolver socket\n" );
        return;
    }

    // Setup for select()
    fdmax = 0;
    FD_ZERO( &fds );
    FD_SET( readfd, &fds );
    fdmax = max( fdmax, readfd );
    FD_SET( sock, &fds );
    fdmax = max( fdmax, sock );

    gettimeofday( &tvnext, NULL );
    tvtimer.tv_sec = 0;
    tvtimer.tv_usec = 1000*100;
    tv_add( tvnext, tvtimer );

    for(;;)
    {
        fdsread = fds;
        gettimeofday( &tvnow, NULL );
        tvtimer = tvnext;
        tv_sub( tvtimer, tvnow );
        if( tvtimer.tv_sec < 0 )
        {
            // We are late so reset timeout to zero
            tvtimer.tv_sec = tvtimer.tv_usec = 0;
        }
        rc = select( fdmax+1, &fdsread, NULL, NULL, &tvtimer );
        if( rc < 0 && errno != EINTR )
        {
            fprintf( stderr, "FATAL: ResolverMUX::do_hostnames_internal: "
		"select() failed, err = %i\n", errno );
            return;
        }
        else if( rc == 0 )
        {
            // Timeout
            resolver.OnTimer();

            // Bump tvnext by 100msec
            tvtimer.tv_sec = 0;
            tvtimer.tv_usec = 1000*100;
            tv_add( tvnext, tvtimer );
        }
        else if( rc > 0 )
        {
            if( FD_ISSET( readfd, &fdsread ) )
            {
                n = read(readfd, &req, sizeof(req));
                if (n <= 0 && errno == EINTR)
                {
                    microsleep(10000);
                    continue;
                }
                
                if (n != sizeof(req))
                {
                    fprintf(stderr, 
			    "FATAL: ResolverMUX::do_hostnames_internal: read() "
			    "returned %i (errno=%d)\n", n, errno);
                    return;
                }
                resolver.GetHost( req.name );
            }
            if( FD_ISSET( sock, &fdsread ) )
            {
                resolver.OnReadReady();
            }
        }
    }
}

#endif /* _VXWORKS */
