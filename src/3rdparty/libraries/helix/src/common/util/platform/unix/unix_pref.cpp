/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unix_pref.cpp,v 1.20 2008/10/24 06:51:22 vkathuria Exp $
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

#ifdef _BEOS
#include <Path.h>
#include <FindDirectory.h>
#endif

#include <string.h>
#include <unistd.h>
#ifndef _VXWORKS
#include <strings.h>
#include <sys/uio.h> 
#include <sys/file.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#include "hxslist.h"
#include "hxmap.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxheap.h"
#include "hxstrutl.h"

#include "hxcom.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "pckunpck.h"

#include "unix_pref.h"
#include "dbcs.h"  // HXReverseFindChar prototype
#include "unix_misc.h"
#include "cunixprefutils.h"
#define ARE_PREFS_LOADED_PREF "ArePrefsLoaded"

extern char** environ;

static CHXString FixupCompany (const char *pCompany)
{
    char *pTmpCompany = new_string(pCompany);
    char *pos  = strchr(pTmpCompany, ',');
    if (pos) *pos = 0;
    strlwr(pTmpCompany);
    CHXString ret = pTmpCompany;
    HX_VECTOR_DELETE(pTmpCompany);
    return ret;
}

static char *CIGetenv(const char *pKey)
{
    UINT32 keylen = strlen(pKey);
    char *pArr = new char[keylen + 2];
    sprintf(pArr, "%s=", pKey); /* Flawfinder: ignore */
    for (char **ppEnv = environ ; *ppEnv ; ++ppEnv)
    {
	if (!strnicmp(*ppEnv, pArr, keylen +1))
	{
	    HX_VECTOR_DELETE(pArr);
	    return *ppEnv + keylen + 1;
	}
    }
    HX_VECTOR_DELETE(pArr);
    return NULL;
}

static int CIPutenv(const char *pKey, REF(CHXSimpleList) rEnvList)
{
    char *pNewVal = new_string(pKey);
    char *pEquals = strchr(pNewVal, '=');
    HX_ASSERT(pEquals);
    if (pEquals)
    {
        *pEquals = '\0';
        strlwr(pNewVal);
        *pEquals = '=';
    }
    rEnvList.AddTail(pNewVal);
    return putenv(pNewVal);
}

static void CIUnsetEnv(const char *pKey, REF(CHXSimpleList) rEnvList)
{
    unsigned int keylen = strlen(pKey);
    char *pArr = new char[keylen + 2];
    sprintf(pArr, "%s=", pKey); /* Flawfinder: ignore */

    for (char **ppEnv = environ ; *ppEnv ; ++ppEnv)
    {
        if (!strnicmp(*ppEnv, pArr, keylen +1))
        {
            //delete [] *ppEnv;  // pjg all uses of this delete the string
            while (*ppEnv)
            {
                *ppEnv = *(ppEnv+1);
                ppEnv++;
            }

            CHXSimpleList::Iterator ndx = rEnvList.Begin();
            for (; ndx != rEnvList.End(); ++ndx)
            {
                if(!strnicmp((const char*)(*ndx),pArr,strlen(pArr)))
                {
                	delete_string((char*)*ndx);
                	rEnvList.RemoveAt(ndx);
			break;					
                }
            }


            HX_VECTOR_DELETE(pArr);
            return;
        }
    }
    HX_VECTOR_DELETE(pArr);
}


void CUnixPref::ConstructFamily(CHXString &ret)
{
    HX_ASSERT(!strchr(m_pCompany, '\n'));
    HX_ASSERT(!strchr(m_pCompany, ' '));
    HX_ASSERT(!strchr(m_pCompany, '\t'));
    HX_ASSERT(!strchr(m_pCompany, '='));
    HX_ASSERT(!strchr(m_pProduct, '\n'));
    HX_ASSERT(!strchr(m_pProduct, ' '));
    HX_ASSERT(!strchr(m_pProduct, '\t'));
    HX_ASSERT(!strchr(m_pProduct, '='));
    
    UINT32 nEstimatedSize = strlen ("HXPref_") + 
	6 * 1 +   // for underscores
	strlen(m_pCompany) +
	strlen(m_pProduct) +
	30; // 2 ints + fudge;

    char *pTmp = new char[nEstimatedSize];

    SafeSprintf(pTmp, nEstimatedSize, "HXPref_%s_%s_%d_%d_", 
	    (const char *)m_pCompany, (const char *)m_pProduct, m_nMajor, m_nMinor);
    ret = pTmp;
    HX_VECTOR_DELETE(pTmp);
}

void CUnixPref::ConstructPref(const char *pref,
			      CHXString &ret)
{
    HX_ASSERT(!strchr(m_pCompany, '\n'));
    HX_ASSERT(!strchr(m_pCompany, ' '));
    HX_ASSERT(!strchr(m_pCompany, '\t'));
    HX_ASSERT(!strchr(m_pCompany, '='));
    HX_ASSERT(!strchr(m_pProduct, '\n'));
    HX_ASSERT(!strchr(m_pProduct, ' '));
    HX_ASSERT(!strchr(m_pProduct, '\t'));
    HX_ASSERT(!strchr(m_pProduct, '='));
    
    UINT32 nEstimatedSize = strlen ("HXPref_") + 
	6 * 1 +   // for underscores
	strlen(m_RootKeyName) +
	strlen(m_pCompany) +
	strlen(m_pProduct) +
	strlen(pref) + 
	30; // 2 ints + fudge;

    char *pTmp = new char[nEstimatedSize];

    if (strlen(m_RootKeyName))
    {
	SafeSprintf(pTmp, nEstimatedSize, "HXPref_%s_%s_%s_%d_%d_%s", 
		m_RootKeyName, (const char *)m_pCompany, (const char *)m_pProduct, 
		m_nMajor, m_nMinor, pref);
    }
    else
    {
	SafeSprintf(pTmp, nEstimatedSize, "HXPref_%s_%s_%d_%d_%s", 
		(const char *)m_pCompany, (const char *)m_pProduct, 
		m_nMajor, m_nMinor, pref);
    }
    ret = pTmp;
    HX_VECTOR_DELETE(pTmp);
}

void CUnixPref::ConstructPrefAssignment(const char *pref,
					const char *pValue,
					CHXString &ret,
                                        HXBOOL bEscapeValue)
{
    HX_ASSERT(!strchr(m_pCompany, '\n'));
    HX_ASSERT(!strchr(m_pCompany, ' '));
    HX_ASSERT(!strchr(m_pCompany, '\t'));
    HX_ASSERT(!strchr(m_pCompany, '='));
    HX_ASSERT(!strchr(m_pProduct, '\n'));
    HX_ASSERT(!strchr(m_pProduct, ' '));
    HX_ASSERT(!strchr(m_pProduct, '\t'));
    HX_ASSERT(!strchr(m_pProduct, '='));

    char *pEscapedValue = NULL;
    
    if (bEscapeValue)
    {
        EscapeNewLine(pValue, pEscapedValue);
    }

    UINT32 nEstimatedSize = strlen ("HXPref_") + 
	6 * 1 +   // for underscores
	strlen(m_RootKeyName) +
	strlen(m_pCompany) +
	strlen(m_pProduct) +
	strlen(pref) + 
	strlen(pEscapedValue ? pEscapedValue : pValue) +
	30; // 2 ints + fudge;

    char *pTmp = new char[nEstimatedSize];

    if (strlen (m_RootKeyName))
    {
	SafeSprintf(pTmp, nEstimatedSize, "HXPref_%s_%s_%s_%d_%d_%s=%s", 
		m_RootKeyName, (const char *)m_pCompany, (const char *)m_pProduct, 
		m_nMajor, m_nMinor, pref, 
		pEscapedValue? pEscapedValue :pValue);
    }
    else
    {
	SafeSprintf(pTmp, nEstimatedSize, "HXPref_%s_%s_%d_%d_%s=%s", 
		(const char *)m_pCompany, (const char *)m_pProduct, 
		m_nMajor, m_nMinor, pref, 
		pEscapedValue? pEscapedValue :pValue);
	
    }
    ret = pTmp;
    HX_VECTOR_DELETE(pTmp);
    HX_VECTOR_DELETE(pEscapedValue);
}

HX_RESULT CUnixPref::read_pref(const char* pPrefKey, IHXBuffer*& pBuffer)
{
    pBuffer = NULL;
    CHXString key;
    ConstructPref(pPrefKey, key);
    char *value;
    if ((value = (char*)CIGetenv(key)))
    {
	char *pUnescapedValue = NULL;
	UnescapeNewLine(value, pUnescapedValue);
	if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
	{
	    if (pUnescapedValue)
	    {
		pBuffer->Set((UCHAR*)pUnescapedValue, strlen(pUnescapedValue) + 1);
	    }
	    else
	    {
		pBuffer->Set((UCHAR*)value, strlen(value) + 1);
	    }
	}
	HX_VECTOR_DELETE(pUnescapedValue);
    }
    return pBuffer ? HXR_OK : HXR_FAIL;
}


HX_RESULT CUnixPref::write_pref(const char* pPrefKey, IHXBuffer* pBuffer)
{
    m_bWrite = TRUE;
    CHXString key;
    ConstructPref(pPrefKey, key);
    key += "=";

    if (pBuffer)
    {
	CHXString pValue = (const char *)(pBuffer->GetBuffer());
	ConstructPrefAssignment(pPrefKey, pValue, key, TRUE);
        CIPutenv(key,m_EnvList);
    }
    else
    {
	CHXString oldKey;
	ConstructPref(pPrefKey, oldKey);
	CIUnsetEnv(oldKey, m_EnvList);  
    }
    
    return HXR_OK;
}
    


/* commit_prefs saves all changes to the prefs to disk (e.g. on Unix) */ 
HX_RESULT CUnixPref::commit_prefs()
{
    if (!m_bWrite)
    {
	return HXR_OK;
    }

#ifndef _VXWORKS
    // if any new prefs were written, write prefs
    // open for writing
    mFile =  ::fopen ( m_pPath, "w");
    if (!mFile)
    {
	mFile = ::fopen( m_pPath, "w+");
    }
    
    // try to create it
    if (mFile)
    {
	/* change permissions to allow everyone to read the file 
	 * and owner/group to write only if I have to create this file
	 */
	
	/* PJG:  why does everyone get the right to read my prefs?  What about my MRU list? :) */
#if !defined(_VXWORKS) && !defined(_BEOS)
	mFileID = fileno(mFile);
	fchmod( mFileID, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
#endif
    }
    
    if (mFile)
    {
	mFileID = fileno(mFile);
	CHXString prefFamily;
	CHXString areLoadedPref;
	ConstructPref(ARE_PREFS_LOADED_PREF, areLoadedPref);
	UINT32 nAreLoadedPrefLen = strlen(areLoadedPref);
	ConstructFamily(prefFamily);
	UINT32 nPrefFamilyLen = strlen(prefFamily);
	
	for (char **ppEnv = environ ; *ppEnv ; ++ppEnv)
	{
	    if (!strnicmp(prefFamily, *ppEnv, nPrefFamilyLen))
	    {
		if (strnicmp(areLoadedPref, *ppEnv, nAreLoadedPrefLen)) // don't write out the placeholder
		{
		    fprintf(mFile, "%s\n", *ppEnv + nPrefFamilyLen);
		}
	    }
	}
    }
    else
    {
#ifdef _DEBUG
	fprintf(stderr,"Can't open file to write prefs: %s.\n", m_pPath);
#endif
    }
    
    if (mFile > 0) 
    {
	::fclose(mFile);       
	mFile = NULL;
	mFileID = -1;
    }

    // Remove file if it has zero length
    struct stat stat_buf;
    int err = stat(m_pPath, &stat_buf);
    if ( !err && stat_buf.st_size == 0 )
    {
	// if file is empty, delete the file.
	unlink(m_pPath);
    }
#endif

    m_bWrite = FALSE;
    return HXR_OK;
}



/////////////////////////////////////////////////////////////////////////////
// 	call open_pref() to automatically create the correct UNIX specific 
//	preference object.    
//
CUnixPref * CUnixPref::open_pref(
			const char* pCompanyName, 
			const char* pProductName, 
			int nProdMajorVer, 
			int nProdMinorVer,
			IUnknown* pContext)
{
    CUnixPref *cp =  new CUnixPref(
	pCompanyName, 
	pProductName, 
	nProdMajorVer, 
	nProdMinorVer,
	pContext);
    if (!SUCCEEDED(cp->last_error())) {
	delete cp;
	cp = NULL;
    }
    return cp;
}

/////////////////////////////////////////////////////////////////////////////
// 	Constructor NOTE: use open_pref() to create an instance of this class
//
CUnixPref::CUnixPref(
    const char* pCompanyName, 
    const char* pProductName, 
    int nProdMajorVer, 
    int nProdMinorVer,
    IUnknown* pContext):CPref(pContext)
{   
    m_pPath	= NULL;
    mFile	= NULL;
    mFileID	= -1;
    mLastError	= HXR_OK;
    m_bWrite	= FALSE;
    m_RootKeyName[0] = '\0';
    init_pref( pCompanyName,pProductName,nProdMajorVer,nProdMinorVer);          
}

/////////////////////////////////////////////////////////////////////////////
// 	class destructor 
CUnixPref::~CUnixPref (void)
{
     char *pStrToBeUnset = NULL;
     char *pEquals = NULL;
#ifdef _SUN
     char *pCopyStrToBeUnset =NULL;
#endif
    commit_prefs();
    while( !m_EnvList.IsEmpty() )
    {
        pStrToBeUnset = (char *)(m_EnvList.RemoveHead());
#ifdef _SUN
	pCopyStrToBeUnset = new_string(pStrToBeUnset);
	pEquals = strchr(pCopyStrToBeUnset, '=');
#else
	pEquals = strchr(pStrToBeUnset, '=');
#endif
        if (pEquals)
          {
            *pEquals = '\0';
         }
#ifdef _SUN
	unsetenv(pCopyStrToBeUnset);
	delete_string(pCopyStrToBeUnset);
#else
    char **ppEnv = NULL;
    for (ppEnv = environ ; *ppEnv ; ++ppEnv)
    {
        if (strstr(*ppEnv, pStrToBeUnset))
        {
            unsetenv(pStrToBeUnset);
            while (*ppEnv)
            {
                *ppEnv = *(ppEnv+1);
                ppEnv++;
            }
            break;					
        }
    }
#endif
      delete_string(pStrToBeUnset);


    }    


    HX_VECTOR_DELETE(m_pPath);
}

/*  delete_pref deletes the preference specified by Key from the Buffer. */	                                                                            
HX_RESULT CUnixPref::delete_pref(const char* pPrefKey)
{   
    return write_pref(pPrefKey, NULL);
}


/////////////////////////////////////////////////////////////////////////////
//   init opens pref file if found, otherwise it creates it;
//   reads the pref file into an internal buffer.
//
HX_RESULT CUnixPref::init_pref( 
            const char* pCompanyName, 
            const char* pProductName, 
            int nProdMajorVer, 
            int nProdMinorVer)
{
    m_nMajor = nProdMajorVer;
    m_nMinor = nProdMinorVer;

    m_pCompany = FixupCompany(pCompanyName);
    m_pProduct = pProductName;

    
    HX_RESULT		theErr = HXR_OK;
    char 					stmp[32]; /* Flawfinder: ignore */
#if defined(_SUN) || defined(_HPUX)
    char        rcPath[ _POSIX_PATH_MAX ]; /* Flawfinder: ignore */
    CUnixPrefUtils::GetPrefPath(rcPath, PATH_MAX, pCompanyName);
#else
    char        rcPath[ PATH_MAX ]; /* Flawfinder: ignore */
    CUnixPrefUtils::GetPrefPath(rcPath, PATH_MAX, pCompanyName);
#endif

#ifndef _VXWORKS
#if defined(_BEOS)
    SafeStrCat( rcPath, "/", sizeof(rcPath));
#else
    //::strcat( rcPath, "/." );
    SafeStrCat( rcPath, "/", sizeof(rcPath));  //Note that the "." has been removed. Files no longuer start with "."
#endif /* _BEOS */

    if ( pProductName )
    {
	SafeStrCat( rcPath, pProductName, sizeof(rcPath));
	// remove any , or space
    	char * pComa = (char*) HXFindChar(rcPath, ',');
    	if(pComa)
    	{
      	    *pComa = 0;
    	}

    	pComa = (char*) HXFindChar(rcPath, ' ');
    	if(pComa)
    	{
      	    *pComa = 0;
    	}

	SafeStrCat( rcPath, "_", sizeof(rcPath));
    }

    ::sprintf(stmp, "%d_%d", nProdMajorVer,nProdMinorVer); /* Flawfinder: ignore */
    SafeStrCat(rcPath, stmp, sizeof(rcPath));

    m_pPath = new char[ ::strlen( rcPath ) + 1 ];
    strcpy( m_pPath, rcPath ); /* Flawfinder: ignore */

    // we're done now if the environment has been primed
    CHXString prefTester ;
    ConstructPref("ArePrefsLoaded", prefTester);
    if (CIGetenv(prefTester))
    {
	return HXR_OK; // already loaded :)
    }

    // open pref file for reading and writing; if it doesn't exist
    // create one.
    if ( (mFile = fopen ( m_pPath, "r"))  == NULL )
    {
#ifdef _DEBUG
	fprintf(stderr,"Can't open file: %s.\n", m_pPath);
#endif
    }
    else
    {
	mFileID = fileno(mFile);
    }

#endif /* _VXWORKS */

    
    IHXBuffer* buf = NULL;
    if (HXR_OK == CreateAndSetBufferCCF(buf, (UCHAR*)"1", 2, m_pContext))
    {
	write_pref(ARE_PREFS_LOADED_PREF, buf);
	buf->Release();
    }

#ifndef _VXWORKS
    if (!theErr && mFile > 0)
    {
	// Read the contents of the file into memory.
	size_t lNumTotalRead = 0;
	struct stat stat_buf;
	char* pfile_data = 0;
	int err = stat(m_pPath, &stat_buf);
	if ( !err && stat_buf.st_size > 0 )
	{
	    pfile_data = new char [ stat_buf.st_size + 1];

            lNumTotalRead = fread(pfile_data+lNumTotalRead, 1, stat_buf.st_size, mFile);

            pfile_data[lNumTotalRead] = '\0';

	    HX_ASSERT(lNumTotalRead == (size_t) stat_buf.st_size);
	}

	// Now break the data into lines delimited by newlines, and then
	// break those strings up into name/value pairs.
	if ( lNumTotalRead > 0 && stat_buf.st_size > 0 )
	{
	    char* snl = 0;
	    char* seq = 0;
	    char* stmp = pfile_data;
	    int name_len = 0;
	    snl = strtok( stmp, "\n");
	    while( snl )
	    {
		// now break up each line into name/value pairs...
		seq = strstr( snl, "=" );

		if(seq)
		{
		    name_len = seq-snl;

		    char* keyname = new char [name_len+1];
		    strncpy( keyname, snl, name_len); /* Flawfinder: ignore */
		    keyname[name_len] = '\0';

		    char* lwr = new char[strlen(keyname) + 1];
		    strcpy(lwr, keyname); /* Flawfinder: ignore */

		    seq++;
		    
		    // if we find in environ already, ignore
		    IHXBuffer *pBuffer = NULL;
		    if (HXR_OK ==read_pref(lwr, pBuffer))
		    {
			pBuffer->Release();
		    }
		    else
		    {
			CHXString assn;
			ConstructPrefAssignment(lwr, seq, assn, FALSE);
			HX_VERIFY(-1 != CIPutenv(assn, m_EnvList));
		    }
		    HX_VECTOR_DELETE(keyname);
		    HX_VECTOR_DELETE(lwr);
		}

		snl = strtok( NULL, "\n");
	    }
	}

	HX_VECTOR_DELETE(pfile_data);
    }

    if ( mFile )
    {
	// unlock the file 
        ::fclose( mFile );
        mFile = NULL;
	mFileID = -1;
    }
#endif /* _VXWORKS */

    m_bWrite = FALSE;      // we use write_pref internally, but we don't want to consider our
                           // database to be modified at this point.
    mLastError = theErr;
    return theErr;      
}

HX_RESULT CUnixPref::remove_pref(const char* pPrefKey)
{
    return write_pref(pPrefKey, NULL);
}

HX_RESULT CUnixPref::remove_indexed_pref(const char* pPrefKey)
{
    return HXR_NOTIMPL;
}


//
// Access a preferences file subtree by appending a string to the Root
// Key.  
//
HX_RESULT CUnixPref::BeginSubPref(const char* szSubPref)
{

    HX_ASSERT((strlen(m_RootKeyName) + 1 + strlen(szSubPref) + 1) <= _MAX_KEY);

    // Just add sub-pref to the root key name
    SafeStrCat(m_RootKeyName,"\\", _MAX_KEY);
    SafeStrCat(m_RootKeyName,szSubPref, _MAX_KEY);

    return HXR_OK;
}

HX_RESULT CUnixPref::EndSubPref()
{
    // Find the \ character from the end of the string
    char* pSlash = HXReverseFindChar(m_RootKeyName,'\\');

    // if we find the \ character we NULL terminate at this point removing the last sub-pref added
    if (pSlash)
    {
	*pSlash = '\0';
	return HXR_OK;
    }
    else
	return HXR_FAIL;
}


//
// Return the value of subkey number nIndex under the current m_RootKeyName.
//
HX_RESULT CUnixPref::GetPrefKey(UINT32 nIndex, IHXBuffer*& pBuffer)
{

    CHXString		key;

    CHXString prefFamily;
    ConstructFamily(prefFamily);
    UINT32 nPrefFamilyLen = strlen(prefFamily);
    
    UINT32 counter = 0;
    
    for (char **ppEnv = environ ; *ppEnv ; ++ppEnv)
    {
	if (!strnicmp(prefFamily, *ppEnv, nPrefFamilyLen))
	{
	    // make sure this isn't the head of the family
	    if ('=' == (*ppEnv)[nPrefFamilyLen])
		continue;
	    
	    if (counter == nIndex)
	    {
		char *value = (*ppEnv) + nPrefFamilyLen + 1;
                char *ueValue = NULL;
                UnescapeNewLine(value, ueValue); 
		if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
		{
		    pBuffer->Set((UCHAR*)(ueValue? ueValue : value), strlen(ueValue ? ueValue : value) + 1);
		    HX_VECTOR_DELETE(ueValue);
		    return HXR_OK;
		}
		else
		{
                    HX_ASSERT("Could not create pref key!"==NULL);
		    break;
		}
	    }
	    counter++;
	}
    }
    
    return HXR_FAIL;
}

HXBOOL
CUnixPref::EscapeNewLine(const char* pLine, char*& pTmpOutLine)
{
    char    hexBuf[3] = {0}; /* Flawfinder: ignore */
    pTmpOutLine = NULL;
    char* pOutLine = NULL;

    const char* pInputLine = pLine;
    while (*pLine)
    {
	// escape \n, = and %
        if (*pLine == '\n' || *pLine == '=' || *pLine == '%')
        {
	    if (!pOutLine)
	    {
	        pTmpOutLine = pOutLine = new char[strlen(pInputLine) * 3 + 1];
		*pOutLine = '\0';
		strncpy(pOutLine, pInputLine, pLine - pInputLine); /* Flawfinder: ignore */
		pOutLine += pLine - pInputLine;
	    }

	    sprintf(hexBuf,"%02x", (UCHAR)*pLine); /* Flawfinder: ignore */
	    *pOutLine++ = '%';
	    *pOutLine++ = hexBuf[0];
	    *pOutLine++ = hexBuf[1];
        }
	else if (pOutLine)
	{
	    *pOutLine++ = *pLine;
	}

	pLine++;
    }

    if (pOutLine) *pOutLine = '\0';

    return (pOutLine ? TRUE : FALSE);
}

HXBOOL
CUnixPref::UnescapeNewLine(const char* pLine, char*& pTmpOutLine)
{
    pTmpOutLine = NULL;
    char* pOutLine = NULL;

    const char* pInputLine = pLine;
    while (*pLine)
    {
        if (*pLine == '%')
        {
	    if (!pOutLine)
	    {
	        pTmpOutLine = pOutLine = new char[strlen(pInputLine)+1];
		*pOutLine = '\0';
		strncpy(pOutLine, pInputLine, pLine - pInputLine); /* Flawfinder: ignore */
		pOutLine += pLine - pInputLine;
	    }

            char hexBuf[3] = {0}; /* Flawfinder: ignore */
	    if(pLine[1] &&    // check for overbound condition
	       pLine[2])
	    {
	        pLine++;  // walk past '%'
	        hexBuf[0] = *pLine++;
	        hexBuf[1] = *pLine;
	        hexBuf[2] = '\0';
	        *pOutLine++ = (char)strtol(hexBuf, NULL, 16);
  	    }	
        }
	else if (pOutLine)
	{
	    *pOutLine++ = *pLine;
	}

	pLine++;
    }

    if (pOutLine) *pOutLine = '\0';

    return (pOutLine ? TRUE : FALSE);
}


