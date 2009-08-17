/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pref.h,v 1.7 2006/02/07 19:21:32 ping Exp $
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

#ifdef _MACINTOSH
#pragma once
#endif

#ifndef _PREF
#define  _PREF

#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"

struct IHXBuffer;
struct IUnknown;

typedef struct _HXxRect	    HXxRect;
typedef struct _HXxSize	    HXxSize;
typedef struct _HXxPoint    HXxPoint;
typedef ULONG32		    HXxColor;

class CHXBuffer;
class CPref;

typedef _INTERFACE IHXPreferences IHXPreferences;

#define MAX_PREF_NAME	      256
#define MAX_PREF_SIZE	      1024
#define MAX_INT_BUFFER	      20
#define MAX_RECT_BUFFER	      60
#define MAX_SIZE_BUFFER	      30
#define MAX_POINT_BUFFER      30
#define HX_PRODUCTNAME_SHARED "HelixShared"    // Special shared product name. 

typedef struct _PrefTableEntry
{
    char*   szPrefName;
    char*   pDefaultValue;
} PrefTableEntry;

// Helper class used by CPref, but can also be used outside of CPref if a pref table is being used
class CPrefTable
{
public:
    CPrefTable(PrefTableEntry* pPrefTable,INT32 nTableEntries,CPref* pPrefs, IUnknown* pContext);
    CPrefTable(PrefTableEntry* pPrefTable,INT32 nTableEntries,IUnknown* pContext);
    virtual ~CPrefTable();

   HXBOOL		IsPrefSet(INT32 nPrefKey,INT32 nIndex=0);
   HX_RESULT	ReadPrefInt(INT32 nPrefKey,INT32& nValue,INT32 nIndex=0);
   HX_RESULT	ReadPrefColor(INT32 nPrefKey,HXxColor& color,INT32 nIndex=0);
   HX_RESULT	ReadPrefRect(INT32 nPrefKey,HXxRect& rect,INT32 nIndex=0);
   HX_RESULT	ReadPrefSize(INT32 nPrefKey,HXxSize& size,INT32 nIndex=0);
   HX_RESULT	ReadPrefPoint(INT32 nPrefKey,HXxPoint& pt,INT32 nIndex=0);
   HX_RESULT	ReadPrefString(INT32 nPrefKey,char* szString,INT32 nStrLen,INT32 nIndex=0);
   HX_RESULT	ReadPrefBuffer(INT32 nPrefKey,IHXBuffer*& pBuffer,INT32 nIndex=0);
    
   HX_RESULT	WritePrefInt(INT32 nPrefKey,INT32 nValue,INT32 nIndex=0);
   HX_RESULT	WritePrefColor(INT32 nPrefKey,const HXxColor& color,INT32 nIndex=0);
   HX_RESULT	WritePrefRect(INT32 nPrefKey,const HXxRect& rect,INT32 nIndex=0);
   HX_RESULT	WritePrefSize(INT32 nPrefKey,const HXxSize& size,INT32 nIndex=0);
   HX_RESULT	WritePrefPoint(INT32 nPrefKey,const HXxPoint& pt,INT32 nIndex=0);
   HX_RESULT	WritePrefString(INT32 nPrefKey,const char* szString,INT32 nIndex=0);
   HX_RESULT	WritePrefBuffer(INT32 nPrefKey,IHXBuffer* pBuffer,INT32 nIndex=0);
   HX_RESULT	RemoveIndexedPref(INT32 nPrefKey);
   HX_RESULT	RemovePref(INT32 nPrefKey);

   HX_RESULT	BeginSubPref(INT32 nPrefKey);
protected:
   HX_RESULT	ReadPref(INT32 nPrefKey,INT32 nIndex,IHXBuffer*& pBuffer);
   HXBOOL		ReadPoints(const char* pBuffer,HXxPoint* pt,int nNumPoints);

   HX_RESULT	WritePref(INT32 nPrefKey,INT32 nIndex,IHXBuffer* pBuffer);
   IHXBuffer*	CreateIHXBuffer(const char* szString);

   INT32	    m_nTableEntries;
   PrefTableEntry*  m_pPrefTable;   
   CPref*	    m_pCPref;
   IUnknown*	    m_pContext;
   IHXPreferences*  m_pIHXPrefs;
};

class CPref 
{
public:
/*    last_error() returns the last platform specific error that occurred */
   HX_RESULT last_error(void)   
   {
       return mLastError;
   };

/*    class destructor */  
   virtual              ~CPref         (void);

/*  read_pref reads the preference specified by Key to the Buffer. */
   virtual HX_RESULT read_pref(const char* pPrefKey, IHXBuffer*& pBuffer) = 0;

   /* To use these methods you need to call UsePrefTable() and setup up a product specific pref table */
   inline HXBOOL IsPrefSet(INT32 nPrefKey,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->IsPrefSet(nPrefKey,nIndex) : FALSE);
   }

   inline HX_RESULT ReadPrefInt(INT32 nPrefKey,INT32& nValue,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->ReadPrefInt(nPrefKey,nValue,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT ReadPrefColor(INT32 nPrefKey,HXxColor& color,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->ReadPrefColor(nPrefKey,color,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT ReadPrefRect(INT32 nPrefKey,HXxRect& rect,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->ReadPrefRect(nPrefKey,rect,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT ReadPrefSize(INT32 nPrefKey,HXxSize& size,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->ReadPrefSize(nPrefKey,size,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT ReadPrefPoint(INT32 nPrefKey,HXxPoint& pt,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->ReadPrefPoint(nPrefKey,pt,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT ReadPrefString(INT32 nPrefKey,char* szString,INT32 nStrLen,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->ReadPrefString(nPrefKey,szString,nStrLen,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT ReadPrefBuffer(INT32 nPrefKey,IHXBuffer*& pBuffer,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->ReadPrefBuffer(nPrefKey,pBuffer,nIndex) : HXR_FAIL);
   }

/*  write_pref writes (saves) the preference specified by Key from the Buffer. */       
   virtual HX_RESULT write_pref(const char* pPrefKey, IHXBuffer* pBuffer) = 0; 

/*  write_pref deletes the preference specified by Key from the Buffer. */       
   virtual HX_RESULT delete_pref(const char* pPrefKey) = 0; 

   /* commit_prefs saves all changes to the prefs to disk (e.g. on Unix) */ 
   virtual HX_RESULT commit_prefs() {return HXR_OK;};
   
   /* To use these methods you need to call UsePrefTable() and setup up a product specific pref table */
   inline HX_RESULT WritePrefInt(INT32 nPrefKey,INT32 nValue,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->WritePrefInt(nPrefKey,nValue,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT WritePrefColor(INT32 nPrefKey,const HXxColor& color,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->WritePrefColor(nPrefKey,color,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT WritePrefRect(INT32 nPrefKey,const HXxRect& rect,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->WritePrefRect(nPrefKey,rect,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT WritePrefSize(INT32 nPrefKey,const HXxSize& size,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->WritePrefSize(nPrefKey,size,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT WritePrefPoint(INT32 nPrefKey,const HXxPoint& pt,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->WritePrefPoint(nPrefKey,pt,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT WritePrefString(INT32 nPrefKey,const char* szString,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->WritePrefString(nPrefKey,szString,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT WritePrefBuffer(INT32 nPrefKey,IHXBuffer* pBuffer,INT32 nIndex=0)
   {
       return (m_pPrefTable ? m_pPrefTable->WritePrefBuffer(nPrefKey,pBuffer,nIndex) : HXR_FAIL);
   }

   inline HX_RESULT RemoveIndexedPref(INT32 nPrefKey)
   {
       return (m_pPrefTable ? m_pPrefTable->RemoveIndexedPref(nPrefKey) : HXR_FAIL);
   }

   inline HX_RESULT RemovePref(INT32 nPrefKey)
   {
       return (m_pPrefTable ? m_pPrefTable->RemovePref(nPrefKey) : HXR_FAIL);
   }

/*  call open_pref() to automatically create the correct platform specific preference 
    object. If open_pref() returns NULL, an error occurred and the CPref object was 
    not created. Call last_error to get the error */
#if defined(_CARBON) || defined(_MAC_UNIX)
   // for the Mac, we default to bCommon=FALSE since we want non-admin users to be able to run a new install, so by default
   // prefs get written per-user
   static CPref* open_pref(const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer, HXBOOL bCommon, IUnknown* pContext);
#else
   static CPref* open_pref(const char* pCompanyName, const char* pProductName, int nProdMajorVer, int nProdMinorVer, HXBOOL bCommon, IUnknown* pContext);
#endif

   static CPref* open_shared_pref(const char* pCompanyName, IUnknown* pContext);
   static CPref* open_shared_user_pref(const char* pCompanyName, IUnknown* pContext);

   void SetupPrefTable(PrefTableEntry* pPrefTable,INT32 nTableEntries);

/*  remove_indexed_pref removes indexed preference specified by Key */       
   virtual HX_RESULT remove_indexed_pref(const char* pPrefKey) = 0; 

   /* Sub-Preference Support */
   /* 
    * After calling BeginSubPref all calls to Read/Write preferences will write the preferences under the Sub-Prefernece
    * passed into BeginSubPref.  When done writing Sub-Preferences call EndSubPref.  Each call to BeginSubPref must
    * be matched with a call to EndSubPref otherwise preferences may be written to the incorrect place.  You can call
    * BeginSubPref multiple times to traverse deeper into sub-preferences, but each call must be matched with an EndSubPref
    * on the way out.
    */
   virtual HX_RESULT	BeginSubPref(const char* szSubPref) = 0;
   inline  HX_RESULT	BeginSubPref(INT32 nPrefKey)
   {
       return (m_pPrefTable ? m_pPrefTable->BeginSubPref(nPrefKey) : HXR_FAIL);
   }
   virtual HX_RESULT	EndSubPref() = 0;

   /* Preference Enumeration Support */
   /*
    * Gets a specified pref key for the specified index.  Use 0 to get the first pref key and then just keep incrementing
    * until GetPrefKey returns HXR_FAIL.  pBuffer will contain the pref key
    */
   virtual HX_RESULT GetPrefKey(UINT32 nIndex,IHXBuffer*& pBuffer) = 0;

   // remove the preference specified
   virtual HX_RESULT remove_pref(const char* pPrefKey)
   { HX_ASSERT(FALSE); return(HXR_FAIL); }
   
   virtual HXBOOL IsCommonPref() { return m_bIsCommonPref; }

protected:
/*  Constructor NOTE: use open_pref() to create an instance of this class */
   CPref(IUnknown* pContext);

   IUnknown*	   m_pContext;
   HX_RESULT       mLastError;       // last error to occur 
   CPrefTable*	   m_pPrefTable;     // The pref table information
   HXBOOL	   m_bIsCommonPref;  // pref was created bCommon (machine-wide)
};              

#endif // _PREF      
