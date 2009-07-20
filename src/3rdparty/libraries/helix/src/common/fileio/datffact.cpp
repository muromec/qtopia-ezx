/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: datffact.cpp,v 1.13 2009/03/02 17:22:32 qluo Exp $
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

#ifndef _MACINTOSH
//#include "hlxclib/stdio.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "hxdataf.h"
#include "datffact.h"
#include "bufdataf.h"
#include "mmapdatf.h"

#ifdef _SYMBIAN
#include "symbihxdataf.h"
#endif	// SYMBIAN
#ifdef _OPENWAVE
#include "opwavehxdataf.h"
#endif
#ifdef _BREW
#include "brewihxdataf.h"
#endif	// BREW
#ifdef ANDROID
#include "fdbufdataf.h"
#endif	// ANDROID

#include "debug.h"
#include "hxperf.h"

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXDataFileFactory::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
HXDataFileFactory::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXDataFileFactory))
    {
        AddRef();
        *ppvObj = (IHXDataFileFactory*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXDataFileFactory::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXDataFileFactory::AddRef()
{
    DPRINTF(0x5d000000, ("HXDFF::AddRef() = %ld\n", m_lRefCount+1));
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXDataFileFactory::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXDataFileFactory::Release()
{
    DPRINTF(0x5d000000, ("HXDFF::Release() = %ld\n", m_lRefCount-1));
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

/* Creates a datafile for buffered or unbuffered I/O
 * By default create an IHXDataFile for BUFFERED file I/O 
 * for all platforms, except SOLARIS, where UNBUFFERED 
 * file I/O is required.
 */
STDMETHODIMP_(HX_RESULT)
HXDataFileFactory::CreateFile(REF(IHXDataFile*) pDataFile, IUnknown*
    pContext, REF(IUnknown*) pPersistantObject, HXBOOL bDisableMemoryMappedIO,
    UINT32 ulChunkSize, HXBOOL bEnableFileLocking, HXBOOL bPreferAsyncIO)
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    HX_LOG_BLOCK( "HXDataFileFactory::CreateFile" );
    
#if defined(_SYMBIAN)
    pDataFile = new CSymbIHXDataFile(pContext, &pPersistantObject);
#elif defined(_OPENWAVE)
		pDataFile = new OpenwaveHXDataFile(pContext, &pPersistantObject);
#elif defined(_BREW)
    pDataFile = new CBrewIHXDataFile(pContext, &pPersistantObject);
#elif defined(HELIX_FEATURE_MEMMAP_IO)
    pDataFile = new MemoryMapDataFile(pContext, pPersistantObject,
	bDisableMemoryMappedIO, ulChunkSize, bEnableFileLocking);
#elif defined(ANDROID)
    pDataFile = new FDBufferedDataFile(pContext);
#else	// default
    pDataFile = new BufferedDataFile(pContext);
#endif	// HELIX_FEATURE_MEMMAP_IO

    if (pDataFile)
    {
	pDataFile->AddRef();
	retVal = HXR_OK;
    }

    return retVal;
}
#endif
