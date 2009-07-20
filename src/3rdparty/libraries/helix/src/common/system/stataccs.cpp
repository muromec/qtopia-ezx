/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stataccs.cpp,v 1.15 2005/03/14 19:35:25 bobclark Exp $
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
#include "hxcom.h"
#include "hlxclib/string.h"
#include "hlxclib/stdio.h"
#include "hxassert.h"
#include "dllacces.h"
#include "dllpath.h"
#include "stataccs.h"
#include "hxstrutl.h"

#ifndef HELIX_CONFIG_NOSTATICS
extern HXBOOL g_bReportHXCreateInstancePointer;
#endif

//#define DLLACCESS_VERBOSE

#if defined(_DEBUG) && defined(DLLACCESS_VERBOSE)
#define DLLACCESS_FPRINTF(x) fprintf x
#else 
#define DLLACCESS_FPRINTF(x)
#endif 

#if defined(_WINDOWS) || defined(_SYMBIAN)
#define DLL_SUFFIX ".dll"
#elif defined(_MACINTOSH)
#if defined(_CARBON)
#ifdef _MAC_MACHO
#define DLL_SUFFIX ".bundle"
#else
#define DLL_SUFFIX ".shlb"
#endif
#else // #if defined(_CARBON)
#define DLL_SUFFIX ".dll"
#endif // #if defined(_CARBON) #else
#endif // #elif defined(_MACINTOSH)

class StaticDLLAccess : public DLLAccessImp
{
public:
    StaticDLLAccess();
    virtual ~StaticDLLAccess();
    virtual int Open(const char* dllName);
    virtual int Close() ;
    virtual void* GetSymbol(const char* symbolName);
    virtual const char* GetErrorStr() const;
    virtual char* CreateVersionStr(const char* dllName) const;
    virtual DLLAccessPath* GetDLLAccessPath();
    virtual void CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor);

private:
    enum {ErrorBufSize = 256};

    char* m_pDLLName;
    char m_pErrorMesg[ErrorBufSize]; /* Flawfinder: ignore */
};

StaticDLLAccess::StaticDLLAccess() :
    m_pDLLName(0)
{
    m_pErrorMesg[0] = '\0';
}

StaticDLLAccess::~StaticDLLAccess()
{
    HX_VECTOR_DELETE(m_pDLLName);
}

int StaticDLLAccess::Open(const char* dllName)
{
    int ret = DLLAccess::NO_LOAD;
    
    DLLACCESS_FPRINTF((stderr, "open dll %s...", dllName));

    HXBOOL found = FALSE;
    
    DLLACCESS_FPRINTF((stderr, "received request to open %s\n", dllName));
    
    char* pTmpDllName = new_string(dllName);
    
    // Make sure this is lower-case
    strlwr(pTmpDllName);

#if defined(_UNIX)
    char* searcher = pTmpDllName + strlen(pTmpDllName);
    while (--searcher && (searcher > pTmpDllName) && (*(searcher - 1) != '/'))
    {
        if (*searcher == '.') //strip off .so.x.y
            *searcher = '\0';
    }
#ifdef HELIX_FEATURE_SERVER //is this really server-specific?
    if (*(searcher - 1) == '/')
    {
        memmove(pTmpDllName, searcher, (strlen(searcher) + 1));
    }
#endif
#elif defined(DLL_SUFFIX)
    // Strip off DLL_SUFFIX
    char* searcher = strstr(pTmpDllName, DLL_SUFFIX);
    if (searcher)
    {
        *searcher = '\0';
    }
#endif

    m_pDLLName = new_string(pTmpDllName);

    delete [] pTmpDllName;
    pTmpDllName = 0;

    for (const DLLMAP *mapentry = g_dllMap; mapentry->dllName; mapentry++)
    {
	if (!strcmp(mapentry->dllName, m_pDLLName))
	{
	    found = TRUE;
	    ret = DLLAccess::DLL_OK;

#if !defined (_MACINTOSH) && !defined(_WIN16) &&!defined(HELIX_CONFIG_NOSTATICS)
            if(g_bReportHXCreateInstancePointer)
            {
                printf ("    Entry Point %s %p\n", mapentry->dllName, mapentry->funcptr);
            }	    
#endif
	    DLLACCESS_FPRINTF((stderr, "succeeded\n"));
	}	
    }

    if (!found) 
    {        
	SafeSprintf(m_pErrorMesg, ErrorBufSize, 
		    "Application not linked against module %s", m_pDLLName);

	DLLACCESS_FPRINTF((stderr, "%s\n", errMsg));
	DLLACCESS_FPRINTF((stderr, "failed\n"));

    }

    return ret;
}

int StaticDLLAccess::Close()
{
    return DLLAccess::DLL_OK;
}

void* StaticDLLAccess::GetSymbol(const char* symbolName)
{
    void* pRet = 0;

    for (const DLLMAP *item = &g_dllMap[0]; item->dllName; item++) 
    {
	if (!strcmp(item->dllName, m_pDLLName) && 
	    !strcmp(item->entryPoint, symbolName))
	{
	    DLLACCESS_FPRINTF((stderr, "succeeded\n"));	

	    pRet = (void*)item->funcptr;
	}
    }

    if (!pRet)
    {
	SafeSprintf(m_pErrorMesg, ErrorBufSize, 
		    "Symbol \"%s\" not available in dll \"%s\"", 
		    symbolName, m_pDLLName);

	DLLACCESS_FPRINTF((stderr, "%s\n", m_pErrorMesg));
    }

    return pRet;
}

const char* StaticDLLAccess::GetErrorStr() const
{
    return m_pErrorMesg;
}

char* StaticDLLAccess::CreateVersionStr(const char* dllName) const
{
    return 0;
}

DLLAccessPath* StaticDLLAccess::GetDLLAccessPath()
{
    // The static code does not use an access path
    return 0;
}

void
StaticDLLAccess::CreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor)
{
    out_buf[0] = 0;
    SafeStrCpy(out_buf, long_name, out_buf_len);
}

DLLAccessImp* DLLAccess::CreateStaticDLLImp()
{
    return new StaticDLLAccess();
}
