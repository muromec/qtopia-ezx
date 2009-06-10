/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: minifilesys.cpp,v 1.13 2008/03/11 05:41:01 gahluwalia Exp $
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

/****************************************************************************
 * Defines
 */
#define   INITGUID     /* Interface ID's */


/****************************************************************************
 * Includes
 */
#include "hlxclib/string.h" /* strcpy */

#include "hxtypes.h"
#include "hxver.h"    /* HXVER_COPYRIGHT */
#include "hxcom.h"    /* IUnknown */
#include "hxcomm.h"   /* IHXCommonClassFactory */
#include "ihxpckts.h" /* IHXValues */

#include "minifilesys.h" /* CMiniFileSystem */
#include "minifileobj.h" /* CMiniFileObject */

#include "../smplfsys.ver" /* version info */

#include "debug.h"
#include "hxerror.h"

#define D_MINI_FS 0x1000000


/****************************************************************************
 *
 *  Function:
 *
 *    HXCreateInstance()
 *
 *  Purpose:
 *
 *    Function implemented by all plugin DLL's to create an instance of
 *    any of the objects supported by the DLL. This method is similar to
 *    Window's CoCreateInstance() in its purpose, except that it only
 *    creates objects from this plugin DLL.
 *
 *    NOTE: Aggregation is never used. Therefore and outer unknown is
 *    not passed to this function, and you do not need to code for this
 *    situation.
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)(IUnknown** ppFileSystemObj)
{
    DPRINTF(D_MINI_FS, ("smplfsys HXCREATEINSTANCE()\n"));

    HX_RESULT res = HXR_OUTOFMEMORY;

    *ppFileSystemObj = (IUnknown*)(IHXPlugin*)new CHXMiniFileSystem();

    if (*ppFileSystemObj != NULL)
    {
        (*ppFileSystemObj)->AddRef();
        res = HXR_OK;
    }

    return res;
}


// CHXMiniFileSystem Class Methods

/****************************************************************************
 *  CHXMiniFileSystem static variables
 *
 *  These variables are passed to the Helix core to provide information about
 *  this plug-in. They are required to be static in order to remain valid
 *  for the lifetime of the plug-in.
 */
const char* const CHXMiniFileSystem::zm_pDescription = "RealNetworks Mini Local File System";
const char* const CHXMiniFileSystem::zm_pCopyright   = HXVER_COPYRIGHT;
const char* const CHXMiniFileSystem::zm_pMoreInfoURL = HXVER_MOREINFO;
const char* const CHXMiniFileSystem::zm_pShortName   = "pn-mini-local";
const char* const CHXMiniFileSystem::zm_pProtocol    = FILE_SYS_PROTOCOL;


/****************************************************************************
 *  CHXMiniFileSystem::CHXMiniFileSystem
 *
 *  Constructor
 */
CHXMiniFileSystem::CHXMiniFileSystem(void)
    : m_RefCount      (0), 
      m_pClassFactory (NULL),
      m_pBasePath     (new char[1]),
      m_pContext      (NULL)
{
    DPRINTF(D_MINI_FS, ("CHXMiniFileSystem()\n"));

    if (m_pBasePath)
    {
        m_pBasePath[ 0 ] = '\0';
    }
}


/****************************************************************************
 *  CHXMiniFileSystem::~CHXMiniFileSystem
 *
 *  Destructor. Be sure to release all outstanding references to objects.
 */
CHXMiniFileSystem::~CHXMiniFileSystem(void)
{
    DPRINTF(D_MINI_FS, ("~CHXMiniFileSystem()\n"));

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pContext);
    HX_VECTOR_DELETE(m_pBasePath);
}


// IHXFileSystemObject Interface Methods

/****************************************************************************
 *  IHXFileSystemObject::GetFileSystemInfo
 *
 *  This routine returns crucial information required to associate this
 *  plug-in with a given protocol. This information tells the core which
 *  File System plug-in to use for a particular protocol. For example, in the
 *  URL: "file://myfile.txt", the protocol would be "file". This routine is
 *  called when the Helix core application is launched.
 */
STDMETHODIMP
CHXMiniFileSystem::GetFileSystemInfo(REF(const char*) pShortName,
				     REF(const char*) pProtocol)
{
    DPRINTF(D_MINI_FS, ("GetFileSystemInfo()\n"));

    pShortName = zm_pShortName;
    pProtocol  = zm_pProtocol;

    return HXR_OK;
}


/****************************************************************************
 *  IHXFileSystemObject::InitFileSystem
 *
 *  This routine performs any additional initialization steps required for
 *  the file system.  It is called prior to the CreatFile() request. Any
 *  options provided usually refer to mounting options related to the server,
 *  such as base path or authentication preferences.
 */
STDMETHODIMP
CHXMiniFileSystem::InitFileSystem(IHXValues*  options )
{
    DPRINTF(D_MINI_FS, ("InitFileSystem()\n"));

    HX_RESULT res = HXR_OK;

    // Retrieve the platform's base path, if specified
    if (options != NULL )
    {
        IHXBuffer* pPathBuffer = NULL;

        if (options->GetPropertyBuffer("BasePath", pPathBuffer) == HXR_OK)
        {
            if (pPathBuffer->GetBuffer() != NULL)
            {
                HX_VECTOR_DELETE(m_pBasePath);
                m_pBasePath = new char[strlen((char*)pPathBuffer->GetBuffer())];

                if (m_pBasePath)
                {
                    strcpy(m_pBasePath, (char*)pPathBuffer->GetBuffer());
                }
                else
                {
                    res = HXR_OUTOFMEMORY;
                }
            }
            pPathBuffer->Release();
        }
    }

    return res;
}


/****************************************************************************
 *  IHXFileSystemObject::CreateFile
 *
 *  This routine creates a new File Object which handles all of the file I/O
 *  functionality of this class. This File Object is eventually handed off
 *  to a File Format plug-in which handles file I/O through this File Object.
 *  This method is called called when an URL with a protocol associated with
 *  this plug-in is opened.
 */
STDMETHODIMP
CHXMiniFileSystem::CreateFile(IUnknown** ppFileObject)
{
    DPRINTF(D_MINI_FS, ("CreateFile()\n"));

    HX_RESULT res = HXR_OUTOFMEMORY;

    // Create a new File Object which implements the file I/O methods
    CHXMiniFileObject* pFileObj = new CHXMiniFileObject( m_pClassFactory,
                                                         m_pBasePath,
                                                         m_pContext );

    if (pFileObj != NULL)
    {
        pFileObj->QueryInterface(IID_IUnknown, (void**)ppFileObject);

        res = (pFileObj != NULL) ? HXR_OK : HXR_UNEXPECTED;
    }

    return res;
}


/****************************************************************************
 *  IHXFileSystemObject::CreateDir
 *
 *  This routine is analagous to CreatFile, except directories instead of
 *  files are of concern. It is not implemented in this example.
 */
STDMETHODIMP
CHXMiniFileSystem::CreateDir(IUnknown** /* ppDirectoryObject */)
{
    DPRINTF(D_MINI_FS, ("CreateDir()\n"));

    return HXR_NOTIMPL;
}


// IHXPlugin Interface Methods

/****************************************************************************
 *  IHXPlugin::GetPluginInfo
 *
 *  This routine returns descriptive information about the plug-in, most
 *  of which is used in the About box of the user interface. It is called
 *  when the Helix core application is launched.
 */
STDMETHODIMP
CHXMiniFileSystem::GetPluginInfo(REF(HXBOOL)      bLoadMultiple,
                                 REF(const char*) pDescription,
                                 REF(const char*) pCopyright,
                                 REF(const char*) pMoreInfoURL,
                                 REF(UINT32)      versionNumber)
{
    DPRINTF(D_MINI_FS, ("GetPluginInfo()\n"));

    bLoadMultiple = TRUE;
    pDescription  = zm_pDescription;
    pCopyright    = zm_pCopyright;
    pMoreInfoURL  = zm_pMoreInfoURL;
    versionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}


/****************************************************************************
 *  IHXPlugin::InitPlugin
 *
 *  This routine performs initialization steps such as determining if
 *  required interfaces are available. It is called when the Helix core
 *  application is launched, and whenever an URL with a protocol associated
 *  with this plug-in is opened.
 */
STDMETHODIMP
CHXMiniFileSystem::InitPlugin(IUnknown* pContext)
{
    DPRINTF(D_MINI_FS, ("InitPlugin()\n"));

    HX_RESULT res = HXR_OK;

    m_pContext = pContext;
    m_pContext->AddRef();

    if (pContext->QueryInterface(IID_IHXCommonClassFactory,
                 (void**)&m_pClassFactory) != HXR_OK)
    {
        res = HXR_NOINTERFACE;
    }

    return res;
}


// IUnknown COM Interface Methods

/****************************************************************************
 *  IUnknown::AddRef
 *
 *  This routine increases the object reference count in a thread safe
 *  manner. The reference count is used to manage the lifetime of an object.
 *  This method must be explicitly called by the user whenever a new
 *  reference to an object is used.
 */
STDMETHODIMP_(UINT32) CHXMiniFileSystem::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


/****************************************************************************
 *  IUnknown::Release
 *
 *  This routine decreases the object reference count in a thread safe
 *  manner, and deletes the object if no more references to it exist. It must
 *  be called explicitly by the user whenever an object is no longer needed.
 */
STDMETHODIMP_(UINT32) CHXMiniFileSystem::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


/****************************************************************************
 *  IUnknown::QueryInterface
 *
 *  This routine indicates which interfaces this object supports. If a given
 *  interface is supported, the object's reference count is incremented, and
 *  a reference to that interface is returned. Otherwise a NULL object and
 *  error code are returned. This method is called by other objects to
 *  discover the functionality of this object.
 */
STDMETHODIMP CHXMiniFileSystem::QueryInterface(REFIID interfaceID,
                           void** ppInterfaceObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown),  (IUnknown*)(IHXPlugin*)this },
        { GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*) this },
        { GET_IIDHANDLE(IID_IHXFileSystemObject), (IHXFileSystemObject*) this },
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), interfaceID, ppInterfaceObj);
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return (CHXBaseCountingObject::ObjectsActive() > 0 ? HXR_FAIL : HXR_OK);
}
