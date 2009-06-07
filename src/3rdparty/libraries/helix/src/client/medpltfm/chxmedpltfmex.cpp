/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: chxmedpltfmex.cpp,v 1.5 2007/04/17 23:44:44 milko Exp $
* 
* Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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


#include "chxmedpltfmex.h"
#include "ihxobserver.h"
#include "ihxobservable.h"
#include "ihxinstancecacheobserver.h"
#include "ihxcontext.h"
#include "hxheap.h"
#include "hxassert.h"
#include "hxperf.h"
#include "hxgenericcontext.h"
#include "hxtimer.h"
#include "hxobjbrokrids.h"

//-------------------------------------- Private class

class CObsToCLSID 
{
public:
    CObsToCLSID (CLSID clsid, IHXObserver *pObserver);
    ~CObsToCLSID ();

    CLSID GetCLSID();
    IHXObserver* GetObserver();
    
private:
    CLSID m_clsid;
    IHXObserver*  m_pObserver;
};



CObsToCLSID::CObsToCLSID (CLSID clsid, IHXObserver *pObserver) :
    m_clsid(clsid)
    , m_pObserver(pObserver)
{
    if (pObserver)
	pObserver->AddRef();
}



CObsToCLSID::~CObsToCLSID ()
{
    HX_RELEASE(m_pObserver);
}

CLSID 
CObsToCLSID::GetCLSID()
{
    return m_clsid;
}


IHXObserver* 
CObsToCLSID::GetObserver()
{
    return m_pObserver;
}

//-------------------------------------- Private class
class CRefcountedCLSID
{
public:
    CRefcountedCLSID (CLSID clsid) : m_clsid(clsid), m_nRefCount(0) {};
    ~CRefcountedCLSID ();

    inline CLSID GetCLSID() {return m_clsid;}
    inline UINT32 GetCount() {return m_nRefCount;};
    inline UINT32 AddRef() {return ++m_nRefCount;};
    inline UINT32 Release() {HX_ASSERT(m_nRefCount != 0); return --m_nRefCount;};
    
    HX_RESULT AddObserver(IUnknown *pIUnkObserver);
    HX_RESULT RemoveObserver(IUnknown *pIUnkObserver);
    inline CHXSimpleList &GetObserverList() {return m_ObserverList;};

private:
    CLSID m_clsid;
    UINT32 m_nRefCount;
    CHXSimpleList   m_ObserverList;
};

CRefcountedCLSID::~CRefcountedCLSID()
{
    HX_ASSERT(m_ObserverList.IsEmpty());

    IUnknown *pIUnkObserver = NULL;
    while (!m_ObserverList.IsEmpty())
    {
	pIUnkObserver = (IUnknown*) m_ObserverList.RemoveTail();
	HX_RELEASE(pIUnkObserver);
    }
}

HX_RESULT CRefcountedCLSID::AddObserver(IUnknown *pIUnkObserver)
{
    HX_RESULT outResult = HXR_FAIL;
    SPIUnknown spIUnkObserver(pIUnkObserver);
    if (!m_ObserverList.Find(spIUnkObserver.Ptr()))
    {
	pIUnkObserver->AddRef();
	m_ObserverList.AddTail(spIUnkObserver.Ptr());
	outResult = HXR_OK;
    }

    return outResult;
}

HX_RESULT CRefcountedCLSID::RemoveObserver(IUnknown *pIUnkObserver)
{
    HX_RESULT outResult = HXR_FAIL;
    SPIUnknown spIUnkObserver(pIUnkObserver);
    LISTPOSITION pos = m_ObserverList.Find(spIUnkObserver.Ptr());
    if (pos)
    {
	m_ObserverList.RemoveAt(pos);
	HX_RELEASE(pIUnkObserver);
	outResult = HXR_OK;
    }

    return outResult;
}

BEGIN_INTERFACE_LIST_NOCREATE (CHXMediaPlatformEx::CTimerObserver_)
    INTERFACE_LIST_ENTRY_SIMPLE (IHXTimerObserver) 
END_INTERFACE_LIST

// CTimerObserver_ private member class
CHXMediaPlatformEx::CTimerObserver_::CTimerObserver_ (CHXMediaPlatformEx& objManager)
    : m_ObjManager (objManager)
{
}

STDMETHODIMP_ (void)
CHXMediaPlatformEx::CTimerObserver_::OnTimerStarted (IHXTimer*)
{
}
 
STDMETHODIMP_ (void)
CHXMediaPlatformEx::CTimerObserver_::OnTimerStopped (IHXTimer*)
{
}

STDMETHODIMP_ (void)
CHXMediaPlatformEx::CTimerObserver_::OnTimerReset (IHXTimer*)
{
}

STDMETHODIMP_ (void)
CHXMediaPlatformEx::CTimerObserver_::OnTimerPinged (IHXTimer*, UINT32 const pingNumber)
{
    m_ObjManager.OnTimerPinged_ (pingNumber);
}
    
//------------------------------- CHXMediaPlatformEx

BEGIN_INTERFACE_LIST( CHXMediaPlatformEx )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXObjectManager ) 
    INTERFACE_LIST_ENTRY_SIMPLE( IHXAliasCacheManagerPrivate )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXObserverManager )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXContextUser ) 
END_INTERFACE_LIST_BASE(CHXMediaPlatform)


CHXMediaPlatformEx::CHXMediaPlatformEx (CHXMediaPlatformEx* pParent, CHXMediaPlatformEx* pRoot) :
    CHXMediaPlatform(pParent, pRoot),
    m_TimerInterval (2000) // XXXHP - is this a good rate?
{
    HXAutoLock lock( &m_DefaultHoldTimeLockKey );

    m_DefaultHoldTime.SetInfinite ();

    HXAutoLock lockTemplateCacheNode( &m_TemplateCacheNodeLockKey );

    m_TemplateCacheNode.SetTargetHold (m_DefaultHoldTime);
}


CHXMediaPlatformEx::~CHXMediaPlatformEx ()
{
    if (m_spTimer.IsValid())
    {
	m_spTimer->StopTimer ();
	SPIHXObservable spObservable = m_spTimer.Ptr ();
	HX_ASSERT (spObservable.IsValid ());
	if (spObservable.IsValid ())
	{
	    spObservable->RemoveObserver (m_pTimerObserver);
	}
    }
    RemoveAllInstanceCacheObjects();
    RemoveAllAliasCacheObjects();
    RemoveAllObservers();

    HX_RELEASE( m_pTimerObserver );
}


STDMETHODIMP
CHXMediaPlatformEx::RegisterContext(IUnknown* pIContext)
{
    m_pTimerObserver = new CTimerObserver_( *this );
    CHECK_NONNULL( m_pTimerObserver );

    // We're holding a reference to this internal COM object to make sure it
    // doesn't get released prematurely.
    m_pTimerObserver->AddRef();		// Released in dtor

    SPIHXCommonClassFactory spParentFactory(pIContext);
    if(spParentFactory.IsValid())
    {
	SPIUnknown spIUnk;
	spParentFactory->CreateInstance(CLSID_HXTimer, (void**)spIUnk.AsInOutParam());

	m_spTimer = spIUnk.Ptr();
	HX_ASSERT (m_spTimer.IsValid ());
	if (m_spTimer.IsValid ())
	{
	    SPIHXObservable spObservable = m_spTimer.Ptr ();
	    HX_ASSERT (spObservable.IsValid ());
	    if (spObservable.IsValid ())
	    {
		spObservable->AddObserver ( m_pTimerObserver );
	    }
	    m_spTimer->SetInfiniteTimer (m_TimerInterval);
	    m_spTimer->StartTimer ();
	}
    }

    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatformEx::CreateChildContext(IHXMediaPlatform** ppChildContext)
{
    CHXMediaPlatformEx* pNewMedPltfm = new CHXMediaPlatformEx(this, (CHXMediaPlatformEx*)m_pRoot);
    if (!pNewMedPltfm)
    {
	return HXR_OUTOFMEMORY;
    }

    return pNewMedPltfm->QueryInterface(IID_IHXMediaPlatform, (void**)ppChildContext);
}


STDMETHODIMP
CHXMediaPlatformEx::SetDefaultHoldTime (UINT32 const holdTime)
{
    HXAutoLock lock( &m_DefaultHoldTimeLockKey );
    
    m_DefaultHoldTime = holdTime;

    HXAutoLock lockTemplateCacheNode( &m_TemplateCacheNodeLockKey );

    m_TemplateCacheNode.SetTargetHold (m_DefaultHoldTime);

    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatformEx::SetInfiniteDefaultHoldTime ()
{
    HXAutoLock lock( &m_DefaultHoldTimeLockKey );
    
    m_DefaultHoldTime.SetInfinite ();

    HXAutoLock lockTemplateCacheNode( &m_TemplateCacheNodeLockKey );

    m_TemplateCacheNode.SetTargetHold (m_DefaultHoldTime);

    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatformEx::SetCachedObjectHoldTime (REFCLSID clsid, UINT32 const holdTime)
{
    HXBOOL inCache;
    PObjectFromInstanceCache_ (clsid, inCache);
    if (!inCache) return HXR_UNEXPECTED;

    HXAutoLock lock( &m_CacheLockKey );

    (m_Cache[clsid]).SetTargetHold(holdTime);
    
    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatformEx::SetInfiniteCachedObjectHoldTime (REFCLSID clsid)
{
    HXBOOL inCache;
    PObjectFromInstanceCache_ (clsid, inCache);
    if (!inCache) return HXR_UNEXPECTED;

    CHoldTime_ holdTime;
    holdTime.SetInfinite ();

    HXAutoLock lock( &m_CacheLockKey );

    (m_Cache [clsid]).SetTargetHold (holdTime);
    
    return HXR_OK;
}

STDMETHODIMP
CHXMediaPlatformEx::AddObjectToInstanceCache(REFCLSID clsid) 
{
    HX_LOG_BLOCK( "CHXMediaPlatformEx::AddObjectToInstanceCache" );

    //When an object is aliased in a parent, it must be aliased for all child object managers as well.
    //Because ObjectFromCLSIDPrivate() looks for the clsid entry in the cache before it searches the parent
    //hierarchy for a higher level alias, adding an object to the instance cache breaks aliasing. Therefore,
    //whenever an object is cached check to see if it is aliased by a parent. If so, add the alias to this object
    //manager, and cache it too. When clsid is uncached from this object manager or unaliased in the parent, then
    //remove the alias and the cache.
    const CLSID *pTargetCLSID = &clsid;
    CLSID aliasedCLSID;
    SPIUnknown spIUnkParent;
    SPIHXAliasCacheManagerPrivate spIHXAliasCacheManagerPrivate((IHXMediaPlatform*)m_pParent);
    if (spIHXAliasCacheManagerPrivate.IsValid() &&
	SUCCEEDED(spIHXAliasCacheManagerPrivate->IsCLSIDAliasedPrivate(clsid, &aliasedCLSID, spIUnkParent.AsInOutParam())))
    {
	//if the class id is aliased by a parent, then we must observe that parent so we can remove
	//our alias when our parent does.
	AddAliasToInstanceCache(clsid, aliasedCLSID);
	pTargetCLSID = &aliasedCLSID;

	//add ourselves as an observer of our parents aliased CLSID. We need to be notified when it
	//is removed so that we can remove our alias too.
	spIHXAliasCacheManagerPrivate = spIUnkParent.Ptr();
	HX_ASSERT(spIHXAliasCacheManagerPrivate.IsValid());

	if (spIHXAliasCacheManagerPrivate.IsValid())
	{
	    spIHXAliasCacheManagerPrivate->AddAliasCacheObserverPrivate(clsid, GetUnknown());
	}

	AddObjectToInstanceCache(aliasedCLSID);	
    }

    // Only add the object if it isn't already in the cache

    HXAutoLock lock( &m_CacheLockKey );

    CCacheNode_ cacheNode;
    HXBOOL fIsCached = m_Cache.Lookup( clsid, cacheNode);
    if ( fIsCached )
    {
	cacheNode.AddRef();
	m_Cache.SetAt( clsid, cacheNode );
    }
    else
    {
	HXAutoLock lock( &m_TemplateCacheNodeLockKey );
	
	m_Cache.SetAt( clsid, m_TemplateCacheNode ); // Add the slot, but not the object.
    }
    return HXR_OK;
}


STDMETHODIMP
CHXMediaPlatformEx::AddAliasToInstanceCache(REFCLSID alias, 
					  REFCLSID actualCLSID)
{
    HXAutoLock lock( &m_AliasCacheLockKey );
    
    HX_RESULT res = HXR_UNEXPECTED;

    // do we already have an alias set up?
    CLSID* pTargetCLSID = NULL;
    CRefcountedCLSID* pRefCountCLSID = (CRefcountedCLSID*)m_AliasCache[alias];
    if ( pRefCountCLSID )
    {
	// if we have already aliased this CLSID give a friendly return code
	if ( IsEqualIID( actualCLSID, pRefCountCLSID->GetCLSID() ) )
	{
	    pRefCountCLSID->AddRef();
	    res = HXR_OK;
	}
	else
	{
	    HX_ASSERT(!"Can't alias the same CLSID twice with different target GUIDs");
	}
    }
    else
    {
	// add an alias entry so we can keep track of the alias
	res = HXR_OUTOFMEMORY;
	pRefCountCLSID = new CRefcountedCLSID(actualCLSID);
	if ( pRefCountCLSID )
	{
	    pRefCountCLSID->AddRef();
	    m_AliasCache[alias] = pRefCountCLSID;

	    res = HXR_OK;
	}
    }

    return res;
}


STDMETHODIMP
CHXMediaPlatformEx::RemoveAliasFromInstanceCache(REFCLSID alias)
{
    HXAutoLock lock( &m_AliasCacheLockKey );

    HX_RESULT res = HXR_UNEXPECTED; // XXXSEH: Questionable? If it has already been removed, is it an error?

    CRefcountedCLSID* pRefCountCLSID = (CRefcountedCLSID*)m_AliasCache[alias];
    if ( pRefCountCLSID )
    {
	if (0 == pRefCountCLSID->Release())
	{
	    //when we remove the last alias refcount from our instance cache, notify all alias cache
	    //observers. Some might be aliasing this guid because we were. When we remove it, they
	    //should too.
	    CHXSimpleList &observerList = pRefCountCLSID->GetObserverList();
	    IUnknown *pIUnkObserver = NULL;
	    while (!observerList.IsEmpty())
	    {
		pIUnkObserver = (IUnknown*) observerList.RemoveTail();
		SPIHXAliasCacheManagerPrivate spIHXAliasCacheManagerPrivate = pIUnkObserver;
		HX_ASSERT(spIHXAliasCacheManagerPrivate.IsValid());

		if (spIHXAliasCacheManagerPrivate.IsValid())
		{
		    spIHXAliasCacheManagerPrivate->OnAliasRemovedFromInstanceCachePrivate(alias);
		}

		HX_RELEASE(pIUnkObserver);
	    }

	    m_AliasCache.Remove( alias );
	    HX_DELETE( pRefCountCLSID );
	}
	res = HXR_OK;
    }

    return res;
}


STDMETHODIMP
CHXMediaPlatformEx::RemoveObjectFromInstanceCache(REFCLSID clsid) 
{
    HX_RESULT res = HXR_UNEXPECTED; // XXXSEH: Questionable? If it has already been removed, is it an error?

    const CLSID *pTargetCLSID = &clsid;
    CLSID aliasedCLSID;
    SPIUnknown spIUnkParent;
    SPIHXAliasCacheManagerPrivate spIHXAliasCacheManagerPrivate((IHXMediaPlatform*)m_pParent);
    if (spIHXAliasCacheManagerPrivate.IsValid() &&
	SUCCEEDED(spIHXAliasCacheManagerPrivate->IsCLSIDAliasedPrivate(clsid, &aliasedCLSID, spIUnkParent.AsInOutParam())))
    {
	RemoveAliasFromInstanceCache(clsid);
	pTargetCLSID = &aliasedCLSID;

	//remove ourselves as an observer of our parents aliased CLSID. We aren't caching the aliased guid, so
	//we no longer need to alias it.
	spIHXAliasCacheManagerPrivate = spIUnkParent.Ptr();
	HX_ASSERT(spIHXAliasCacheManagerPrivate.IsValid());

	if (spIHXAliasCacheManagerPrivate.IsValid())
	{
	    spIHXAliasCacheManagerPrivate->RemoveAliasCacheObserverPrivate(clsid, GetUnknown());
	}

	RemoveObjectFromInstanceCache(aliasedCLSID);	
    }

    // remove the object from the cache, if there is one

    HXAutoLock lock( &m_CacheLockKey );

    CCacheNode_ cacheNode;
    if ( m_Cache.Lookup( clsid, cacheNode ) )
    {
	if (0 == cacheNode.Release())
	{
	    m_Cache.RemoveKey( clsid );
	    RemoveObservers_( clsid, cacheNode.GetObject() );
	    cacheNode.ReleaseObject();
	} else
	{
	    m_Cache.SetAt(clsid, cacheNode);
	}

	res = HXR_OK;
    }

    return res;
}


STDMETHODIMP
CHXMediaPlatformEx::IsCachedObjectLoaded( REFCLSID clsid, HXBOOL* pbResult )
{
    HX_RESULT result = HXR_INVALID_PARAMETER;

    if( pbResult )
    {
	CLSID aliasedCLSID;
	if( SUCCEEDED( GetAliasedCLSID( clsid, &aliasedCLSID ) ) )
	{
	    result = IsCachedObjectLoaded( aliasedCLSID, pbResult );

	    return result;
	}
        
	// Initialize out parameters
	// We will return HXR_UNEXPECTED if the object is not, in fact, cached
	result = HXR_UNEXPECTED;
	*pbResult = FALSE;

	HXBOOL bIsInCache = FALSE;
	IUnknown* pInstance = PObjectFromInstanceCache_ ( clsid, bIsInCache );
    
	if( bIsInCache )
	{
	    *pbResult = ( pInstance == NULL ) ? FALSE : TRUE;
	    result = HXR_OK;
	}
	else
	{
	    SPIHXObjectManager spIObjMgr((IHXMediaPlatform*)m_pParent);
	    if( spIObjMgr.IsValid() )
	    {
		result = spIObjMgr->IsCachedObjectLoaded( clsid, pbResult );
	    }
	}
    }

    return result;
}


STDMETHODIMP
CHXMediaPlatformEx::FlushCaches()
{
    HXAutoLock lock( &m_CacheLockKey );
	
    for (CCacheMap_::Iterator i = m_Cache.Begin (); i != m_Cache.End (); ++i)
    {
	RemoveObservers_(*i.get_key(), (*i).GetObject());
	(*i).ReleaseObject ();
    }

    RemoveAllObservers();

    return HXR_OK;
}


STDMETHODIMP
CHXMediaPlatformEx::CreateParentCachedInstance(REFCLSID clsid, IUnknown** ppIUnknown)
{
    HX_ASSERT( m_pParent );

    HX_RESULT result = HXR_FAIL;
    SPIHXObjectManager spIParentObjMgr((IHXMediaPlatform*)m_pParent);
    if( spIParentObjMgr.IsValid() )
    {
	HXBOOL isCached;
	result = spIParentObjMgr->IsCachedObjectLoaded( clsid, &isCached );
	if( SUCCEEDED( result ) )
	{
	    result = m_pParent->ObjectFromCLSIDPrivate( clsid, *ppIUnknown, NULL, m_pParent->GetUnknown() );
	}
    }
    return result;
}

STDMETHODIMP
CHXMediaPlatformEx::GetAliasedCLSID(THIS_ REFCLSID clsid, CLSID* pAliasCLSID)
{
    HXAutoLock lock( &m_AliasCacheLockKey );

    HX_RESULT outResult = HXR_FAIL;
    CRefcountedCLSID* pRefCountCLSID = (CRefcountedCLSID*)m_AliasCache[clsid];
    if ( pRefCountCLSID )
    {
	*pAliasCLSID = pRefCountCLSID->GetCLSID();

	outResult = HXR_OK;
    }

    return outResult;
}

// IHXCommonClassFactory methods


STDMETHODIMP
CHXMediaPlatformEx::CreateInstance(REFCLSID clsid, void** ppObject) 
{
    HX_RESULT result = HXR_FAIL;
    IUnknown *pUnk = NULL;

    result = CreateInstanceAggregatable( clsid, pUnk, NULL );
    if( SUCCEEDED( result ) )
    {
	*ppObject = pUnk;
    }

    return result;
}


STDMETHODIMP
CHXMediaPlatformEx::CreateInstanceAggregatable
				(THIS_
				REFCLSID	    /*IN*/  clsid,
				REF(IUnknown*)  /*OUT*/ pUnknown,
				IUnknown*	    /*IN*/  pUnkOuter) 
{
    HX_RESULT result  = ObjectFromCLSIDPrivate(clsid, pUnknown, pUnkOuter, GetUnknown() );
    
    return result;
}

STDMETHODIMP
CHXMediaPlatformEx::ObjectFromCLSIDPrivate(REFCLSID clsid,REF(IUnknown*)pObject, 
					    IUnknown* pUnkOuter, IUnknown* pContext )
{
    HX_RESULT result = HXR_FAIL;

    // Set up a default value
    pObject = NULL;

    CLSID aliasedCLSID;
    if( SUCCEEDED( GetAliasedCLSID( clsid, &aliasedCLSID ) ) )
    {
	result = ObjectFromCLSIDPrivate( aliasedCLSID, pObject, pUnkOuter, pContext );

	return result;
    }

    // The somewhat odd structure here is to try and keep the cache locked for as little time as possible.

    // If the object's in the cache give 'em that.
    HXBOOL objectCached = FALSE;
    
    PObjectFromInstanceCache_( clsid, objectCached );

    if ( objectCached )
    {
	HXBOOL objectAlreadyExists = FALSE;

	// this scope is necessary to get the locking correct
	{
	    HXAutoLock lock( &m_CacheLockKey );
	
	    // XXXHP - COMING SOON: this ugliness to be replaced by iterators... 
	    CCacheNode_& cacheNodeRef = m_Cache [clsid];
	    cacheNodeRef.Unqueue ();
	    if (cacheNodeRef.GetObject ())
	    {
		pObject = cacheNodeRef.GetObject ();
		pObject->AddRef ();
		result = HXR_OK;
		objectAlreadyExists = TRUE;
	    }
	}
	
	if( ! objectAlreadyExists )
	{
	    // Construct object here, and add it to the cache.  
	    result = CHXMediaPlatform::ObjectFromCLSIDPrivate(clsid, pObject, pUnkOuter, pContext);

	    if( SUCCEEDED( result ) )
	    {
		HXAutoLock lock( &m_CacheLockKey );
		
		CCacheNode_& cacheNodeRef = m_Cache [clsid];
		
		cacheNodeRef.SetObject (pObject);
		pObject->AddRef ();
		AddObservers_( clsid, pObject );
	    }
	}
    
	return result;
    }

    // The object wasn't in the cache, so defer to our parent.
    // CLSID_HXObjectManager is a special case, it should not be cached and 
    // has to be created using it's parent (i.e. this), not the root CCF.
    if( !IsEqualCLSID(clsid, CLSID_HXObjectManager ) && m_pParent )
    {
	result = m_pParent->ObjectFromCLSIDPrivate( clsid, pObject, pUnkOuter, pContext );
    }
    else
    {
	result = CHXMediaPlatform::ObjectFromCLSIDPrivate(clsid, pObject, pUnkOuter, pContext);
    }

    return result;
}

HX_RESULT
CHXMediaPlatformEx::CreateIntrinsicType( REFCLSID rclsid, REF(IUnknown*) pUnknown, IUnknown* pOuter )
{
    HX_RESULT result = HXR_FAIL;
    pUnknown = NULL;

    if( IsEqualCLSID(rclsid, CLSID_HXObjectManager ) )
    {
	CHXMediaPlatformEx* pChildMediaPlatform = new CHXMediaPlatformEx(this, (CHXMediaPlatformEx*)m_pRoot);
	if(pChildMediaPlatform)
	{
	    pChildMediaPlatform->SetupAggregation(pOuter, &pUnknown);
	}
    }
    else if( IsEqualCLSID(rclsid, CLSID_HXGenericContext ) )
    {
	CRNGenericContext::CreateInstance( pOuter, &pUnknown );
    }
    else if (IsEqualCLSID (rclsid, CLSID_HXTimer))
    {
	CRNTimer::CreateUnmanagedInstance (pOuter, &pUnknown);
    }

    if( pUnknown )
    {
	return HXR_OK;
    }

    return CHXMediaPlatform::CreateIntrinsicType( rclsid, pUnknown, pOuter );
}

// IHXAliasCacheManagerPrivate methods

STDMETHODIMP
CHXMediaPlatformEx::IsCLSIDAliasedPrivate(THIS_ REFCLSID clsid, CLSID* pAliasCLSID, IUnknown **ppIParentContext)
{
    PRE_REQUIRE_RETURN(pAliasCLSID, HXR_INVALID_PARAMETER);
    PRE_REQUIRE_RETURN(ppIParentContext, HXR_INVALID_PARAMETER);
    HX_ASSERT(!*ppIParentContext);

    HX_RESULT result = HXR_FAIL;

    if( SUCCEEDED( GetAliasedCLSID( clsid, pAliasCLSID ) ) )
    {
	*ppIParentContext = GetUnknown();
	(*ppIParentContext)->AddRef();

	result = HXR_OK;
    }
    else
    {
	SPIHXAliasCacheManagerPrivate spIHXAliasCacheManagerPrivate((IHXMediaPlatform*)m_pParent);
	if(spIHXAliasCacheManagerPrivate.IsValid())
	{
	    result = spIHXAliasCacheManagerPrivate->IsCLSIDAliasedPrivate( clsid, pAliasCLSID, ppIParentContext);
	}
    }

    return result;
}

STDMETHODIMP_( void )
CHXMediaPlatformEx::AddAliasCacheObserverPrivate( THIS_ REFCLSID alias, IUnknown *pIChildObserver )
{
    HXAutoLock lock( &m_AliasCacheLockKey );

    //if an alias observer is being added, the CLSID had better be aliased
    CRefcountedCLSID* pRefCountCLSID = (CRefcountedCLSID*)m_AliasCache[alias];
    HX_VERIFY( pRefCountCLSID );
    if (pRefCountCLSID)
    {
	pRefCountCLSID->AddObserver(pIChildObserver);
    }
}

STDMETHODIMP_( void )
CHXMediaPlatformEx::RemoveAliasCacheObserverPrivate( THIS_ REFCLSID alias, IUnknown *pIChildObserver )
{
    HXAutoLock lock( &m_AliasCacheLockKey );

    //if an alias observer is being removed, the CLSID had better be aliased
    CRefcountedCLSID* pRefCountCLSID = (CRefcountedCLSID*)m_AliasCache[alias];
    HX_VERIFY( pRefCountCLSID );
    if (pRefCountCLSID)
    {
	pRefCountCLSID->RemoveObserver(pIChildObserver);
    }
}

STDMETHODIMP_( void )
CHXMediaPlatformEx::OnAliasRemovedFromInstanceCachePrivate( THIS_ REFCLSID alias )
{
    CLSID aliasedCLSID;
    if( SUCCEEDED( GetAliasedCLSID( alias, &aliasedCLSID ) ) )
    {
	RemoveAliasFromInstanceCache(alias);
    }
}

// IHXObserverManager methods -----------------------------------------------------------------------

STDMETHODIMP
CHXMediaPlatformEx::AddObserver(REFCLSID clsid, IHXObserver* pObserver) 
{
    HX_RESULT result = HXR_FAIL;

    CLSID aliasedCLSID;
    if( SUCCEEDED( GetAliasedCLSID( clsid, &aliasedCLSID ) ) )
    {
	result = AddObserver( aliasedCLSID, pObserver );

	return result;
    }
    
    CObsToCLSID* pSobs = NULL;
    HXBOOL fInCache = FALSE;
    
    IUnknown* pUnk = PObjectFromInstanceCache_ (clsid, fInCache);
    if (fInCache)
    {
	// Add the observer to the late binding list.
    	pSobs = new CObsToCLSID(clsid, pObserver);

	// This block exists to get the correct scope on the lock
	{
	    HXAutoLock lock( &m_ObserverListLockKey );
    	    m_ObserverList.AddTail((void*) pSobs);
	}

	// If the object is already created then add the observer now.
	if ( pUnk )
	{
	    SPIHXObservable spObservable = pUnk;
	    SPIHXInstanceCacheObserver spIInstanceCacheObserver = pObserver;
	    if ( spIInstanceCacheObserver.IsValid() )
	    {
		spIInstanceCacheObserver->OnCachedInstanceCreated( clsid, pUnk );
	    }
	    if ( spObservable.IsValid() )
	    {
		spObservable->AddObserver( pObserver );
	    }
	}
    	result = HXR_OK;
    }    	
    else
    {
	SPIHXObserverManager spObsMgr((IHXMediaPlatform*)m_pParent);
	if ( spObsMgr.IsValid() )
	{
	    result = spObsMgr->AddObserver( clsid, pObserver );
	}
    }	

    return result;

}


STDMETHODIMP
CHXMediaPlatformEx::RemoveObserver(REFCLSID clsid, IHXObserver*pObserver) 
{
    HXAutoLock lock( &m_ObserverListLockKey );
    
    HX_RESULT result = HXR_FAIL;

    CLSID aliasedCLSID;
    if( SUCCEEDED( GetAliasedCLSID( clsid, &aliasedCLSID ) ) )
    {
	result = RemoveObserver( aliasedCLSID, pObserver );

	return result;
    }
    CObsToCLSID* pSobs = NULL;

    LISTPOSITION pos;
    pSobs = FindSobs_(clsid, pObserver, pos);

    if (pSobs )
    {
	HXBOOL fInCache;
	IUnknown* pUnk = PObjectFromInstanceCache_ (clsid, fInCache);
	if ( pUnk )
	{
	    SPIHXObservable spObservable = pUnk;
	    SPIHXInstanceCacheObserver spIInstanceCacheObserver = pObserver;
	    if ( spObservable.IsValid() )
	    {
		spObservable->RemoveObserver( pObserver );
	    }
	    if ( spIInstanceCacheObserver.IsValid() )
	    {
		spIInstanceCacheObserver->OnCachedInstanceReleased( clsid, pUnk );
	    }
	}
        m_ObserverList.RemoveAt(pos);
        HX_DELETE(pSobs);
        result = HXR_OK;
    }
    else
    {
	SPIHXObserverManager spObsMgr((IHXMediaPlatform*)m_pParent);
	if( spObsMgr.IsValid() )
	{
	    result = spObsMgr->RemoveObserver( clsid, pObserver );
	}
    }	

    return result;

}

//------------------------------------- Protected methods

void
CHXMediaPlatformEx::RemoveAllInstanceCacheObjects()
{
    HXAutoLock lock( &m_CacheLockKey );
	
    for(CCacheMap_::Iterator i = m_Cache.Begin (); i != m_Cache.End (); ++i)
    {
	RemoveObservers_(*i.get_key(), (*i).GetObject());
	(*i).ReleaseObject ();
    }
    m_Cache.RemoveAll();
}


void
CHXMediaPlatformEx::RemoveAllAliasCacheObjects()
{
    HXAutoLock lock( &m_AliasCacheLockKey );

    for(CHXMapGUIDToObj::Iterator iter = m_AliasCache.Begin(); iter != m_AliasCache.End(); iter++)
    {
	// Get the IUnknown 
	CRefcountedCLSID* pRefcountedCLSID = (CRefcountedCLSID*)*iter;
	HX_DELETE(pRefcountedCLSID);
    }
    m_AliasCache.RemoveAll();
}



void
CHXMediaPlatformEx::RemoveAllObservers()
{
    HXAutoLock lock( &m_ObserverListLockKey );
    
    CObsToCLSID * pSobs = NULL;
    while (!m_ObserverList.IsEmpty())
    {
	pSobs = (CObsToCLSID*) m_ObserverList.RemoveHead();
	HX_DELETE(pSobs);
    }
}


//---------------------------------- Private Methods

CObsToCLSID* 
CHXMediaPlatformEx::FindSobs_ (REFCLSID clsid, IHXObserver* pObserver, LISTPOSITION& pos)
{
    HXAutoLock lock( &m_ObserverListLockKey );

    CObsToCLSID* pSobs = NULL;
    LISTPOSITION posNext = m_ObserverList.GetHeadPosition();
    while (posNext)
    {
	pos = posNext;
	pSobs = (CObsToCLSID*)(m_ObserverList.GetNext(posNext));
	if ((clsid == pSobs->GetCLSID()) && (pObserver == pSobs->GetObserver()))
	{
	    return pSobs;
	}
    }
    return NULL;

}


void 
CHXMediaPlatformEx::AddObservers_ (REFCLSID clsid, IUnknown* pObject)
{
    HXAutoLock lock( &m_ObserverListLockKey );
    
    SPIHXObservable spObservable = pObject;
    
    LISTPOSITION pos = m_ObserverList.GetHeadPosition();
    while ( pos )
    {
	CObsToCLSID* pSobs = ( CObsToCLSID* ) ( m_ObserverList.GetNext( pos ) );
	if ( IsEqualCLSID( clsid, pSobs->GetCLSID() ) )
	{
	    IHXObserver* pIObserver = pSobs->GetObserver();
	    SPIHXInstanceCacheObserver spIInstanceCacheObserver = pIObserver;
	    if ( spIInstanceCacheObserver.IsValid() )
	    {
		spIInstanceCacheObserver->OnCachedInstanceCreated( clsid, pObject );
	    }
	    if ( spObservable.IsValid() )
	    {
		spObservable->AddObserver( pIObserver );
	    }
	}
    }
}


void 
CHXMediaPlatformEx::RemoveObservers_ (REFCLSID clsid, IUnknown* pObject)
{
    HXAutoLock lock( &m_ObserverListLockKey );
    
    SPIHXObservable spObservable = pObject;
    
    LISTPOSITION pos = m_ObserverList.GetHeadPosition();
    while ( pos )
    {
	CObsToCLSID* pSobs = ( CObsToCLSID* ) ( m_ObserverList.GetNext( pos ) );
	if ( IsEqualCLSID( clsid, pSobs->GetCLSID() ) )
	{
	    IHXObserver* pIObserver = pSobs->GetObserver();
	    SPIHXInstanceCacheObserver spIInstanceCacheObserver = pIObserver;
	    if ( spObservable.IsValid() )
	    {
		spObservable->RemoveObserver( pIObserver );
	    }
	    if ( spIInstanceCacheObserver.IsValid() )
	    {
		spIInstanceCacheObserver->OnCachedInstanceReleased( clsid, pObject );
	    }
	}
    }
}


IUnknown* 
CHXMediaPlatformEx::PObjectFromInstanceCache_ (REFCLSID clsid, REF(HXBOOL) fIsCached)
{
    HXAutoLock lock( &m_CacheLockKey );

    CCacheNode_ cacheNode;
    fIsCached = m_Cache.Lookup( clsid, cacheNode);
    return cacheNode.GetObject ();	    
}

void CHXMediaPlatformEx::OnTimerPinged_ (UINT32 const pingNumber)
{
    HXAutoLock lock( &m_CacheLockKey );
	
    AddRef();
    for (CCacheMap_::Iterator i = m_Cache.Begin (); i != m_Cache.End (); ++i)
    {
	UpdateInstance_ (i);
    }
    Release();
}

inline void CHXMediaPlatformEx::UpdateInstance_ (CCacheMap_::Iterator& i)
{
    if ((*i).IsQueued ())
    {
	(*i).Age (m_TimerInterval);

	// the object got released, let's try and get rid of the DLL
	// JE 3/26/01 - temporary usage until actual garbage collection implemented
	// I expect this will be removed when garbage collection implemented
	if(!((*i).IsQueued ()) && NULL == ((*i).GetObject()))
	{
	    UnloadPluginPrivate(*i.get_key());
	}
    }
}

CHXMediaPlatformEx::CCacheNode_::CCacheNode_ (CHoldTime_ target)
    : m_HoldCount (0), m_TargetHold (target), m_pIUnkObject (NULL), m_QueuedForUnloading (FALSE), m_RefCount(1)
{
}

inline void CHXMediaPlatformEx::CCacheNode_::ReleaseObject ()
{
    // This must be called before releasing the object to
    // avoid a possible access violation if the object
    // being released holds the last reference to this
    // object manager (i.e., context).
    Unqueue( );

    // We must set the data member to NULL before releasing 
    // this object because it may cause the release of the 
    // last reference to this Object Manager, which in turn
    // will release this object again, causing too many
    // releases.
    IUnknown* pIUnkObject = m_pIUnkObject;
    m_pIUnkObject = NULL;
    HX_RELEASE( pIUnkObject );
}

inline void CHXMediaPlatformEx::CCacheNode_::Unqueue ()
{
    m_HoldCount = 0;
    m_QueuedForUnloading = FALSE;
}

// XXXHP - i'm inling just about every small method as this code is it is highly important for this to be efficient...
inline HXBOOL CHXMediaPlatformEx::CCacheNode_::IsQueued ()
{
    if (!m_pIUnkObject) return FALSE;
    
    if (!m_QueuedForUnloading && !m_TargetHold.IsInfinite()) 
    {
	m_pIUnkObject->AddRef ();
	ULONG32 const refCount = m_pIUnkObject->Release ();
	if (1 == refCount)
	{
	    QueueForUnloading_ ();
	}
    }

    return m_QueuedForUnloading;
}

inline void CHXMediaPlatformEx::CCacheNode_::Age (CHoldTime_ const interval)
{
    m_HoldCount += interval;
    if (m_HoldCount >= m_TargetHold)
    {
	ReleaseObject ();
    }
}

inline void CHXMediaPlatformEx::CCacheNode_::QueueForUnloading_ ()
{
    m_HoldCount = 0;
    m_QueuedForUnloading = TRUE;
}
