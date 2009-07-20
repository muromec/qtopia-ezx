/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cresload.cpp,v 1.7 2007/07/06 20:39:19 jfinnecy Exp $
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

#include "hxmap.h"
#include "platform/mac/cresload.h"


#if defined(_MAC_UNIX)
CResourceLoader::CResourceLoader(CFBundleRef theLibrary)
#else
CResourceLoader::CResourceLoader(FSSpec& theLibrary)
#endif
 : m_theResFile(-1)
 , m_theLibrary(theLibrary)
 , m_ulRefCount(0)
 , m_theHandleMap(0)
{
    m_theHandleMap = new CHXMapPtrToPtr();
}

CResourceLoader::~CResourceLoader()
{
    zm_theResourceLoader = NULL;

    HX_ASSERT(m_theHandleMap->IsEmpty());
    delete m_theHandleMap;
    m_theHandleMap = 0;
    
    // close the resource file
    ::CloseResFile(m_theResFile);
    m_theResFile = -1;
}

CResourceLoader* 
#if defined(_MAC_UNIX)
CResourceLoader::CreateInstance(CFBundleRef theLibrary)
#else
CResourceLoader::CreateInstance(FSSpec& theLibrary)
#endif
{
    if (NULL == zm_theResourceLoader)
    {
    	zm_theResourceLoader = new CResourceLoader(theLibrary);
    }

    zm_theResourceLoader->AddRef();
    return zm_theResourceLoader;
}

UINT32 
CResourceLoader::AddRef()
{
    InterlockedIncrement(&m_ulRefCount);
    return m_ulRefCount;
}

UINT32 
CResourceLoader::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
    	return m_ulRefCount;
    }

    delete this;
    return 0;
}

Handle 
CResourceLoader::LoadResource(ResType theType, INT16 id)
{
    Handle hRes = 0;

    // save the old file
    INT16 thePrevResFile = ::CurResFile();

    if (m_theResFile == -1)
    {
#if defined(_MAC_UNIX)
        CFArrayRef resourceArray = ::CFBundleCopyResourceURLsOfType(m_theLibrary, CFSTR(".rsrc"), NULL);
        if (resourceArray)
        {
            if (::CFArrayGetCount(resourceArray) > 0)
            {
                // xxxbobclark we'd have a little more "renovating" to
                // do in CResourceLoader to handle multiple .rsrc files
                // in a single bundle.
                HX_ASSERT(::CFArrayGetCount(resourceArray) <= 1);

                CFURLRef resourceFileURL = (CFURLRef)::CFArrayGetValueAtIndex(resourceArray, 0);

                HX_ASSERT(resourceFileURL);

                FSRef resourceFSRef;

                Boolean bGotFSRefSucceeded = ::CFURLGetFSRef(resourceFileURL, &resourceFSRef);

                HX_ASSERT(bGotFSRefSucceeded);

                if (bGotFSRefSucceeded)
                {
                    // FSOpenResFile opens the resource fork, while
                    // FSOpenResourceFile (by default) opens the DATA FORK
                    // as a resource file. Allows us to use data-fork-based
                    // resource files.
                    short theRefNum = 0;
                    OSErr err = FSOpenResourceFile(&resourceFSRef, 0, NULL, fsRdPerm, &theRefNum);
                    if (err == noErr)
                    {
                        m_theResFile = theRefNum;
                    }
                }
            }
            ::CFRelease(resourceArray);
        }
#else
    	m_theResFile = ::FSpOpenResFile(&m_theLibrary, fsRdPerm);
#endif
    }

    if (m_theResFile != -1)
    {
    	// set our's current
    	::UseResFile(m_theResFile);

    	// load the resource
    	hRes = ::Get1Resource(theType, id);

    	// do the mapping stuff here in case we already have this handle around...
    	if (hRes)
    	{
	    long thisHandleCount = 0;
	    (void)m_theHandleMap->Lookup(hRes, (void*&)thisHandleCount);
	    thisHandleCount++;
	    (void)m_theHandleMap->SetAt(hRes, (void*)thisHandleCount);
    	}
    	
    	// reset old res file
    	::UseResFile(thePrevResFile);
    }
    return hRes;
}

void
CResourceLoader::UnloadResource(Handle hRes)
{
    // see if the handle has more than one mapped to it; we may not need to
    // release the resource.
    HX_ASSERT(hRes != nil);
    if (!hRes) return;
    
    long thisHandleCount = 0;
    (void)m_theHandleMap->Lookup(hRes, (void*&)thisHandleCount);
    HX_ASSERT(thisHandleCount > 0);
    thisHandleCount--;
    
    if (thisHandleCount == 0)
    {
	// unload the resource
	::ReleaseResource(hRes);
	
	(void)m_theHandleMap->RemoveKey(hRes);
    }
    else
    {
	(void)m_theHandleMap->SetAt(hRes, (void*)thisHandleCount);
    }
}

