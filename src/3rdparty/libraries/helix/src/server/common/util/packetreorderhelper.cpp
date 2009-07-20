/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: packetreorderhelper.cpp,v 1.1 2004/12/01 21:04:54 hwatson Exp $ 
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

#include "hxcom.h"
#include "debug.h"
#include "hxtypes.h"

#include "hxbuffer.h"
#include "servlist.h"

#include "hxsrc.h"		//  IHXRawSinkObject
#include "ihxpacketorderer.h"	//  IHXPacketOrderer

#include "packetreorderhelper.h"

COrderedPacketsHelper::COrderedPacketsHelper()
: m_pOrderedPacketsSinkObject(NULL)
{
    m_pOrderedPacketsSinkObject = new COrderedPacketsSinkObject(this);
    HX_ADDREF(m_pOrderedPacketsSinkObject);
}

COrderedPacketsHelper::~COrderedPacketsHelper()
{
    HX_RELEASE(m_pOrderedPacketsSinkObject);
}

COrderedPacketsHelper::COrderedPacketsSinkObject::COrderedPacketsSinkObject(COrderedPacketsHelper* pOrderedPacketsHelper)
    : m_pOrderedPacketsHelper(pOrderedPacketsHelper)
{
    HX_ASSERT(m_pOrderedPacketsHelper);
}

//	*** IUnknown ***
BEGIN_INTERFACE_LIST(COrderedPacketsHelper::COrderedPacketsSinkObject)
    INTERFACE_LIST_ENTRY(IID_IHXRawSinkObject, IHXRawSinkObject)
END_INTERFACE_LIST

//  IHXRawSinkObject methods
STDMETHODIMP
COrderedPacketsHelper::COrderedPacketsSinkObject::InitDone(HX_RESULT status)
{
    HX_RESULT res = HXR_FAIL;
    HX_ASSERT(FALSE);
    return res;
}

STDMETHODIMP
COrderedPacketsHelper::COrderedPacketsSinkObject::FileHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT res = HXR_FAIL;
    HX_ASSERT(FALSE);
    return res;
}

STDMETHODIMP
COrderedPacketsHelper::COrderedPacketsSinkObject::StreamHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT res = HXR_FAIL;
    HX_ASSERT(FALSE);
    return res;
}

HX_RESULT
COrderedPacketsHelper::COrderedPacketsSinkObject::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    HX_RESULT res = status;

    if(!m_pOrderedPacketsHelper)
    {
	res = HXR_UNEXPECTED;
	HX_ASSERT(FALSE);
    }
    else
    {
	res = m_pOrderedPacketsHelper->OrderedPacketReady(status, pPacket);
    }

    return res;
}

STDMETHODIMP
COrderedPacketsHelper::COrderedPacketsSinkObject::StreamDone(UINT16 unStreamNumber)
{
    HX_RESULT res = HXR_FAIL;
    HX_ASSERT(FALSE);
    return res;
}



