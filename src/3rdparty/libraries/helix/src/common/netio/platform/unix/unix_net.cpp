/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unix_net.cpp,v 1.24 2007/07/06 20:43:56 jfinnecy Exp $
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

#if defined _LINUX && defined __GLIBC__ && 0
#define _JAVA_GREENTHREADS
#endif

// Java with green threads needs you to use the internal entry points
// for these system calls
#ifdef _JAVA_GREENTHREADS
#define READ ::__read
#define CONNECT ::__connect
#define RECVFROM ::__recvfrom
#else
#define READ ::read
#define CONNECT ::connect
#define RECVFROM ::recvfrom
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "hxcom.h"
#include "hxbuffer.h"
#include "timebuff.h"
#include "hxtick.h"
#include "netbyte.h"
#include "platform/unix/unix_net.h"             //      Our declaration
#include "hxheap.h"
#include "hxslist.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxstrutl.h"
#include "pckunpck.h"
#include <errno.h>
#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"

#ifdef _IRIX
#include <bstring.h>            //      used in FD_ZERO
#endif
// #include "netplay.h"         //      why?

#ifdef _AIX
#include <sys/select.h>
#endif

#include <sys/types.h> //for waitpid
#include <sys/wait.h>  //for waitpid

#include <fcntl.h>

#ifdef _UNIX_THREADS_SUPPORTED
#include "hxthread.h"
#endif  

int unix_net::InBlockingMode = 0;

// This thing doesn't get initialized if this is linked into a shared library
CHXSimpleList* unix_net::readers = NULL;
// So I've done a silly thing and labelled it so.
static HXBOOL silly_unix_hack_initialized = FALSE;

const int HOSTSTRUCTSIZE = sizeof( hostent ); 

unix_net::unix_net(IUnknown* pContext) 
    :conn(pContext)
{
    set_sock( INVALID_SOCKET );
    mLastError = HXR_OK;
    callRaConnect = 1;    
    bReadyToWrite = 0;
    m_SocketState = CONN_CLOSED;

    //Async DNS vars.
    CurrentAddr    = 0;
    m_DNSOnly      = FALSE;
    m_anDNSPipe[0] = nInvalidPipe;
    m_anDNSPipe[1] = nInvalidPipe;
    m_nChildProcID = 0;
    m_szPipeIP[0]  = '\0';

        
    read_pending = FALSE;
    m_lRefCount         = 0;
    // Don't allocate this yet. Not all unix_net instances are actually
    // used to read, and if one isn't, allocating here wastes memory.
    m_pInBuffer = NULL;

    m_bReuseAddr = FALSE;
    m_bReusePort = FALSE;
    m_pAsyncHost = NULL;
        
#ifdef _UNIX_THREADS_SUPPORTED
    m_pResolver  = NULL;
#endif        
}

unix_net::~unix_net() 
{
    m_SocketState = CONN_CLOSING;
    if ((get_sock() != INVALID_SOCKET) )       
    {
        ::close(get_sock());
        set_sock( INVALID_SOCKET );
    }
    m_SocketState = CONN_CLOSED;
    mConnectionOpen = 0;
    
    LISTPOSITION listpos = readers->Find(this);
    if(listpos)
    {
        readers->RemoveAt(listpos);
    }
    HX_VECTOR_DELETE(m_pInBuffer); 

    //If the DNS forked proccess is still running lets
    //kill it here.
    //Ignore any returned error, what would we do anyway?
    CleanUpChildProc();
    HX_VECTOR_DELETE(m_pAsyncHost);

#ifdef _UNIX_THREADS_SUPPORTED
    if( m_bThreadedDNS )
    {
        if(m_pResolver)
        {
            m_pResolver->Exit(0);
            HX_RELEASE( m_pResolver );
        }
        m_nResolved = 0;
    }
#endif  

}

void unix_net::CleanUpChildProc()
{
    //Close any open pipes as well.
    if( m_anDNSPipe[0] >= 0 )
    {
        ::close( m_anDNSPipe[0] );
        m_anDNSPipe[0] = nInvalidPipe;
    }
    
    if( m_anDNSPipe[1] >= 0 )
    {
        ::close( m_anDNSPipe[1] );
        m_anDNSPipe[1] = nInvalidPipe;
    }
    
    if( m_nChildProcID != 0 )
    {
        kill( m_nChildProcID, SIGKILL );
        // wait until child exits
        waitpid(m_nChildProcID, 0, 0);
        m_nChildProcID = 0;
    }
}

unix_net * unix_net::new_socket(IUnknown* pContext, UINT16 type)
{
    unix_net *c = NULL;
    
    if(!silly_unix_hack_initialized)
    {
        readers = new CHXSimpleList;
        silly_unix_hack_initialized = TRUE; 
    }
    
    switch(type)
    {
       case HX_TCP_SOCKET:
           c = new unix_TCP(pContext);
           readers->AddTail(c);
           break;
        
       case HX_UDP_SOCKET:
           c = new unix_UDP(pContext);
           readers->AddTail(c);
           break;
    }
    
    return(c);
}


// init_drivers() should do any network driver initialization here
// params is a pointer to a platform specfic defined struct that 
// contains an required initialization data

HX_RESULT unix_net::init_drivers(void *params)
{
    return(HXR_OK);
}


/*      close_drivers() should close any network drivers used by the program
        NOTE: The program MUST not make any other calls to the network drivers
        until init_drivers() has been called */

HX_RESULT unix_net::close_drivers(void *params)
{
    return(HXR_OK);
}

HX_RESULT 
unix_net::get_host_name(char *name, int namelen)
{
    if (::gethostname(name, namelen) == 0)
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

HX_RESULT 
unix_net::get_host_by_name(char *name, REF(struct hostent*) pHostent)
{
    if ((pHostent = ::gethostbyname(name)) != 0)
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }    
}

HX_RESULT unix_net::host_to_ip_str( char *host, char *ip, UINT32 ulIPBufLen)
{
    HX_RESULT           theErr = HXR_OK;
    ULONG32             dwAddress;
    struct sockaddr_in  rInAddress;
    struct hostent*     pHostEntry;

    //  Let's look for this in the cache first
    if (conn::is_cached( host, &dwAddress))
    {
        //Found it, copy the 32bit address into rInAddress
        //w/o calling memcpy()
        rInAddress.sin_addr.s_addr  = dwAddress;
    }
    else
    { 
        // Do DNS on the host name
        if (!(pHostEntry = gethostbyname( host )))
        {
            // Error
            theErr = HXR_DNR;
        }

        // Return w/o attempting any copies if there's an error
        if (theErr != HXR_OK)
        {
            goto FuncExit;
        }
                
        // copy the ip address into rInAddress w/o calling memcpy()                    
        struct in_addr** ppAddr = (struct in_addr**)(pHostEntry->h_addr_list);
        memcpy(&rInAddress.sin_addr, ppAddr[0], sizeof(struct in_addr)); /* Flawfinder: ignore */
        
        // add to the dns cache
        conn::add_to_cache(host, (ULONG32) rInAddress.sin_addr.s_addr ); 
    }

    // Convert the ULONG32 IP address into a string and copy it into ip
    SafeStrCpy( ip, inet_ntoa( rInAddress.sin_addr ) , ulIPBufLen);

    // Single exit point
  FuncExit:

    return( theErr );
}

ULONG32 unix_net::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 unix_net::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *   reuse_addr/reuse_port has to be called before a sock binds.  however, 
 *   socket is not available until it binds as it is implemented.  So, set a 
 *   flag here and do the actual setsockopt right before a sock binds.
 *   Look in init_unix().
 */
HX_RESULT       
unix_net::reuse_addr(HXBOOL enable)
{
    m_bReuseAddr = enable;
    return HXR_OK;
}

HX_RESULT       
unix_net::reuse_port(HXBOOL enable)
{
    m_bReusePort = enable;
    return HXR_OK;
}

HX_RESULT unix_net::init_unix(UINT16 type, UINT32 local_addr, UINT16 port, 
                              UINT16 blocking)
{                                        
    int                s = INVALID_SOCKET;
    struct sockaddr_in addr;
#ifdef _BEOS
    char    mode = 1;
#else
    int     mode = 1;
#endif
    
    mLastError = HXR_OK;
#ifdef _BEOS
    s = socket(AF_INET, type, 0);
#else
    s = socket(PF_INET, type, 0);
#endif
    if (s < 0) 
    {
        mLastError = HXR_NET_SOCKET_INVALID;
        return mLastError;
    }
    
#if defined(HELIX_FEATURE_SERVER) && defined (_SOLARIS)
    //XXXDPL - Sockets open on FDs less than 256 count against fopen() limit on Solaris.
    //HACK: Reassign to a higher FD so that we can still log.
    if (s < 256)
    {
        int temp_fd = s;
        int dup_fd = fcntl(s, F_DUPFD, 256);
        if (dup_fd != INVALID_SOCKET)
        {
            s = dup_fd;
            close(temp_fd);
        }
    }
#endif

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&m_bReuseAddr, sizeof(m_bReuseAddr)) < 0) 
    {
        mLastError = HXR_NET_SOCKET_INVALID;
        goto sock_err;
    }

#if defined SO_REUSEPORT
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&m_bReusePort, sizeof(m_bReusePort)) < 0) 
    {
        mLastError = HXR_NET_SOCKET_INVALID;
        goto sock_err;
    }
#endif    
        
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(local_addr);
    addr.sin_port = htons(port);
    
    if (::bind(s, (sockaddr*)&addr, sizeof addr) < 0) 
    {
        mLastError = HXR_NET_SOCKET_INVALID;
        goto sock_err;
    }
#ifdef FIONBIO
    if (!blocking && ioctl(s, FIONBIO, &mode) < 0) 
#elif SO_NONBLOCK
        if (!blocking && setsockopt(s,SOL_SOCKET,SO_NONBLOCK,&mode,1)<0)
#else
            if (!blocking && ::fcntl(get_sock(), F_SETFL, ::fcntl(get_sock(), F_GETFL, 0) | O_NONBLOCK) < 0)
#endif
            {
                mLastError = HXR_NET_SOCKET_INVALID;
                goto sock_err;
            }
    DPRINTF(D_MSG,("unix_net::socket opened: %d\n", s));
    m_SocketState = CONN_OPEN;
    set_sock( s );

    return mLastError;
    
  sock_err:
    ::close(s);
    m_SocketState = CONN_NO_CONN;
    return mLastError;
}

HX_RESULT unix_net::connect(const char* host, UINT16 port, UINT16 blocking, ULONG32 ulPlatform)
{
    DPRINTF(D_MSG,("unix_net::connect(): b: %d\n", blocking));
    
    bReadyToWrite = 0;
    
    //Get a host at all?
    if(!host)                 
    {
        mLastError = HXR_DNR;
        return mLastError;
    }

    //Do we have a socket yet?
    if(get_sock() < 0)                 
    {
        mLastError = HXR_NET_SOCKET_INVALID;
        return mLastError;
    }
    
    if( blocking )
    {
        //Set our state.
        m_SocketState = CONN_DNS_INPROG;
        
        //check and see if we were passed a dot format IP address.
        memset(&m_sSockAddrIn, 0, sizeof(struct sockaddr_in));    
        char* pTemp = (char*)strrchr(host, '.');
        if (pTemp && atoi(pTemp + 1))
        {
            m_sSockAddrIn.sin_addr.s_addr = inet_addr(host);
            
            if ((ULONG32)m_sSockAddrIn.sin_addr.s_addr == (ULONG32)-1) 
            {
                mLastError = HXR_DNR;
                return mLastError;
            }
            //Set state to show we have the address ready to go.
            m_SocketState = CONN_CONNECT_INPROG;
        } 
        
        //do a blocking gethostbyname() call.
        if( m_SocketState == CONN_DNS_INPROG )
        { 
            struct hostent *h = gethostbyname(host);
            if (!h || !h->h_addr ) 
            {
                mLastError = HXR_DNR;
                DPRINTF(D_MSG,("unix_net::connect() HXR_INVALID_HOST\r\n"));
                CB_ConnectionComplete(0);
                return HXR_DNR;
            }
            
            struct in_addr** ppAddr = (struct in_addr**)(h->h_addr_list);
            memcpy(&m_sSockAddrIn.sin_addr, ppAddr[0], sizeof(struct in_addr)); /* Flawfinder: ignore */

            if (m_pAsyncHost != host)
            {
                HX_VECTOR_DELETE(m_pAsyncHost);
                m_pAsyncHost = ::new_string(host);
            }
            m_AsyncPort = port; 
        }
                        
        m_sSockAddrIn.sin_family = AF_INET;
        m_sSockAddrIn.sin_port = htons(port);

        // this stores info about current addr 
        CurrentAddr = m_sSockAddrIn.sin_addr.s_addr;

        if(CONNECT(get_sock(), (sockaddr*)&m_sSockAddrIn, sizeof(struct sockaddr_in)))
        {   
            if(!blocking && (errno == EWOULDBLOCK || errno == EINPROGRESS))
            {  
                mConnectionOpen = 1;
                nonblocking();
                CB_ConnectionComplete(1);
                return HXR_OK;
            }
            
            DPRINTF(D_MSG,("unix_net::connect() HXR_NET_CONNECT\r\n"));
            mLastError = HXR_NET_CONNECT;
            CB_ConnectionComplete(0);
            return HXR_NET_CONNECT;
        }       
                      
        mConnectionOpen = 1;
        nonblocking();
        m_SocketState = CONN_OPEN; // Rahul
        bReadyToWrite = 1;
        CB_ConnectionComplete(1);
        return HXR_OK;
    }//blocking

    return ConnectAsync( host, port );
}





#ifdef _UNIX_THREADS_SUPPORTED
void *unix_net::_ResolveIt( void* pArg )
{
    unix_net* pIt = (unix_net*)pArg;
    if(NULL !=pIt)
    {
        pIt->m_nResolved = 0;
        if( NULL != pIt->m_pAsyncHost )
        {
            struct hostent *h = gethostbyname(pIt->m_pAsyncHost);
            if( (NULL!=h) && (NULL!=h->h_addr) ) 
            {
                //Got good IP, send it back in dot format.
                const u_char *src = (u_char*)h->h_addr_list[0];
                static const char fmt[] = "%u.%u.%u.%u";
                sprintf(pIt->m_szPipeIP, fmt, src[0], src[1], src[2], src[3]); /* Flawfinder: ignore */
            }
            pIt->m_nResolved = 1;
        }
    }
    return NULL;
}
#endif





HX_RESULT unix_net::DoStartAsyncConn()
{
    DPRINTF(D_MSG,("unix_net::DoStartAsyncConn()\r\n"));
    m_SocketState = CONN_DNS_INPROG;
    
#ifdef _UNIX_THREADS_SUPPORTED
    //Make the thread if we haven't already.
    if( m_bThreadedDNS )
    {
        if( NULL == m_pResolver)
        {
            m_nResolved    = 0;
            CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pResolver, m_pContext);
            HX_ASSERT( m_pResolver );
        }
        //Set the state
        m_nResolved = 0;
        //Start the thread
        m_pResolver->CreateThread( _ResolveIt, (void*)this, 0 );
        //That is it!
        return( mLastError = HXR_WOULD_BLOCK );
    }
#endif
    //
    // Fork here to start the DNS process. The process will
    // be monitored by calls from proccess_idle to the method
    // 'CheckOnDNS()'.
    //
        
    //Create pipe to communicate between the child and parent.
    if ( 0 != pipe(m_anDNSPipe) )
    {
        //Can't create pipe.
        m_anDNSPipe[0] = nInvalidPipe;
        m_anDNSPipe[1] = nInvalidPipe;
        mLastError = HXR_GENERAL_NONET;
        return mLastError;
    }
    if( 0 > (m_nChildProcID = fork()))
    {
        //Error trying to fork.
        //What should we do?
        mLastError = HXR_GENERAL_NONET;
        m_SocketState = CONN_DNS_FAILED;
        CB_DNSComplete( 0 );
        return HXR_GENERAL_NONET;
    }
    
    if( 0 == m_nChildProcID )
    {
        //This is the child proc....

        //Close the read end of the pipe.
        if ( 0 != ::close( m_anDNSPipe[0]) )
        {
            //close, error. Kill this child proc.
            //This proc just exits. Return codes will come
            //from the parent proc. Just write a NULL to 
            //the pipe.
            ::write( m_anDNSPipe[1], "\0", 1 );
            exit(1);
        }

        m_anDNSPipe[0] = nInvalidPipe;

        //
        // Do the blocking DNS here
        //
        struct hostent *h = gethostbyname(m_pAsyncHost);
        if ( (NULL==h) || (NULL==h->h_addr) ) 
        {
            //Bad host or other nasty.
            //printf("Bad host or some other nasty\n");     
            //Send back \0 for now.
            ::write( m_anDNSPipe[1], "\0", 1 );
            if ( 0 != ::close( m_anDNSPipe[1]) )
            {
                //Unlikly, but if it does happen then we won't get
                //an EOF written to the pipe and the AsyncDNS will
                //never complete.
#ifdef _DEBUG            
                fprintf( stderr, "AsyncDNS can't close pipe. Disable AsyncDNS with NoAsyncDNS=1\n");
                fprintf( stderr, "If you have problems connecting.\n");
#endif            

            }
            exit(1);
        }
        //Got good IP, send it back in dot format.
        const u_char *src = (u_char*)h->h_addr_list[0];
        char szTmp[20]; /* Flawfinder: ignore */ //Just big enough to hold "255.255.255.255"
        static const char fmt[] = "%u.%u.%u.%u";
        SafeSprintf(szTmp, 20, fmt, src[0], src[1], src[2], src[3]);
        //printf("The Address is: %s\n", szTmp);        
        ::write( m_anDNSPipe[1], szTmp, strlen(szTmp)+1 );

        //Now close the pipe to ensure an EOF is written.
        //Close the read end of the pipe.
        if ( 0 != ::close( m_anDNSPipe[1]) )
        {
            //Unlikly, but if it does happend then we won't get
            //an EOF written to the pipe and the AsyncDNS will
            //never complete.
#ifdef _DEBUG            
            fprintf( stderr, "AsyncDNS can't close pipe. Disable AsyncDNS with NoAsyncDNS=1\n");
            fprintf( stderr, "If you have problems connecting.\n");
#endif            
        }
        _exit(0);
    }//m_nChildProcID

    //In parent....
    //Close the write end of the pipe.

    if ( 0 != ::close(m_anDNSPipe[1]) )
    {
        //close error.

        m_anDNSPipe[0] = nInvalidPipe;
        m_anDNSPipe[1] = nInvalidPipe;
        
        mLastError = HXR_GENERAL_NONET;
        return mLastError;
    }

    m_anDNSPipe[1] = nInvalidPipe;
    
    //We are now ready to read off of anDNSPipe[0] the
    //IP address of the host in dot format.

    //Set the pipes to non blocking.
    int flags;
    if( (flags = fcntl( m_anDNSPipe[0], F_GETFL, 0)) < 0 )
    {
        //Error, can't get the current flags for this pipe.
        mLastError = HXR_GENERAL_NONET;
        m_SocketState = CONN_DNS_FAILED;

        //Close the pipes and kill the child proc.
        CleanUpChildProc();

        //Report the disaster.
        CB_DNSComplete(0);
        return HXR_GENERAL_NONET;
    }
    flags |= O_NONBLOCK;
    if( fcntl( m_anDNSPipe[0], F_SETFL, flags) < 0 )
    {
        //Error, can't set the flags for this pipe.
        mLastError = HXR_GENERAL_NONET;
        m_SocketState = CONN_DNS_FAILED;
        
        //Close the pipes and kill the child proc.
        CleanUpChildProc();

        //Report the disaster.
        CB_DNSComplete(0);
        return HXR_GENERAL_NONET;
    }
    return( mLastError = HXR_WOULD_BLOCK );
}


HX_RESULT unix_net::CheckOnDNS()
{
    //Return this if nothing changes.
    mLastError = HXR_WOULD_BLOCK;

#ifdef _UNIX_THREADS_SUPPORTED
    if( m_bThreadedDNS )
    {
        AddRef();
        //Is the thread done yet?
        if( m_nResolved == 1 )
        {
            //Yeah, we are done. Wait on the thread.
            if( m_pResolver )
            {
                m_pResolver->Exit(0);
                HX_RELEASE(m_pResolver);
//                m_nResolved = 0;
            }

            //use it.
            if( strlen(m_szPipeIP) == 0 )
            {
                //badhost or DNS error. Close the pipe and go home.
                mLastError = HXR_DNR;
                CB_DNSComplete(0);
            }
            else
            {
                //Assume at this point that m_szPipeIP has a good IP in it.
                m_sSockAddrIn.sin_addr.s_addr = inet_addr(m_szPipeIP);
                if((ULONG32)m_sSockAddrIn.sin_addr.s_addr == (ULONG32)-1) 
                {
                    mLastError = HXR_DNR;
                    CB_DNSComplete(0);
                }
                else
                {
                    //Clear it for next time.
                    m_szPipeIP[0]='\0';
                    
                    //Set our current address...
                    CurrentAddr = m_sSockAddrIn.sin_addr.s_addr;
                    CB_DNSComplete(1);
                    mLastError = HXR_OK;
                }
            }
        }
        HX_RESULT res = mLastError;
        Release();
        return res;
    }
    
#endif
    //Keep checking and see if the the DNS lookup is
    //done or not.

    char      szBuff[256]; /* Flawfinder: ignore */
    int       status = 0;


    //If its there grab it.
    memset(szBuff, 0, 256);
    status = ::read(m_anDNSPipe[0], szBuff, 255);
    
    if( status > 0 )
    {
        strncat( m_szPipeIP, szBuff, status ); /* Flawfinder: ignore */
    }

    //Did we find EOF?
    if( 0 == status )
    {
        //At this point m_szPipeIP has a good IP address or
        //an error (NULL byte)
        

        //close the last pipe.
        ::close(m_anDNSPipe[0]); //Don't care about an error here.
        m_anDNSPipe[0] = nInvalidPipe;
        
        if( strlen(m_szPipeIP) == 0 )
        {
            //badhost or DNS error. Close the pipe and go home.
            mLastError = HXR_DNR;
            CB_DNSComplete(0);
        }
        else
        {
            //Assume at this point that m_szPipeIP has a good IP in it.
            m_sSockAddrIn.sin_addr.s_addr = inet_addr(m_szPipeIP);
            if((ULONG32)m_sSockAddrIn.sin_addr.s_addr == (ULONG32)-1) 
            {
                mLastError = HXR_DNR;
                CB_DNSComplete(0);
            }
            else
            {
                
                //Clear it for next time.
                m_szPipeIP[0]='\0';
                
                
                //grab the zombie child.
                ::waitpid( m_nChildProcID, NULL,  0 );
                m_nChildProcID = 0;
                
                //Set our current address...
                CurrentAddr = m_sSockAddrIn.sin_addr.s_addr;
                
                CB_DNSComplete(1);
                mLastError = HXR_OK;
            }
        }
    } else if( status<0 && EAGAIN!=errno )
    {
        //Just make sure the read returned EAGAIN and not
        //some other error on the pipe.
        
        m_szPipeIP[0]='\0';

        //Kill the DNS child proc and close the pipe we're going home.
        CleanUpChildProc();
        
        //Report the problem.
        mLastError = HXR_GENERAL_NONET;
        CB_DNSComplete(0);
    }

    return mLastError;
}

HX_RESULT unix_net::CheckForConnection()
{

    sockaddr_in cliaddr;
    HX_SOCKLEN_T addrlen = sizeof(sockaddr_in);
    //memset(&cliaddr, 0, addrlen);
    
    //Return this if nothing changes.
    mLastError = HXR_WOULD_BLOCK;
    
    int newSock = accept(&cliaddr, &addrlen);

    if ( newSock == INVALID_SOCKET )
    {
        // igno all errors...  r
        return HXR_WOULD_BLOCK;
    }
    else
    {
        unix_net* pNewConn = (unix_net*)conn::actual_new_socket(m_pContext, HX_TCP_SOCKET);
        pNewConn->AddRef();
        conn::add_connection_to_list(m_pContext, pNewConn);
        if ( pNewConn )
        {
            pNewConn->set_sock(newSock);
            if ( SUCCEEDED(pNewConn->connect_accept(&cliaddr)) )
            {
                mLastError = HXR_OK;
                CB_NewConnectionReady (TRUE, pNewConn);
            }
            else
            {
                CB_NewConnectionReady(FALSE, NULL);
            }
        }
        else
        {
            mLastError = HXR_OUTOFMEMORY;
        }
    }

    return mLastError;
}


// This method get's called by connect() in the case of an async request
// It doesn't however actually start the connection.  It just registers
// that we need to do the connection.  DoStartAsyncConn() will really do it.
HX_RESULT unix_net::ConnectAsync( const char* host, UINT16 port )
{
    //If we have our child(forked) process going on then
    //make sure we kill it and start a new one. Also, close
    //any open pipes.
    CleanUpChildProc();

    bReadyToWrite = 0;

    if (!host)                 
    {
        mLastError = HXR_DNR;
        return mLastError;
    }
        
    if (get_sock() == INVALID_SOCKET)
    {
        mLastError = HXR_NET_SOCKET_INVALID;
        return mLastError;
    }
        
    char* pTemp = (char*)strrchr(host, '.');

    if (pTemp && atoi(pTemp + 1))
    {   /* IP address. */
        m_sSockAddrIn.sin_addr.s_addr = inet_addr(host);
        
        if ((UINT)m_sSockAddrIn.sin_addr.s_addr == (UINT)-1) 
        {
            mLastError = HXR_DNR;
            CB_DNSComplete(0);
            return mLastError;
        }
        else
        {
            // this stores info about current addr 
            CurrentAddr = m_sSockAddrIn.sin_addr.s_addr;

            m_AsyncPort = port;
            if (m_pAsyncHost != host)
            {
                HX_VECTOR_DELETE(m_pAsyncHost);
                m_pAsyncHost = ::new_string(host);
            }
            CB_DNSComplete(1);
        }
    } 
    else if (conn::is_cached((char *)host,(ULONG32 *) &m_sSockAddrIn.sin_addr.s_addr))
    {
        // this stores info about current addr 
        CurrentAddr = m_sSockAddrIn.sin_addr.s_addr;
        m_AsyncPort = port;
        if (m_pAsyncHost != host)
        {
            HX_VECTOR_DELETE(m_pAsyncHost);
            m_pAsyncHost = ::new_string(host);
        }

        CB_DNSComplete(1);
    }
    else
    {
        //We are going to do Async DNS.....
        m_AsyncPort = port;
        if (m_pAsyncHost != host)
        {
            HX_VECTOR_DELETE(m_pAsyncHost);
            m_pAsyncHost = ::new_string(host);
        }
        m_SocketState = CONN_NO_CONN;
        return(DoStartAsyncConn());
    }

    return( HXR_OK );
}
 


// Once async DNS has commpleted then we'll call this guy to do the
// connection (again asynchronously).
void unix_net::ContinueAsyncConnect()
{
    DPRINTF(D_MSG,("unix_net::ContinueAsyncConnect() socket: %d\r\n", get_sock()));
    int nResult=0;
    nResult = CONNECT( get_sock(),
                       (sockaddr*)&m_sSockAddrIn,
                       sizeof(m_sSockAddrIn) );

    if( nResult != 0 && errno != EISCONN )
    {   
        if( errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY )
        {
            m_SocketState = CONN_CONNECT_INPROG;
        }
        else
        {
            mLastError = HXR_NET_CONNECT;
            m_SocketState = CONN_CONNECT_FAILED;
            nonblocking();
            DPRINTF(D_MSG,("unix_net::ContinueAsyncConnect() CONN_CONNECT_FAILED nResult: %d errno: %d(%s)\r\n", nResult, errno, strerror(errno)));
            CB_ConnectionComplete(0);
        }
    }
    else
    {
        mConnectionOpen = 1;
        CB_ConnectionComplete(1);
    }
    return;
}


// Called by the notifier to tell us that the DNS request completed
void unix_net::CB_DNSComplete( int iSuccess )
{
    ULONG32 ulNotUsed;
    
    mDNSDone = TRUE;

    //Put it into the cache if its good and not there.
    if( iSuccess && 
        m_pAsyncHost &&
        0 == conn::is_cached(m_pAsyncHost, &ulNotUsed)
        )
    {
        conn::add_to_cache(m_pAsyncHost, m_sSockAddrIn.sin_addr.s_addr);
    }


    if( TRUE == m_DNSOnly )
    {
        //This is an DNSOnly object. Don't do the connect.
        if (iSuccess)
        {
            mHostIPValid = TRUE;
            mHostIPAddr = get_addr();   
        }
        else
        {
            mHostIPValid = FALSE;
        }
    }
    else 
    {
        if(iSuccess)
        {
            m_SocketState = CONN_CONNECT_INPROG;
        }
        else
        {
            m_SocketState = CONN_DNS_FAILED;
        }
    }//TRUE==m_DNSOnly


    //
    // Handle any DNS notification callbacks.
    //
    if (mCallBack)
    {
        mCallBack->Func(DNS_NOTIFICATION, iSuccess);
    }

    if( FALSE==m_DNSOnly && iSuccess )
    {
        m_sSockAddrIn.sin_family = AF_INET;
        m_sSockAddrIn.sin_port = htons( m_AsyncPort );
        ContinueAsyncConnect();
    }
    return;
}

void unix_net::CB_NewConnectionReady(int iSuccess, unix_net* pConn)
{
    if ( mCallBack )
    {
        mCallBack->Func(ACCEPT_NOTIFICATION, iSuccess?TRUE:FALSE, (conn*)pConn);
    }
}

// Called by the notifier to tell us that the Connection completed
void unix_net::CB_ConnectionComplete( int iSuccess )
{
    DPRINTF(D_MSG,("CB_ConnectionComplete(%d)\r\n", iSuccess) );

    if (iSuccess)
    {
        m_SocketState = CONN_OPEN;
    }
    else
    {
        m_SocketState = CONN_CONNECT_FAILED;
    }
    
    if (mCallBack)
    {
        mCallBack->Func(CONNECT_NOTIFICATION, iSuccess?TRUE:FALSE);
    }
}

//      Called by the notifier when data ready for read/write
void unix_net::CB_ReadWriteNotification( int iType )
{
    DPRINTF(D_MSG,("CB_ReadWriteNotification()....\r\n") );
    //  Should do something here....
    if (mCallBack && (m_SocketState == CONN_OPEN))
    {
#if 0
        if (iType == FD_WRITE)
        {
            mCallBack->Func(WRITE_NOTIFICATION);
        }
        else if (iType == FD_READ)
        {
            mCallBack->Func(READ_NOTIFICATION);
        }
#endif // 0
        mCallBack->Func(READ_NOTIFICATION);
        
        //mCallBack->callback_task( HX_UDP_CALLBACK, NULL );
    }
}

void unix_net::CB_CloseNotification()
{
    DPRINTF(D_MSG,("CB_CloseNotification()....\r\n") );
    m_SocketState = CONN_CLOSED;
}


HX_RESULT unix_net::connect( sockaddr_in *addr )
{
    //XXXgfw Is this method used?????
    //XXXgfw If this is used, should there be a non-blocking
    //XXXgfw version of this? If so, we need to pass in a flag
    //XXXgfw or something. Eitherway, I can't see where this is
    //XXXgfw used at all. Only blocking for now I guess.
    
    if(CONNECT( get_sock(), (sockaddr*)addr, sizeof( addr ) ))
    {
        mLastError = HXR_NET_CONNECT;
        return mLastError;
    }                   
    
    mConnectionOpen = 1;
    return HXR_OK;     
}


HX_RESULT unix_net::write(void * buf, UINT16  *len) 
{
    int got;

    DPRINTF(D_MSG, ("unix_net::write() s: %d l: %d\n",get_sock(),*len));

    if (get_sock() == INVALID_SOCKET) 
    {
        // Not connected
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    if (m_SocketState != CONN_OPEN ) //&& m_SocketState != CONN_DNS_INPROG)
    {
        //      We won't be able to write anything here, so clear this
        //      we'll return why we didn't write anything.
        *len = 0;

        switch( m_SocketState )
        {
           case CONN_DNS_INPROG:
           case CONN_CONNECT_INPROG:
           case CONN_CLOSING:
               return( mLastError = HXR_WOULD_BLOCK );

           case CONN_CLOSED:
               return( mLastError = HXR_NET_SOCKET_INVALID );

           case CONN_NO_CONN:
               return( mLastError = HXR_NET_CONNECT );
               // return( DoStartAsyncConn() );

           case CONN_DNS_FAILED:
               return( mLastError = HXR_DNR );

           case CONN_CONNECT_FAILED:
               return( mLastError = HXR_NET_CONNECT );

           default:
               //      HUH???
               assert( 0 );
               return( mLastError = HXR_NET_READ );
        };
    }
    else
    {
#if defined(_BEOS)
        got = ::send( get_sock(), (void *)buf, *len, 0 );
#else
        got = ::write( get_sock(), (char *)buf, *len );
#endif
        if (got == -1)
        {       
            *len = 0;
            // Mask the "so what?" errors
            if (errno == EWOULDBLOCK || errno == EINPROGRESS) 
            {
                return HXR_WOULD_BLOCK;
            }
            else
            {
                mLastError = HXR_NET_WRITE;
                return mLastError;
            }
        }
        *len = got;
                
        return HXR_OK;
    }
}

HX_RESULT unix_net::WriteComplete   (char * Buffer, int length)
{
    int sent = 0;
    unsigned short cur_sent=0;

    while(sent < length)
    {
        cur_sent = length - sent;

        HX_RESULT ret = write(Buffer + sent, &cur_sent);
        if(ret != HXR_OK && ret != HXR_WOULD_BLOCK)
            break;
        sent += cur_sent;
    }

//      m_SocketState = CONN_NO_CONN;

    if(sent < length)
    {
        mLastError = HXR_NET_WRITE;
        return mLastError;
    }

    return HXR_OK;
}

int unix_net::ReadyToWrite()
{
    if(get_sock() < 0)
    {
        bReadyToWrite = 0;
        return bReadyToWrite;
    }

    if(bReadyToWrite)
        return 1;
        
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(get_sock(), &writefds);
        
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

#ifdef _HPUX
    // mwebb added HP-specifix lines (for the curious, do "man select")
    // also changed timeout call to be a non-blocking "poll"
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
#endif
    if(select(0, NULL, &writefds,NULL, &timeout) == 1)
        bReadyToWrite = 1;
    return bReadyToWrite;        
}

void unix_net::done (void)
{
    m_SocketState = CONN_CLOSING;
    if ((get_sock() != INVALID_SOCKET))
    {
        ::close(get_sock());
    }   
    set_sock( INVALID_SOCKET );
    m_SocketState = CONN_CLOSED;
    mConnectionOpen = 0;
    
    LISTPOSITION listpos = readers->Find(this);
    if(listpos)
    {
        readers->RemoveAt(listpos);
    }
}

inline HX_RESULT unix_net::listen (UINT16       backlog)
{
    if ( ::listen(get_sock(), backlog) != -1 )
    {
        m_SocketState = CONN_LISTENNING;
        mConnectionOpen = 0;
        return HXR_OK;
    }
    else
    {
        m_SocketState = CONN_NO_CONN;   
        mLastError = HXR_NET_CONNECT;
        return mLastError;
    }
}

/*
  inline HX_RESULT unix_net::blocking (void)
  {
  unsigned long nonblocking = 0;
  return ::ioctl(get_sock(), FIONBIO, (char*)&nonblocking); 
  }
        
  inline HX_RESULT unix_net::nonblocking (void) 
  {
  unsigned long nonblocking = 1;
  return ::ioctl(get_sock(), FIONBIO, (char*)&nonblocking); 
  }
*/

inline HX_RESULT unix_net::connect_accept(sockaddr_in *addr)
{
    CurrentAddr = addr->sin_addr.s_addr;
    memcpy(&m_sSockAddrIn, addr, sizeof(sockaddr_in)); /* Flawfinder: ignore */
    mConnectionOpen = 1;
    m_SocketState = CONN_OPEN;
    mLastError = HXR_OK;
    nonblocking();
    return HXR_OK;
}
/*                                                                               
                                                                                 inline HX_RESULT unix_net::accept (sockaddr_in *addr, UINT16 *addrlen)
                                                                                 {
                                                                                 return ::accept(get_sock(), (sockaddr*)addr, (int *)addrlen);
                                                                                 }

                                                                                 inline HX_RESULT unix_net::bind (sockaddr_in *addr)
                                                                                 {
                                                                                 return ::bind(get_sock(), (sockaddr*)addr, sizeof(addr));
                                                                                 }
*/

HX_RESULT unix_net::read(void * buf, UINT16 *len) 
{
    int             got;
    static int breakpoint = 0;

    assert( (PTR_INT)buf );
    assert( (PTR_INT)len );


    if (get_sock() == INVALID_SOCKET || !callRaConnect) 
    {
        // Not connected
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    //      Is the TCP socket actually connected yet?
    if (m_SocketState != CONN_OPEN)
    {
        //      No
        //      We won't be able to write anything here, so clear this
        //      we'll return why we didn't write anything.
        *len = 0;

        switch( m_SocketState )
        {
           case CONN_DNS_INPROG:
           case CONN_CONNECT_INPROG:
           case CONN_CLOSING:
               return( mLastError = HXR_WOULD_BLOCK );

           case CONN_CLOSED:
               return( mLastError = HXR_NET_SOCKET_INVALID );

           case CONN_NO_CONN:
               return( mLastError = HXR_NET_CONNECT );
               // return( DoStartAsyncConn() );

           case CONN_DNS_FAILED:
               return( mLastError = HXR_DNR );

           case CONN_CONNECT_FAILED:
               return( mLastError = HXR_NET_CONNECT );

           default:
               //      HUH???
               assert( 0 );
               return( mLastError = HXR_NET_READ );
        };
    }
    else
    {
        //      Now we can actually do the read
#ifdef _BEOS
        got = ::recv( get_sock(), (char *)buf, *len, 0 ); 
#else
        got = READ( get_sock(), (char *)buf, *len );
#endif

        // Did we get an error?
        if (got < 0 ) 
        {   
            *len = 0;

            //      Translate the error
            switch (errno)
            {
               case EWOULDBLOCK:
                   add_read_request();
                   return( mLastError = HXR_WOULD_BLOCK );

               case ECONNRESET:
                   return( mLastError = HXR_SERVER_DISCONNECTED );

               default:
                   return( mLastError = HXR_NET_READ );   // Error we don't know what to do about
            }
        }
        else if (got == 0) 
        {
            return (mLastError = HXR_SERVER_DISCONNECTED);
        }
        else
        {
            //      This should be our exit point for successful read
            DPRINTF(D_MSG, ("unix_net::read() s: %d l: %d\n",get_sock(),*len));
            *len = got;
            return( HXR_OK );
        }
    }
}

HX_RESULT unix_net::readfrom (REF(IHXBuffer*)   pBuffer,
                              REF(UINT32)       ulAddress,
                              REF(UINT16)       ulPort)
{
    HXLOGL4(HXLOG_NETW, "%p::unix_net::readfrom()", this);
    
    int                 got = 0;

    pBuffer = NULL;
    ulAddress = 0;
    ulPort = 0;

    // Allocate m_pInBuffer now, since we know we need it.
    if( m_pInBuffer == NULL )
    {
        m_pInBuffer = new char[TCP_BUF_SIZE];
    }

    if (get_sock() == INVALID_SOCKET || callRaConnect)
    {
        // Not connected
        HXLOGL4(HXLOG_NETW, "%p::unix_net::readfrom() - not connected", this);
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    //  Is the TCP socket actually connected yet?
    if (m_SocketState != CONN_OPEN)
    {
        HXLOGL4(HXLOG_NETW, "%p::unix_net::readfrom() - if (m_SocketState != CONN_OPEN)", this);
        //      No
        //      We won't be able to write anything here, so clear this
        //      we'll return why we didn't write anything.      
        switch( m_SocketState )
        {
           case CONN_DNS_INPROG:
           case CONN_CONNECT_INPROG:
           case CONN_CLOSING:
               return( mLastError = HXR_WOULD_BLOCK );

           case CONN_CLOSED:
               return( mLastError = HXR_NET_SOCKET_INVALID );

           case CONN_NO_CONN:
               return( mLastError = HXR_NET_CONNECT );
               // return( DoStartAsyncConn() );

           case CONN_DNS_FAILED:
               return( mLastError = HXR_DNR );

           case CONN_CONNECT_FAILED:
               return( mLastError = HXR_NET_CONNECT );

           default:
               //      HUH???
               assert( 0 );
               return( mLastError = HXR_NET_READ );
        }
    }
    else
    {
#if defined _AIX42
        //SCO's Unixware compiler will choke and die otherwise:
        size_t      fromlen = 0;
#elif defined _HPUX
        int fromlen = 0;
#else
        socklen_t   fromlen = 0;
#endif
        struct  sockaddr_in from;

        fromlen = sizeof(from);

        //      Now we can actually do the read
        got = RECVFROM( get_sock(), m_pInBuffer, TCP_BUF_SIZE, 0, (struct sockaddr*)&from, (HX_SOCKLEN_T *)&fromlen);
#if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
        UINT16 usRTPSeq       = 0;
        UINT32 ulRTPTimestamp = 0;
        if (got >= 8)
        {
            BYTE* pTmp     = m_pInBuffer;
            usRTPSeq       = ((pTmp[2] << 8) | pTmp[3]);
            ulRTPTimestamp = ((pTmp[4] << 24) | (pTmp[5] << 16) | (pTmp[6] << 8)  | pTmp[7]);
        }
        HXLOGL4(HXLOG_NETW, "%p::unix_net::readfrom() - recvfrom() returned %d (rtpSeq=%u,rtpTS=%lu)", this, got, usRTPSeq, ulRTPTimestamp);
#endif /* #if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL) */
        // Did we get an error?
        if (got <= 0 ) 
        {       
            //  Translate the error
            switch (errno)
            {
               case EWOULDBLOCK:
                   add_read_request();
                   return( mLastError = HXR_WOULD_BLOCK );

               case ECONNRESET:
                   return( mLastError = HXR_SERVER_DISCONNECTED );

               default:
                   return( mLastError = HXR_NET_READ );   // Error we don't know what to do about
            }
        }


        //      This should be our exit point for successful read
        CHXTimeStampedBuffer* pTimeBuffer = new CHXTimeStampedBuffer;
        pTimeBuffer->AddRef();
        pTimeBuffer->SetTimeStamp(HX_GET_TICKCOUNT_IN_USEC());
        pTimeBuffer->Set((UCHAR*)m_pInBuffer, got);
        pBuffer = (IHXBuffer*) pTimeBuffer;
        ulAddress = DwToHost(from.sin_addr.s_addr);
        ulPort = WToHost(from.sin_port);
            
        return( HXR_OK );
    }
}

unix_UDP::unix_UDP(IUnknown* pContext)
    :unix_net(pContext)
{
}

HX_RESULT unix_UDP::connect(const char* host, UINT16 port, UINT16 blocking, ULONG32 ulPlatform ) 
{
    blocking = GetAsyncDNSPref() ? 1 : blocking;

    HX_RESULT ret = HXR_OK;
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    {                
        if(ret == HXR_BLOCK_CANCELED)
            return ret;
              
        mLastError = HXR_NET_CONNECT;
        return mLastError;
    }

    ret = unix_net::connect(host, port, blocking, ulPlatform );
    if ( !ret )
        nonblocking();
        
    return ret;
}

HX_RESULT unix_UDP::connect(sockaddr_in * addr, UINT16 blocking) 
{

    blocking = GetAsyncDNSPref() ? 1 : blocking;

    HX_RESULT ret = HXR_OK;
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    { 
        if(ret == HXR_BLOCK_CANCELED)
            return ret;                                          
              
        mLastError = HXR_NET_CONNECT;
        return mLastError;
    }

    return unix_net::connect(addr);
}                                  

unix_TCP::unix_TCP(IUnknown* pContext)
    :unix_net(pContext)
{
}

HX_RESULT unix_TCP::connect(const char* host, UINT16 port, UINT16 blocking, ULONG32 ulPlatform ) 
{
    blocking = GetAsyncDNSPref() ? 1 : blocking;

    HX_RESULT ret = HXR_OK;
    
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    {    
        if(ret == HXR_BLOCK_CANCELED)
            return ret;                                          
        
        mLastError = HXR_NET_CONNECT;
        return mLastError;
    }
    ret = unix_net::connect(host, port, blocking, ulPlatform );
#if 0
    if ( !ret )
        nonblocking();
#endif
    return  ret;
}

HX_RESULT unix_TCP::connect(sockaddr_in * addr, UINT16 blocking) 
{
    blocking = GetAsyncDNSPref() ? 1 : blocking;

    HX_RESULT ret = HXR_OK;
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    { 
        if(ret == HXR_BLOCK_CANCELED)
            return ret;                                          
              
        mLastError = HXR_NET_CONNECT;
        return mLastError;
    }

    return unix_net::connect(addr);
}

HX_RESULT unix_TCP::listen(ULONG32 ulLocalAddr, UINT16 port,
                           UINT16 backlog, UINT16 blocking, 
                           ULONG32 ulPlatform) 
{
    // We will ignor the blockng parameter, and block for this
    // function but leave the socket in a non blocking state...
    HX_RESULT ret = HXR_OK;
    if ( get_sock() < 0 )
    {
        if ( ulLocalAddr == HX_INADDR_ANY )
            ret = init(INADDR_ANY, port, 1);
        else
            ret = init(ulLocalAddr, port, 1);
    }
    if ( FAILED(ret) )
    {
        if(ret == HXR_BLOCK_CANCELED)
            return ret;
        mLastError = HXR_NET_CONNECT;
        return mLastError;
    }
    if ( unix_net::listen(backlog) != HXR_OK)
    {
        ret = HXR_NET_CONNECT;
    }

    if ( SUCCEEDED(ret) )
    {
        ret = unix_net::nonblocking();
    }
    return ret;
}

HX_RESULT
unix_UDP::set_multicast()
{
#ifdef NO_MULTICAST
    return HXR_MULTICAST_UDP;
#endif /* NO_MULTICAST */
    INT32               ret;
    sockaddr_in         addr;
#if defined _AIX42
    size_t addr_len = sizeof addr;
#elif defined _HPUX || defined _OSF1
    int addr_len = sizeof addr;
#else
    socklen_t addr_len = sizeof addr;
#endif

    if (get_sock() == INVALID_SOCKET) 
    {
        // Not connected
        mLastError = HXR_NET_SOCKET_INVALID;
        return mLastError;
    }

    memset(&addr, 0, HX_SAFESIZE_T(addr_len));
    ret = getsockname(get_sock(), (sockaddr*)&addr, &addr_len);
    if (ret < 0)
    {
        return HXR_MULTICAST_UDP;
    }
/*
 * Linux and Solairs seem to return 127.0.0.1 from getsockname() which they
 * don't accept as a valid interface in setsockopt(IP_MULTICAST_IF)
 */
#if defined _LINUX || defined _SOLARIS
    addr.sin_addr.s_addr = INADDR_ANY;
#endif /* _LINUX || _SOLARIS */
#ifdef _BEOS
    // FIXME no multicast for BeOS
    return HXR_MULTICAST_UDP;
#else
    ret = ::setsockopt(get_sock(), IPPROTO_IP, IP_MULTICAST_IF,
                       (char*) &addr.sin_addr.s_addr,
                       sizeof (addr.sin_addr.s_addr));
    if (ret < 0)
    {
        return HXR_MULTICAST_UDP;
    }
    return HXR_OK;
#endif /* _BEOS */
}

HX_RESULT
unix_UDP::set_multicast_ttl(unsigned char ttl)
{
#ifdef NO_MULTICAST
    return HXR_MULTICAST_UDP;
#endif /* NO_MULTICAST */
    INT32         ret;
    unsigned char       ttl_proxy;
    ttl_proxy = ttl;

#ifdef _BEOS
    // FIXME no multicast for BeOS
    return HXR_MULTICAST_UDP;
#else
    if (get_sock() == INVALID_SOCKET) 
    {
        // Not connected
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }
    
    ret = ::setsockopt(get_sock(), IPPROTO_IP, IP_MULTICAST_TTL,
                       (char*) &ttl_proxy, sizeof (ttl_proxy));
    if (ret < 0)
    {
        return HXR_MULTICAST_UDP;
    }
    return HXR_OK;
#endif /* _BEOS */
}

HX_RESULT unix_UDP::join_multicast_group(ULONG32 addr, ULONG32 if_addr)
{   
#if defined(_BEOS) 
    return HXR_NOTIMPL; 
#else
    
    int ret;
    ip_mreq multicast_group;

    if (get_sock() == INVALID_SOCKET) 
    {
        // Not connected
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    u_char ttl = 254; // As Per Sujal.

    ret = ::setsockopt(get_sock(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl, 
                       sizeof(ttl));
    if (ret == -1)
    {
        return HXR_MULTICAST_JOIN;
    }

    multicast_group.imr_multiaddr.s_addr = htonl(addr);
#ifdef _UNIX    
    multicast_group.imr_interface.s_addr = htonl(INADDR_ANY);
#elif    
    multicast_group.imr_interface.s_addr = htonl(if_addr);
#endif    

    ret = ::setsockopt(get_sock(), IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                       (char*) &multicast_group, 
                       sizeof (multicast_group));

    if (ret < 0)
    {
        return HXR_MULTICAST_JOIN;
    }

    return HXR_OK;
#endif
}
HX_RESULT unix_UDP::leave_multicast_group(ULONG32 addr, ULONG32 if_addr)
{
#if defined(_BEOS) 
    return HXR_NOTIMPL; 
#else

    
    int ret;
    ip_mreq     multicast_group;

    if (get_sock() == INVALID_SOCKET) 
    {
        // Not connected
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    multicast_group.imr_multiaddr.s_addr = htonl(addr);
    multicast_group.imr_interface.s_addr = htonl(if_addr);

    ret = ::setsockopt(get_sock(), IPPROTO_IP, IP_DROP_MEMBERSHIP,
                       (char*) &multicast_group, sizeof (multicast_group));
    if (ret == -1)
    {
        return HXR_GENERAL_MULTICAST;
    }
    return HXR_OK;
#endif
}

HX_RESULT 
unix_UDP::set_multicast_if(ULONG32 ulInterface)
{
    int ret;
    int s = get_sock();
    if(s == INVALID_SOCKET)
    {
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    unsigned long addr = htonl(ulInterface);
    ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, 
                     (char*)&addr, sizeof(addr));

    if(ret == -1)
        ret = HXR_GENERAL_MULTICAST;
    return ret;
}


HX_RESULT 
unix_UDP::set_broadcast(HXBOOL enable)
{
    int ret;
    int s = get_sock();
    if(s == INVALID_SOCKET)
    {
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }
    ret = setsockopt( s, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof(UINT32) );
    if(ret == -1)
        ret = HXR_BIND;
    return ret;
}

HX_RESULT 
unix_UDP::set_send_size(UINT32 send_size)
{
#ifdef _BEOS
    // Be is messed up
    return -1;
#else
    int s = get_sock();
    int ret = 0;
  again:
    ret = setsockopt(s, SOL_SOCKET, SO_SNDBUF,
                     (char*)&send_size, sizeof(INT32));
    if (ret < 0 && send_size > 8192)
    {
        send_size >>= 1;
        goto again;
    }
    return ret;
#endif
}


HX_RESULT unix_net::writeto(    void * buf,     
                                UINT16  *len, 
                                ULONG32 addr, 
                                UINT16 port)
{
    int got;

    sockaddr_in resend_addr;
    

    ::memset( &resend_addr, 0, sizeof( resend_addr ) );
    resend_addr.sin_family = AF_INET;
    resend_addr.sin_addr.s_addr = addr;
    resend_addr.sin_port = htons(port);

    got = sendto(get_sock(), (char *) buf, *len, 0, (sockaddr *)&resend_addr, sizeof (resend_addr)); 

    if (got == -1)
    {
        /*            int code;
         *len = 0;
         code = sockObj->HXWSAGetLastError();
         // Mask the "so what?" errors
         if (code == WSAEWOULDBLOCK || code == WSAEINPROGRESS) 
         {
         return HXR_WOULD_BLOCK;
         }
         else
         {
         mLastError = HXR_NET_WRITE;
         return mLastError;
         }
        */
    }

    *len = got;

    return HXR_OK;
}

ULONG32 unix_net::get_addr()
{
    return m_sSockAddrIn.sin_addr.s_addr;
}

UINT16 unix_net::get_local_port()
{
    sockaddr_in addr;
#if defined _AIX42
    size_t addr_len = sizeof addr;
#elif defined _HPUX || defined _OSF1
    int addr_len = sizeof addr;
#else
    socklen_t addr_len = sizeof addr;
#endif
    memset(&addr, 0, HX_SAFESIZE_T(addr_len));
    int ret = getsockname(get_sock(), (sockaddr*)&addr, &addr_len);
 
    return (ret < 0) ? -1 : WToHost(addr.sin_port);
}



HX_RESULT unix_net::dns_find_ip_addr(const char * host, UINT16 blocking)
{
    mHostIPValid = FALSE;
    mDNSDone     = TRUE;

    blocking = GetAsyncDNSPref() ? 1 : blocking;

    if(!host)                 
    {
        mLastError = HXR_DNR;
        return mLastError;
    }

    if(get_sock() < 0)                 
    {
        mLastError = HXR_NET_SOCKET_INVALID;
        return mLastError;
    }

    if (conn::is_cached((char *) host, &mHostIPAddr))
    {
        mHostIPValid  = TRUE;

        mLastError = HXR_OK;
        if (mCallBack) 
        {       
            mCallBack->Func(DNS_NOTIFICATION, TRUE);
        }
        return HXR_OK;
    }

    //Check for a dot format IP string.
    char* pTemp = (char*)strrchr(host, '.');
    if (pTemp && isdigit(*(pTemp + 1)))
    {   /* IP address. */
        
        mHostIPValid = FALSE;
        mHostIPAddr  = 0;
        mDNSDone     = TRUE;

        m_sSockAddrIn.sin_addr.s_addr = inet_addr(host);
        
        if ((UINT)m_sSockAddrIn.sin_addr.s_addr == (UINT)-1) 
        {
            mLastError = HXR_DNR;
            return mLastError;
        }

        mHostIPValid = TRUE;
        mHostIPAddr = m_sSockAddrIn.sin_addr.s_addr;
        conn::add_to_cache((char *)host, mHostIPAddr);

        if (mCallBack)   
        {
            mCallBack->Func(DNS_NOTIFICATION, TRUE);
        }
        return HXR_OK;
    } 

    if( blocking )
    { 
        hostent *pHost = gethostbyname((char *) host);
        
        mHostIPValid = FALSE;
        mHostIPAddr  = 0;
        mDNSDone     = TRUE;

        if( !pHost || !pHost->h_addr )
        {
            mLastError = HXR_DNR;
            return mLastError;
        }

        mHostIPValid = TRUE;
        
        struct in_addr** ppAddr = (struct in_addr**)(pHost->h_addr_list);
        mHostIPAddr = ppAddr[0]->s_addr;

        conn::add_to_cache((char *) host, mHostIPAddr); 
        if (mCallBack) 
        {       
            mCallBack->Func(DNS_NOTIFICATION, TRUE);
        }
        return HXR_OK;
    }
    else
    {
        //Non blocking code here.
        if (m_pAsyncHost != host)
        {
            HX_VECTOR_DELETE(m_pAsyncHost);
            m_pAsyncHost = ::new_string(host);
        }
        //Tell everyone we just want DNS and no connect.
        m_DNSOnly = TRUE;

        //Start up the non-blocking DNS.
        DoStartAsyncConn();
        
    }//blocking

    mLastError = HXR_OK;
    return mLastError;
}


HXBOOL unix_net::dns_ip_addr_found(HXBOOL* valid, ULONG32* addr)
{
    if (mDNSDone)
    {
        *valid = mHostIPValid;
        *addr  = mHostIPAddr;
        return TRUE;
    }
    else
        return FALSE;
}

void unix_net::add_read_request()
{
    read_pending = TRUE;
}

void 
unix_net::add_select(int *n, 
                     fd_set* readfds, 
                     fd_set* writefds, 
                     fd_set* exceptfds)
{
    unix_net* un;
    int sock;
    if(!silly_unix_hack_initialized)
    {
        readers = new CHXSimpleList;
        silly_unix_hack_initialized = TRUE; 
    }
    if(!unix_net::readers->IsEmpty())
    {
        CHXSimpleList::Iterator i;
        for(i = unix_net::readers->Begin(); i != unix_net::readers->End(); ++i)
        {
            un = (unix_net*)(*i);
            
            sock = un->get_sock();
            if(un->read_pending && sock >= 0)
            {
                if(sock > *n)
                    *n = sock + 1;
                FD_SET(sock, readfds);
            }
        }
    }
}

void 
unix_net::process_select(int n, 
                         fd_set* readfds, 
                         fd_set* writefds, 
                         fd_set* exceptfds)
{
    unix_net* un;
    int sock;
    if(!unix_net::readers->IsEmpty())
    {
        CHXSimpleList::Iterator i, next;
        for(i = unix_net::readers->Begin(); i != unix_net::readers->End(); i = next)
        {
            next = i;
            ++next;
            un = (unix_net*)(*i);
            
            sock = un->get_sock();
            if(sock >= 0)
            {
                if(FD_ISSET(sock, readfds))
                {
                    un->read_pending = FALSE;
                    un->CB_ReadWriteNotification(0);
                }
            }
        }
    }
}

void unix_net::process_idle()
{
    unix_net* un;
    fd_set read_fds;
    struct timeval tv;
    int maxfd = 0;
    int sock = INVALID_SOCKET;

    if(!silly_unix_hack_initialized)
    {
        readers = new CHXSimpleList;
        silly_unix_hack_initialized = TRUE; 
    }

    FD_ZERO(&read_fds);
    if(!unix_net::readers->IsEmpty())
    {
        CHXSimpleList::Iterator i;
        for(i = unix_net::readers->Begin(); i != unix_net::readers->End(); ++i)
        {
            un = (unix_net*)(*i);

            //Hold on to this reference.
            un->AddRef();

            //Check the progress of each unix_net object's async DNS
            //and connect.
            if( un->GetSocketState() == CONN_DNS_INPROG )
            {
                un->CheckOnDNS();
            }
            else if( un->GetSocketState() == CONN_CONNECT_INPROG )
            {
                un->ContinueAsyncConnect();
            }
            else if ( un->GetSocketState() == CONN_LISTENNING )
            {
                un->CheckForConnection();
            }
            
            sock = un->get_sock();
            if(un->read_pending && sock >= 0)
            {
                if(sock > maxfd)
                    maxfd = sock;
                FD_SET(sock, &read_fds);
            }

            //Release this reference.
            if( 0 == un->Release() )
            {
                //This object just got deleted. Make sure we adjust
                //the iterator back to a valid one because deleting
                //the object an iterator points to invalidates the 
                //iterator.
                
                //Make sure we don't iterate past the end with the
                //upcoming i++ in the for loop above.
                if( unix_net::readers->IsEmpty() )
                {
                    break;
                }

                //XXXgfw Can we do better then just setting it all
                //XXXgfw the way back to the begining?
                i = unix_net::readers->Begin();
            }
        }
    }

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    if(select(maxfd + 1, &read_fds, 0, 0, &tv) > 0)
    {
        if(!unix_net::readers->IsEmpty())
        {
            CHXSimpleList::Iterator i;
            for(i = unix_net::readers->Begin(); i != unix_net::readers->End(); ++i)
            {
                un = (unix_net*)(*i);
                
                sock = un->get_sock();
                if(un->read_pending && sock >= 0)
                {
                    if(FD_ISSET(sock, &read_fds))
                    {
                        un->read_pending = FALSE;
                        un->CB_ReadWriteNotification(0);
                    }
                }
            }
        }
    }
}
