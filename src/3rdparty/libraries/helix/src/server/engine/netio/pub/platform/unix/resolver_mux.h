/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: resolver_mux.h,v 1.3 2006/02/27 20:08:04 atin Exp $ 
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
#ifndef _RESOLVER_MUX_H
#define _RESOLVER_MUX_H

#include <sys/types.h>
#include <sys/time.h>

#include "hxmap.h"
#include "hxslist.h"

#define MAX_HOST_LEN        256

class ResolverMUXResp
{
public:
    virtual HX_RESULT GetHostByNameDone(HX_RESULT status, ULONG32 ulAddr) = 0;
};

class ResolverGateway : public IHXCallback
{
public:
    ResolverGateway( int readfd, int writefd, unsigned int reqmax, IHXAsyncIOSelection* );
    ~ResolverGateway( void );

    UINT32 Count( void );
    HX_RESULT AddRequest( const char* pHostname, ResolverMUXResp* pResp );

    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_(ULONG32,AddRef)  ( THIS );
    STDMETHOD_(ULONG32,Release) ( THIS );

    STDMETHOD(Func)             ( THIS );
    
private:
    int                     m_writeto;
    int                     m_readfrom;
    unsigned int            m_nRequestMax;
    CHXMapStringToOb        m_mapRequests;
    IHXAsyncIOSelection*   m_pSelect;
};

class ResolverMUX;

#if !defined _LINUX && defined PTHREADS_SUPPORTED
struct resolver_thread_args
{
    int process_number;
    ResolverMUX* resolver_mux;
    int readfd;
    int writefd;
};
#endif

class ResolverMUX
{
public:
    /*
     * Only need to pass a fork_func_t if you have special forking
     * needs like in the server.  If not, just pass NULL and I'll call
     * fork().
     * capacity is total amount allowed to be queued up per fork'd
     * resolver.
     * max is max number of fork'd resolvers.  If more requests than
     * max * capacity are outstanding you will get failures.
     */
    ResolverMUX( int capacity, int max, IHXServerFork* pFork,
                 IHXAsyncIOSelection* pSel, IHXErrorMessages* pErrMsg );
    ~ResolverMUX( void );
    
    HX_RESULT GetHostByName( const char* hostname, ResolverMUXResp* );

#if !defined _LINUX && defined PTHREADS_SUPPORTED
    HX_RESULT DoChild(int topipe, int frompipe);
    pthread_t MakeThread(resolver_thread_args* pArgs);
#endif

    static void do_hostnames( int readfd, int writefd );
    static void do_hostnames_internal( int readfd, int writefd );

private:
    int                     m_max;
    int                     m_capacity;
    int                     m_current;
    ResolverGateway**       m_pResolvers;
    IHXServerFork*         m_pFork;
    IHXAsyncIOSelection*   m_pSelect;
    IHXErrorMessages*      m_pErrMsg;
};

struct dns_hdr
{
    UINT16 id;
    UINT16 flags; // QR:1, OPCODE:4, AA:1, TC:1, RD:1, RA:1, Z:3, RCODE:4
    UINT16 qdcnt;
    UINT16 ancnt;
    UINT16 nscnt;
    UINT16 arcnt;
};
struct dns_qr_hdr
{
    char    szHost[MAX_HOST_LEN];
    UINT16  qtype;
    UINT16  qclass;
};
struct dns_rr_hdr
{
    UINT16  rtype;
    UINT16  rclass;
    UINT32  ttl;
    UINT16  rdlen;
};

class CHostQuery
{
private: // Unimplemented
    CHostQuery( void );
    CHostQuery( const CHostQuery& other );
    CHostQuery& operator=( const CHostQuery& other );

public:
    CHostQuery( const char* szHost )
    {
        strcpy( m_szHost, szHost );
        m_nCurDomain = 0;
        m_nCurServer = 0;
    }

    struct timeval  m_tvExpire;
    struct timeval  m_tvDelta;
    UINT            m_nRetries;
    char            m_szHost[MAX_HOST_LEN];
    int             m_nCurDomain;
    int             m_nCurServer;
};

class CResolver
{
private: // Unimplemented
    CResolver( void );
    CResolver( const CResolver& );
    CResolver& operator=( const CResolver& );

public:
    CResolver( int writefd );
    virtual ~CResolver( void );

    BOOL GetHost( const char* szHost );
    // BOOL GetHost( struct in_addr addr );

    inline int GetSocket( void ) { return m_sock; }
    void OnReadReady( void );
    void OnTimer( void );

protected:
    void SendQuery( UINT16 qtype, UINT16 qid, CHostQuery* pQuery, const char* pHost );
    void SendResponse( int res, const char* pHost, struct in_addr addr );

    void AddHostEntry( struct timeval tvExpire, const char* szHost, struct in_addr addr );
    void ReadConfig( void );

    BOOL EncodeName( const char* szName, BYTE*& rpbuf, size_t& rlen );

    BOOL ParseQuestionHeader( dns_qr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos );
    BOOL ParseAnswerHeader( dns_rr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos );
    BOOL DecodeName( char* pname, const BYTE* pbuf, size_t buflen, size_t& rpos );

    // Produce an arbitrary nonlinear sequence that cycles through all 16-bit values
    UINT16 NextQueryID( void ) { m_usQueryID = m_usQueryID*9+5; return m_usQueryID; }

protected:
    int                 m_sock;
    int                 m_writefd;
    UINT16              m_usQueryID;
    CHXMapStringToOb    m_mapHostInfo;      // Host name -> addr cache
    CHXMapLongToObj     m_mapHostQueries;   // Pending host queries
};

#endif /* _RESOLVER_MUX_H */
