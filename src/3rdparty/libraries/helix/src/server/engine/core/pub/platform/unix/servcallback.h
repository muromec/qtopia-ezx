/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servcallback.h,v 1.13 2005/09/02 20:21:10 seansmith Exp $ 
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

#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include "platform_config.h"

#if defined DEV_POLL_SUPPORT && defined _SOLARIS
#  include <sys/devpoll.h>
#endif // defined DEV_POLL_SUPPORT && defined _SOLARIS
#if defined LINUX_EPOLL_SUPPORT
#include <sys/epoll.h>
#include <errno.h>
#endif //LINUX_EPOLL_SUPPORT

enum SocketType
{
    HX_READERS,
    HX_WRITERS,
    HX_ACCEPTORS
};

class IO;

#if !defined(_NETBSD)
#include <pthread.h>
#endif

#include "hxtypes.h"
#include "hxcom.h"
#include "hxassert.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "fdset.h"
#include "asyncio.h"
#include "trycatch.h"
#include "timeval.h"
#include "globals.h"

#ifdef DEV_POLL_SUPPORT

typedef struct FDInfo
{
    IHXCallback* pReadCB;
    IHXCallback* pWriteCB;
    int           nPollMask;
} FDInfo;

#  ifdef _SOLARIS
#    include <sys/utsname.h> // for uname()
#  endif // _SOLARIS

#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT

typedef struct FDInfo
{
    IHXCallback* pReadCB;
    IHXCallback* pWriteCB;
    int           nPollMask;
} FDInfo;

#endif // LINUX_EPOLL_SUPPORT

class Callbacks {
public:
			Callbacks();
			~Callbacks();
    BOOL		init(int limit, BOOL bUseDevPoll);
    void		add(int fd, IHXCallback* cb, int type, BOOL bThreadSafe = FALSE);
    void		add(AsyncIO* i, IHXCallback* cb, int type, BOOL bThreadSafe = FALSE);
    void		remove(int type, AsyncIO* i);
    void		remove(int type, int fd);
    void		enable(AsyncIO* i, int type, BOOL bThreadSafe = FALSE);
    void		disable(AsyncIO* i, int type);
    void		enable(int fd, int type, BOOL bThreadSafe = FALSE);
    void		disable(int fd, int type);

    void		invoke_start();
    int 		invoke_one(UINT32* pNumReads=0, UINT32* pNumWrites=0);
    int 		invoke_one_ts();  // invoke one threadsafe callback
    void		prepare();
    void		clear();
    int			fd(int idx);
    int			empty();
    int			members();
    fd_set*		set();
    int			high();
    int			count;
    int*		memberlist;	// for converting fdset to a list of fds
    BOOL*       ts_memberlist;  // for tracking which fds are threadsafe 
    BOOL*       enlist;
    FD_set		enabled;	// set of enabled fds: arg to select
    FD_set		ready;		// set of ready fds: returned by select
    IHXCallback**	tab;

#ifdef DEV_POLL_SUPPORT
    int Poll(int timeout);
    FDInfo*             m_FDTable;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    int EventPoll(int timeout);
    FDInfo*             m_FDTable;
#endif // LINUX_EPOLL_SUPPORT

private:
    int*		invoke_members;
    int*                invoke_members_ts;
    int			invoke_n;
    int                 invoke_n_ts;
    BOOL		m_bCleanFinish;

#ifdef DEV_POLL_SUPPORT
    int                 m_nPollFD;
    struct pollfd*      m_pPollMemberList;
    struct pollfd*      m_pChangedFDs;
    struct pollfd*      m_pRemovedFDs;
    unsigned char*      m_bAlreadyChanged;
    unsigned char*      m_bAlreadyRemoved;
    size_t              m_ulNumChanges;
    size_t              m_ulNumRemoves;
    int                 m_nPos;
    int                 m_nTSPos;
    int                 m_nReadyFDs;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    int                 m_nPollFD;
    struct epoll_event* m_pEPollEvents;
    int                 m_nPos;
    int                 m_nTSPos;
    int                 m_nReadyFDs;
#endif // LINUX_EPOLL_SUPPORT
};


#ifdef LINUX_EPOLL_SUPPORT
#define EPOLL_SIZE_HINT 16384 //guess for now, make configurable
#define EPOLL_EVENTS_MAX 1000 //guess for now, make configurable
#endif


inline
Callbacks::Callbacks()
{
    count = 0;
    memberlist = 0;

#ifdef DEV_POLL_SUPPORT
    m_nPollFD = -1;
    m_pPollMemberList = 0;
    m_pChangedFDs = 0;
    m_pRemovedFDs = 0;
    m_ulNumRemoves = 0;
    m_ulNumChanges = 0;
    m_bAlreadyChanged = 0;
    m_bAlreadyRemoved = 0;
    m_nPos = 0;
    m_nReadyFDs = 0;
    m_nTSPos = 0;
    m_FDTable = 0;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    m_nPollFD = -1;
    m_pEPollEvents = 0;
    m_FDTable = 0;
    m_nPos = 0;
    m_nReadyFDs = 0;
    m_nTSPos = 0;
#endif // LINUX_EPOLL_SUPPORT
}

inline
Callbacks::~Callbacks()
{
#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        close(m_nPollFD);
        m_nPollFD = -1;
    }
    if (m_pPollMemberList) delete[] m_pPollMemberList;
    if (m_pChangedFDs) delete[] m_pChangedFDs;
    if (m_pRemovedFDs) delete[] m_pRemovedFDs;
        
    if (m_nPollFD < 0)
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        close(m_nPollFD);
        m_nPollFD = -1;
    }
    if (m_pEPollEvents) delete[] m_pEPollEvents;
    if (m_nPollFD < 0)
#endif // LINUX_EPOLL_SUPPORT
    {
        ready.destroy();
        enabled.destroy();
    }

    if (tab) delete[] tab;
    if (memberlist) delete[] memberlist;
    if (ts_memberlist) delete [] ts_memberlist;
    if (enlist) delete [] enlist;
}

inline BOOL
Callbacks::init(int limit, BOOL bUseDevPoll)
{

#ifdef DEV_POLL_SUPPORT
    // we try to open /dev/poll. If we can't we indicate it and
    // the caller should switch to not using /dev/poll
    if (bUseDevPoll)
    {

# ifdef _SOLARIS
        // if we're running on solaris 8 the patchlevel needs to be high
        // enough otherwise the box will crash
        struct utsname unameInfo;
        if (uname(&unameInfo) < 0)
        {
            perror("/dev/poll uname failed");
        }
        else
        {
            char* major = strchr(unameInfo.version, '_');
            char* minor = 0;
            long lMajor = strtol(major + 1, &minor, 10);
            long lMinor = strtol(minor + 1, 0, 10);

            if (strcmp(unameInfo.release, "5.8") == 0 &&
                (lMajor < 108528 || (lMajor >= 108528 && lMinor < 15)))
            {
                fprintf(stderr,
                        "WARNING: you should run with --ndp or upgrade to patchlevel 108528-15\n");
            }

            if ((m_nPollFD = open("/dev/poll", O_RDWR)) < 0)
            {
                perror("Failed to open /dev/poll");
                return FALSE;
            }
        }
#  endif // _SOLARIS

        m_pPollMemberList = new struct pollfd[limit];
        m_pChangedFDs = new struct pollfd[limit];
        m_pRemovedFDs = new struct pollfd[limit];
        m_bAlreadyChanged = new unsigned char[limit];
        memset(m_bAlreadyChanged, 0, limit);
        m_bAlreadyRemoved = new unsigned char[limit];
        memset(m_bAlreadyRemoved, 0, limit);

        // preset all the events to POLLREMOVE for the list that is
        // only going to be used for that
        for (int i = 0; i < limit; i++)
        {
            m_pRemovedFDs[i].events = POLLREMOVE;
        }
        count = limit;

        m_FDTable = new FDInfo[limit];
        memset(m_FDTable, 0, sizeof(FDInfo) * limit);

        ts_memberlist = new BOOL [limit];
        memset(ts_memberlist, FALSE, sizeof(BOOL) * limit);

        enlist = new BOOL [limit];
        memset(enlist, FALSE, sizeof(BOOL) * limit);

        return TRUE;
    }
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    // we try to initialize epoll. If we can't we indicate it and
    // the caller should switch to not using epoll
    if (bUseDevPoll)
    {
        //
        // XXXDC should check we're on kernel 2.6.8 or later.
        // Prior 2.6 kernels had a fd leak, and epoll was
        // not supported in normal 2.4.x kernels.
        //
        // The epoll_create argument is a hint as to the dimensions
        // of internal kernel structures that will be needed, it is
        // not a maximum.
        //
        if ((m_nPollFD = epoll_create(EPOLL_SIZE_HINT)) < 0)
        {
            perror("epoll_create error");
            return FALSE;
        }

        m_pEPollEvents = new struct epoll_event[limit];
        memset(m_pEPollEvents, 0, sizeof(epoll_event) * limit);
        count = limit;

        m_FDTable = new FDInfo[limit];
        memset(m_FDTable, 0, sizeof(FDInfo) * limit);

        ts_memberlist = new BOOL [limit];
        memset(ts_memberlist, FALSE, sizeof(BOOL) * limit);

        enlist = new BOOL [limit];
        memset(enlist, FALSE, sizeof(BOOL) * limit);

        return TRUE;
    }
#endif // LINUX_EPOLL_SUPPORT

    count = limit;

    tab = new IHXCallback*[limit];
    memset(tab, 0, sizeof(IHXCallback*) * limit);

    ts_memberlist = new BOOL [limit];
    memset(ts_memberlist, FALSE, sizeof(BOOL) * limit);

    memberlist = new int[limit];

    enlist = new BOOL [limit];
    memset(enlist, FALSE, sizeof(BOOL) * limit);

    ready.init(limit);
    enabled.init(limit);

    return TRUE;
}

inline void
Callbacks::enable(int fd, int type, BOOL bThreadSafe)
{
    if (fd >= count || fd < 0)
        return;

    enlist[fd] = TRUE;
    ts_memberlist[fd] = bThreadSafe;

#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        if (type == HX_WRITERS)
            m_FDTable[fd].nPollMask |= POLLOUT;
        else
            m_FDTable[fd].nPollMask |= POLLIN;

        // we're going to write the changes all at once before polling
        struct pollfd pollInfo;
        pollInfo.fd = fd;
        pollInfo.events = m_FDTable[fd].nPollMask;
        pollInfo.revents = 0;

        if (write(m_nPollFD, &pollInfo, sizeof(struct pollfd))
            != sizeof(struct pollfd))
        {
            perror("error writing FD to poll");
        }

        return;
    }
#endif // DEV_POLL_SUPPORT

#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        int nOldMask = m_FDTable[fd].nPollMask;
        if (type == HX_WRITERS)
            m_FDTable[fd].nPollMask |= EPOLLOUT;
        else ///HX_READERS and HX_ACCEPTORS
            m_FDTable[fd].nPollMask |= EPOLLIN;

        if (nOldMask != m_FDTable[fd].nPollMask)
        {
            struct epoll_event pollInfo;
            pollInfo.events = m_FDTable[fd].nPollMask | EPOLLERR | EPOLLHUP | EPOLLPRI;
            pollInfo.data.fd = fd;

            if (nOldMask)
            {
                // the fd is already in the epoll set so modify it
                if (epoll_ctl(m_nPollFD, EPOLL_CTL_MOD, fd, &pollInfo) < 0)
                {
                    //perror("error modifying FD with epoll_ctl");
                }
            }
            else
            {
                // add the fd to the epoll set
                if (epoll_ctl(m_nPollFD, EPOLL_CTL_ADD, fd, &pollInfo) < 0)
                {
                    //perror("error adding FD with epoll_ctl");
                }
            }
        }

        return;
    }
#endif // LINUX_EPOLL_SUPPORT

    enabled.add(fd);
}

inline void
Callbacks::enable(AsyncIO* i, int type, BOOL bThreadSafe)
{
    enable(i->fd(), type, bThreadSafe);
}

inline void
Callbacks::disable(int fd, int type)
{
    if (fd >= count || fd < 0)
        return;

#if !defined DEV_POLL_SUPPORT && !defined LINUX_EPOLL_SUPPORT
    enlist[fd] = FALSE;
    ts_memberlist[fd] = FALSE;
#endif

#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        // we remove the appropriate bit from the poll mask depending on
        // which of the fd's behaviour we are disabling
        if (type == -1) // disable all behaviour
            m_FDTable[fd].nPollMask = 0;
        else if (type == HX_WRITERS)
            m_FDTable[fd].nPollMask &= ~POLLOUT;
        else ///HX_READERS and HX_ACCEPTORS
            m_FDTable[fd].nPollMask &= ~POLLIN;

        struct pollfd pollInfo;
        pollInfo.fd = fd;
        pollInfo.events = POLLREMOVE;
        pollInfo.revents = 0;

        // this removes the fd from the poll array completely
        if (write(m_nPollFD, &pollInfo, sizeof(struct pollfd))
            != sizeof(struct pollfd))
        {
            perror("error removing FD from poll");
        }

        // if the fd still wants to be watched for the other behaviour, we
        // add it back to the poll array.
        if (m_FDTable[fd].nPollMask != 0)
        {
            pollInfo.events = m_FDTable[fd].nPollMask;
            if (write(m_nPollFD, &pollInfo, sizeof(struct pollfd))
                != sizeof(struct pollfd))
            {
                perror("error adding FD to poll");
            }
        }

        return;
    }
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        int nOldMask = m_FDTable[fd].nPollMask;

        // we remove the appropriate bit from the poll mask depending on
        // which of the fd's behaviour we are disabling
        if (type == -1) // disable all behaviour
            m_FDTable[fd].nPollMask = 0;
        else if (type == HX_WRITERS)
            m_FDTable[fd].nPollMask &= ~EPOLLOUT;
        else
            m_FDTable[fd].nPollMask &= ~EPOLLIN;

        if (nOldMask != m_FDTable[fd].nPollMask)
        {
            struct epoll_event pollInfo;
            pollInfo.events = m_FDTable[fd].nPollMask | EPOLLERR | EPOLLHUP;
            pollInfo.data.fd = fd;

            // if the fd still wants to be watched for other behaviour, we
            // leave it in the epoll set and modify its mask.
            if (m_FDTable[fd].nPollMask != 0)
            {
                if (epoll_ctl(m_nPollFD, EPOLL_CTL_MOD, fd, &pollInfo) < 0)
                {
                    perror("error modifying FD with epoll_ctl");
                }
            }
            // this removes the fd from the epoll set completely
            else
            {
                if (nOldMask != 0 && epoll_ctl(m_nPollFD, EPOLL_CTL_DEL, fd, &pollInfo) < 0)
                {
                    //perror("error removing FD with epoll_ctl");
                }
            }
        }

        return;
    }
#endif // LINUX_EPOLL_SUPPORT

    enabled.remove(fd);
}

inline void
Callbacks::disable(AsyncIO* i, int type)
{
    disable(i->fd(), type);
}

inline void
Callbacks::add(int fd, IHXCallback *cb, int type, BOOL bThreadSafe)
{
    ASSERT(fd >= 0);

    if (fd >= count)
	return;

#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        // add the appropriate callback
        if (type == HX_WRITERS)
        {
            m_FDTable[fd].pWriteCB = cb;
        }
        else
        {
            m_FDTable[fd].pReadCB = cb;
        }
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        // add the appropriate callback
        if (type == HX_WRITERS)
        {
            m_FDTable[fd].pWriteCB = cb;
        }
        else
        {
            m_FDTable[fd].pReadCB = cb;
        }
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        tab[fd] = cb;
    }
 
    ts_memberlist[fd] = bThreadSafe;
    enable(fd, type, bThreadSafe);
}

inline void
Callbacks::add(AsyncIO* i, IHXCallback *cb, int type, BOOL bThreadSafe)
{
    add(i->fd(), cb, type, bThreadSafe);
}

inline void
Callbacks::remove(int type, int fd)
{
    HX_ASSERT(fd >= 0);
    if (fd >= count)
	return;

    disable(fd, type);

#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        if (type == -1)
        {
            m_FDTable[fd].pWriteCB = 0;
            m_FDTable[fd].pReadCB = 0;
        }
        else if (type == HX_WRITERS)
        {
            m_FDTable[fd].pWriteCB = 0;
        }
        else
        {
            m_FDTable[fd].pReadCB = 0;
        }
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        if (type == -1)
        {
            m_FDTable[fd].pWriteCB = 0;
            m_FDTable[fd].pReadCB = 0;
        }
        else if (type == HX_WRITERS)
        {
            m_FDTable[fd].pWriteCB = 0;
        }
        else
        {
            m_FDTable[fd].pReadCB = 0;
        }
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        tab[fd] = 0;
        /*
         * remove from the ready list as well,
         * we are no longer interested in what this
         * fd has to tell us.
         */
        ready.remove(fd);
    }
}

inline void
Callbacks::remove(int type, AsyncIO* i)
{
    remove(type, i->fd());
}

inline void
Callbacks::prepare()
{
#ifdef DEV_POLL_SUPPORT
    // noop when using /dev/poll
    if (m_nPollFD < 0)
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    // noop when using epoll
    if (m_nPollFD < 0)
#endif // LINUX_EPOLL_SUPPORT
    {
        enabled.copy(&ready);
    }
}

inline int
Callbacks::members()
{
#ifdef DEV_POLL_SUPPORT
    HX_ASSERT(m_nPollFD < 0);
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    HX_ASSERT(m_nPollFD < 0);
#endif // LINUX_EPOLL_SUPPORT

    return ready.members(memberlist);
}

inline void
Callbacks::invoke_start()
{
    m_bCleanFinish = TRUE;

#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        m_nPos = 0;        
        m_nTSPos = 0;
        return;
    }
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        m_nPos = 0;        
        m_nTSPos = 0;
        return;
    }
#endif // LINUX_EPOLL_SUPPORT
    invoke_members = memberlist;
    invoke_members_ts = invoke_members;
    invoke_n = ready.members(memberlist);
    invoke_n_ts = invoke_n;
}

inline int
Callbacks::invoke_one_ts()
{
#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        if (m_nTSPos >= m_nReadyFDs)
            return 0;

	int nFD = m_pPollMemberList[m_nTSPos].fd;
	struct FDInfo* pFDInfo = &m_FDTable[nFD];
        while (!nFD ||
	       !ts_memberlist[nFD] ||
	       !pFDInfo->nPollMask || 
	       (!pFDInfo->pReadCB && !pFDInfo->pWriteCB))
        {
            m_nTSPos++;
            if (m_nTSPos >= m_nReadyFDs)
                return 0;
	    nFD = m_pPollMemberList[m_nTSPos].fd;
	    pFDInfo = &m_FDTable[nFD];
        }

        if (m_bCleanFinish)
        {
            m_bCleanFinish = FALSE;

	    struct pollfd* pPollInfo = &m_pPollMemberList[m_nTSPos];

            // remove the fd if it's gone bad
            if (pPollInfo->revents & (POLLNVAL | POLLERR | POLLHUP))
            {
                // fprintf(stderr, "Removing invalid fd %d\n", nFD);
		/*
		 * force a read to cause the connection to fail and begin
		 * cleanup. without this the connection will stay until a 
		 * message (like the keep alive) is written to the socket
		 * and it fails.
		 */
		if (pFDInfo->pReadCB)
		    pFDInfo->pReadCB->Func();
                remove(-1, pPollInfo->fd);
            }
            else
            {
                // execute the read callback if the fd is ready for it
		if (pPollInfo->revents & POLLIN && pFDInfo->pReadCB)
		{
		    pFDInfo->pReadCB->Func();
            }
                // execute the write callback if the fd is ready for it
		if (pPollInfo->revents & POLLOUT && pFDInfo->pWriteCB)
		{
		    pFDInfo->pWriteCB->Func();
		}
            }

            m_bCleanFinish = TRUE;
        }
        else
        {
            m_bCleanFinish = TRUE;
        }

        return m_nReadyFDs - m_nTSPos++;
    }
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        if (m_nTSPos >= m_nReadyFDs)
            return 0;

	int nFD = m_pEPollEvents[m_nTSPos].data.fd;
	struct FDInfo* pFDInfo = &m_FDTable[nFD];
        while (!nFD ||
	       !ts_memberlist[nFD] || 
	       !pFDInfo->nPollMask ||
	       (!pFDInfo->pReadCB && !pFDInfo->pWriteCB))
        {
            m_nTSPos++;
            if (m_nTSPos >= m_nReadyFDs)
                return 0;
	    nFD = m_pEPollEvents[m_nTSPos].data.fd;
	    pFDInfo = &m_FDTable[nFD];
        }

        if (m_bCleanFinish)
        {
            m_bCleanFinish = FALSE;

            struct epoll_event* pEvent = &m_pEPollEvents[m_nTSPos];

            // remove the fd if it's gone bad
            if ((pEvent->events & (EPOLLERR | EPOLLHUP)))
            {
                // fprintf(stderr, "Removing invalid fd %d\n", nFD);
		/*
		 * force a read to cause the connection to fail and begin
		 * cleanup. without this the connection will stay until a 
		 * message (like the keep alive) is written to the socket
		 * and it fails.
		 */
		if (pFDInfo->pReadCB)
		    pFDInfo->pReadCB->Func();
                remove(-1, pEvent->data.fd);
            }
            else
            {
                // execute the read callback if the fd is ready for it
                if (pEvent->events & EPOLLIN && pFDInfo->pReadCB)
                {
                    pFDInfo->pReadCB->Func();
                }
                // execute the write callback if the fd is ready for it
                if (pEvent->events & EPOLLOUT && pFDInfo->pWriteCB)
                {
                    pFDInfo->pWriteCB->Func();
                }
            }

            m_bCleanFinish = TRUE;
        }
        else
        {
            m_bCleanFinish = TRUE;
        }

        return m_nReadyFDs - m_nTSPos++;
    }
#endif // LINUX_EPOLL_SUPPORT

    if (invoke_n_ts <= 0)
    {
	return 0;
    }
    int i = *invoke_members_ts++;

again:
    if (!tab[i] || !ts_memberlist[i]  || !enlist[i])
    {
	invoke_n_ts--;
	if (invoke_n_ts > 0)
	{
	    i = *invoke_members_ts++;
	    goto again;
	}
	else
	{
	    return 0;
	}
    }

    if (m_bCleanFinish)
    {
	m_bCleanFinish = FALSE;
	tab[i]->Func();
	m_bCleanFinish = TRUE;
    }
    else
    {
	m_bCleanFinish = TRUE;
    }


    invoke_n_ts--;

    return invoke_n_ts + 1;
}

inline int
Callbacks::invoke_one(UINT32* pNumReads, UINT32* pNumWrites)
{
#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        if (m_nPos >= m_nReadyFDs)
            return 0;

	int nFD = m_pPollMemberList[m_nPos].fd;
	struct FDInfo* pFDInfo = &m_FDTable[nFD];
        while (!nFD ||
	       ts_memberlist[nFD] ||
	       !pFDInfo->nPollMask ||
	       !enlist[nFD] || 
	       (!pFDInfo->pReadCB && !pFDInfo->pWriteCB))
        {
            m_nPos++;
            if (m_nPos >= m_nReadyFDs)
                return 0;
	    nFD = m_pPollMemberList[m_nPos].fd;
	    pFDInfo = &m_FDTable[nFD];
        }

        if (m_bCleanFinish)
        {
            m_bCleanFinish = FALSE;
            
	    struct pollfd* pPollInfo = &m_pPollMemberList[m_nPos];

            // remove the fd if it's gone bad
            if (pPollInfo->revents & (POLLNVAL | POLLERR | POLLHUP))
            {
                // fprintf(stderr, "Removing invalid fd %d\n", nFD);
		/*
		 * force a read to cause the connection to fail and begin
		 * cleanup. without this the connection will stay until a 
		 * message (like the keep alive) is written to the socket
		 * and it fails.
		 */
		if (pFDInfo->pReadCB)
		    pFDInfo->pReadCB->Func();
                remove(-1, pPollInfo->fd);
            }
            else
            {
                // execute the read callback if the fd is ready for it
                if (pPollInfo->revents & POLLIN && pFDInfo->pReadCB)
                {
                    pFDInfo->pReadCB->Func();
                    (*pNumReads)++;
                }
                // execute the write callback if the fd is ready for it
                if (pPollInfo->revents & POLLOUT && pFDInfo->pWriteCB)
                {
		    pFDInfo->pWriteCB->Func();
                    (*pNumWrites)++;
                }
            }

            m_bCleanFinish = TRUE;
        }
        else
        {
            m_bCleanFinish = TRUE;
        }

        return m_nReadyFDs - m_nPos++;
    }
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        if (m_nPos >= m_nReadyFDs)
            return 0;

	int nFD = m_pEPollEvents[m_nPos].data.fd;
	struct FDInfo* pFDInfo = &m_FDTable[nFD];
        while (!nFD ||
	       ts_memberlist[nFD] ||
	       !pFDInfo->nPollMask ||
	       !enlist[nFD] || 
	       (!pFDInfo->pReadCB && !pFDInfo->pWriteCB))
        {
            m_nPos++;
            if (m_nPos >= m_nReadyFDs)
                return 0;
	    nFD = m_pEPollEvents[m_nPos].data.fd;
	    pFDInfo = &m_FDTable[nFD];
        }

        if (m_bCleanFinish)
        {
            m_bCleanFinish = FALSE;
            
            struct epoll_event* pEvent = &m_pEPollEvents[m_nPos];
            
            // remove the fd if it's gone bad
            if ((pEvent->events & (EPOLLERR | EPOLLHUP)))
            {
                // fprintf(stderr, "Removing invalid fd %d\n", nFD);
		/*
		 * force a read to cause the connection to fail and begin
		 * cleanup. without this the connection will stay until a 
		 * message (like the keep alive) is written to the socket
		 * and it fails.
		 */
		if (pFDInfo->pReadCB)
		    pFDInfo->pReadCB->Func();
                remove(-1, nFD);
            }
            else
            {
                // execute the read callback if the fd is ready for it
                if (pEvent->events & EPOLLIN && pFDInfo->pReadCB)
                {
                    pFDInfo->pReadCB->Func();
                    (*pNumReads)++;
                }
                // execute the write callback if the fd is ready for it
                if (pEvent->events & EPOLLOUT && pFDInfo->pWriteCB)
                {
                    pFDInfo->pWriteCB->Func();
                    (*pNumWrites)++;
                }
            }

            m_bCleanFinish = TRUE;
        }
        else
        {
            m_bCleanFinish = TRUE;
        }

        return m_nReadyFDs - m_nPos++;
    }
#endif // LINUX_EPOLL_SUPPORT

    if (invoke_n <= 0)
    {
	return 0;
    }
    int i = *invoke_members++;

again:
    if (!tab[i] || ts_memberlist[i]  || !enlist[i])
    {
	invoke_n--;
	if (invoke_n > 0)
	{
	    i = *invoke_members++;
	    goto again;
	}
	else
	{
	    return 0;
	}
    }

    if (m_bCleanFinish)
    {
	m_bCleanFinish = FALSE;
	tab[i]->Func();
	m_bCleanFinish = TRUE;
    }
    else
    {
	m_bCleanFinish = TRUE;
    }


    invoke_n--;

    return invoke_n + 1;
}

inline void
Callbacks::clear()
{
#ifdef DEV_POLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        close(m_nPollFD);
        if ((m_nPollFD = open("/dev/poll", O_RDWR)) < 0)
        {
            printf("Failed to open /dev/poll (%s)\n", strerror(errno));
            fflush(stdout);
        }
        return;
    }
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_nPollFD >= 0)
    {
        close(m_nPollFD);
        if ((m_nPollFD = epoll_create(EPOLL_SIZE_HINT)) < 0)
        {
            perror("epoll_create error");
        }
        return;
    }
#endif // LINUX_EPOLL_SUPPORT

    int* members = memberlist;
    for (int n = ready.members(memberlist); n > 0; n--)
	remove(-1, *members++);
}

inline int
Callbacks::empty()
{
#ifdef DEV_POLL_SUPPORT
    // this should only be called when we're not using /dev/poll
    HX_ASSERT(m_nPollFD < 0);
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    // this should only be called when we're not using epoll
    HX_ASSERT(m_nPollFD < 0);
#endif // LINUX_EPOLL_SUPPORT

    return enabled.members(memberlist) == 0;
}

inline fd_set*
Callbacks::set()
{
#ifdef DEV_POLL_SUPPORT
    // this isn't supported when using /dev/poll
    if (m_nPollFD >= 0)
        return NULL;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    // this isn't supported when using epoll
    if (m_nPollFD >= 0)
        return NULL;
#endif // LINUX_EPOLL_SUPPORT

    return ready.set;
}

inline int
Callbacks::high()
{
#ifdef DEV_POLL_SUPPORT
    // not supported when using /dev/poll
    HX_ASSERT(m_nPollFD < 0);
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    // not supported when using epoll
    HX_ASSERT(m_nPollFD < 0);
#endif // LINUX_EPOLL_SUPPORT

    return ready.high;
}

inline int
Callbacks::fd(int idx)
{
#ifdef DEV_POLL_SUPPORT
    // not supported when using /dev/poll
    HX_ASSERT(m_nPollFD < 0);
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    // not supported when using epoll
    HX_ASSERT(m_nPollFD < 0);
#endif // LINUX_EPOLL_SUPPORT

    if (idx < count)
	return memberlist[idx];
    else
	return -1;
}

#ifdef DEV_POLL_SUPPORT
inline int
Callbacks::Poll(int timeout)
{
    if (m_nPos < m_nReadyFDs || m_nTSPos < m_nReadyFDs)
        {
        //we still have events to process, don't overwrite the events list
        return m_nReadyFDs;
    }

    struct dvpoll pollInfo;
    pollInfo.dp_fds = m_pPollMemberList;
    pollInfo.dp_nfds = count;
    pollInfo.dp_timeout = timeout;

    m_nReadyFDs = ioctl(m_nPollFD, DP_POLL, &pollInfo);
    if (m_nReadyFDs < 0 && errno != EINTR)
        perror("/dev/poll ioctl DP_POLL failed");

    if (m_nReadyFDs < 0)
        m_nReadyFDs = 0;
    return m_nReadyFDs;
}
#endif // DEV_POLL_SUPPORT

#ifdef LINUX_EPOLL_SUPPORT
inline int
Callbacks::EventPoll(int timeout)
{
    if (m_nPos < m_nReadyFDs || m_nTSPos < m_nReadyFDs)
    {
        //we still have events to process, don't overwrite the events list
        return m_nReadyFDs;
    }

    // if (!timeout)
	// timeout = 1;

    m_nReadyFDs = epoll_wait(m_nPollFD, m_pEPollEvents, EPOLL_EVENTS_MAX, timeout);
    if (m_nReadyFDs < 0 && errno != EINTR && errno != 0)
        perror("epoll_wait failed");

    if (m_nReadyFDs < 0)
        m_nReadyFDs = 0;
    return m_nReadyFDs;
}
#endif // LINUX_EPOLL_SUPPORT

#endif/*_CALLBACK_H_*/
