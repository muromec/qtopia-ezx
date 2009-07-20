/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clntxres.h,v 1.8 2007/07/06 21:58:52 jfinnecy Exp $
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

#ifndef _clntxres_h
#define _clntres_h

#include "hxtypes.h"
#include "chxdataf.h"
#include "hxpeff.h"
#include "hxunicod.h"
#include "hxxres.h"
#include "hxplugn.h"
#include "hxcomm.h"
#include "hxstring.h"
#include "hxslist.h"
#include "baseobj.h"


/* CONSTANTS */


#define kDefaultCacheLimit      1024*8          // 8KB



/* STRUCTURES */


/* RESOURCE STRUCTURES  */




/* INTERNAL STRUCTURES */

typedef struct  _HX_IMAGE_RESOURCE_DIRECTORY
{
	ULONG32         Characteristics;
	ULONG32         TimeDateStamp;

	UINT16          MajorVersion;
	UINT16          MinorVersion;

	UINT16          NumberOfNamedEntries;
	UINT16          NumberOfIdEntries;


} HX_IMAGE_RESOURCE_DIRECTORY, *PHX_IMAGE_RESOURCE_DIRECTORY;


typedef struct _HX_IMAGE_RESOURCE_DIRECTORY_ENTRY
{

	ULONG32         Name;
	ULONG32         OffsetToData;

} HX_IMAGE_RESOURCE_DIRECTORY_ENTRY, *PHX_IMAGE_RESOURCE_DIRECTORY_ENTRY;


typedef struct  _HX_IMAGE_RESOURCE_DATA_ENTRY
{

	ULONG32         OffsetToData;
	ULONG32         Size;
	ULONG32         CodePage;
	ULONG32         Reserved;

} HX_IMAGE_RESOURCE_DATA_ENTRY, *PHX_IMAGE_RESOURCE_DATA_ENTRY;


/*
This structure is used to create a map of all the resources, id's and their location in the file.
*/
typedef struct tagXResCacheEntry
{
	ULONG32 type;                   // Resource type;
	ULONG32 id;                             // Resource ID;
	ULONG32 location;               // Location in file from, offset from the beginning of the file.
	ULONG32 language;               // Language of the data.
	ULONG32 size;                   // size of the data. 
	HXBOOL    cached;                 // Is the resource already cached in memory?
	UCHAR*  cached_data;    // Data of the resource when cached in memory.
} XResCacheEntry;


class   CHXSimpleList;
class   CHXXResource;

class   CHXXResFile : public IHXPlugin,
		      public IHXXResFile, 
		      public CHXPeff, 
		      public CHXUnicode,
		      public CHXBaseCountingObject
{
public:

	// IUnknown methods...
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj);
	STDMETHOD_(ULONG32, AddRef) (THIS);
	STDMETHOD_(ULONG32, Release) (THIS);


	// IHXPlugin methods...
	STDMETHOD(GetPluginInfo)    (THIS_
				    REF(HXBOOL)	 /*OUT*/ bMultipleLoad,
				    REF(const char*) /*OUT*/ pDescription,
				    REF(const char*) /*OUT*/ pCopyright,
				    REF(const char*) /*OUT*/ pMoreInfoURL,
				    REF(ULONG32)	 /*OUT*/ ulVersionNumber);

	STDMETHOD(InitPlugin)	    (THIS_
				    IUnknown*   /*IN*/  pContext);


	//
	//      CHXXResFile Methods
	//

	CHXXResFile();

	STDMETHOD_(HX_RESULT,Open)			(THIS_ const char* path);
	STDMETHOD_(HX_RESULT,Close)			(THIS);
	
	STDMETHOD_(HX_RESULT,SetLanguage)               (THIS_ ULONG32 id);
	STDMETHOD_(HX_RESULT,GetResource)               (THIS_ ULONG32 type, ULONG32 ID, IHXXResource** resource);
			
	STDMETHOD_(IHXXResource*,GetString)            (THIS_ ULONG32 ID);	
	STDMETHOD_(IHXXResource*,GetBitmap)            (THIS_ ULONG32 ID);
	STDMETHOD_(IHXXResource*,GetDialog)            (THIS_ ULONG32 ID);
	STDMETHOD_(IHXXResource*,GetVersionInfo)	(THIS);

	STDMETHOD(GetFirstResourceLanguage)		(THIS_ REF(ULONG32) ulLangID);
	STDMETHOD(GetNextResourceLanguage)		(THIS_ REF(ULONG32) ulLangID);

	STDMETHOD_(HXBOOL,IncludesShortName)		(THIS_ const char* pShortName);
	
	STDMETHOD_(HX_RESULT,FlushCache)                (THIS);
	STDMETHOD_(HX_RESULT,SetCacheLimit)             (THIS_ ULONG32 MaxCachedData);

	STDMETHOD(UseResourceFile)			(THIS_
			 				INT16   /*IN*/  nResourceFileRef);
		
private:

	virtual ~CHXXResFile				();

	HX_RESULT       FindInCache			(ULONG32 type,
							ULONG32 ID,  
							XResCacheEntry**  ppEntry);
	HX_RESULT       KillCache			(void);
	HX_RESULT       FindResourceData		(void);
	HX_RESULT       TrimCachedData			(ULONG32 needed);

	HX_RESULT       CacheResourceEntries		(void);
	HX_RESULT       ReadResourceHeader		(void);
	HX_RESULT       ReadInAllResources		(void);
	HX_RESULT       GetResourceEntry		(HX_IMAGE_RESOURCE_DIRECTORY_ENTRY& h);
	HX_RESULT       GetResourceDirEntry		(HX_IMAGE_RESOURCE_DIRECTORY& h);
	BYTE*		GetResInfo			(BYTE* pData, 
							UINT16& ulResInfoLen,
							UINT16& uResInfoType,
							CHXString& key);
	UINT32		GetCodePage			(void);


	IUnknown*				m_pContext;
	IHXCommonClassFactory*			m_pCommonClassFactory;

	ULONG32                                 mResourceDataPosition;          // Start of the Resource data in the file.
	ULONG32                                 mResSectionVirtualAddress;  // Virtual Address of the resource section.
	HX_IMAGE_RESOURCE_DIRECTORY             mResourceHeader;                        // Resource Header.
	ULONG32                                 mLanguageId;                        // Language ID of resources to load.

	ULONG32                                 mMaxCachedData;                         // Total number of bytes allowed to be cached.
	LONG32                                  m_lRefCount;                            // RefCount

	CHXString                               mFilePath;                                      // Path to the file being used.
	HXBOOL                                    mFileOpen;                                      // Is the file we are using open?
	LISTPOSITION				mCachePos;		    // used in GetFirst/GetNext resource methods

	CHXSimpleList*  mCacheList;     // Used to cache the resources as they are loaded.
	CHXSimpleList*  mLoadedCache; // Items which are loaded into memory.

	static const char* const		zm_pName;
	static const char* const		zm_pDescription;
	static const char* const		zm_pCopyright;
	static const char* const		zm_pMoreInfoURL;

	UINT32					m_nCodePage;
	INT16					m_nResFileRef;
};



class   CHXXResource: public IHXXResource
{
public:
	CHXXResource				(void* data,
						ULONG32 datalength, 
						ULONG32 ID, 
						ULONG32 Type, 
						ULONG32 Language, 
						IHXXResFile*  file);
	~CHXXResource				();
	

	STDMETHOD(QueryInterface)		(THIS_ REFIID riid, void **ppvObj);
	STDMETHOD_(ULONG32, AddRef)		(THIS);
	STDMETHOD_(ULONG32, Release)		(THIS);

	//
	//      Functions for determining information from a loaded resource.
	//
	
	STDMETHOD_(ULONG32,ID)                  (THIS);
	STDMETHOD_(ULONG32,Type)                (THIS); 
	STDMETHOD_(ULONG32,Length)              (THIS);
	STDMETHOD_(ULONG32,Language)		(THIS);

	STDMETHOD_(IHXXResFile*,ResFile)	(THIS);
	
	//
	//      Data accessors
	//

	STDMETHOD_(void*,ResourceData)		(THIS);

private:

	LONG32                  m_lRefCount;
	IHXXResFile*           mResFile;
	void*                   mResData;
	
	ULONG32                 mID;
	ULONG32                 mType;
	ULONG32                 mLength;
	ULONG32			mLanguage;
	

};

#endif // _clntxres_h
