/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllpath.h,v 1.8 2006/07/19 14:15:28 damann Exp $
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

#ifndef _DLL_PATH
#define _DLL_PATH

#include "hxcom.h"
#include "hxmap.h"
#include "hxstring.h"

#ifdef _MACINTOSH
#pragma export on
STDAPI SetDLLAccessPath(const char* pPathDescriptor);
#pragma export off
#endif

#if defined(HELIX_CONFIG_NOSTATICS)
#include "globals/hxglobals.h"
#endif

/*
 *      Used to identify dll types.
 */
typedef enum dll_types
{
        DLLTYPE_NOT_DEFINED = 0,              // Arbitrary DLLs (no predefined path used)
        DLLTYPE_PLUGIN,                       // Plug-ins
        DLLTYPE_CODEC,                        // Codecs
        DLLTYPE_ENCSDK,                       // Encoder SDK DLLs
        DLLTYPE_COMMON,                       // Common libraries
        DLLTYPE_UPDATE,                       // Setup/Upgrade libraries
        DLLTYPE_OBJBROKR,                     // Special entry for the object broker
        DLLTYPE_RCAPLUGIN,                    // Gemini plugins
        DLLTYPE_NUMBER                        // Not a type, used as number of predefined types.
} DLLTYPES;

typedef HX_RESULT (HXEXPORT_PTR FPSETDLLACCESSPATH) (const char*);


class DLLAccessPath
{
public:
    DLLAccessPath();
    virtual ~DLLAccessPath();

    // This class is only ref-counted if it is used as such.
    // Most of the system uses this as a non-refcounted class.
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    HX_RESULT SetAccessPaths(const char* pPathDescriptor);
    HX_RESULT SetPath(UINT16 nLibType, const char* szPath);
    HX_RESULT SetPath(const char* szLibType, const char* szPath);

    const char* GetPath(UINT16 nLibType);
    const char* GetPath(const char* szLibType);
    const char* GetLibTypeName(UINT16 nLibType);

    HX_RESULT PassDLLAccessPath(FPSETDLLACCESSPATH pSetDLLAccessPath);

    HX_RESULT AddPathToEnvironment(const char* szPath);
    HX_RESULT RestoreEnvironment();
    UINT32 GetNumPaths() {return m_mapPathes.GetCount();}

protected:

    static const char* const zm_pszDllTypeNames[DLLTYPE_NUMBER];

private:

    LONG32               m_lRefCount;

    CHXMapStringToString m_mapPathes;
    CHXString            m_strPathEnvVar;
};

extern DLLAccessPath* GetDLLAccessPath();

class DLLAccessDestructor
{
public:
    DLLAccessDestructor() {};
    ~DLLAccessDestructor()
    {
#ifndef _VXWORKS
        if (GetDLLAccessPath())
        {
            GetDLLAccessPath()->Release();
        }
#endif
    }
};


//
// Macros for setting DLL loading paths
//
#ifndef _VXWORKS

#if defined(_STATICALLY_LINKED) && !defined(HELIX_FEATURE_SERVER)

// We need this since many DLLs have this listed
// as an export, so we have to have it defined
#define ENABLE_DLLACCESS_PATHS(GLOBAL)                           \
STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor) \
{                                                                \
    return HXR_OK;                                               \
}

#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                 \
STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor) \
{                                                                \
        return HXR_OK;                                           \
}

#elif defined(HELIX_CONFIG_NOSTATICS)

#define ENABLE_DLLACCESS_PATHS(GLOBAL)                                      \
    static const DLLAccessPath* const _g_##GLOBAL = NULL;                   \
                                                                            \
    DLLAccessPath* ENTRYPOINT(GetDLLAccessPath)()                           \
    {                                                                       \
        return &HXGlobalDLLAccessPath::Get(&_g_##GLOBAL);                    \
    }                                                                       \
                                                                            \
    STDAPI HXEXPORT ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return (GetDLLAccessPath()->SetAccessPaths(pPathDescriptor));       \
    }

#else /* #if defined(_STATICALLY_LINKED) && !defined(HELIX_FEATURE_SERVER) */

#define ENABLE_DLLACCESS_PATHS(GLOBAL)                                      \
    DLLAccessPath GLOBAL;                                                   \
                                                                            \
    DLLAccessPath* ENTRYPOINT(GetDLLAccessPath)()                           \
    {                                                                       \
        return &GLOBAL;                                                     \
    }                                                                       \
                                                                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return (GetDLLAccessPath()->SetAccessPaths(pPathDescriptor));       \
    }

#ifdef _UNIX

#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    DLLAccessPath* GLOBAL = NULL;                                           \
                                                                            \
    DLLAccessPath* GetDLLAccessPath()                                       \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
        return GLOBAL;                                                      \
    }                                                                       \
                                                                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)                \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
                                                                            \
        return (GLOBAL->SetAccessPaths(pPathDescriptor));                   \
    }

#else /* #ifdef _UNIX */

#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    DLLAccessPath* GLOBAL = NULL;                                           \
    DLLAccessDestructor GLOBALDestructor;                                   \
                                                                            \
    DLLAccessPath* GetDLLAccessPath()                                       \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
        return GLOBAL;                                                      \
    }                                                                       \
                                                                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)                \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
                                                                            \
        return (GLOBAL->SetAccessPaths(pPathDescriptor));                   \
    }

#endif /* #ifdef _UNIX #else */

#endif /* #if defined(_STATICALLY_LINKED) #else */

#else /* #ifndef _VXWORKS */

#define ENABLE_DLLACCESS_PATHS(GLOBAL)                                      \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return 0;                                                           \
    }

#ifdef _SERVER
#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return 0;                                                           \
    }
#else
#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return 0;                                                           \
    }
#endif

#endif /* #ifndef _VXWORKS #else */

#endif /* #ifndef _DLL_PATH */

