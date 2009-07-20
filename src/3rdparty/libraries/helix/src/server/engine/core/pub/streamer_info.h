/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: streamer_info.h,v 1.6 2007/08/31 20:49:41 dcollins Exp $ 
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

#ifndef _STREAMER_INFO_H_
#define _STREAMER_INFO_H_

#include <stdio.h>

#include "debug.h"
#include "proc.h"
#include "regdb_misc.h"
#include "server_resource.h"

extern UINT32* g_pNumStreamers;
extern UINT32* g_pCPUCount;

class StreamerInfo {
public:
    	    	    	StreamerInfo();
    int			NewStreamer(Process* proc);
    void		PlayerConnect(Process* proc, int s);
    void		PlayerDisconnect(Process *proc);
    int			BestStreamer(Process* proc);
    void		SetSessionCapacity(UINT32);
    void		SetImportantThingCount(Process*, UINT32);
    int			Number() { return num; }
    int                 GetProcNum(UINT32 num) {return MapNumToProcNum[num];}
    void		EnableStreamer(int procnum);

private:
    int			num;			// Number of Streamers
    UINT32*		capacity;		// Streamer Capacity
    UINT32		_capacity;
    int			player_count[MAX_THREADS];
    int			important_thing_count[MAX_THREADS]; //fds on unix, 
							    //SOCKETs on win
    int			MapNumToProcNum[MAX_THREADS];
    BOOL		enabled[MAX_THREADS];
};

inline
StreamerInfo::StreamerInfo()
{
    num      = 0;

#if defined _WIN32
    capacity = &SOCK_CAPACITY_VALUE;
#else
    capacity = &DESCRIPTOR_CAPACITY_VALUE;
#endif
}


inline int
StreamerInfo::NewStreamer(Process* proc)
{
    int my_num;

    // Not thread-safe
    MapNumToProcNum[num] = proc->procnum();

    player_count[MapNumToProcNum[num]] = 0;
    important_thing_count[MapNumToProcNum[num]] = 0;
    enabled[MapNumToProcNum[num]] = FALSE;

    my_num = num;
    num++;

    return my_num;
}

inline void
StreamerInfo::EnableStreamer(int procnum)
{
    enabled[procnum] = TRUE;
}

inline void
StreamerInfo::PlayerConnect(Process* proc, int s)
{
    player_count[s]++;
}

inline void
StreamerInfo::SetSessionCapacity(UINT32 ul)
{
    _capacity = ul;
    capacity = &_capacity;
}

inline void
StreamerInfo::PlayerDisconnect(Process* proc)
{
    player_count[proc->procnum()]--;
}


inline int
StreamerInfo::BestStreamer(Process* proc)
{
    unsigned int lowest = ~0;
    int best = -1;
    int strIcpu;
    unsigned int total;
    
    strIcpu = *g_pCPUCount;
    if (strIcpu > (int)*g_pNumStreamers)
    {
	strIcpu = *g_pNumStreamers;
    }

    for (int i = 0; i < strIcpu; i++)
    {
	total = important_thing_count[MapNumToProcNum[i]];
	// SR: Do not check capacity if we are sharing descirptors
	// We need to determine whether there is any reason to launch
	// more streams at a given capacity
#ifdef SHARED_DESCRIPTORS
	if (enabled[MapNumToProcNum[i]] &&
	    lowest > total && ((total < (unsigned int)*capacity)||g_bSharedDescriptors))
#else
	if (enabled[MapNumToProcNum[i]] && 
	    lowest > total && (total < (unsigned int)*capacity))
#endif
	{
	    lowest = total;
	    best = i;
	}
    }

    return best < 0 ? -1 : MapNumToProcNum[best];
}

inline void
StreamerInfo::SetImportantThingCount(Process* p, UINT32 ulCount)
{
    if (this)
	important_thing_count[p->procnum()] = ulCount;
}
#endif
