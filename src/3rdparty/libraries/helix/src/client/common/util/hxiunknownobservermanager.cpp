/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxiunknownobservermanager.cpp,v 1.2 2006/04/19 22:39:45 bobclark Exp $
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

#ifndef HELIX_FEATURE_ALLOW_EXTENDED
#define HELIX_FEATURE_ALLOW_EXTENDED
#endif

#include "hxiunknownobservermanager.h"

#include "hxassert.h"
#include "hxheap.h"
#include "hxslist.h"
#include "hxcomptr.h"


BEGIN_INTERFACE_LIST_NOCREATE( CHXIUnknownElementFunctor )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXIUnknownElementFunctor )
END_INTERFACE_LIST

IMPLEMENT_COM_CREATE_FUNCS( CHXIUnknownElementFunctor )

STDMETHODIMP_( void )
CHXIUnknownElementFunctor::Invoke( IUnknown* pIUnknownListElement )
{
    (*this)(pIUnknownListElement);
}

STDMETHODIMP_( void )
CHXIUnknownElementFunctor::Invoke( const IUnknown* pIUnknownListElement )
{
    (*this)(pIUnknownListElement);
}

STDMETHODIMP_( HXBOOL )
CHXIUnknownElementFunctor::ShouldStopInvoking( THIS ) 
{
    return ShouldStopIterating();
}

HXBOOL
CHXIUnknownElementFunctor::ShouldStopIterating( void ) 
{
    HX_ASSERT(!"Superclasses wishing to make use of this method must override it");
    return TRUE;
}

void
CHXIUnknownElementFunctor::operator() ( IUnknown* pIUnknownListElement )
{
    HX_ASSERT(!"Superclasses wishing to make use of this method must override it");
}

void
CHXIUnknownElementFunctor::operator() ( const IUnknown* pIUnknownListElement )
{
    HX_ASSERT(!"Superclasses wishing to make use of this method must override it");
}

//-------------IHXIUnknownObserverManager
STDMETHODIMP_( void )
CHXIUnknownObserverManagerMixin::IterateOverIUnknowns( THIS_ IHXIUnknownElementFunctor *pIUnkFunctor )
{
    HX_VERIFY(pIUnkFunctor);
    if (!pIUnkFunctor)
	return;

    for(CHXSimpleList::Iterator iter = m_ListElements.Begin(); iter != m_ListElements.End(); ++iter)
    {
	IUnknownObserverManagerEntry* pIUnknownObserverManagerEntry = (IUnknownObserverManagerEntry *)*iter;
	HX_ASSERT( pIUnknownObserverManagerEntry );

	pIUnkFunctor->Invoke( pIUnknownObserverManagerEntry->pIUnknown );
	if ( pIUnkFunctor->ShouldStopInvoking() )
	{
	    break;
	}
    }
}


//-----------------------------------------------------------
CHXIUnknownObserverManagerBase::~CHXIUnknownObserverManagerBase( void )
{
    RemoveAllIUnknowns();
}

CHXIUnknownObserverManagerBase::CHXIUnknownObserverManagerBase( void )
{
}

void
CHXIUnknownObserverManagerBase::AddIUnknownToList( IUnknown* pIUnknown )
{
    HX_ASSERT( pIUnknown );

    // Ensure we have the base IUnknown pointer since we'll be doing an object compare in RemoveIUnknownFromList().
    HXCOMPtr<IUnknown> spIUnkToAdd( pIUnknown );
    HX_ASSERT( spIUnkToAdd.IsValid() );

    IUnknownObserverManagerEntry *pIUnknownObserverManagerEntry = FindIUnknownListEntry(spIUnkToAdd.Ptr());
    if (pIUnknownObserverManagerEntry)
    {
	InterlockedIncrement(&pIUnknownObserverManagerEntry->ulCount);
    }
    else
    {
	pIUnknownObserverManagerEntry = new IUnknownObserverManagerEntry;
	HX_ASSERT(pIUnknownObserverManagerEntry);

	if (pIUnknownObserverManagerEntry)
	{
	    spIUnkToAdd.AsUnknown(&pIUnknownObserverManagerEntry->pIUnknown);
	    pIUnknownObserverManagerEntry->ulCount = 1;

	    m_ListElements.AddTail( pIUnknownObserverManagerEntry );
	}
    }
}

void
CHXIUnknownObserverManagerBase::RemoveIUnknownFromList( IUnknown* pIUnknown )
{
    HX_ASSERT( pIUnknown );
    
    HXCOMPtr<IUnknown> spIUnkToRemove(pIUnknown);
    IUnknownObserverManagerEntry *pIUnknownObserverManagerEntry = FindIUnknownListEntry(spIUnkToRemove.Ptr());
    HX_ASSERT(pIUnknownObserverManagerEntry);

    if (pIUnknownObserverManagerEntry)
    {
	InterlockedDecrement(&pIUnknownObserverManagerEntry->ulCount);
	
	//as usual, when our refcount gets to 0, delete the entry from the list. If
	//the list count goes to 0, then delete the list too.
	if (0 == pIUnknownObserverManagerEntry->ulCount)
	{
	    LISTPOSITION thePositionToRemove = m_ListElements.Find( ( void* ) pIUnknownObserverManagerEntry );
	    HX_ASSERT( thePositionToRemove );
	    if ( thePositionToRemove )
	    {
		m_ListElements.RemoveAt( thePositionToRemove );
	    }

	    HX_RELEASE( pIUnknownObserverManagerEntry->pIUnknown );
	    HX_DELETE( pIUnknownObserverManagerEntry );
	}
    }
}

void
CHXIUnknownObserverManagerBase::RemoveAllIUnknowns( void )
{
    while(!m_ListElements.IsEmpty())
    {
	IUnknownObserverManagerEntry* pIUnknownObserverManagerEntry = (IUnknownObserverManagerEntry *)m_ListElements.RemoveTail(); 
	HX_ASSERT( pIUnknownObserverManagerEntry );

	if (pIUnknownObserverManagerEntry)
	{
	    HX_RELEASE( pIUnknownObserverManagerEntry->pIUnknown );
	    HX_DELETE( pIUnknownObserverManagerEntry );
	}
    }
}

void
CHXIUnknownObserverManagerBase::IterateOverIUnknowns( CHXIUnknownElementFunctor& IUnknownElementFunctor )
{
    CHXSimpleList::Iterator iter = m_ListElements.Begin();

    while (iter != m_ListElements.End())
    {
	IUnknownObserverManagerEntry *pIUnknownObserverManagerEntry = ( IUnknownObserverManagerEntry* )*iter;
	HX_ASSERT( pIUnknownObserverManagerEntry );

        // By incrementing the iterator first it allows for safe removal of receivers
        // from the list while we are iterating since the implementation is with a list.
        ++iter;

	IUnknownElementFunctor( pIUnknownObserverManagerEntry->pIUnknown );
	if ( IUnknownElementFunctor.ShouldStopIterating() )
	{
	    break;
	}
    }
}

void
CHXIUnknownObserverManagerBase::IterateOverIUnknowns( CHXIUnknownElementFunctor& IUnknownElementFunctor ) const
{
    LISTPOSITION thePosition = m_ListElements.GetHeadPosition();
    while ( thePosition )
    {
	IUnknownObserverManagerEntry* pIUnknownObserverManagerEntry = ( IUnknownObserverManagerEntry* ) m_ListElements.GetNext( thePosition );
	HX_ASSERT( pIUnknownObserverManagerEntry );
	IUnknownElementFunctor( pIUnknownObserverManagerEntry->pIUnknown );
	if ( IUnknownElementFunctor.ShouldStopIterating() )
	{
	    break;
	}
    }
}


CHXIUnknownObserverManagerBase::IUnknownObserverManagerEntry *
CHXIUnknownObserverManagerBase::FindIUnknownListEntry(IUnknown* pIUnknown)
{
    HX_ASSERT( pIUnknown );

    for(CHXSimpleList::Iterator iter = m_ListElements.Begin(); iter != m_ListElements.End(); ++iter)
    {
	IUnknownObserverManagerEntry *pIUnknownObserverManagerEntry = ( IUnknownObserverManagerEntry* )*iter;
	HX_ASSERT( pIUnknownObserverManagerEntry );
	if (pIUnknownObserverManagerEntry->pIUnknown == pIUnknown)
	{
	    return pIUnknownObserverManagerEntry;
	}
    }

    return NULL;
}

//Leave a CR/LF before EOF to prevent CVS from getting angry

