/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: clienttracker.h,v 1.2 2003/01/23 23:42:55 damonlan Exp $ 
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

#include "client.h"

class ClientTracker
{
public:
    ClientTracker();
    ~ClientTracker();

    void Add(Client*);
    void Remove(Client*);
    void Report();

    Client** m_pClients;
    time_t* m_pTimes;
    int fill;

};

inline
ClientTracker::ClientTracker()
: fill(0)
{
    m_pClients = new Client*[4096];
    m_pTimes = new time_t[4096];
} 

inline
ClientTracker::~ClientTracker()
{
}

inline void
ClientTracker::Add(Client* p)
{
    time_t now;
    time(&now);
    int i;
    for (i = 0; i < fill; i++)
    {
	if (m_pClients[i] == 0)
	{
	    m_pClients[i] = p;
	    m_pTimes[i] = now;
	    return;
	}
    }

    if (fill < 4096)
    {
	m_pClients[fill] = p;
	m_pTimes[fill] = now;
	fill++;
    }
}


inline void
ClientTracker::Remove(Client* p)
{
    int i;
    for (i = 0; i < fill; i++)
    {
	if (m_pClients[i] == p)
	{
	    m_pClients[i] = 0;
	    return;
	}
    }
}

inline void
ClientTracker::Report()
{
    time_t now;
    time(&now);

    printf("Client times:\n");
    int i;
    for (i = 0; i < fill; i++)
    {
	if (m_pClients[i])
	{
	    printf("Client 0x%x: %lu secs\n",
		m_pClients[i], now - m_pTimes[i]);
	    printf("\tis_cloak: %d\n", m_pClients[i]->is_cloak);
	    printf("\tis_cloaked_get: %d\n", m_pClients[i]->is_cloaked_get);
	    printf("\tis_multiple_put: %d\n", m_pClients[i]->is_multiple_put);
	    printf("\tis_dummy: %d\n", m_pClients[i]->is_dummy);
	    printf("\tstate: ");
	    switch (m_pClients[i]->m_state)
	    {
	    case Client::ALIVE:
		printf("ALIVE\n");
		break;

	    case Client::DEATH_CB_QED:
		printf("DEATH_CB_QED\n");
		break;

	    case Client::DEAD:
		printf("DEAD\n");
		break;

	    default:
		printf("WHAT!\n");
		break;
	    }
	}
    }

    printf("Done.\n");
    fflush(stdout);
}
	
