/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef HXPACKAGE_H_
#define HXPACKAGE_H_

#define HX_REGISTER_ALL_COMPONENTS_

#include "unkimp.h"
#include "hxplugn.h"
#include "hxccf.h"
#include "chxmaplongtoobj.h"
#include "chxmapguidtoobj.h"
#include "hxcomponent.h"
#include "ihxtimer.h"
#include "ihxtimerobserver.h"
#include "ihxpackageunloader.h"
#include "hxnotify.h"
#include "hxstring.h"

#ifdef ENABLE_LOG_STREAMS
#include <fstream>
using namespace std;
#endif

enum EInfoType
{
    kInfo_BeginEntry,
    kInfo_EndTable,
    kInfo_IntType,
    kInfo_StringType,
    kInfo_BufferType
};


typedef HX_RESULT (*CREATEINSTANCEFUNC) (IHXPackage_&, IUnknown*, IUnknown** );

struct SComponentInfoListEntry
{
    EInfoType m_type;
    const char* m_pName;

    void* m_pData1;
    UINT32 m_dataSz;
    CREATEINSTANCEFUNC m_pFunc;
};


/*!
    @class CHXPackage
*/
class CHXPackage : 
    public CUnknownIMP,
    public IHXPlugin,
    public IHXComponentPlugin,
    public IHXPluginNamespace,
    public IHXPackage_
{
    typedef CUnknownIMP Super;

    DECLARE_ABSTRACT_COMPONENT (CHXPackage)
public:
    STDMETHOD(InitPlugin)(THIS_ IUnknown* pContext);

    // IHXComponentPlugin
    STDMETHOD(CreateComponentInstance)
				    (THIS_
				    REFCLSID	    rclsid,
				    REF(IUnknown*)  ppUnknown,
				    IUnknown*	    pUnkOuter);

    STDMETHOD_(UINT32, GetNumComponents)(THIS);
    STDMETHOD_(char const*, GetPackageName)(THIS) CONSTMETHOD;
    STDMETHOD(GetComponentInfoAtIndex) (THIS_ UINT32 nIndex, REF(IHXValues*) pInfo );

    STDMETHOD(GetPluginNamespace)	(THIS_
					REF(IHXBuffer*)  /*OUT*/ pBuffer);

    void OnComponentBirth ();
    void OnComponentDeath ();
    void CancelTimer ();
    long GetLiveComponents() { return m_LiveComponents; };
    
#ifdef ENABLE_LOG_STREAMS
    friend std::ostream& operator <<( std::ostream&, CHXPackage& );
#endif
    
protected:
    CHXPackage (CHXString const& module, SComponentInfoListEntry*, const char*, UINT32 interval = 2000);
    virtual ~CHXPackage ();

    void AddStringProperty_ (IHXValues* pValues, const char* pName, const char* pValue);
    void AddBufferProperty_ (IHXValues* pValues, const char* pName, const unsigned char* pValue, UINT32 dataLen);

    UINT32 m_numComponents;
    CHXMapGUIDToObj m_creationInfo;
    CHXMapLongToObj m_indexedInfo;

    IHXCommonClassFactory* m_pCCF;
    SPIHXPackageUnloader m_spPackageUnloader;

private:
    CHXString m_Module;
    long m_LiveComponents;
    SPIHXTimer m_spTimer;
    UINT32 m_TimerInterval;

    SComponentInfoListEntry* m_pComponentInfo;
    char const* m_pNamespace;

    // undefined
    CHXPackage (CHXPackage const&);
    CHXPackage& operator= (CHXPackage const&);
};



#define DECLARE_FACTORY_INFO( moduleName, multiLoad, name, copyright, moreInfo, version ) \
STDMETHODIMP CHX##moduleName##Package_::GetPluginInfo( REF(HXBOOL) bMultipleLoad,  \
						REF(const char*) pDescription, \
						REF(const char*) pCopyright,  \
						REF(const char*) pMoreInfoURL,  \
						REF(ULONG32) ulVersionNumber) \
{ \
    bMultipleLoad = multiLoad; \
    pDescription = name; \
    pCopyright = copyright; \
    pMoreInfoURL = moreInfo; \
    ulVersionNumber = version; \
    return HXR_OK; \
} 

#define DECLARE_FACTORY_SUPPORT_EX( moduleName, multiLoad, name, copyright, moreInfo, version, delay ) \
class CHX##moduleName##Package_ : public CHXPackage						    \
{												    \
    DECLARE_UNMANAGED_COMPONENT (CHX##moduleName##Package_)					    \
public:												    \
    CHX##moduleName##Package_ () : CHXPackage (name, m_pComponentInfoD, m_pNamespaceD, delay) { } \
    ~CHX##moduleName##Package_ () { m_pSelf = NULL; }						    \
												    \
    STDMETHOD (GetPluginInfo) (REF(HXBOOL) bMultipleLoad, REF(char const*) pDescription,		    \
	REF(char const*) pCopyright, REF(char const*) pMoreInfoURL, REF(ULONG32) ulVersionNumber);  \
												    \
    static IUnknown* GetInstance_ () { return m_pSelf; }					    \
    static void SetInstance_ (IUnknown* pIUnk) { m_pSelf = pIUnk; }				    \
												    \
private:											    \
    static SComponentInfoListEntry m_pComponentInfoD [];					    \
    static const char* m_pNamespaceD;								    \
    static IUnknown* m_pSelf;									    \
};												    \
												    \
IUnknown* CHX##moduleName##Package_::m_pSelf = NULL;						    \
												    \
DECLARE_FACTORY_INFO (moduleName, multiLoad, name, copyright, moreInfo, version)		    \
												    \
IMPLEMENT_UNMANAGED_COMPONENT (CHX##moduleName##Package_)					    \
												    \
BEGIN_COMPONENT_INTERFACE_LIST (CHX##moduleName##Package_)					    \
END_INTERFACE_LIST_BASE (CHXPackage)								    \
												    \
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCreateInstance) (IUnknown** ppIUnknown)						    \
{												    \
    if (!CHX##moduleName##Package_::GetInstance_ ())						    \
    { \
	HX_RESULT const res = CHX##moduleName##Package_::CreateUnmanagedInstance( ppIUnknown );\
	if (ppIUnknown && *ppIUnknown) CHX##moduleName##Package_::SetInstance_ (*ppIUnknown); \
    } \
    \
    if (!ppIUnknown) return HXR_INVALID_PARAMETER; \
    *ppIUnknown = CHX##moduleName##Package_::GetInstance_ (); \
    ULONG32 const refcount = (*ppIUnknown)->AddRef (); \
    \
    return HXR_OK;\
}\
\
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXShutdown) ()\
{ \
    IUnknown* pIUnk = CHX##moduleName##Package_::GetInstance_ (); \
    if (!pIUnk) return HXR_FAIL; \
    CHX##moduleName##Package_* pPackage = reinterpret_cast<CHX##moduleName##Package_*> (pIUnk); \
    ENFORCE_NOTIFY (pPackage, HXR_FAIL); \
    pPackage->CancelTimer (); \
    ENFORCE_NOTIFY (!pIUnk->Release (), HXR_FAIL); \
    return HXR_OK; \
} \
\
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2) ()\
{ \
    IUnknown* pIUnk = CHX##moduleName##Package_::GetInstance_ (); \
    if (!pIUnk) return HXR_FAIL; \
    CHX##moduleName##Package_* pPackage = reinterpret_cast<CHX##moduleName##Package_*> (pIUnk); \
    ENFORCE_NOTIFY (pPackage, HXR_FAIL); \
    return (pPackage->GetLiveComponents()) ? HXR_FAIL : HXR_OK; \
} \

#define DECLARE_FACTORY_SUPPORT( moduleName, multiLoad, name, copyright, moreInfo, version ) \
    DECLARE_FACTORY_SUPPORT_EX( moduleName, multiLoad, name, copyright, moreInfo, version, 2000 )

#define DECLARE_FACTORY_NAMESPACE( moduleName, namespace) \
    const char* CHX##moduleName##Package_::m_pNamespaceD = namespace;

#define BEGIN_FACTORY_INFO_LIST(moduleName) \
    SComponentInfoListEntry CHX##moduleName##Package_::m_pComponentInfoD[] = {	\

#define END_FACTORY_INFO_LIST	\
    { kInfo_EndTable, NULL, NULL, 0, NULL } };

#define BEGIN_FACTORY_INFO_LIST_ENTRY( clsid, classname )	\
    { kInfo_BeginEntry, NULL, (void*) &clsid, 0, HXM_CONCAT3 (&CHXManagedComponent_,classname,_::CreateManagedInstance) },	\
    INT_FACTORY_INFO_LIST_ENTRY( PLUGIN_LOADMULTIPLE, TRUE ) \

#define BEGIN_FACTORY_INFO_LIST_ENTRY_NOCREATE( clsid )	\
    { kInfo_BeginEntry, NULL, (void*) &clsid, 0, NULL },	\

#define INT_FACTORY_INFO_LIST_ENTRY( name, value )	\
    { kInfo_IntType, name, (void*) value, 0, NULL },	\

#define STRING_FACTORY_INFO_LIST_ENTRY( name, pString )	\
    { kInfo_StringType, name, (void*) pString, 0, NULL },	\

#define BUFFER_FACTORY_INFO_LIST_ENTRY( name, pData, size )	\
    { kInfo_BufferType, name, (void*) pData, size, NULL },	\

#define GUID_FACTORY_INFO_LIST_ENTRY( name, clsid )	\
    { kInfo_BufferType, name, (void*) &clsid, sizeof(GUID), NULL },	\

#endif // _HXPACKAGE_H_


