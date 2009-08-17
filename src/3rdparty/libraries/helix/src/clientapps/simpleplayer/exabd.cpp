/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include <stdio.h>

#include "hxtypes.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "hxcore.h"
#include "hxengin.h"
#include "chxpckts.h"
#include "hxclsnk.h"
#include "hxstrutl.h"
#include "exabd.h"

#include "print.h"

#include "globals.h"

struct _stGlobals*& GetGlobal(); //in main.cpp

ExampleABD::ExampleABD(IUnknown* pUnknown)
           : m_lRefCount (0)
           , m_pUnknown (NULL)
           , m_ulABDStartTime(0)
           , m_ulABDResult(0)
           , m_bABDDone(FALSE)
{
    if (pUnknown)
    {
	m_pUnknown = pUnknown;
	m_pUnknown->AddRef();
    }
}

ExampleABD::~ExampleABD(void)
{
    HX_RELEASE(m_pUnknown);
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP ExampleABD::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXAutoBWCalibrationAdviseSink*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAutoBWCalibrationAdviseSink))
    {
	AddRef();
	*ppvObj = (IHXAutoBWCalibrationAdviseSink*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) ExampleABD::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) ExampleABD::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXAutoBWCalibrationAdviseSink methods
 */
STDMETHODIMP
ExampleABD::AutoBWCalibrationStarted (const char* pszServer)
{
    if (pszServer)
    {
        STDOUT("ABD: contacting %s\n", pszServer);        
    }

    return HXR_OK;
}

STDMETHODIMP
ExampleABD::AutoBWCalibrationDone(HX_RESULT  status,
                                  UINT32     ulBW)
{
    m_bABDDone = TRUE;

    if (HXR_OK == status)
    {
        m_ulABDResult = ulBW;
        STDOUT("ABD: %lu(Kbps) %lu(ms)\n", ulBW, GetTickCount() - m_ulABDStartTime);        
    }
    else
    {
        STDOUT("ABD failed: %lu\n", status);        
    }

    return HXR_OK;
}

HX_RESULT
ExampleABD::DoABD(const char*   pszServer,
                  UINT32        mode,
                  UINT32        packetSize,
                  UINT32        packetNum)
{
    HX_RESULT               rc = HXR_OK;
    UINT32                  length = 0;
    IHXAutoBWCalibration*   pABD = NULL;
    IHXValues*              pValues = NULL;
    IHXBuffer*              pBuffer = NULL;

    m_ulABDStartTime = GetTickCount();
    m_ulABDResult = 0;
    m_bABDDone = FALSE;

    if (m_pUnknown &&
        HXR_OK == m_pUnknown->QueryInterface(IID_IHXAutoBWCalibration, (void**)&pABD))
    {
        pValues = (IHXValues*) new CHXHeader();
        pValues->AddRef();

	pBuffer = (IHXBuffer*) new CHXBuffer();
	pBuffer->AddRef();

	pBuffer->Set((UCHAR*)pszServer, strlen(pszServer)+1);        
        pValues->SetPropertyCString("ABDServers", pBuffer);

        pValues->SetPropertyULONG32("AutoBWDetectionMode", mode);
        pValues->SetPropertyULONG32("AutoBWDetectionPacketSize", packetSize);
        pValues->SetPropertyULONG32("AutoBWDetectionPackets", packetNum);       

        pABD->AddAutoBWCalibrationSink((IHXAutoBWCalibrationAdviseSink*)this);
        pABD->InitAutoBWCalibration(pValues);

        pABD->StartAutoBWCalibration();

        while (!m_bABDDone)
        {
            MSG msg;
            DWORD starttime, endtime, i;
            BOOL sleep = TRUE;
            static const int checkinterval = 10;

            starttime = GetTickCount();
            endtime = starttime + (20);
            i = 0;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                DispatchMessage(&msg);
                if ((i % checkinterval) == 0)
                {
                    if (GetTickCount() > endtime)
                        break;
                    ++i;
                }
                sleep = FALSE;
            }

            if (sleep)
                Sleep(10);
        }
    }

cleanup:

    if (pABD)
    {
        pABD->RemoveAutoBWCalibrationSink((IHXAutoBWCalibrationAdviseSink*)this);
    }

    HX_RELEASE(pBuffer);
    HX_RELEASE(pValues);
    HX_RELEASE(pABD);

    return rc;
}


