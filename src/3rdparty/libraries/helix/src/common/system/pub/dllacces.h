/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllacces.h,v 1.15 2005/03/14 19:35:27 bobclark Exp $
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

#ifndef _DLLACCES_H_
#define _DLLACCES_H_

#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_OPENWAVE)
    #define OS_DLL_PATTERN_STRING   "*.dll"	
#elif _MAC_UNIX
    #define OS_DLL_PATTERN_STRING   "*.bundle"
#elif _UNIX
    #define OS_DLL_PATTERN_STRING   "*.so*"
#elif _MACINTOSH
#ifdef _CARBON
#ifdef _MAC_MACHO
    #define OS_DLL_PATTERN_STRING   "*.bundle"
#else
    #define OS_DLL_PATTERN_STRING   "*.shlb"
#endif
#else
    #define OS_DLL_PATTERN_STRING   "*.dll"
#endif
#else	// undefined platform
#error dll extension undefined on platform
#endif

#include "hxtypes.h"


class DLLAccessPath;

class DLLAccessImp
{
public:
    virtual ~DLLAccessImp() {}
    virtual int Open(const char* dllName) = 0;
    virtual int Close() = 0;
    virtual void* GetSymbol(const char* symbolName) = 0;
    virtual const char* GetErrorStr() const = 0;
    virtual char* CreateVersionStr(const char* dllName) const = 0;
    virtual DLLAccessPath* GetDLLAccessPath();
    virtual void CreateName(const char* short_name, const char* long_name, char* out_buf,
			   UINT32& out_buf_len, UINT32 nMajor, UINT32 nMinor)=0;
};

//
// This class defines the OS independent interface to loading, accessing,
// and unloading dynamic code from shared libraries.
//

class DLLAccess
{
public:
    /////////////////////////////////////////////////////////////
    // Function: 
    //     DLLAccess
    //
    // Parameters:
    //     None
    //
    // Returns:
    //     Nothing
    //
    // Notes:
    //     Default constructor initializes internal structures for
    //     subsequent call to open()
    //
    DLLAccess();

    /////////////////////////////////////////////////////////////
    // Function:
    //     DLLAccess
    //
    // Parameters:
    //     dllName - Name of shared library
    //
    // Returns:
    //     Nothing
    //
    // Notes:
    //     Attempts to load library dllName. If unsuccessful, m_curError
    //     is set to DLLAccess::NO_LOAD and platform specific error
    //     info is stored in m_curStringError.
    //     
    DLLAccess(const char* dllName, UINT16 nLibType = 0);

    //////////////////////////////////////////////////////////////
    // Function:
    //     ~DLLAccess
    //
    // Paramters:
    //     None
    //
    // Returns:
    //     Nothing
    //
    // Notes:
    //     Unloads library from memory. See 'close' below.
    //
    ~DLLAccess();

    ///////////////////////////////////////////////////////////////
    // Function:
    //     open(const char* dllName)
    //
    // Parameters:
    //     dllName - Name of shared library
    //
    // Returns:
    //     DLLAccess::OK if successful, else DLLAccess::NO_LOAD.
    //     Platform specific error info is stored in m_curStringError.
    //
    // Notes:
    //     
    int open(const char* dllName, UINT16 nLibType = 0);

    ///////////////////////////////////////////////////////////////
    // Function:
    //     close()
    //
    // Parameters:
    //     none
    //
    // Returns:
    //     DLLAccess::OK if successful, else DLL_ACCESS::NO_LOAD.
    //
    // Notes:
    //     Shared library usage is typically reference counted by the
    //     OS: the library is actually unloaded when the reference count
    //     reaches zero. Thus this call does not guarantee that the 
    //     library will be removed from memory.
    //
    int close();


    ///////////////////////////////////////////////////////////////
    // Function:
    //     getSymbol(const char* symName)
    //
    // Parameters:
    //     symName: symbol to retrieve from shared library
    //
    // Returns:
    //     ptr to code in library if successful, else returns NULL
    //     and m_curError is set to DLLAccess::BAD_SYMBOL.
    //
    // Notes:
    //
    void* getSymbol(const char* symName);

    ///////////////////////////////////////////////////////////////
    // Function:
    //     getError
    //
    // Parameters:
    //     none
    //
    // Returns:
    //     value of m_curError
    //
    // Notes:
    //     none
    //
    int getError() { return m_curError; }


    ///////////////////////////////////////////////////////////////
    // Function:
    //     getErrorString
    //
    // Parameters:
    //     none
    //
    // Returns:
    //     value of m_curErrorString
    //
    // Notes:
    //     none
    //
    const char* getErrorString() { return m_curErrorString; }

    ///////////////////////////////////////////////////////////////
    // Function:
    //     getDLLName
    //
    // Parameters:
    //     none
    //
    // Returns:
    //     value of m_DLLName
    //
    // Notes:
    //     none
    //
    const char* getDLLName() { return m_dllName; }

    ///////////////////////////////////////////////////////////////
    // Function:
    //     getVersion
    //
    // Parameters:
    //     none
    //
    // Returns:
    //     value of m_Version
    //
    // Notes:
    //     none
    //
    const char* getVersion() { return m_version; }

    /////////////////////////////////////////////////////////////
    // Function: 
    //     PreCloseNotification  (not a member function)
    //
    // Parameters:
    //     None
    //
    // Returns:
    //     Nothing
    //
    // Notes:
    //     This function does not exist in dllaccess, but it may exist in the
    //     dll that dllaccess is wrapped around.  Just before closing down the
    //     the dll, dllaccess checks to see if there is a function named
    //     PreCloseNotification, and if there is, it is called.  This lets
    //     the dll know it is about to be closed down, before its OS close
    //	   function is called.  This was helpful for Win16 (because you can't
    //     call FreeLibrary from the WEP), and could be useful for other
    //     platforms.  As of sept98, it was only implemented for windows.
    static const UINT32 EXTRA_BUF_LEN;
    static void CreateName(const char* short_name, const char* long_name, char* out_buf,
			   UINT32& out_buf_len);

    // This overloaded version has been added to allow user to specify major and minor
    // version of the DLL name you are trying to create. It will append major and minor version
    // to the name. If you use the other function, then the major and minor versions of pnmisc
    // will be used. See pnmisc.ver in that case.
    static void CreateName(const char* short_name, const char* long_name, char* out_buf,
			   UINT32& out_buf_len, UINT32 nMajor, UINT32 nMinor);

    enum { DLL_OK, NO_LOAD, BAD_SYMBOL, OUT_OF_MEMORY };

    ///////////////////////////////////////////////////////////////
    // Function:
    //     isOpen
    //
    // Parameters:
    //     none
    //
    // Returns:
    //     value of m_isOpen
    //
    // Notes:
    //     none
    //
    HXBOOL isOpen(void)
    { return(m_isOpen); }
    
private:
    DLLAccess(const DLLAccess&);	    // no defaut copy constructor
    DLLAccess& operator=(const DLLAccess&); // no default assignment op

    void setDLLName(const char* str);	    // set m_dllName
    void setVersion(const char* str);       // set m_version
    void setErrorString(const char* str);   // set m_curErrorString

    static DLLAccessImp* CreateDLLImp();
    friend class MetaDLLAccess;
    static DLLAccessImp* CreateMetaDLLImp();
    static DLLAccessImp* CreatePlatformDLLImp();
    static DLLAccessImp* CreateStaticDLLImp();

    DLLAccessImp* m_dllImp;
    int m_curError;            // last error
    char* m_curErrorString;    // last error string
    int m_isOpen;              // don't open lib more than once
    char* m_dllName;           // DLL name
    char* m_version;	       // DLL version
};

#endif	// _DLLACCES_H_
