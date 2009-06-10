/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: memorymonitor.cpp,v 1.4 2008/05/28 22:41:09 ashkunar Exp $
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

#include "memorymonitor.h"
#include "symbian_gm_inst.h"
SymbianMemoryMonitor::SymbianMemoryMonitor():
    m_pErrorMessages(NULL),
    m_pCallback(NULL),
    m_TempMem(NULL)
{
}

SymbianMemoryMonitor::~SymbianMemoryMonitor()
{
    // If error messages is not released, 
    //close api is not called by the caller.
    HX_ASSERT(!m_pErrorMessages);
    HX_ASSERT(!m_TempMem);
}

void SymbianMemoryMonitor::Close()
{
    if (m_pCallback)
    {
        if (m_pCallback->IsActive())
        {
    		m_pCallback->Cancel();
    		}
    	  delete m_pCallback;
    	  m_pCallback = NULL;
    }
    if (m_pErrorMessages)
    {
        m_pErrorMessages->Release();
        m_pErrorMessages = NULL;
    }

    // Release Mem out buffersize
    RelaseTempMem();
}

static void DestroySymbianMemoryMonitor(void* pObj )
{
    SymbianMemoryMonitor* pData = (SymbianMemoryMonitor*)pObj;
    delete pData;
}

SymbianMemoryMonitor* SymbianMemoryMonitor::Instance()
{
    SymbianMemoryMonitor* pRet = NULL;
    HXGlobalManager* pGM = HXGlobalManager::Instance();
    //Use Global unique ID so that all Dlls creating object get already exsting SymbianMemoryMonitor
    //SYMBIAN_GLOBAL_MEMORY_MONITOR_ID
    SymbianMemoryMonitor** pInstance = reinterpret_cast<SymbianMemoryMonitor**>(pGM->Get((GlobalID)SYMBIAN_GLOBAL_MEMORY_MONITOR_ID));

    if (!pInstance)
    {
        //Create New if it doesnot already exist SymbianMemoryMonitor
        pRet = new SymbianMemoryMonitor();
        if (pRet)
        {
            pGM->Add( (GlobalID)SYMBIAN_GLOBAL_MEMORY_MONITOR_ID, (GlobalType)pRet, &DestroySymbianMemoryMonitor );
        }
    }
    else
    {
        //Use already existing SymbianMemoryMonitor
        pRet = *pInstance;
    }

    return pRet;
}

// NOTE: If Init API is called, the caller must call Close API
HXBOOL  SymbianMemoryMonitor::Init(IUnknown *pContext, const TUint32 buffersize)
{
    HXBOOL retVal = FALSE;
    if (pContext && m_pErrorMessages == NULL)
    {
        IHXErrorMessages* pErrMsg = NULL;
        pContext->QueryInterface( IID_IHXErrorMessages, (void**)&pErrMsg);
        if (pErrMsg)
        {
            m_pErrorMessages = pErrMsg;
            retVal = TRUE;
        }
    }
    if (m_pCallback == NULL)
    {
        //Create AsyncCallback for intimating the Helix about Mem Failure 
        TCallBack cb(&(SymbianMemoryMonitor::_Callback), (TAny *)this); 
        m_pCallback = new CAsyncCallBack (cb, EPriorityNormal);
        if (!m_pCallback)
        {
            retVal = FALSE;
        }
    }
    //Allcoate Mem out buffersize
    AllocateTempMem(buffersize);

    return retVal;
}

void SymbianMemoryMonitor::SendEvent(MemoryEvent event)
{
    //Call the AsyncCallback for Mem out  	
    if (event == EOutOfMemory && m_pErrorMessages && m_pCallback)
    {
        m_pCallback->CallBack();
    }
}

TInt SymbianMemoryMonitor::_Callback(TAny *p)
{
    SymbianMemoryMonitor *pSelf = (SymbianMemoryMonitor *) p;
    pSelf->Callback();
    return 0;
}

void SymbianMemoryMonitor::Callback()
{
	  //Reporting Memout Error back to Helix
    if (m_pErrorMessages)
    {
        m_pErrorMessages->Report(HXLOG_ERR, HXR_OUTOFMEMORY , 0, NULL , NULL);
    }
}

void SymbianMemoryMonitor::AllocateTempMem(const TUint32 buffersize)
{
	  //Allocate Memout Buffer
		if(!m_TempMem)
    {
		    m_TempMem = User::Alloc(buffersize); 
		
	  }
}

void SymbianMemoryMonitor::RelaseTempMem()
{
  //Release Memout Buffer
	if(m_TempMem)
	{
		User::Free(m_TempMem);
		m_TempMem = NULL;
	}
}
