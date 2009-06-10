/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cookies.h,v 1.16 2009/01/06 20:33:15 cbailey Exp $
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

#ifndef _COOKIES_H_
#define _COOKIES_H_

#ifdef _WINDOWS
typedef HXBOOL (PASCAL FAR	*INTERNETGETCOOKIE)(LPCSTR, LPCSTR, LPSTR, LPDWORD);
typedef HXBOOL (PASCAL FAR	*INTERNETSETCOOKIE)(LPCSTR, LPCSTR, LPCSTR);
#endif /* _WINDOWS */

#if defined(_MACINTOSH) || defined(_WINDOWS)
#include <time.h>
#endif

#include "ihxcookies.h"
#include "ihxcookies2.h"

struct CookieStruct
{
    CookieStruct()
    {
	pPath = NULL;
	pHost = NULL;
	pCookieName = NULL;
	pCookieValue = NULL;
    }

    ~CookieStruct()
    {
	HX_DELETE(pPath);
	HX_DELETE(pHost);
	HX_DELETE(pCookieName);
	HX_DELETE(pCookieValue);
    }

    CHXString*  pPath;
    CHXString*  pHost;
    CHXString*  pCookieName;
    CHXString*  pCookieValue;
    time_t	expires;
    HXBOOL	bIsDomain;
    HXBOOL	bMemoryOnly; //wasn't read from disk, but arrived via SetCookies (usually from playing a stream)
};

class HXCookies : public IHXCookies, public IHXCookies2, public IHXCookies3
{
protected:  

    LONG32		m_lRefCount;
    IUnknown*		m_pContext;
    
    UINT32              m_ulExternalCookies;
    HXBOOL		m_bInitialized;
    HXBOOL		m_bSaveCookies;
    HXBOOL              m_bMemoryOnly;
    char*		m_pNSCookiesPath;
    char*               m_pFFTextCookiesPath;
    char*               m_pFFSQLiteCookiesPath;
    char*               m_pFFProfilePath;
    char*		m_pRMCookiesPath;
    
    time_t		m_lastRMCookiesFileModification;
    time_t              m_lastFFTextCookiesFileModification;
    time_t              m_lastFFSQLiteCookiesFileModification;

    CHXSimpleList*	m_pNSCookies;
    CHXSimpleList*      m_pFFCookies;
    CHXSimpleList*	m_pRMCookies;
    CHXSimpleList*      m_pSFCookies;

    IHXPreferences*	m_pPreferences;
    IHXCookiesHelper*	m_pCookiesHelper;

#ifdef _WINDOWS
    HINSTANCE		m_hLib;
    IHXEvent*		m_pLock;

    INTERNETSETCOOKIE	_pInternetSetCookie;
    INTERNETGETCOOKIE	_pInternetGetCookie;
#elif _UNIX
    int			m_fileID;
#endif /* _WINDOWS */
    
    HX_RESULT       PrepareCookies(void);
    HX_RESULT	    OpenCookies(char* pCookieFile, HXBOOL bRMCookies, CHXSimpleList*& pCookiesList);
    HX_RESULT       OpenCookiesSQLite(char* pCookieFile, CHXSimpleList*& pCookiesList);
    HX_RESULT	    SaveCookies(void);
    HX_RESULT	    AddCookie(CookieStruct* pCookie, CHXSimpleList*& pCookiesList);

    HX_RESULT	    FileReadLine(FILE* fp, char* pLine, UINT32 ulLineBuf, UINT32* pBytesRead);    

    void	    ResetCookies(CHXSimpleList* pCookieList);    
    HXBOOL	    WasCookieAdded(CHXSimpleList* pCookiesFound, CookieStruct* pCookie);
    HXBOOL	    IsCookieEnabled(void);
    CookieStruct*   CheckForPrevCookie(char * path,
				       char * hostname,
				       char * name);

    void	    UpdateModificationTime(const char* pFile, time_t& lastModification);
    HXBOOL	    IsCookieFileModified(const char* pFile, time_t lastModification);
    HX_RESULT	    MergeCookieList(CHXSimpleList* pFromList, CHXSimpleList* pToList);
    /*
    IHXBuffer*	    ConvertToAsciiString(char* pBuffer, UINT32 nBuffLen);
    IHXBuffer*	    ChecksumFile(char* pszFileName);
    HX_RESULT	    GenerateCookieFileChecksum(REF(IHXBuffer*) pChecksum);
    UINT32	    GetHDSerialNumber();
    void	    DeleteTamperProofCookies();
    bool	    IsTimeProgressing();
    void	    DecryptTime(const char* szTime, char* szDecryptedTime, int cbEncryptedTime, const char* szKey);
    void	    EncryptTime(const char* szTime, char* szEncryptedTime, int cbEncryptedTime, const char* szKey);
    */
    HX_RESULT       GetCookiesInternal(const char* pHost,
				 const char*	    pPath,
				 REF(IHXBuffer*)   pCookies,
				 REF(IHXBuffer*)   pPlayerCookies);
    HXBOOL	    DoesDomainMatch(const char* szDomain, const char* szDomainToParse);
    HXBOOL          DoPathsMatch   (const char* szCookiesPath, const char* szUrlPath);

    HX_RESULT	    _prepareRMCookies(void);
    HX_RESULT       _prepareIECookies(void);
    HX_RESULT       _prepareNSCookies(void);
    HX_RESULT       _prepareFFTextCookies(void);
    HX_RESULT       _prepareFFSQLiteCookies(void);
    HX_RESULT       _prepareSFCookies(void);
    HXBOOL          _isFFCookiesEnabled(const char* pszPrefsPath);

    HX_RESULT       SyncFFCookies(UINT32 ulCookieType);
    HX_RESULT       GetFFPaths(UINT32 ulCookieType, char*& pszCookiesPath, char*& pszProfilePath);

    // prefix '/' to pPathIn if it's not present
    void            FixPath(const char* pPathIn, char*& pPathOut);

public:
    HXCookies(IUnknown* pContext, HXBOOL bMemoryOnly = FALSE);
    virtual ~HXCookies();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXCookies methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCookies::SetCookies
     *	Purpose:
     *	    Set cookies
     */
    STDMETHOD(SetCookies)	(THIS_
				 const char*	pHost,
				 const char*	pPath,
				 IHXBuffer*	pCookies);

    /************************************************************************
     *	Method:
     *	    IHXCookies::GetCookies
     *	Purpose:
     *	    Get cookies
     */
    STDMETHOD(GetCookies)	(THIS_
				 const char*	    pHost,
				 const char*	    pPath,
				 REF(IHXBuffer*)   pCookies);

    /************************************************************************
     *	Method:
     *	    IHXCookies2::GetExpiredCookies
     *	Purpose:
     *	    Get expired cookies
     */
    STDMETHOD(GetExpiredCookies)(THIS_ const char*  pHost,
    				 const char*	    pPath,
    				 REF(IHXBuffer*)   pCookies);

    /************************************************************************
     *	Method:
     *	    IHXCookies2::GetCookies
     *	Purpose:
     *	    Get cookies
     */
    STDMETHOD(GetCookies)	(THIS_
				 const char*	    pHost,
				 const char*	    pPath,
				 REF(IHXBuffer*)   pCookies,
				 REF(IHXBuffer*)   pPlayerCookies);

    /************************************************************************
     *	Method:
     *	    IHXCookies3::SyncRMCookies
     *	Purpose:
     *	    Synchronize RM cookies
     */
    STDMETHOD(SyncRMCookies)	(THIS_ HXBOOL bSave);

    virtual HX_RESULT	    SecureCookies();
    virtual HX_RESULT	    CheckCookies();

    HX_RESULT	Initialize(void);
    void	Close(void);
    void        SetMemoryOnlyFlag(HXBOOL bMemoryOnly) { m_bMemoryOnly = bMemoryOnly; }
    HXBOOL      GetMemoryOnlyFlag() const           { return m_bMemoryOnly;        }

    void        ProcessSelectResultPerRow(int num_fields, char **p_fields, char **p_col_names);

#ifdef _TEST
    void	DumpCookies();
#endif // _TEST
};

#endif /* _COOKIES_H_ */

