/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxplugin.cpp,v 1.10 2006/08/17 17:15:54 ping Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxstring.h"
#include "hxccf.h"    // IHXCommonClassFactory
#include "ihxpckts.h" // IHXBuffer
#include "hxplugn.h" //IHXComponentPlugin
#include "hxformt.h" //IHXFileFormatObject
#include "hxfwrtr.h" //IHXFileWriter
#include "hxfiles.h" //IHXFileSystemObject
#include "hxrendr.h" //IHXRenderer
#include "hxdtcvt.h" //IHXDataRevert
#include "hxsdesc.h" //IHXStreamDescription
#include "hxplgns.h" //IHXPluginProperties
#include "hxmeta1.h" //IHXMetaFileFormatObject
#include "chxpckts.h" //CHXHeader
#include "pckunpck.h"
#include "hxpluginarchive.h"

#include "hxstrutl.h" //SafeStrCat

#include "dllacces.h"
#include "dllpath.h"

#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_STATICALLY_LINKED)
#include "staticff.h"
#endif

#include "hxplugindll.h"
#include "hxplugin.h"
#include "pckunpck.h"

BEGIN_INTERFACE_LIST_NOCREATE(HXPlugin)
END_INTERFACE_LIST

const char* const k_pszValueSeperator = "|";

HXPlugin::HXPlugin(IUnknown* pContext)
: m_idxPlugin(0)
, m_pDll(0)
, m_pValues(0)
, m_pContext(pContext)
, m_pClassFactory(NULL)
{
    HX_ASSERT(m_pContext);
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pClassFactory);
    HX_ASSERT(m_pClassFactory);
    m_pClassFactory->CreateInstance(CLSID_IHXValues,(void**)&m_pValues);
}

// deserializing constructor
HXPlugin::HXPlugin(IUnknown* pContext, HXPluginArchiveReader& ar)
: m_idxPlugin(0)
, m_pDll(0)
, m_pValues(0)
, m_pContext(pContext)
, m_pClassFactory(NULL)
{
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pClassFactory);

    ar.Read(m_pValues);
}

// serialize object
void HXPlugin::Archive(HXPluginArchiveWriter& ar)
{
    ar.Write(m_pValues);
}


HXPlugin::~HXPlugin()
{
    HX_RELEASE(m_pValues);
    //HX_RELEASE(m_pDll); weak ref
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pContext);
}

void HXPlugin::SetPluginProperty(const char* pszPluginType)
{
    HXLOGL3(HXLOG_CORE,"HXPlugin()::SetPluginProperty(): type = %s", pszPluginType);
    IHXBuffer* pBuffer = NULL;
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    HX_ASSERT(pBuffer);
    pBuffer->Set((UCHAR*)pszPluginType, strlen(pszPluginType)+1);
    m_pValues->SetPropertyCString(PLUGIN_CLASS, pBuffer);
    pBuffer->Release();
}


HXBOOL HXPlugin::DoesMatch(IHXValues* pValues)
{
    CHXSimpleList   PossibleValues1;
    CHXSimpleList   PossibleValues2;
    const char*	    pPropName = NULL;
    ULONG32	    nInValue;
    ULONG32	    nOutValue;
    IHXBuffer*	    pInBuffer = NULL;
    IHXBuffer*	    pOutBuffer = NULL;

    // Check ULONGS 1st
    if (HXR_OK == pValues->GetFirstPropertyULONG32(pPropName, nInValue))
    {
	if (HXR_OK==m_pValues->GetPropertyULONG32(pPropName, nOutValue))
	{
	    if (nInValue != nOutValue)
	    {
		goto notFoundexit;
	    }
	}
	else
	{
	    goto notFoundexit;
	}
	while (HXR_OK == pValues->GetNextPropertyULONG32(pPropName, nInValue))
	{
	    if (HXR_OK == m_pValues->GetPropertyULONG32(pPropName, nOutValue))
	    {
		if (nInValue != nOutValue)
		{
		    goto notFoundexit;
		}
	    }
	    else
	    {
		goto notFoundexit;
	    }
	}
    }


    // Check String Props.
    if (HXR_OK == pValues->GetFirstPropertyCString(pPropName, pInBuffer))
    {
	if (HXR_OK == m_pValues->GetPropertyCString(pPropName, pOutBuffer))
	{
	    if (!AreBufferEqual(pOutBuffer, pInBuffer))
	    {
		goto notFoundexit;
	    }
	}
	else
	{
	    goto notFoundexit;
	}

	HX_RELEASE(pInBuffer);
	HX_RELEASE(pOutBuffer);

	while (HXR_OK == pValues->GetNextPropertyCString(pPropName, pInBuffer))
	{
	    if (HXR_OK == m_pValues->GetPropertyCString(pPropName, pOutBuffer))
	    {
		if ( !AreBufferEqual(pOutBuffer, pInBuffer))
		{
		    goto notFoundexit;
		}
	    }
	    else
	    {
		goto notFoundexit;
	    }

	    HX_RELEASE(pInBuffer);
	    HX_RELEASE(pOutBuffer);
	}
    }


     // Check Buffer Properties
    if (HXR_OK == pValues->GetFirstPropertyBuffer(pPropName, pInBuffer))
    {
	// XXXND  Make some utility functions for doing this...
	if (HXR_OK == m_pValues->GetPropertyBuffer(pPropName, pOutBuffer))
	{
	    if( pOutBuffer->GetSize() == pInBuffer->GetSize() )
	    {
		if( ::memcmp( pOutBuffer->GetBuffer(), pInBuffer->GetBuffer(), pOutBuffer->GetSize() ) )
		{
		    goto notFoundexit;
		}
	    }
	}
	else
	{
	    goto notFoundexit;
	}

	HX_RELEASE(pInBuffer);
	HX_RELEASE(pOutBuffer);

	while (HXR_OK == pValues->GetNextPropertyBuffer(pPropName, pInBuffer))
	{
	    if (HXR_OK == m_pValues->GetPropertyBuffer(pPropName, pOutBuffer))
	    {
		// XXXND  Make some utility functions for doing this...
		if( pOutBuffer->GetSize() == pInBuffer->GetSize() )
		{
		    if( ::memcmp( pOutBuffer->GetBuffer(), pInBuffer->GetBuffer(), pOutBuffer->GetSize() ) )
		    {
			goto notFoundexit;
		    }
		}
	    }
	    else
	    {
		goto notFoundexit;
	    }

	    HX_RELEASE(pInBuffer);
	    HX_RELEASE(pOutBuffer);
	}
    }

    return TRUE;    // we made it!

notFoundexit:
    HX_RELEASE(pInBuffer);
    HX_RELEASE(pOutBuffer);
    return FALSE;
}

HX_RESULT HXPlugin::Init(HXPluginDLL* pDll, UINT16 idxPlugin)
{
    HX_ASSERT(!m_pDll);
    HX_ASSERT(pDll);
    m_pDll = pDll; // weak ref

    m_idxPlugin = idxPlugin;
    m_pValues->SetPropertyULONG32(PLUGIN_INDEX, idxPlugin);
    
    IHXBuffer* pBuff = HXBufferUtil::CreateBuffer(m_pClassFactory, m_pDll->GetFileName());
    m_pValues->SetPropertyCString(PLUGIN_FILENAME, pBuff);
    HX_RELEASE(pBuff);
    
    return HXR_OK;
}


HXBOOL HXPlugin::AreBufferEqual(IHXBuffer* pBigBuff,
					   IHXBuffer* pSmallBuff)
{
    char*   pTemp;
    HXBOOL    bRetVal = FALSE;

    pTemp = new char[pBigBuff->GetSize()];
    strcpy(pTemp, (char*)pBigBuff->GetBuffer()); /* Flawfinder: ignore */

    char* token;
    token = strtok(pTemp, k_pszValueSeperator);
    while (token)
    {
	CHXString tokenCHXstring;
	CHXString smallCHXstring;

	tokenCHXstring = token;
	smallCHXstring = (char*)pSmallBuff->GetBuffer();
	tokenCHXstring.TrimLeft();
	tokenCHXstring.TrimRight();
	smallCHXstring.TrimLeft();
	smallCHXstring.TrimRight();

	if (!strcasecmp(tokenCHXstring, smallCHXstring))
	{
	    bRetVal = TRUE;
	    break;
	}
	token = strtok(NULL, k_pszValueSeperator);
    }
    delete[] pTemp;

    return bRetVal;
}

HX_RESULT HXPlugin::GetValuesFromDLL(IHXPlugin* pHXPlugin)
{
    HX_RESULT hr = GetBasicValues(pHXPlugin);
    if(SUCCEEDED(hr))
    {
	hr = GetExtendedValues(pHXPlugin);
    }
    return hr;
}

HX_RESULT HXPlugin::GetPlugin(IUnknown*& pUnknown )
{
    pUnknown = NULL;

    HX_RESULT hr = HXR_FAIL;

    HX_ASSERT(m_pDll);
    if (m_pDll)
    {
        // load DLL if not loaded
        if (!m_pDll->IsLoaded())
        {
	    hr = m_pDll->Load();
        }
        
        if(m_pDll->IsLoaded())
        {
            hr = m_pDll->CreateInstance(&pUnknown, m_idxPlugin);
        }
    }

    return hr;
}

HX_RESULT HXPlugin::GetInstance(IUnknown*& pUnknown, IUnknown* pIUnkOuter )
{
    IHXPlugin* pIHXPlugin = NULL;
    IHXComponentPlugin* pComponentPlugin = NULL;

    // Initialize out parameter
    pUnknown = NULL;

    IUnknown* pUnkPlugin = NULL;
    HX_RESULT hr = GetPlugin( pUnkPlugin );
    if( SUCCEEDED(hr) )
    {
        hr = HXR_FAIL;

        if( SUCCEEDED( pUnkPlugin->QueryInterface(IID_IHXPlugin, (void**)&pIHXPlugin ) ) )
        {
            pIHXPlugin->InitPlugin(m_pContext);
        }

	if( SUCCEEDED( pUnkPlugin->QueryInterface( IID_IHXComponentPlugin, (void**) &pComponentPlugin ) ) )
	{
	    // Ask for the correct object by CLSID
	    IHXBuffer* pCLSID = NULL;
	    if( SUCCEEDED( m_pValues->GetPropertyBuffer( PLUGIN_COMPONENT_CLSID, pCLSID ) ) )
	    {
		hr = pComponentPlugin->CreateComponentInstance( *(GUID*) pCLSID->GetBuffer(), pUnknown, pIUnkOuter );
		HX_RELEASE( pCLSID );
	    }
	    else
	    {
		// component plugins must have CLSID
                HX_ASSERT(false);
	    }

	    HX_RELEASE( pComponentPlugin );
	}
	else
	{
	    if( !pIUnkOuter )
	    {
		pUnknown = pUnkPlugin;
                pUnknown->AddRef();
                hr = HXR_OK;
	    }
            else
            {
                // we can't aggregate anything because this is not a component plugin
                HX_ASSERT(false);
            }
	}
    }

    HX_RELEASE(pIHXPlugin);
    HX_RELEASE(pUnkPlugin);

    return hr;
}




HX_RESULT HXPlugin::GetPluginInfo(IHXValues*& pVals)
{
    if (m_pValues)
    {
	pVals = m_pValues;
        pVals->AddRef();
	return HXR_OK;
    }
    pVals = NULL;
    return HXR_FAIL;
}


HX_RESULT
HXPlugin::GetBasicValues(IHXPlugin* pHXPlugin)
{
    HXLOGL3(HXLOG_CORE, "HXPlugin()::GetBasicValues(): returning nothing");
    return HXR_OK;

#if(0)
    const char*	pszDescription = NULL;
    const char* pszCopyright = NULL;
    const char* pszMoreInfoUrl = NULL;
    ULONG32	ulVersionNumber = 0;
    HXBOOL	nload_multiple = 0;

    if (HXR_OK != pHXPlugin->GetPluginInfo(nload_multiple, pszDescription,
				       pszCopyright, pszMoreInfoUrl, ulVersionNumber))
    {
        return HXR_FAIL;
    }

    IHXBuffer* pBuffer = NULL;
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    if (pszDescription)
    {
	pBuffer->Set((UCHAR*)pszDescription, strlen(pszDescription)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_DESCRIPTION2, pBuffer);
    pBuffer->Release();
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    if (pszCopyright)
    {
	pBuffer->Set((UCHAR*)pszCopyright, strlen(pszCopyright)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_COPYRIGHT2, pBuffer);
    pBuffer->Release();
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
    if (pszMoreInfoUrl)
    {
	pBuffer->Set((UCHAR*)pszMoreInfoUrl, strlen(pszMoreInfoUrl)+1);
    }
    m_pValues->SetPropertyCString(PLUGIN_COPYRIGHT, pBuffer);
    pBuffer->Release();

    m_pValues->SetPropertyULONG32(PLUGIN_LOADMULTIPLE, nload_multiple);
    m_pValues->SetPropertyULONG32(PLUGIN_VERSION, ulVersionNumber);
    return HXR_OK;
#endif
}


HX_RESULT
HXPlugin::GetExtendedValues(IHXPlugin* pHXPlugin)
{

    IHXFileFormatObject*		pFileFormat	    = NULL;
    IHXFileWriter*			pFileWriter	    = NULL;
    IHXFileSystemObject*		pFileSystem	    = NULL;
    IHXRenderer*			pRenderer	    = NULL;
    IHXStreamDescription*		pStreamDescription  = NULL;
    IHXCommonClassFactory*		pClassFactory       = NULL;
    IHXPluginProperties*		pIHXPluginPropertiesThis = NULL;
    UINT32				nCountInterfaces    = 0;


    HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues():");

    // file system
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileSystemObject, (void**) &pFileSystem))
    {
        HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues(): file system object...");
	const char* pszShortName;
	const char* pszProtocol;

	if (HXR_OK != pFileSystem->GetFileSystemInfo(pszShortName, pszProtocol))
	{
	    HX_RELEASE (pFileSystem);
	    return  HXR_FAIL;
	}

	SetPluginProperty(PLUGIN_FILESYSTEM_TYPE);

	IHXBuffer* pBuffer = NULL;
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
	if (pszShortName)
	{
	    pBuffer->Set((UCHAR*)pszShortName, strlen(pszShortName)+1);
	}
	m_pValues->SetPropertyCString(PLUGIN_FILESYSTEMSHORT, pBuffer);
	pBuffer->Release();
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
	if (pszProtocol)
	{
	    pBuffer->Set((UCHAR*)pszProtocol, strlen(pszProtocol)+1);
	}
	m_pValues->SetPropertyCString(PLUGIN_FILESYSTEMPROTOCOL, pBuffer);
	pBuffer->Release();

	pFileSystem->Release();
	nCountInterfaces++;

        HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues(): fs proto = '%s'", pszProtocol);
    }

    // file format
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileFormatObject, (void**)&pFileFormat) ||
	HXR_OK == pHXPlugin->QueryInterface(IID_IHXMetaFileFormatObject, (void**)&pFileFormat) ||
	HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileWriter, (void**)&pFileWriter))
    {
        HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues(): file format...");
	// fine we are in now we will get the correct type.
	if (pFileFormat)
	{
	    pFileFormat->Release();
	}
	else
	{
	    pFileWriter->Release();
	}

	IHXMetaFileFormatObject* pMetaFileFormat;

	const char**		ppszMimeTypes = NULL;
	const char**		ppszExtensions = NULL;
	const char**		ppszOpenNames = NULL;

	if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileFormatObject, (void**)&pFileFormat))
	{
	    pFileFormat->GetFileFormatInfo( ppszMimeTypes,
					    ppszExtensions,
					    ppszOpenNames);
	    pFileFormat->Release();
	    SetPluginProperty(PLUGIN_FILEFORMAT_TYPE);
	}

	if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXMetaFileFormatObject, (void**)&pMetaFileFormat))
	{
	    pMetaFileFormat->GetMetaFileFormatInfo( ppszMimeTypes,
						    ppszExtensions,
						    ppszOpenNames);
	    pMetaFileFormat->Release();

	    SetPluginProperty(PLUGIN_METAFILEFORMAT_TYPE);
	}

	if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXFileWriter, (void**)&pFileWriter))
	{
	    pFileWriter->GetFileFormatInfo( ppszMimeTypes,
					    ppszExtensions,
					    ppszOpenNames);
	    pFileWriter->Release();

	    SetPluginProperty(PLUGIN_FILEWRITER_TYPE);
	}

	IHXBuffer* pBuffer = NULL;
	if (ppszMimeTypes)
	{
            CatStringsCCF(pBuffer, ppszMimeTypes, k_pszValueSeperator, m_pContext);
	    m_pValues->SetPropertyCString(PLUGIN_FILEMIMETYPES, pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (ppszExtensions)
	{
            CatStringsCCF(pBuffer, ppszExtensions, k_pszValueSeperator, m_pContext);
	    m_pValues->SetPropertyCString(PLUGIN_FILEEXTENSIONS, pBuffer);
	    HX_RELEASE(pBuffer);
	}

	if (ppszOpenNames)
	{
            CatStringsCCF(pBuffer, ppszOpenNames, k_pszValueSeperator, m_pContext);
	    m_pValues->SetPropertyCString(PLUGIN_FILEOPENNAMES, pBuffer);
	    HX_RELEASE(pBuffer);
	}
	nCountInterfaces++;
    }

    // renderer
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXRenderer, (void**)&pRenderer))
    {
        HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues(): renderer...");
	char**	ppszMimeTypes;
        UINT32	initial_granularity = 0;

	// get the basic info
	if (HXR_OK == pRenderer->GetRendererInfo((const char**&)ppszMimeTypes, initial_granularity))
	{
	    IHXBuffer* pBuffer = NULL;
	    if (ppszMimeTypes)
	    {
                CatStringsCCF(pBuffer, (const char**)ppszMimeTypes, k_pszValueSeperator, m_pContext);
	    }
	    m_pValues->SetPropertyCString(PLUGIN_RENDERER_MIME, pBuffer);
	    pBuffer->Release();
	    m_pValues->SetPropertyULONG32(PLUGIN_RENDERER_GRANULARITY, initial_granularity);
	    SetPluginProperty(PLUGIN_RENDERER_TYPE);
	}

	HX_RELEASE(pRenderer);
	nCountInterfaces++;
    }

    // stream description
    if (HXR_OK == pHXPlugin->QueryInterface(IID_IHXStreamDescription, (void**)&pStreamDescription))
    {
        HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues(): stream description...");
	const char* pszMimeType;
	IHXBuffer* pBuffer;
	if (HXR_OK != pStreamDescription->GetStreamDescriptionInfo(pszMimeType))
	{
	    HX_RELEASE (pStreamDescription);
	    return HXR_FAIL;
	}
	pStreamDescription->Release();
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuffer);
	if (pszMimeType)
	{
	    pBuffer->Set((UCHAR*)pszMimeType, strlen(pszMimeType)+1);
	}
	m_pValues->SetPropertyCString(PLUGIN_STREAMDESCRIPTION, pBuffer);
	pBuffer->Release();

	SetPluginProperty(PLUGIN_STREAM_DESC_TYPE);
	nCountInterfaces++;
    }

    // common class factory
    if(HXR_OK == pHXPlugin->QueryInterface(IID_IHXCommonClassFactory,
					(void**)&pClassFactory))
    {
        HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues(): class factory plugin...");
	SetPluginProperty(PLUGIN_CLASS_FACTORY_TYPE);
	HX_RELEASE (pClassFactory);
	nCountInterfaces++;
    }


    if( SUCCEEDED( pHXPlugin->QueryInterface( IID_IHXPluginProperties, (void**)&pIHXPluginPropertiesThis ) ) )
    {
        HXLOGL3(HXLOG_CORE, "HXPlugin()::GetExtendedValues(): plugin properties interface...");
	IHXValues* pIHXValuesProperties = NULL;

	pHXPlugin->InitPlugin(m_pContext);

	if( SUCCEEDED( pIHXPluginPropertiesThis->GetProperties( pIHXValuesProperties ) ) && pIHXValuesProperties )
	{
	    CHXHeader::mergeHeaders( m_pValues, pIHXValuesProperties );
	}

	HX_RELEASE(pIHXValuesProperties);

    }

    HX_RELEASE(pIHXPluginPropertiesThis);

    HX_ASSERT(nCountInterfaces<2);
    return HXR_OK;
}

HX_RESULT HXPlugin::AddComponentInfo( IHXValues* pVal )
{
    CHXHeader::mergeHeaders(m_pValues, pVal);
    return HXR_OK; // no way to know if above succeeded or not
}

