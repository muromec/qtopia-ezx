/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clntxres.cpp,v 1.17 2006/11/30 17:36:26 ping Exp $
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

#include "hxresult.h"
#include "hxslist.h"
#include "netbyte.h"
#include "hxstrutl.h"
#include "hxver.h"

#include "hxassert.h"
#include "hxheap.h"
#include "clntxres.ver"
#include "clntxres.h"
#include "hxmarsh.h"

#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"

#ifdef _MACINTOSH
#pragma export on
#endif

#ifdef _AIX
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Pnxres);
#endif

STDAPI HXCreateInstance(IUnknown**  /*OUT*/	ppIUnknown);
		
#ifdef _MACINTOSH
#pragma export off
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


/*
 * PNCreateInstance
 * -----------------
 * Entry point into this resource manager.
 * 
 * input:
 * IUnknown** ppIUnknown	- Pointer to mem where we store pointer to new ob.
 *
 * output:
 * STDAPI						
 * 
 */
STDAPI 
ENTRYPOINTCALLTYPE ENTRYPOINT(HXCreateInstance)
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CHXXResFile;
    if (*ppIUnknown != NULL) 
    {
	(*ppIUnknown)->AddRef();
	return HXR_OK;
    }

    // Out of memory...
    return HXR_OUTOFMEMORY;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return (CHXBaseCountingObject::ObjectsActive() > 0 ? HXR_FAIL : HXR_OK );
}



/*
 * QueryInterface
 * --------------
 * Used to get interfaces supported by us.
 *
 * input:
 * REFIID riid				- Id of interface.
 * void **ppvObj			- Place to copy interface pointer.
 *
 */
STDMETHODIMP 
CHXXResFile::QueryInterface
(
    REFIID riid, 
    void** ppvObj
)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXXResFile), (IHXXResFile*)this },
            { GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXXResFile*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}




/* 
 * AddRef
 * ------
 * Increments the ref count by one.
 *
 * input:
 * void
 *
 * output:
 * ULONG32			- The count.
 *
 */
STDMETHODIMP_ (ULONG32) 
CHXXResFile::AddRef
(
    void
)
{
    return InterlockedIncrement(&m_lRefCount);
}





/*
 * Release
 * -------
 * Decrements the ref count by one, deleting 
 * self if count reaches zero.
 *
 * input:
 * void
 * 
 * output:
 * ULONG32			- Current ref count.
 *
 */
STDMETHODIMP_(ULONG32) 
CHXXResFile::Release
(
    void
)
{
    // Decrement, return count if possible.
    if (InterlockedDecrement(&m_lRefCount) > 0) return m_lRefCount; 

    // Else, delete self.
    delete this;
    return 0;
}



// IHXPlugin methods

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CHXXResFile::InitPlugin(IUnknown* /*IN*/ pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
	    (void**)&m_pCommonClassFactory);

    return HXR_OK;
}

const char* const CHXXResFile::zm_pName		    = "PNXRes";
const char* const CHXXResFile::zm_pDescription	    = "External Resource File Reader";
const char* const CHXXResFile::zm_pCopyright	    = HXVER_COPYRIGHT;
const char* const CHXXResFile::zm_pMoreInfoURL	    = HXVER_MOREINFO;

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP CHXXResFile::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

CHXXResFile::CHXXResFile():
     m_pContext(NULL)
    ,m_pCommonClassFactory(NULL)
    ,mLanguageId(0x0409)    // default US English
    ,mMaxCachedData(kDefaultCacheLimit)
    ,m_lRefCount(0)
    ,mCachePos(0)
    ,mCacheList(NULL)
    ,mLoadedCache(NULL)
    ,m_nCodePage(0)
    ,m_nResFileRef(0)
{
}

CHXXResFile::~CHXXResFile()
{
    KillCache();

    if (mCacheList)
    {
	delete mCacheList;
	mCacheList=NULL;
    }

    if (mLoadedCache)
    {	
	delete mLoadedCache;
	mLoadedCache=NULL;
    }

    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);
}


//
//	Open the specified file and read in all the data about where resources
//  are stored in the file.
//
STDMETHODIMP_(HX_RESULT)
CHXXResFile::Open(const char*  path)
{    
    HX_RESULT rc = CHXPeff::open(path, m_pContext);
    
    if (rc != HXR_OK) 
    {
	return rc;
    }

    //
    //	Read the data in the file header, and determine where to start reading from in the file.
    //	
    if(HXR_OK != FindResourceData())
    {
	return HXR_RESOURCE_NODATA;
    }
    
    CacheResourceEntries();

    return rc;
}

//
//	Close the file.
//
STDMETHODIMP_(HX_RESULT)
CHXXResFile::Close()
{
    CHXPeff::close();
    FlushCache();
    return HXR_OK;
}


//
//	Allow the resources to be accessed by type, and ID.
//

STDMETHODIMP_(HX_RESULT)	
CHXXResFile::GetResource	  (ULONG32	type, ULONG32  ID,  IHXXResource** resource)
{
    XResCacheEntry*	entry = NULL;
    HX_RESULT		rc = HXR_OK;	
    char*		buffer = NULL;
    HX_RESULT		err;
    CHXXResource* 	newresource;
    UCHAR*		data;
    ULONG32 		readsize;
    UCHAR*		tempdata;
    
    HX_ASSERT(resource);
    
    if (!resource)
    {
	return HXR_INVALID_PARAMETER;
    }
 
#ifdef _MACINTOSH
    INT16 nSavedResFile = 0;
    // if loading from resource fork, set current res file.
    if (m_nResFileRef)
    {
    	nSavedResFile = ::CurResFile();
    	::UseResFile(m_nResFileRef);
    }
#endif

    rc = FindInCache(type, ID, &entry);

    if (rc != HXR_OK)
    {
	err = HXR_RESOURCE_NOT_FOUND;
	goto Cleanup;
    }

    if (!mLoadedCache)
    {
	mLoadedCache = new CHXSimpleList();
    }

    HX_ASSERT(mLoadedCache);

    if (!mLoadedCache)
    {
	err = HXR_OUTOFMEMORY;
	goto Cleanup;
    }
    
    //
    //	We can't continue if the requested resource wasn't in the cache.
    //
    if (!entry)
    {
	err = HXR_FAIL;
	goto Cleanup;
    }
    
    //
    //	Okay new we need a buffer
    //
    
    buffer = new char[entry->size];
    
    if (!buffer)
    {
        err = HXR_OUTOFMEMORY;
	goto Cleanup;
    }
    
    newresource = 
	new CHXXResource(buffer, entry->size, entry->id,
			entry->type, entry->language,this);
    
    if (!newresource)
    {
	delete [] buffer;
	err = HXR_OUTOFMEMORY;
	goto Cleanup;
    }	

    *resource = newresource;
    newresource->AddRef();

    //
    //	Okay now check to see if the data for the resource was already cached.
    //
    //	If the data was cached already then return the pointer to the data already in memory.
    //

    if (entry->cached == TRUE)
    {
	if (entry->cached_data == NULL)
	{
	    err = HXR_RESOURCE_NODATA;
	    goto Cleanup;
	}

	//
	//	BUFFER
	//
	memcpy(buffer, entry->cached_data, entry->size); /* Flawfinder: ignore */
	err = HXR_OK;
	goto Cleanup;
    }

    
    //
    //	If we got here, we need to load the resource from the file.
    //

    if (!entry->location)
    {
	delete newresource;
	err = HXR_RESOURCE_NODATA;
	goto Cleanup;
    }


    //
    //	Seek to the resource's physical location in the file.
    //
    rc = mFile->Seek(entry->location, 0);

    if (rc != HXR_OK)
    {
        delete newresource;
        err = rc;
	goto Cleanup;
    }

    //
    //	Allocate a buffer to load the data into.
    //
    data = new UCHAR[entry->size];
    HX_ASSERT(data);
    if (!data)
    {
        delete newresource;
        err = HXR_OUTOFMEMORY;
	goto Cleanup;
    }

    //
    //	Okay read in the data.
    //

    readsize = mFile->Read((char*)data, entry->size);
    if (readsize != entry->size)
    {
	delete newresource;
	err = HXR_AT_END;
	goto Cleanup;
    }
    
    //
    //	Copy in the end of resource marker.
    //
    //	This is used in certain resources to detect the end of the resource data.
    //	This makes it easy for us to just pass around one ptr, instead of having to 
    //	otherwise pass the length around to the processing functions.
    //
    
    tempdata = data;
    
    tempdata = data + entry->size;
    tempdata -= sizeof(kEndOfResourceMarker);
    if(TestBigEndian())
    {
	putlong(tempdata,kEndOfResourceMarker);
    }
    else
    {
	(*(ULONG32*)tempdata)=kEndOfResourceMarker;
    }	

    //
    //	Trim the Cached Data now so we don't delete the data we loaded
    //	for the cached entry.
    //
    TrimCachedData(entry->size);


    //
    //	Setup the cached entry.
    //	Setup the data that we return.
    //
    entry->cached = TRUE;
    entry->cached_data = data;

    

    mLoadedCache->AddHead(entry);
    memcpy(buffer, data, entry->size); /* Flawfinder: ignore */
    err = HXR_OK;
    
Cleanup:
    //
    //	Okay we got here, no errors.
    //
#ifdef _MACINTOSH
    if (nSavedResFile)
    {
    	::UseResFile(nSavedResFile);
    }
#endif

    return err;
}



	//
	//	HIGH LEVEL FUNCTIONS
	//

	//
	//	Return a 'C' string from the given ID.
	//
#define	STRINGTABLEMASK	0xFFF0
#define	STRINGENTRYMASK	0x000F

STDMETHODIMP_(IHXXResource*)
CHXXResFile::GetString	  (ULONG32	ID)
{
    HX_ASSERT(this);

    UINT16  wID = (UINT16)ID;
    
    UINT16  StringTableID = wID & STRINGTABLEMASK;
    StringTableID = StringTableID >> 4;		
    StringTableID++;

    UINT16  StringEntryID = wID & STRINGENTRYMASK;

    //
    //	Okay now after determining the string ID, we must
    //	actually load the data.
    //

    char*			StringTable = NULL; 
    char*			ResultString = NULL;
    char*			tempstring = NULL;
    IHXXResource*		OriginalStringTable = NULL;
    IHXXResource*		result = NULL;

    HX_RESULT			rc = HXR_OK;
    UINT16			counter = 0;
    UINT16			len = 0;


    rc = GetResource(HX_RT_STRING,StringTableID,&OriginalStringTable);
	    
    if (rc != HXR_OK)
    {
	goto CleanUp;
    }
    
    HX_ASSERT(OriginalStringTable);

    StringTable=(char*)OriginalStringTable->ResourceData();
    
    //
    //	Okay now we have the String Table resource data. Let's parse it.
    //

    while (counter < StringEntryID)
    {
	UINT16  len = *((UINT16*)StringTable);
	ReverseWORD(len);

	//
	//	Jump past the length word.
	//
	StringTable += 2;
	
	HX_ASSERT(StringTable);

	//
	//	Jump ahead len * 2
	//

	StringTable += (2 * len);
	HX_ASSERT(StringTable);
	
	counter++;
    }

    //
    //	Okay we should now be at the string we wnat.
    //
    
    len = *((UINT16*)StringTable);
    ReverseWORD(len);
    
    if (!len)
    {
	goto CleanUp;
    }
    
    StringTable += 2;

    //
    //	Make buffer for holoding the string, add 2 bytes to make it long enough for two \0's
    //
    ResultString = new char[(len * 2) + 2];
    HX_ASSERT(ResultString);

    if (!ResultString)
    {
        return NULL;
    }

    memset(ResultString, 0, (len * 2) + 2);
    memcpy(ResultString, StringTable, (len * 2)); /* Flawfinder: ignore */

    //
    //	Okay turn the string into a normal ASCII string.
    //

    tempstring = new char[ (len+1) * 2];
    HX_ASSERT(tempstring);

    if (!tempstring)
    {
        goto CleanUp;
    }

#ifdef _WIN32
    WideCharToMultiByte(GetCodePage(),  0, (LPCWSTR)ResultString, len+1, tempstring, (len+1) * 2, "", 0);
#else
    if (HXR_OK != ProcessFromUnicode((const char*)ResultString, len * 2, 
				    tempstring, len * 2))
    {
	delete [] ResultString;
	ResultString=NULL;

	delete [] tempstring;
	tempstring=NULL;

	goto CleanUp;
    }
#endif

    //
    //	Here we return the string that was output from the ProcessFromUnicode function.
    //
    delete [] ResultString;

    ResultString = tempstring;

    //
    //      We return it as an IHXXResource so that deletion happens correctly.
    //
    result = new CHXXResource(ResultString, strlen(ResultString)+1,
	    ID, HX_RT_STRING, OriginalStringTable->Language(), this);
    
    HX_ASSERT(result);
    
    if (result)
    {
	result->AddRef();
    }
    
    if (!result)
    {
	delete [] ResultString;
    }

CleanUp:
    
    if (OriginalStringTable)
    {
	OriginalStringTable->Release();
	OriginalStringTable=NULL;
	StringTable=NULL;
    }

    return result;

}

STDMETHODIMP_(IHXXResource*)
CHXXResFile::GetVersionInfo()
{
    IHXXResource* pRes = NULL;

    GetResource(HX_RT_VERSION, 1, &pRes);
    return pRes;
}


//
//	Return a "BITMAP" from the given ID.
//
	
STDMETHODIMP_(IHXXResource*)
CHXXResFile::GetBitmap	  (ULONG32	ID)
{
    IHXXResource*	result = NULL;
    GetResource(HX_RT_BITMAP, ID, &result);
    
    return result;
}


//
//	Return a "DIALOG" from the given ID.
//
STDMETHODIMP_(IHXXResource*)
CHXXResFile::GetDialog	  (ULONG32	ID)
{
    IHXXResource*	result = NULL;
    GetResource(HX_RT_DIALOG, ID, &result);
    
    return result;
}


//
//	This function removes extra resource data chunks, that are loaded
//	when a resource is loaded.
//
STDMETHODIMP_(HX_RESULT)
CHXXResFile::FlushCache(void)
{
    if (!mCacheList) 
    {
        return HXR_OK;
    }

    //
    //	Scan all entries looking for the matching type, and id. 
    //
    XResCacheEntry*	 curEntry = NULL;
    
    LISTPOSITION	 listpos = mCacheList->GetHeadPosition();
    
    while (listpos)
    {
        curEntry=(XResCacheEntry*) mCacheList->GetNext(listpos);
	if (curEntry->cached_data)
	{
	    delete [] curEntry->cached_data;
	    curEntry->cached_data = NULL;
	    curEntry->cached = FALSE;
	}
    }	
    /*
       This fixes a bug causing an infinite loop in the TrimCacheData
    */
    if (mLoadedCache)
    {
	listpos = mLoadedCache->GetHeadPosition();
    
	while (listpos)
	{		
	    curEntry = (XResCacheEntry*)mLoadedCache->GetAt(listpos);
	    curEntry->cached_data = NULL;
	    curEntry->cached = FALSE;
	    mLoadedCache->RemoveAt(listpos);
	
	    listpos = mLoadedCache->GetHeadPosition();
	}
    }
    return HXR_OK;
}


//
//	Sets the amount of memory that can be filled with cached resource data.
//

STDMETHODIMP_(HX_RESULT)
CHXXResFile::SetCacheLimit(ULONG32	MaxCachedData)  
{ 
    mMaxCachedData=MaxCachedData; 
    return HXR_OK;
};


//
//	Sets the Language ID of resources to be loaded.
//

STDMETHODIMP_(HX_RESULT) 
CHXXResFile::SetLanguage(ULONG32	id)
{
    //
    //	Eventually check a list and determine if the language is valid.
    //	

    mLanguageId=id;
    return HXR_OK;
}

STDMETHODIMP
CHXXResFile::UseResourceFile(INT16 nResourceFileRef)
{
    m_nResFileRef = nResourceFileRef;
    return HXR_OK;
}
		


//
//
//
//	SUPPORT METHODS
//
//


HX_RESULT	CHXXResFile::FindResourceData()
{
    HX_RESULT			rc = HXR_OK;
    ULONG32			size;
    ULONG32			pos;
    HX_IMAGE_SECTION_HEADER	sectionheader;

    rc = GetSectionHeaderNamed(".rsrc", sectionheader);
    if (rc != HXR_OK)
    {	
        return rc;
    }
    mResSectionVirtualAddress = sectionheader.VirtualAddress;

    //
    //	This actually moves us to the data portion of the section.
    //
    rc = FindSectionNamed(".rsrc",size,pos);
    
    if (rc != HXR_OK) 
    {
	return rc;
    }
    
    //
    //	Save the location of the ResourceData for a later time.
    //
    mResourceDataPosition = pos;

    return HXR_OK;
}





//
//	This function searches the resource tree, and caches the resource data.
//	This helps later, we basically just do a search of this cache and save
//	moocho time, instead of reading through the resource tree again.
//
//	Also if the resource had already been loaded, it's data may have been cached already
//	So we check this later, and return the cached data as well, instead of going back to 
//	disk.
//

HX_RESULT	CHXXResFile::CacheResourceEntries(void)
{
    if (mCacheList)
    {
	KillCache();
	delete mCacheList;
	mCacheList = NULL;
    }

    if (mLoadedCache)
    {
	delete mLoadedCache;
	mLoadedCache = NULL;
    }

    mCacheList = new CHXSimpleList();

    if (mCacheList == NULL)
    {
	return HXR_OUTOFMEMORY;
    }

    HX_RESULT	rc = HXR_OK;

    rc = ReadResourceHeader();
    if (rc != HXR_OK)
    {
	return rc;
    }


    rc = ReadInAllResources();

    if (rc != HXR_OK)
    {
	return rc;
    }

    return HXR_OK;
}

HX_RESULT	CHXXResFile::ReadResourceHeader(void)
{

    return GetResourceDirEntry(mResourceHeader);

}

//
//	This function is what actually does the resource reading.
//	Currently named resources are not being supported.
//
HX_RESULT	CHXXResFile::ReadInAllResources(void)
{
    // Skip ahead over the named resources.

    mFile->Seek(8*mResourceHeader.NumberOfNamedEntries,1);

    // Now read each of the tree branches for the entries left.

    ULONG32	 count=mResourceHeader.NumberOfIdEntries;
    ULONG32	 counter=0;

    for (counter=1; counter <= count; counter++)
    {
	HX_IMAGE_RESOURCE_DIRECTORY_ENTRY	   type;
	HX_IMAGE_RESOURCE_DIRECTORY_ENTRY	   id;
	HX_IMAGE_RESOURCE_DIRECTORY_ENTRY	   language;
	HX_IMAGE_RESOURCE_DATA_ENTRY		   data;

	ULONG32	    curtoplevelpos;
	ULONG32	    rootpos;

	//
	//	Get the top level directory entries. It is synonymous with type.
	//
	rootpos = mFile->Tell();
	GetResourceEntry(type);
	curtoplevelpos = mFile->Tell();
	type.OffsetToData = type.OffsetToData ^ 0x80000000;
	mFile->Seek(type.OffsetToData+mResourceDataPosition, 0);

	//
	//	Get teh second level directory.  It is synonymous with ID.
	//
	
	mFile->Tell();
	
	HX_IMAGE_RESOURCE_DIRECTORY	 dir_id;
	
	GetResourceDirEntry(dir_id);

	//
	// scan all the entries in the id directory.
	//

	for (ULONG32 id_counter = 1; 
	    id_counter <= dir_id.NumberOfIdEntries; 
	    id_counter++)
	{
	    GetResourceEntry(id);	
	    ULONG32	 curIdPos = mFile->Tell();
	    
	    id.OffsetToData = id.OffsetToData ^ 0x80000000;
	    mFile->Seek(id.OffsetToData+mResourceDataPosition, 0);


	    //
	    //	Scan all the language entries for the given id.
	    //
	    HX_IMAGE_RESOURCE_DIRECTORY	  dir_language;

	    GetResourceDirEntry(dir_language);

	    for (ULONG32   lang_counter = 1; 
		lang_counter <= dir_language.NumberOfIdEntries; 
		lang_counter++)
	    {
		GetResourceEntry(language);
		ULONG32	 curLangPos = mFile->Tell();

		mFile->Seek(language.OffsetToData+mResourceDataPosition, 0);
		
		//
		//	Get the Data attributes for this file.
		//

		ReadDWord(data.OffsetToData);
		ReadDWord(data.Size);
		ReadDWord(data.CodePage);
		ReadDWord(data.Reserved);

		//
		//	The data offset is VIRTUAL. 
		//	However it is relative to the VIRTUAL address of the section 
		//	we are currently in.  This adjusts for that, giving us the physical location of the resource.
		//

		data.OffsetToData -= mResSectionVirtualAddress;

		//
		//	Okay cache the data. 
		//
		XResCacheEntry*	 newentry = new XResCacheEntry;
		
		HX_ASSERT_VALID_PTR(newentry);
		
		if (!newentry)
		{
    		    return HXR_OUTOFMEMORY;
		}

		memset(newentry, 0, sizeof(XResCacheEntry));

		newentry->type	    = type.Name;
		newentry->id	    = id.Name;
		newentry->language  = language.Name;
		newentry->size	    = data.Size + sizeof(kEndOfResourceMarker);
		newentry->location  = data.OffsetToData + mResourceDataPosition;


		//
		//	Add the entry to the cache for later.
		//
		mCacheList->AddTail(newentry);

		//
		//	Seek back to the next language directory entry.
		//
		mFile->Seek(curLangPos,0);
	    }

	    //
	    //	Seek back to the next ID directory entry
	    //
	    
	    mFile->Seek(curIdPos,0);

	}

	//
	//	Reset to the position in the file where the next resource branch starts.
	//

	mFile->Seek(curtoplevelpos,0);
    }

    return HXR_OK;
}


HX_RESULT	CHXXResFile::GetResourceEntry(HX_IMAGE_RESOURCE_DIRECTORY_ENTRY&		h)
{
    IF_ERROR_RETURN(ReadDWord(h.Name));
    IF_ERROR_RETURN(ReadDWord(h.OffsetToData));

    return HXR_OK;
}


HX_RESULT	CHXXResFile::GetResourceDirEntry(HX_IMAGE_RESOURCE_DIRECTORY&      h)
{
    IF_ERROR_RETURN(ReadDWord(h.Characteristics));
    IF_ERROR_RETURN(ReadDWord(h.TimeDateStamp));
    IF_ERROR_RETURN(ReadWord(h.MajorVersion));
    IF_ERROR_RETURN(ReadWord(h.MinorVersion));
    IF_ERROR_RETURN(ReadWord(h.NumberOfNamedEntries));
    IF_ERROR_RETURN(ReadWord(h.NumberOfIdEntries));

    return HXR_OK;
}


HX_RESULT	CHXXResFile::FindInCache(ULONG32	 type,   ULONG32 ID,  XResCacheEntry**	ppEntry)
{

    if (!mCacheList) 
    {
        return HXR_OK;
    }

    HX_ASSERT(ppEntry);

    //
    //	Scan all entries looking for the matching type, and id. 
    //
    
    XResCacheEntry*	 curEntry = NULL;
    
    LISTPOSITION	 listpos = mCacheList->GetHeadPosition();
    
    while (listpos)
    {
	curEntry=(XResCacheEntry*) mCacheList->GetNext(listpos);

	if (curEntry->type == type && 
	    curEntry->id == ID && 
	    curEntry->language == mLanguageId)
	{
	    *ppEntry=curEntry;
	    return HXR_OK;
	}

    }
    
    if (curEntry->type == type && 
	curEntry->id == ID && 
	curEntry->language == mLanguageId)
    {
	*ppEntry=curEntry;
	return HXR_OK;
    }

    return HXR_RESOURCE_NOT_CACHED;
}

STDMETHODIMP
CHXXResFile::GetFirstResourceLanguage(REF(UINT32) ulLangID)
{
    HX_RESULT rc = HXR_FAIL;
    if(mCacheList)
    {
	mCachePos = mCacheList->GetHeadPosition();
	if(mCachePos)
	{
	    XResCacheEntry* pEntry = (XResCacheEntry*)mCacheList->
					    GetNext(mCachePos);
	    if(pEntry)
	    {
		ulLangID = pEntry->language;
	    }
	    rc = HXR_OK;
	}
    }
    return rc;
}

STDMETHODIMP
CHXXResFile::GetNextResourceLanguage(REF(UINT32) ulLangID)
{
    HX_RESULT rc = HXR_FAIL;
    if(mCacheList &&
	mCachePos)
    {
	XResCacheEntry* pEntry = (XResCacheEntry*)mCacheList->
					    GetNext(mCachePos);
	if(pEntry)
	{
	    ulLangID = pEntry->language;
	}
	rc = HXR_OK;
    }
    return rc;
}

const int SIZEOF_VS_VERSION_KEY = 32; // (strlen("VS_VERSION_INFO") + 1) * 2
const int SIZEOF_FILE_INFO_KEY  = 30; // (strlen("StringFileInfo") + 1) * 2

static UINT32
GetPadding(BYTE* pOrigin, BYTE* pCur)
{
    // return number of bytes of padding needed
    // to align pCur on a 32-bit boundary
    HX_ASSERT(pCur >= pOrigin);
    UINT32 ulOffset = pCur - pOrigin;
    return ulOffset % 4;
}

BYTE*
CHXXResFile::GetResInfo(BYTE* pData, UINT16& uResInfoLen, 
			UINT16& uResInfoType, CHXString& key)
{
    // parses a res info struct and returns the
    // offset to the 'Children' or 'Value' member
    BYTE* pStart = pData;
    uResInfoLen = *((UINT16*)pData); pData += sizeof(UINT16);
    ReverseWORD(uResInfoLen);
    UINT16 ulValueLen = *((UINT16*)pData); pData += sizeof(UINT16);
    ReverseWORD(ulValueLen);
    uResInfoType = *((UINT16*)pData); pData += sizeof(UINT16);
    ReverseWORD(uResInfoType);
    UINT32 ulStringMemLen = StringMemLength((const char*)pData);
    char* pAsciiString = new char[ulStringMemLen];
    ProcessFromUnicode((const char*)pData, (UINT16)ulStringMemLen, 
		pAsciiString, (UINT16)ulStringMemLen);
    key = pAsciiString;
    delete[] pAsciiString;
    pData += ulStringMemLen;
    pData += GetPadding(pStart, pData);
    return pData;
}


STDMETHODIMP_(HXBOOL)
CHXXResFile::IncludesShortName(const char* pShortName)
{
    // walk through the resource VERSION information
    // to find the string key 'ShortName', then
    // see if pShortName is contained in those values
    HXBOOL bFound = FALSE;

    IHXXResource* pRes = GetVersionInfo();
    if(pRes)
    {
	BYTE* pData = (BYTE*)pRes->ResourceData();
	BYTE* pStart = pData;
	
	// walk through VS_VERSION_INFO 
	UINT16 vsInfoLen = *((UINT16*)pData); pData += sizeof(UINT16);
	ReverseWORD(vsInfoLen);
	UINT16 vsInfoValueLen = *((UINT16*)pData); pData += sizeof(UINT16);
	ReverseWORD(vsInfoValueLen);
	pData += sizeof(UINT16);    // skip type
	pData += SIZEOF_VS_VERSION_KEY;
	pData += GetPadding(pStart, pData);
	pData += vsInfoValueLen;    // skip over VS_FIXEDFILEINFO
	pData += GetPadding(pStart, pData);

	CHXString keyStr;
	UINT16 uResInfoLen = 0;
	UINT16 uResInfoType = 0;

	BYTE* pEndOfData = pStart + vsInfoLen;
	while(pData < pEndOfData &&
	      !bFound)
	{
	    // find 'StringFileInfo'
	    pData = GetResInfo(pData, uResInfoLen, uResInfoType, keyStr);
	    if(strcasecmp((const char*)keyStr, "StringFileInfo") == 0)
	    {
		BYTE* pDataEnd = pData + uResInfoLen;
		// get string table
		pData = GetResInfo(pData, uResInfoLen, uResInfoType, keyStr);
		while(pData < pDataEnd &&
		    !bFound)
		{
		    // get string name/value pairs
		    pData = GetResInfo(pData, uResInfoLen, uResInfoType, keyStr);

		    // pData now points to a UNICODE value
		    UINT32 ulStringMemLen = StringMemLength((const char*)pData);
		    char* pAsciiString = new char[ulStringMemLen];
		    ProcessFromUnicode((const char*)pData, (UINT16)ulStringMemLen, 
					pAsciiString, (UINT16)ulStringMemLen);
		    if(strcasecmp((const char*)keyStr, "ShortName") == 0)
		    {
			if(strstr(pAsciiString, pShortName))
			{
			    bFound = TRUE;
			}
		    }
		    delete[] pAsciiString;
		    pData += ulStringMemLen;
		    pData += GetPadding(pStart, pData);
		}
	    }
	}
	HX_RELEASE(pRes);
    }
#if defined (_MACINTOSH) || defined (_UNIX) /* version resource type not supported */
    bFound = TRUE;
#endif
    return bFound;    // not implemented yet...
}

//
//	This function dumps the entire contents of the cache.
//

HX_RESULT	CHXXResFile::KillCache(void)
{
    if (!mCacheList) 
    {
        return HXR_OK;
    }

    //
    //	Scan all entries killing off any cached data.
    //
    XResCacheEntry*	 curEntry = NULL;
    LISTPOSITION listpos = mCacheList->GetHeadPosition();
    
    while (listpos)
    {
	curEntry = (XResCacheEntry*)mCacheList->GetAt(listpos);
	
	if (curEntry->cached == TRUE)
	{
	    HX_ASSERT(curEntry->cached_data);

	    if (curEntry->cached_data != NULL)
	    {
		delete [] curEntry->cached_data;
		curEntry->cached_data = NULL;
	    }
	}

	mCacheList->RemoveAt(listpos);
	
	delete curEntry;
	
	listpos = mCacheList->GetHeadPosition();
    }

    return HXR_OK;
}



HX_RESULT	CHXXResFile::TrimCachedData(ULONG32	needed)
{
    if (needed > mMaxCachedData)
    {
	FlushCache();
	return HXR_OK;
    }

    if (mLoadedCache->GetCount()==0) 
    {
	return HXR_OK;
    }

    LISTPOSITION		listpos = mLoadedCache->GetHeadPosition();

    ULONG32				totalcached = 0;
    XResCacheEntry*                     curEntry;

    while (listpos)
    {
	curEntry = (XResCacheEntry*)mLoadedCache->GetNext(listpos);
	totalcached += curEntry->size;
    }

    
    // never get out of this loop
    while (mLoadedCache->GetCount() && 
	    needed + totalcached > mMaxCachedData)
    {
	//
	//	Find the largest thing and trash it.
	//

	LISTPOSITION	savepos=NULL;
	ULONG32		largestItem = 0;

	listpos = mLoadedCache->GetHeadPosition();

	while (listpos)
	{
	    curEntry = (XResCacheEntry*)mLoadedCache->GetAt(listpos);
	    if (curEntry->size > largestItem)
	    {
		savepos = listpos;
		largestItem = curEntry->size;
	    }
	    mLoadedCache->GetNext(listpos);
	}

	HX_ASSERT( savepos );
	curEntry = (XResCacheEntry*)mLoadedCache->GetAt(savepos);

	    HX_ASSERT(curEntry->cached && curEntry->cached_data);
		delete [] curEntry->cached_data;
		curEntry->cached_data = NULL;
		curEntry->cached = FALSE;
		totalcached -= curEntry->size;
		mLoadedCache->RemoveAt(savepos);
    }

    return HXR_OK;
}

UINT32
CHXXResFile::GetCodePage()
{
    if(!m_nCodePage)
    {
	m_nCodePage = 1252; // Default code page.

	IHXXResource* pRes = GetVersionInfo();
	if(pRes)
	{
	    BYTE* pData = (BYTE*)pRes->ResourceData();
	    BYTE* pStart = pData;
	    
	    // walk through VS_VERSION_INFO 
	    UINT16 vsInfoLen = *((UINT16*)pData); pData += sizeof(UINT16);
	    ReverseWORD(vsInfoLen);
	    UINT16 vsInfoValueLen = *((UINT16*)pData); pData += sizeof(UINT16);
	    ReverseWORD(vsInfoValueLen);
	    pData += sizeof(UINT16);    // skip type
	    pData += SIZEOF_VS_VERSION_KEY;
	    pData += GetPadding(pStart, pData);
	    pData += vsInfoValueLen;    // skip over VS_FIXEDFILEINFO
	    pData += GetPadding(pStart, pData);

	    CHXString keyStr;
	    UINT16 uResInfoLen = 0;
	    UINT16 uResInfoType = 0;

	    BYTE* pEndOfData = pStart + vsInfoLen;
	    while(pData < pEndOfData)
	    {
		// find 'StringFileInfo'
		pData = GetResInfo(pData, uResInfoLen, uResInfoType, keyStr);
		if(strcasecmp((const char*)keyStr, "StringFileInfo") == 0)
		{
		    // get string table
		    pData = GetResInfo(pData, uResInfoLen, uResInfoType, keyStr);
		    if(keyStr.GetLength() == 8)
		    {
			m_nCodePage = strtoul(keyStr.Right(4), NULL, 16);
			break;
		    }
		}
	    }
	}

	HX_RELEASE(pRes);
    }

    return m_nCodePage;
}


//
//
//	CHXXResource methods
//
//	This object is returned from a call to CHXXResFile::GetResource
//

#ifdef _MACINTOSH
#pragma mark -
#pragma mark ***CHXXResource Entries***
#pragma mark -
#endif




CHXXResource::CHXXResource (void* data,
			    ULONG32	datalength, 
			    ULONG32 ID, 
			    ULONG32 Type, 
			    ULONG32	Language,
			    IHXXResFile*  file)
{
    HX_ASSERT(data);
    HX_ASSERT(file);
    
    m_lRefCount = 0;
    mResFile = file;
    mResData = data;
    mID = ID;
    mType = Type;
    mLength = datalength;
    mLanguage = Language;
    mResFile->AddRef();
}


CHXXResource::~CHXXResource()
{
    if (mResData)
    {
	delete [] mResData;
	mResData = NULL;
    }
    
    if (mResFile)
    {
	mResFile->Release();
    }
}
	

/*
 * QueryInterface
 * --------------
 * Used to get interfaces supported by us.
 *
 * input:
 * REFIID riid				- Id of interface.
 * void **ppvObj			- Place to copy interface pointer.
 *
 */
STDMETHODIMP 
CHXXResource::QueryInterface
(
    REFIID riid, 
    void** ppvObj
)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXXResource), (IHXXResource*)this },
            { GET_IIDHANDLE(IID_IHXXResFile), (IHXXResFile*)mResFile },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXXResource*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}




/* 
 * AddRef
 * ------
 * Increments the ref count by one.
 *
 * input:
 * void
 *
 * output:
 * ULONG32			- The count.
 *
 */
STDMETHODIMP_ (ULONG32) 
CHXXResource::AddRef
(
    void
)
{
    return InterlockedIncrement(&m_lRefCount);
}





/*
 * Release
 * -------
 * Decrements the ref count by one, deleting 
 * self if count reaches zero.
 *
 * input:
 * void
 * 
 * output:
 * ULONG32			- Current ref count.
 *
 */
STDMETHODIMP_(ULONG32) 
CHXXResource::Release
(
    void
)
{
    // Decrement, return count if possible.
    if (InterlockedDecrement(&m_lRefCount) > 0) return m_lRefCount; 

    // Else, delete self.
    delete this;
    return 0;
}


	//
	//	Functions for determining information from a loaded resource.
	//
	
STDMETHODIMP_(ULONG32) CHXXResource::ID (void)
{
    return mID;
}
	
STDMETHODIMP_(ULONG32) CHXXResource::Type (void)
{
    return mType;
}

STDMETHODIMP_(ULONG32) CHXXResource::Length (void)
{
    return mLength;
}

STDMETHODIMP_(ULONG32) CHXXResource::Language (void)
{
    return mLanguage;
}

	

STDMETHODIMP_(IHXXResFile*) CHXXResource::ResFile (void)
{
    IHXXResFile*	result = NULL;
    QueryInterface(IID_IHXXResFile, (void**)&result);
    return result;
}
	
	//
	//	Data accessors
	//

STDMETHODIMP_(void*) CHXXResource::ResourceData (void)
{
    return mResData;
}
