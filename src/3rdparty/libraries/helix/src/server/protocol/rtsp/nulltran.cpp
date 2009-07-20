/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: nulltran.cpp,v 1.7 2006/12/21 19:04:55 tknox Exp $ 
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


//XXXTDM: clean this up, most is not needed
#include "hxtypes.h"
#include <stdio.h>
#include "hxcom.h"
#include "hxassert.h"
#include "debug.h"
#include "hxmarsh.h"
#include "hxstrutl.h"
#include "netbyte.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "netbyte.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxdeque.h"
#include "hxbitset.h"
#include "timebuff.h"
#include "timeval.h"
#include "tconverter.h"

#include "ntptime.h"
// we should really man an interface
#include "rtspserv.h"

#include "rtspif.h"
#include "rtsptran.h"
#include "transport.h"
#include "nulltran.h"
#include "basepkt.h"
#include "hxtbuf.h"
#include "transbuf.h"
#include "hxtick.h"
#include "random32.h"        // random32()
#include "hxprefs.h"        // IHXPreferences
#include "hxmime.h"
#include "hxcore.h"

#include "hxheap.h"
#ifdef PAULM_IHXTCPSCAR
#include "objdbg.h"
#endif

#ifdef _DEBUG
#undef HX_THIS_FILE                
static char HX_THIS_FILE[] = __FILE__;
#endif

/*
 * NullSetupTransport methods
 */
NullSetupTransport::NullSetupTransport()
    : Transport()
{
}

NullSetupTransport::~NullSetupTransport()
{
}


/*
 * IUnknown methods
 */
STDMETHODIMP
NullSetupTransport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    return Transport::QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(UINT32)
NullSetupTransport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
NullSetupTransport::Release()
{
    if(InterlockedDecrement(&m_ulRefCount) > 0UL)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

