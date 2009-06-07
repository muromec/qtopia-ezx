/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef _CHXMEDPLTFMEX_H_
#define _CHXMEDPLTFMEX_H_

#if defined(HELIX_FEATURE_EXTENDED_MEDIAPLATFORM)

#include "chxmedpltfm.h"
#include "ihxobjectmanager.h"
#include "ihxobservermanager.h"
#include "hxslist.h"
#include "ihxtimerobserver.h"
#include "ihxtimer.h"
#include "hxmapguidtogeneric.h"
#include "ihxaliascachemanagerprivate.h"
#include "ihxcontextuser.h"
#include "hxthreadsync.h"
#include "hxrangedtype.h"
#include "hxguidmap.h"

_INTERFACE IHXObserver;


class CObsToCLSID;
class CRefcountedCLSID;

class CHXMediaPlatformEx : 
      public CHXMediaPlatform
    , public IHXObjectManager
    , public IHXAliasCacheManagerPrivate
    , public IHXObserverManager
    , public IHXContextUser
{
    DECLARE_UNKNOWN( CHXMediaPlatformEx )

public:
    CHXMediaPlatformEx (CHXMediaPlatformEx* pParent = NULL, CHXMediaPlatformEx* pRoot = NULL);
    ~CHXMediaPlatformEx ();

    //	IHXMediaPlatform methods 
    STDMETHOD(CreateChildContext)   (THIS_
				     IHXMediaPlatform** ppChildContext);

    //  IHXContextUser methods
    STDMETHOD (RegisterContext)(THIS_ IUnknown* pIContext);

    //	IHXObjectManager methods
    STDMETHOD(SetDefaultHoldTime)		(THIS_ UINT32 holdTime);
    STDMETHOD(SetInfiniteDefaultHoldTime)	(THIS);
    STDMETHOD(SetCachedObjectHoldTime)		(THIS_ REFCLSID clsid, UINT32 holdTime);
    STDMETHOD(SetInfiniteCachedObjectHoldTime)  (THIS_ REFCLSID clsid);
    STDMETHOD(AddObjectToInstanceCache)		(THIS_ REFCLSID clsid);
    STDMETHOD(AddAliasToInstanceCache)		(THIS_ REFCLSID alias, 
						       REFCLSID clsid);
    STDMETHOD(RemoveAliasFromInstanceCache)	(THIS_ REFCLSID alias);
    STDMETHOD(RemoveObjectFromInstanceCache)	(THIS_ REFCLSID clsid);
    STDMETHOD(IsCachedObjectLoaded) 		(THIS_ REFCLSID clsid, 
						       HXBOOL* pbResult);
    STDMETHOD(FlushCaches)			(THIS);
    STDMETHOD(CreateParentCachedInstance)	(THIS_ REFCLSID clsid, IUnknown** ppIUnknown);
    STDMETHOD(GetAliasedCLSID)			(THIS_ REFCLSID clsid, CLSID* pAliasCLSID);
    
    // IHXCommonClassFactory methods
    STDMETHOD(CreateInstance)		(THIS_
					REFCLSID    /*IN*/  rclsid,
					void**	    /*OUT*/ ppUnknown) ;

    STDMETHOD(CreateInstanceAggregatable)
				    (THIS_
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter) ;


    // IHXObjectManagerPrivate methods
    STDMETHOD(ObjectFromCLSIDPrivate) (THIS_ REFCLSID clsid,REF(IUnknown *)pObject, 
						IUnknown *pUnkOuter, IUnknown* pContext );
    
    // IHXAliasCacheManagerPrivate methods
    STDMETHOD( IsCLSIDAliasedPrivate )(THIS_ REFCLSID clsid, CLSID* pAliasCLSID, IUnknown **ppIParentContext);
    STDMETHOD_( void, AddAliasCacheObserverPrivate ) ( THIS_ REFCLSID alias, IUnknown *pIChildObserver );
    STDMETHOD_( void, RemoveAliasCacheObserverPrivate ) ( THIS_ REFCLSID alias, IUnknown *pIChildObserver );
    STDMETHOD_( void, OnAliasRemovedFromInstanceCachePrivate ) ( THIS_ REFCLSID alias );

    //	IHXObserverManager methods
    STDMETHOD(AddObserver)			(THIS_ REFCLSID clsid, IHXObserver *pObserver) ;
    STDMETHOD(RemoveObserver)			(THIS_ REFCLSID clsid, IHXObserver *pObserver) ;

protected:

    void RemoveAllInstanceCacheObjects();
    void RemoveAllAliasCacheObjects();
    void RemoveAllObservers();

    virtual HX_RESULT	    CreateIntrinsicType( REFCLSID rclsid, REF(IUnknown*) pUnknown, IUnknown* pOuter );

private:
    // XXXHP - in my opinion this is a mess... some of this should
    // be broken into a seperate object, this interface is too fat.

    // CHoldTime_: provides an encapsulates data type that 
    // takes into account infinity without using a hack of -1...
    HX_RANGED_TYPE_INLINE(CPositiveInteger_, UINT32, 0, UINT_MAX);
    class CHoldTime_ : public CPositiveInteger_
    {
    public:
	CHoldTime_ (UINT32 val = 0)
	    : CPositiveInteger_ (val), m_infinite (FALSE) {}
	
	// XXXHP - define the rest of these someday.
	HXBOOL operator>= (CHoldTime_ const& rhs) const
	{
	    if (rhs.m_infinite) return m_infinite;
	    else if (m_infinite) return TRUE;
	    else
	    {
		return ((UINT32) *this >= (UINT32) rhs);
	    }
	}

	void SetInfinite ()
	{
	    CPositiveInteger_::operator= (0);
	    m_infinite = TRUE;
	}

	CPositiveInteger_ const& operator= (UINT32 rhs)
	{
	    if (m_infinite) m_infinite = FALSE;
	    return CPositiveInteger_::operator= (rhs);
	}

	CHoldTime_ const& operator= (CHoldTime_ const& rhs)
	{
	    m_infinite = rhs.m_infinite;
	    CPositiveInteger_::operator= ((UINT32) rhs);
	    return *this;
	}

	HXBOOL IsInfinite()
	{
	    return m_infinite;
	}

    private:
	HXBOOL DoCanModify_ () { return !m_infinite; }

	HXBOOL m_infinite;
    };

    // CTimerObserver_ is used to keep the timer interface
    // private for safety reasons.
    // *************** CTimerObserver_ ****************** 
    class CTimerObserver_
	: public CUnknownIMP, public IHXTimerObserver
    {
	DECLARE_UNKNOWN (CTimerObserver_)
    
    public:
	CTimerObserver_ (CHXMediaPlatformEx& objManager);
	 
	// IHXTimerObserver
	STDMETHOD_ (void, OnTimerStarted) (IHXTimer* pITimer);
     	STDMETHOD_ (void, OnTimerStopped) (IHXTimer* pITimer);
     	STDMETHOD_ (void, OnTimerReset) (IHXTimer* pITimer);
     	STDMETHOD_ (void, OnTimerPinged) (IHXTimer* pITimer, UINT32 pingNumber);
     
    private:
	CHXMediaPlatformEx& m_ObjManager;
    };

    friend class CTimerObserver_;

    class CCacheNode_;
    friend class CCacheNode_;
    // CCacheNode_ : cache map node.
    class CCacheNode_
    {
    public:
	CCacheNode_ (CHoldTime_ target = 0);
	
	HXBOOL IsQueued (); // XXXHP - perhaps rename to make it more clear that this is NOT just an accessor method
	void Age (CHoldTime_ interval);
	void SetTargetHold (CHoldTime_ target) { m_TargetHold = target; }
	IUnknown*& Object () { return m_pIUnkObject; }
	void SetObject (IUnknown* pIUnkObject) { m_pIUnkObject = pIUnkObject; }
	IUnknown* GetObject () const { return m_pIUnkObject; }
	void ReleaseObject ();
	void Unqueue ();

	inline UINT32 GetCount() {return m_RefCount;};
	inline UINT32 AddRef() {return ++m_RefCount;};
	inline UINT32 Release() {HX_ASSERT(m_RefCount != 0); return --m_RefCount;};
    private:
	void QueueForUnloading_ ();

	CHoldTime_ m_HoldCount;
	CHoldTime_ m_TargetHold;
	IUnknown* m_pIUnkObject;
	HXBOOL m_QueuedForUnloading;
	UINT32 m_RefCount;
    };

    class CCacheMap_;
    friend class CCacheMap_;
    HX_GUID_TO_GENERIC_MAP_INLINE (CCacheMap_, CCacheNode_);
    
    CObsToCLSID* FindSobs_(REFCLSID clsid, IHXObserver* pObserver, LISTPOSITION& pos);
    void AddObservers_(REFCLSID clsid, IUnknown* pObject);
    void RemoveObservers_ (REFCLSID clsid, IUnknown* pObject);
    IUnknown* PObjectFromInstanceCache_ (REFCLSID clsid, REF(HXBOOL) fIsCached);
    void OnTimerPinged_ (UINT32 pingNumber);
    void UpdateInstance_ (CCacheMap_::Iterator& i);

    SPIHXTimer m_spTimer;
    CTimerObserver_* m_pTimerObserver; // safe way of getting timer notifications.
    UINT32 const	    m_TimerInterval;

    // The critical sections are an attempt to make this class thread safe. 
    // They control acess to member variables that might have threading issues. Not every
    // member variable has its access controlled. Those which are only changed / set during
    // initialization should not need critical sections around them.

    // I have tried to keep the critical sections as short as possible - this should mean there
    // are no deadlocks. Typical problems have involved two threads trying to create objects
    // at the same time and the creation of one object causing the creation of another in a
    // different thread (leading to deadlock ).
    
    CCacheNode_ m_TemplateCacheNode; // template cache node for initializing slots.
    HXLockKey m_TemplateCacheNodeLockKey;

    CCacheMap_ m_Cache; // the cache.
    HXLockKey m_CacheLockKey;

    CHXSimpleList   m_ObserverList; // Observers to be late-bound
    HXLockKey m_ObserverListLockKey;

    CHXMapGUIDToObj m_AliasCache; // pending alias caches
    HXLockKey m_AliasCacheLockKey;

    CHoldTime_	    m_DefaultHoldTime;
    HXLockKey m_DefaultHoldTimeLockKey;
};

#endif /* HELIX_FEATURE_EXTENDED_MEDIAPLATFORM */

#endif // _CHXMEDPLTFMEX_H_





