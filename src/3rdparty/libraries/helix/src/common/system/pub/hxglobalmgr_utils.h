/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#if !defined(HXGLOBALMGR_UTILS_H__)
#define HXGLOBALMGR_UTILS_H__

#include "globals/hxglobals.h"

// Macros in this file facilitate defining global ptr destroy/access functions
// and unique key required by HXGlobalManager for access and cleanup.
//
// 1) Define one global destroy and access func pair per type:
//
// HX_DEFINE_GLOBAL_PTR_FUNCS_COM(IHXMyInterface)
// 
// 2) Define one key per variable:
//
// HX_DEFINE_GLOBAL_KEY(g_pMyVar)
// HX_DEFINE_GLOBAL_KEY(g_pMyOtherVar)
//
// 3) Alias the global variable as follows:
//
// #define g_pMyVar         MAKE_GLOBAL_PTR_ALIAS_COM(IHXMyInterface, g_pMyVar)
// #define g_pMyOtherVar    MAKE_GLOBAL_PTR_ALIAS_COM(IHXMyInterface, g_pMyOtherVar)
//
//
// 4) Automatic cleanup will be performed when thee dll unloads. At that point the global
//    manager will delete/release the pointer via the global destroy func before it frees
//    the global slot. (It is safe to delete/release the global ptr and set NULL before
//    automatic cleanup.)
//
// 5) For easier debugging you can use templates by uncommenting the following line. 
//
// #define HELIX_CONFIG_CPP_TEMPLATE_SUPPORT
//
// To do:
// ======
//
// To save space, make funs extern and provide declarion for access and cleanup 
// functions, e.g.
//
// HX_DECLARE_GLOBAL_PTR(IHXMyInterface, g_pMyInterface) // for funcs and key
//
// That way we avoid multiple definitions of the same body if we have
// more than one global ptr for a given type across modules.
//
/*********************************************************************************

Example:
========

//
// Declare globals
//

// global object destroyed via Release()
//
HX_DEFINE_GLOBAL_PTR_FUNCS_COM(IHXSomeInterface)

HX_DEFINE_GLOBAL_KEY(g_pSomeInterface)
#define g_pInstance MAKE_GLOBAL_PTR_ALIAS_COM(IHXSomeInterface, g_pSomeInterface)

HX_DEFINE_GLOBAL_KEY(g_pSomeInterfaceAgain)
#define g_pInstance MAKE_GLOBAL_PTR_ALIAS_COM(IHXSomeInterface, g_pSomeInterfaceAgain)

// global object destroyed via delete
//
HX_DEFINE_GLOBAL_PTR_FUNCS(HXSomeClass)

HX_DEFINE_GLOBAL_KEY(g_pSomeClass)
#define g_pSomeClass MAKE_GLOBAL_PTR_ALIAS(HXSomeClass, g_pSomeClass)


// use globals as though they were declared as follows
//
// static IHXSomeInterface* g_pSomeInterface;
// static HXSomeClass* g_pSomeClass;
//
void SomeFunc(IUnknown* pUnk)
{
    pUnk->QueryInterface(IID_SomeInterface, (void**)&g_pSomeInterface);
    g_pSomeInterfaceAgain = g_pSomeInterface;
    g_pSomeInterfaceAgain->AddRef();
    g_pSomeClass = new HXSomeClass();
}

************************************************************************************/



#define MAKE_GLOBAL_KEY_NAME(varName) varName##key

// generate unique key based on static address
//
#define HX_DEFINE_GLOBAL_KEY(varName) static const INT32 MAKE_GLOBAL_KEY_NAME(varName) = 0;


// generate access and destroy functions for pointers
//
#define HX_DEFINE_GLOBAL_PTR_FUNCS_COM(type) \
HX_DEFINE_GLOBAL_PTR_DESTROY_COM(type) \
HX_DEFINE_GLOBAL_PTR_ACCESS(type)

#define HX_DEFINE_GLOBAL_PTR_FUNCS(type) \
HX_DEFINE_GLOBAL_PTR_DESTROY(type) \
HX_DEFINE_GLOBAL_PTR_ACCESS(type)

// generate access and destroy functions for objects
//
#define HX_DEFINE_GLOBAL_OBJ_FUNCS(type) \
HX_DEFINE_GLOBAL_OBJ_DESTROY(type) \
HX_DEFINE_GLOBAL_OBJ_ACCESS(type)


#if defined(HELIX_CONFIG_CPP_TEMPLATE_SUPPORT)
//
// use (easier to debug and read) templates...

enum DestroyPolicy
{
    DP_RELEASE,
    DP_DELETE
};

template <DestroyPolicy policy>
struct DestroyPolicySelect
{
    // destroy function called for ref-counted object
    template <class T>
    static inline void Destroy(void* pOb)
    {
        if (pOb)
        {
            T* p = reinterpret_cast<T*>(pOb);
            // Crash? Likely reason is that code for Release() 
            // function is located in a now-unloaded dll. Ensure interface is 
            // implemented by a class that derives from CHXBaseCountingObject and
            // that the dll exports CanUnload. That ensures that the dll remains 
            // loaded as long as interfaces have positive ref-counts.
            p->Release();
        }
    }
};

template <>
struct DestroyPolicySelect<DP_DELETE>
{
    // destroy func called for regular pointer
    template <class T>
    static inline void Destroy(void* pOb)
    {
        T* p = reinterpret_cast<T*>(pOb);
        delete p;
    }
};


template <class T, DestroyPolicy policy>
void DestroyGlobalData(void* pOb)
{
    DestroyPolicySelect<policy>::Destroy<T>(pOb);
}

template <class T, DestroyPolicy policy>
static T*& GetGlobalPointer(const void* key)
{
    HXGlobalManager* pGM = HXGlobalManager::Instance();
    void** ppData = pGM->Get(key);
    if (!ppData)
	{
        // storage not yet allocated; stick in a NULL pointer
        pGM->Add(key, NULL, DestroyGlobalData<T, policy>);
        ppData = pGM->Get(key);
        HX_ASSERT(ppData);
    }
    return reinterpret_cast<T*&>(*ppData);
}

template <class T, DestroyPolicy policy>
static T& GetGlobalObject(const void* key)
{
    HXGlobalManager* pGM = HXGlobalManager::Instance();
    void** ppData = pGM->Get(key);
    if (!ppData)
	{
        // storage not yet allocated; stick in a NULL pointer
        pGM->Add(key, NULL, DestroyGlobalData<T, policy>);
        ppData = pGM->Get(key);
        HX_ASSERT(ppData);

        // allocate and default/zero init for user
        *ppData = new T();
        HX_ASSERT(*ppData);
    }
    return reinterpret_cast<T&>(**ppData);
}


// these do nothing; template instanciate handles everything
#define HX_DEFINE_GLOBAL_PTR_DESTROY_COM(type)
#define HX_DEFINE_GLOBAL_PTR_DESTROY(type)
#define HX_DEFINE_GLOBAL_PTR_ACCESS(type) 
#define HX_DEFINE_GLOBAL_OBJ_DESTROY(type)
#define HX_DEFINE_GLOBAL_OBJ_ACCESS(type) 

#define MAKE_GLOBAL_PTR_ALIAS_COM(type, varName)    GetGlobalPointer<type, DP_RELEASE>(&(MAKE_GLOBAL_KEY_NAME(varName)))
#define MAKE_GLOBAL_PTR_ALIAS(type, varName)        GetGlobalPointer<type, DP_DELETE>(&(MAKE_GLOBAL_KEY_NAME(varName)))
#define MAKE_GLOBAL_OBJ_ALIAS(type, varName)        GetGlobalObject<type, DP_DELETE>(&(MAKE_GLOBAL_KEY_NAME(varName)))


#else // HELIX_CONFIG_CPP_TEMPLATE_SUPPORT
//
// use macros instead of templates...

#define HX_GLOBAL_ACCESS_FUNC_NAME(type) Get##type##Instance
#define HX_GLOBAL_DESTROY_FUNC_NAME(type) Destroy##type##Instance

// generate access and destroy functions for pointers
//

#define HX_DEFINE_GLOBAL_PTR_ACCESS(type) \
static type*& HX_GLOBAL_ACCESS_FUNC_NAME(type)(const void* key) \
{ \
    HXGlobalManager* pGM = HXGlobalManager::Instance(); \
    type** ppData = (type**)(pGM->Get(key)); \
    if (!ppData) \
	{ \
        pGM->Add(key, 0, &HX_GLOBAL_DESTROY_FUNC_NAME(type)); \
        ppData = (type**)(pGM->Get(key)); \
        HX_ASSERT(ppData); \
    } \
    return *ppData; \
}

#define HX_DEFINE_GLOBAL_PTR_DESTROY(type) \
static void HX_GLOBAL_DESTROY_FUNC_NAME(type)(void* pOb) \
{ \
    delete (type*)(pOb); \
}

#define HX_DEFINE_GLOBAL_PTR_DESTROY_COM(type) \
static void HX_GLOBAL_DESTROY_FUNC_NAME(type)(void* pOb) \
{ \
    type* p = (type*)(pOb); \
    if (p) \
    { \
        p->Release(); \
    } \
}


// generate access and destroy functions for objects
//

// access global object
#define HX_DEFINE_GLOBAL_OBJ_ACCESS(type) \
static type& HX_GLOBAL_ACCESS_FUNC_NAME(type)(const void* key) \
{ \
    HXGlobalManager* pGM = HXGlobalManager::Instance(); \
    type** ppData = (type**)(pGM->Get(key)); \
    if (!ppData) \
	{ \
        pGM->Add(key, 0, &HX_GLOBAL_DESTROY_FUNC_NAME(type)); \
        ppData = (type**)(pGM->Get(key)); \
        HX_ASSERT(ppData); \
        *ppData = new type(); \
    } \
    return **ppData; \
}

// destroy global object
#define HX_DEFINE_GLOBAL_OBJ_DESTROY(type) \
static void HX_GLOBAL_DESTROY_FUNC_NAME(type)(void* pOb) \
{ \
    delete (type*)(pOb); \
}

// create alias to facilitate transparancy
//

#define MAKE_GLOBAL_OBJ_ALIAS(type, varName) HX_GLOBAL_ACCESS_FUNC_NAME(type)(&(MAKE_GLOBAL_KEY_NAME(varName)))
#define MAKE_GLOBAL_PTR_ALIAS(type, varName) MAKE_GLOBAL_OBJ_ALIAS(type, varName)
#define MAKE_GLOBAL_PTR_ALIAS_COM(type, varName) MAKE_GLOBAL_PTR_ALIAS(type, varName)

 

#endif // !HELIX_CONFIG_CPP_TEMPLATE_SUPPORT


#endif // HXGLOBALMGR_UTILS_H__

