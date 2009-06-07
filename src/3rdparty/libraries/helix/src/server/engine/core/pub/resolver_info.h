/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: resolver_info.h,v 1.2 2003/01/23 23:42:55 damonlan Exp $ 
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

#ifndef _RESOLVER_INFO_H_
#define _RESOLVER_INFO_H_

#include "debug.h"
#include "proc.h"
#include "server_resource.h"

class ResolverInfo {
public:
    	    	    	ResolverInfo();
    int			NewResolver(Process* proc);
    void		RequestStart(Process* proc, int s);
    void		RequestDone(Process* proc);
    int			BestResolver();
private:
    int			num;			// Number of Resolvers
    int			capacity;		// Resolver Capacity
    int			request_count[MAX_THREADS];
    int			MapNumToProcNum[MAX_THREADS];
};

inline
ResolverInfo::ResolverInfo()
{
//XXX...What should the capacity be???
    num      = 0;
    capacity = RESOLVER_CAPACITY_VALUE;
}

inline int
ResolverInfo::NewResolver(Process* proc)
{
    int my_num;

    // Not thread-safe
    MapNumToProcNum[num] = proc->procnum();

    request_count[MapNumToProcNum[num]] = 0;

    my_num = num;
    num++;

    return my_num;
}

inline void
ResolverInfo::RequestStart(Process* proc, int s)
{
    request_count[s]++;
}

inline void
ResolverInfo::RequestDone(Process* proc)
{
    request_count[proc->procnum()]--;
}

/*
 * Returns the resolver number of the least loaded resolver (or -1 if a new
 * resolver is needed)
 */
inline int
ResolverInfo::BestResolver()
{
    int i;
    int best = 100000;
    int best_num = -1;

    for (i = 0; i < num; i++)
    {
	if (request_count[MapNumToProcNum[i]] < best)
	{
	    best = request_count[MapNumToProcNum[i]];
	    best_num = i;
	}
    }

    if (best >= capacity)
	return -1;

    return MapNumToProcNum[best_num];
}

#endif
