/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: core_container.h,v 1.4 2004/10/08 01:57:13 jzeng Exp $
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

#ifndef _CORE_CONTAINER_H_
#define _CORE_CONTAINER_H_

#include "proc_container.h"
#include "core_controller_cont.h"

class CHXSimpleList;
class ConnDispatch;
class INetworkAcceptor;

class CoreContainer : public ProcessContainer, public CoreControllerContainer {
public:
                        CoreContainer(Process* proc);

    CHXSimpleList*      m_pListenResponseList;
    ConnDispatch*       cdispatch;
#if !defined WIN32
    FDPassSocket*       m_fdps[MAX_THREADS];
#endif
};

inline
CoreContainer::CoreContainer(Process* proc) : ProcessContainer(proc)
{
    m_pListenResponseList = NULL;
    cdispatch = NULL;
#if !defined WIN32
    memset (m_fdps, 0, sizeof(FDPassSocket*) * MAX_THREADS);
#endif
}

#endif /* _CORE_CONTAINER_H_ */
