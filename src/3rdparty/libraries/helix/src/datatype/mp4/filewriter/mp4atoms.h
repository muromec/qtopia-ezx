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
/*
 * mp4atoms.h: atom support. not matched with the fileformat, since that atom support
 *             is designed solely for reading, and doesn't have much overlap.
 */

#ifndef _MP4ATOMS_H_
#define _MP4ATOMS_H_

#include "blist.h"

// Macro for generating atom ids
#define MP4_BUILD_ATOMID(a,b,c,d) ( ((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

// Macro for defining GetXXX and SetXXX routines for non buffer members of an atom
#define MP4_ATOM_PROPERTY(name, type, member) \
	STDMETHOD_( type, Get##name ) (THIS) { return member; } \
	STDMETHOD( Set##name ) (THIS_ type __Tmp ) { member = __Tmp; return HXR_OK; }


class CMP4Archiver;
class CMP4Atom;

class CMP4Atom : public IUnknown
{
public:

    /*
     * IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ REFIID ID, void** ppInterfaceObj)
	{
	    return HXR_NOINTERFACE;
	}
    STDMETHOD_(UINT32, AddRef )	    (THIS)
	{
	    return InterlockedIncrement( &m_lRefCount );
	}
    STDMETHOD_(UINT32, Release)	    (THIS)
	{
	    if( InterlockedDecrement(&m_lRefCount) > 0 )
	    {
		return m_lRefCount;
	    }
	    delete this;
	    return 0;
	}

    // Constructor
    CMP4Atom( UINT32 ulAtomID )
	{
	    m_lRefCount = 0;
	    ResetWriteStatus();
	    m_ulAtomID = ulAtomID;
	    m_pChildren = NULL;
	}
    virtual ~CMP4Atom()
	{
	    ChildList* pItem;
	    while( (pItem = m_pChildren) )
	    {
		m_pChildren = pItem->Next();
		CMP4Atom* p = pItem->GetAtom();
		HX_RELEASE( p );
		delete pItem;
	    }
	}

    // Find out how large we are at the moment
    STDMETHOD_(UINT32, GetCurrentSize) (THIS_ BOOL bIncludeChildren = FALSE) PURE;
    
    // Force us to serialize our output to the given buffer
    STDMETHOD( WriteToBuffer )         (THIS_ UCHAR*& pBuffer,
	                                BOOL bIncludeChildren = FALSE) PURE;

    // When we are written to a file, NotifyWriteOffset() is called to
    // let us know where we were written to
    STDMETHOD( NotifyWriteOffset )  (THIS_ UINT32 ulWriteOffset)
	{
	    m_ulWriteOffset = ulWriteOffset;
	    m_bHasWritten = TRUE;
	    return HXR_OK;
	}
    
    // This allows the writer to see if we've been written before, and where to
    // IsFirstWrite will return true until NotifyWriteOffset is set
    STDMETHOD_(BOOL, IsFirstWrite)         (THIS)
	{
	    return !m_bHasWritten;
	}
    STDMETHOD_(UINT32, GetLastWriteOffset) (THIS)
	{
	    return m_ulWriteOffset;
	}
    STDMETHOD_(UINT32, GetLastWriteSize)   (THIS)
	{
	    return m_bHasWritten ? m_ulWriteSize : 0;
	}
    // If the writer decides to regenerate all the atoms, it can call ResetWriteStatus
    // to clear any previous-write-oriented state
    STDMETHOD( ResetWriteStatus )          (THIS)
	{
	    m_ulWriteSize = m_ulWriteOffset = 0;
	    m_bHasWritten = FALSE;
	    return HXR_OK;
	}
    STDMETHOD( AddChild )                  ( THIS_ CMP4Atom* pChild )
	{
	    HX_RESULT retVal = HXR_OUTOFMEMORY;
	    ChildList* pItem = new ChildList(pChild);

	    if( pItem )
	    {
		if( m_pChildren )
		{
		    m_pChildren->Add( pItem );
		}
		else
		{
		    m_pChildren = pItem;
		}
		retVal = HXR_OK;
	    }
	    return retVal;
	}

    STDMETHOD_( BOOL, IsType )             ( THIS_ UINT32 ulAtomID )
	{
	    return m_ulAtomID == ulAtomID;
	}
    
    // XXX Depth first may not be the best choice for this. 
    STDMETHOD_( CMP4Atom*, FindChild)      ( THIS_ UINT32 ulAtomID, BOOL bFullTraversal = FALSE )
	{
	    CMP4Atom* result = NULL;
	    ChildList* p = m_pChildren;
	    while( !result && p )
	    {
		CMP4Atom* tmp = p->GetAtom();
		if( tmp->IsType( ulAtomID ) )
		{
		    result = tmp;
		}
		else if( bFullTraversal )
		{
		    result = tmp->FindChild( ulAtomID, bFullTraversal );
		}
		p = p->Next();
	    }
	    
	    return result;
	}
    STDMETHOD_( CMP4Atom*, FindChild )      ( THIS_ char* pAtomName, BOOL bFullTraversal = FALSE )
	{
	    CMP4Atom* result = NULL;

	    if( pAtomName )
	    {
		result = FindChild(
		    MP4_BUILD_ATOMID( pAtomName[0], pAtomName[1],
			              pAtomName[2], pAtomName[3] ),
		    bFullTraversal);
	    }
	    return result;
	}
    
protected:
    UINT32  GetSizeOfChildren()
	{
	    UINT32 len = 0;
	    ChildList* p = m_pChildren;
	    while( p )
	    {
		len += p->GetAtom()->GetCurrentSize(TRUE);
		p = p->Next();
	    }
	    return len;
	}
    HX_RESULT ChildrenWriteToBuffer( UCHAR*& pBuffer )
	{
	    ChildList* p = m_pChildren;
	    HX_RESULT retVal = HXR_OK;
	    while( p && SUCCEEDED(retVal) )
	    {
		retVal = p->GetAtom()->WriteToBuffer( pBuffer, TRUE );
		p = p->Next();
	    }
	    return retVal;
	}
    
    UINT32  m_ulAtomID;

protected:
    static void WriteToBufferAndInc(UCHAR*& pBuf, UINT64 val)
	{
	    WriteToBufferAndInc( pBuf, (UINT32) (val >> 32) );
	    WriteToBufferAndInc( pBuf, (UINT32) (val & 0xffffffff));
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, UINT32 val)
	{
	    *pBuf++ = (UCHAR) ((val >> 24));
	    *pBuf++ = (UCHAR) ((val >> 16) & 0xff);
	    *pBuf++ = (UCHAR) ((val >>  8) & 0xff);
	    *pBuf++ = (UCHAR) (val & 0xff);
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, UINT16 val)
	{
	    *pBuf++ = val >>   8;
	    *pBuf++ = val & 0xff;
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, UCHAR val)
	{
	    *pBuf++ = val;
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, UINT64* arr, UINT32 len)
	{
	    while(len--)
	    {
		WriteToBufferAndInc( pBuf, *arr++ );
	    }
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, UINT32* arr, UINT32 len)
	{
	    while(len--)
	    {
		WriteToBufferAndInc( pBuf, *arr++ );
	    }
	}
    // XXX these shifts probably sign extend, i.e. the mask needs to happen first
    //     or the values need to be cast to UINT
    static void WriteToBufferAndInc(UCHAR*& pBuf, INT32 val)
	{
	    *pBuf++ = (UCHAR) ((val >> 24));
	    *pBuf++ = (UCHAR) ((val >> 16) & 0xff);
	    *pBuf++ = (UCHAR) ((val >>  8) & 0xff);
	    *pBuf++ = (UCHAR) (val & 0xff);
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, INT16 val)
	{
	    *pBuf++ = val >>   8;
	    *pBuf++ = val & 0xff;
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, INT32* arr, UINT32 len)
	{
	    while(len--)
	    {
		WriteToBufferAndInc( pBuf, *arr++ );
	    }
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, const UCHAR* str)
	{
	    // The +1 forces the null terminator to be copied
	    int len = strlen((const char*)str) + 1;
	    strncpy( (char*)pBuf, (const char*)str, len );
	    pBuf += len;
	}
    static void WriteToBufferAndInc(UCHAR*& pBuf, const CHAR* str)
	{
	    // The +1 forces the null terminator to be copied
	    int len = strlen(str) + 1;
	    strncpy( (char*)pBuf, str, len );
	    pBuf += len;
	}
    static void WriteToBufferAndInc(UCHAR*&pBuf, const UCHAR* buf, UINT32 len)
	{
	    memcpy( pBuf, buf, len );
	    pBuf += len;
	}
    
    class ChildList
    {
    public:
	ChildList( CMP4Atom* pAtom ) : m_pAtom(pAtom), m_pNext(NULL)
	    {
		if( m_pAtom )
		{
		    pAtom->AddRef();
		}
	    }
	// Add to tail
	void Add( ChildList* pNext )
	    {
		if( m_pNext )
		{
		    m_pNext->Add( pNext );
		}
		else
		{
		    m_pNext = pNext;
		}
	    }
	// Traverse
	ChildList* Next()
	    {
		return m_pNext;
	    }
	// Fetch contents
	CMP4Atom* GetAtom()
	    {
		return m_pAtom;
	    }
#if 0
	// Clean up
	void Destroy()
	    {
		m_pAtom->Release();
		if( m_pNext )
		{
		    m_pNext->Destroy();
		}
	    }
#endif
    private:
	CMP4Atom* m_pAtom;
	ChildList* m_pNext;
    };

    LONG              m_lRefCount;

    ChildList*        m_pChildren;

    UINT32            m_ulWriteSize;
    UINT32            m_ulWriteOffset;
    BOOL              m_bHasWritten;
};
    
class CMP4ContainerAtom : public CMP4Atom
{
public:
    CMP4ContainerAtom(UINT32 uiAtomID) : CMP4Atom(uiAtomID)
	{}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    return 8 + (bIncludeChildren ? GetSizeOfChildren() : 0);
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		WriteToBufferAndInc( pBuffer, GetCurrentSize(TRUE) );
		WriteToBufferAndInc( pBuffer, m_ulAtomID );

		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
};

class CMP4VersionedAtom : public CMP4Atom
{
public:
    CMP4VersionedAtom(UINT32 uiAtomID) : CMP4Atom(uiAtomID)
	{
	    m_ucVersion = 0;
	    m_uiFlags   = 0;
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    return 8 + 1 + 3 + (bIncludeChildren ? GetSizeOfChildren() : 0);
	}
protected:
    // The derived object should set up the buffer first, then call us
    void WriteAtomAndVersionInfo( UCHAR*& p )
	{
	    UINT32 verflags = (((UINT32)m_ucVersion) << 24) | (m_uiFlags & 0x00ffffff);
	    // XXX the below GetCurrentSize() call should hit the most derived obj...
	    CMP4Atom::WriteToBufferAndInc( p, GetCurrentSize(TRUE) );
	    CMP4Atom::WriteToBufferAndInc( p, m_ulAtomID );
	    CMP4Atom::WriteToBufferAndInc( p, verflags );
	}
    
    UCHAR  m_ucVersion;
    UINT32 m_uiFlags;    // actually 24 bit
};


// Conceptually, there has to be a 'root' atom which represents
// the file itself; this atom is never written, and isn't really
// a container, but within it are all the top-level atoms,
// represented as children

// atom:      file
// container: n/a
// purpose:   provides a top level atom, specific to our implementation
class CMP4Atom_file : public CMP4Atom
{
public:
    CMP4Atom_file() : CMP4Atom(MP4_BUILD_ATOMID('f','i','l','e'))
	{}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    return bIncludeChildren ? GetSizeOfChildren() : 0;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_NOTIMPL;
	    if( bIncludeChildren )
	    {
		retVal = ChildrenWriteToBuffer( pBuffer );
	    }
	    return retVal;
	}
};

// atom:      ftyp
// container: file
// purpose:   indicates the type and version of the file contents
// XXX needs to be genericized
class CMP4Atom_ftyp : public CMP4Atom
{
public:
    CMP4Atom_ftyp() : CMP4Atom(MP4_BUILD_ATOMID('f','t','y','p'))
	{}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    return 32;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {
		CMP4Atom::WriteToBufferAndInc( pBuffer, GetCurrentSize() );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_ulAtomID );
		CMP4Atom::WriteToBufferAndInc( pBuffer, (UCHAR*)"M4A ", 4 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, (UINT32)0 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, (UCHAR*)"M4A mp42isom", 12);
		CMP4Atom::WriteToBufferAndInc( pBuffer, (UINT32)0 );
		if( bIncludeChildren )
		{
		    ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
};
    
// atom:      moov
// container: file
// purpose:   container for all meta-data
class CMP4Atom_moov : public CMP4ContainerAtom
{
public:
    CMP4Atom_moov() : CMP4ContainerAtom(MP4_BUILD_ATOMID('m','o','o','v'))
	{}
};

// atom:      mvhd
// container: moov
// purpose:   movie header
// notes:     version 0: creation, modification, and duration are 32 bits
//            version 1: above are 64 bits
class CMP4Atom_mvhd : public CMP4VersionedAtom
{
public:
    CMP4Atom_mvhd() : CMP4VersionedAtom(MP4_BUILD_ATOMID('m','v','h','d'))
	{
	    __uiReserved1    = 0;
	    __uiReserved2[0] = __uiReserved2[1] = 0;
	    m_iRate  = 0x00010000;
	    m_iVolume= 0x0100;
	    memset( __predefined, 0, sizeof(__predefined) );
	    m_iMatrix[0] = 0x00010000;
	    m_iMatrix[1] = 0x00000000;
	    m_iMatrix[2] = 0x00000000;
	    m_iMatrix[3] = 0x00000000;
	    m_iMatrix[4] = 0x00010000;
	    m_iMatrix[5] = 0x00000000;
	    m_iMatrix[6] = 0x00000000;
	    m_iMatrix[7] = 0x00000000;
	    m_iMatrix[8] = 0x40000000;
	    memset( &( m_Timestamps ), 0, sizeof( m_Timestamps ) );
	}
    STDMETHOD_(UINT32, GetCurrentSize )       (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    // In order with elements below; 0 is a placeholder for m_Timestamps
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 0 + 4 + 2 + 2 + 8 + 36 + 24 + 4;
	    if( m_ucVersion == 1 )
	    {
		len += sizeof( m_Timestamps.v1 );
	    }
	    else
	    {
		len += sizeof( m_Timestamps.v0 );
	    }
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );
		if( m_ucVersion == 1 )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v1.creation_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v1.modification_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v1.timescale );
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v1.duration );
		}
		else
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v0.creation_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v0.modification_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v0.timescale );
		    CMP4Atom::WriteToBufferAndInc( pBuffer,
			                           m_Timestamps.v0.duration );
		}
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iRate );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iVolume );
		CMP4Atom::WriteToBufferAndInc( pBuffer, __uiReserved1 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, __uiReserved2, 2 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iMatrix, 9 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, __predefined, 6 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiNextTrackID );

		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    
    MP4_ATOM_PROPERTY(Timescale,   UINT32, m_Timestamps.v0.timescale);
    MP4_ATOM_PROPERTY(Duration,    UINT32, m_Timestamps.v0.duration);
    MP4_ATOM_PROPERTY(NextTrackID, UINT32, m_uiNextTrackID);
    
private:
    union 
    {
	struct
	{
	    UINT64 creation_time;
	    UINT64 modification_time;
	    UINT32 timescale;
	    UINT64 duration;
	} v1;
	struct
	{
	    UINT32 creation_time;
	    UINT32 modification_time;
	    UINT32 timescale;
	    UINT32 duration;
	} v0;
    } m_Timestamps;

    INT32 m_iRate;    // 16.16 fixed pt, usually 0x00010000
    INT16 m_iVolume;  // 8.8 fixed pt, usually 0x0100
    UINT16 __uiReserved1;
    UINT32 __uiReserved2[2];
    INT32  m_iMatrix[9]; // transformation matrix
    INT32  __predefined[6];
    UINT32 m_uiNextTrackID;
};

    
// atom:      trak
// container: moov
// purpose:   track
class CMP4Atom_trak : public CMP4ContainerAtom
{
public:
    CMP4Atom_trak() : CMP4ContainerAtom(MP4_BUILD_ATOMID('t','r','a','k'))
	{}
};

// atom:      tkhd
// container: trak
// purpose:   track header
class CMP4Atom_tkhd : public CMP4VersionedAtom
{
public:
    CMP4Atom_tkhd() : CMP4VersionedAtom(MP4_BUILD_ATOMID('t','k','h','d'))
	{
	    m_uiFlags = Track_enabled | Track_in_movie | Track_in_preview;
	    memset( &( m_Timestamps ), 0, sizeof( m_Timestamps ) );
	    __uiReserved1[0] = __uiReserved1[1] = 0;
	    m_iLayer = 0;
	    m_iPredefined = 0;
	    m_iVolume = 0x0100;   // default for audio
	    __uiReserved2 = 0;
	    
	    m_iMatrix[0] = 0x00010000;
	    m_iMatrix[1] = m_iMatrix[2] = m_iMatrix[3] = 0;
	    m_iMatrix[4] = 0x00010000;
	    m_iMatrix[5] = m_iMatrix[6] = m_iMatrix[7] = 0;
	    m_iMatrix[8] = 0x40000000;
	    
	    m_iWidth = m_iHeight = 0;
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len =  8 + 1 + 3 + 0 + 8 + 2 + 2 + 2 + 2 + 36 + 4 + 4;
	    if( m_ucVersion == 1 )
	    {
		len += 8 + 8 + 4 + 4 + 8;
	    }
	    else {
		len += 4 + 4 + 4 + 4 + 4;
	    }
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		if( m_ucVersion == 1 )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.creation_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.modification_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.track_id );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.reserved );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.duration );
		}
		else
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.creation_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.modification_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.track_id );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.reserved );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.duration );
		}
		CMP4Atom::WriteToBufferAndInc( pBuffer, __uiReserved1, 2 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iLayer );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iPredefined );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iVolume );
		CMP4Atom::WriteToBufferAndInc( pBuffer, __uiReserved2 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iMatrix, 9 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iWidth );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iHeight );
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}

    MP4_ATOM_PROPERTY(Duration, UINT32, m_Timestamps.v0.duration);
    MP4_ATOM_PROPERTY(TrackID,  UINT32, m_Timestamps.v0.track_id);
private:
    enum _flags
    {
	Track_enabled    = 0x000001,
	Track_in_movie   = 0x000002,
	Track_in_preview = 0x000004,
    };
    union 
    {
	struct
	{
	    UINT64 creation_time;
	    UINT64 modification_time;
	    UINT32 track_id;
	    UINT32 reserved;
	    UINT64 duration;
	} v1;
	struct
	{
	    UINT32 creation_time;
	    UINT32 modification_time;
	    UINT32 track_id;
	    UINT32 reserved;
	    UINT32 duration;
	} v0;
    } m_Timestamps;
    
    UINT32 __uiReserved1[2];
    INT16  m_iLayer;
    INT16  m_iPredefined;
    INT16  m_iVolume;
    UINT16 __uiReserved2;
    INT32  m_iMatrix[9];
    INT32  m_iWidth;
    INT32  m_iHeight;
};

// atom:      mdia
// container: trak
// purpose:   media box
class CMP4Atom_mdia : public CMP4ContainerAtom
{
public:
    CMP4Atom_mdia() : CMP4ContainerAtom(MP4_BUILD_ATOMID('m','d','i','a'))
	{}
};

// atom:      mdhd
// container: mdia
// purpose:   media header
class CMP4Atom_mdhd : public CMP4VersionedAtom
{
public:
    CMP4Atom_mdhd() : CMP4VersionedAtom(MP4_BUILD_ATOMID('m','d','h','d'))
	{
	    m_pad = 0;
	    __predefined = 0;
	    memset( &( m_Timestamps ), 0, sizeof( m_Timestamps ) );
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 2 + 2;
	    if( m_ucVersion == 1 )
	    {
		len += 8 + 8 + 4 + 8;
	    }
	    else {
		len += 4 + 4 + 4 + 4;
	    }
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );
		if( m_ucVersion == 1 )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.creation_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.modification_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.timescale );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v1.duration );
		}
		else
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.creation_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.modification_time );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.timescale );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_Timestamps.v0.duration );
		}
		// There are no gaurantees as to how the compiler will order the bitfield
		// so we need to assemble this ourselves.
		UINT16 lang = (m_pad       << 15) |
			      (m_language0 << 10) |
			      (m_language1 <<  5) |
			      (m_language2 <<  0);
		CMP4Atom::WriteToBufferAndInc( pBuffer, lang );
		CMP4Atom::WriteToBufferAndInc( pBuffer, __predefined );

		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    
    MP4_ATOM_PROPERTY(Timescale, UINT32, m_Timestamps.v0.timescale);
    MP4_ATOM_PROPERTY(Duration,  UINT32, m_Timestamps.v0.duration);
    
    STDMETHOD( SetLanguage )   (THIS_ UCHAR a, UCHAR b, UCHAR c)
	{
	    HX_RESULT retVal = HXR_FAIL;
	    if( a >= 0x60 && b >= 0x60 && c >= 0x60 )
	    {
		m_language0 = (a - 0x60);
		m_language1 = (b - 0x60);
		m_language2 = (c - 0x60);
		retVal = HXR_OK;
	    }
	    
	    return retVal;
	}
	
private:
    union
    {
	struct
	{
	    UINT64 creation_time;
	    UINT64 modification_time;
	    UINT32 timescale;
	    UINT64 duration;
	} v1;
	struct
	{
	    UINT32 creation_time;
	    UINT32 modification_time;
	    UINT32 timescale;
	    UINT32 duration;
	} v0;
    } m_Timestamps;

    UINT16 m_pad      : 1,
           m_language0: 5,
           m_language1: 5,
           m_language2: 5;
    UINT16 __predefined;
};

// atom:      hdlr
// container: mdia
// purpose:   handler reference box
class CMP4Atom_hdlr : public CMP4VersionedAtom
{
public:
    CMP4Atom_hdlr() : CMP4VersionedAtom(MP4_BUILD_ATOMID('h','d','l','r'))
	{
	    __predefined = 0;
	    m_uiHandlerType = 0;
	    m_reserved[0] = m_reserved[1] = m_reserved[2] = 0;
	    m_pszName = NULL;
	    m_uiNameLength = 0;
	}
    ~CMP4Atom_hdlr()
	{
	    HX_VECTOR_DELETE( m_pszName );
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + 4 + 12 + (m_uiNameLength ? m_uiNameLength : 2);
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );
		
		CMP4Atom::WriteToBufferAndInc( pBuffer, __predefined );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiHandlerType );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved, 3 );

		if( m_pszName )
		{
		    // This will copy the null terminator
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_pszName );
		} else
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, (UINT16)0 );
		}
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    STDMETHOD( SetHandlerType )      (THIS_ const char* pHandlerType )
	{
	    const char* p = pHandlerType;
	    m_uiHandlerType = MP4_BUILD_ATOMID( p[0], p[1], p[2], p[3] );
	    return HXR_OK;
	}
private:
    UINT32 __predefined;
    UINT32 m_uiHandlerType;
    UINT32 m_reserved[3];
    UCHAR* m_pszName;
    
    UINT32 m_uiNameLength;  // includes null terminator, not part of atom
};

// atom:      minf
// container: mdia
// purpose:   media information box
class CMP4Atom_minf : public CMP4ContainerAtom
{
public:
    CMP4Atom_minf() : CMP4ContainerAtom(MP4_BUILD_ATOMID('m','i','n','f'))
	{}
};

// atom:      smhd
// container: minf
// purpose:   sound media header box
class CMP4Atom_smhd : public CMP4VersionedAtom
{
public:
    CMP4Atom_smhd() : CMP4VersionedAtom(MP4_BUILD_ATOMID('s','m','h','d'))
	{
	    m_iBalance = 0;
	    m_reserved = 0;
	}
    
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 2 + 2;
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );
		
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_iBalance );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved );
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    
private:
    INT16  m_iBalance;
    INT16  m_reserved;
};


// atom:      dinf
// container: minf
// purpose:   data information box
class CMP4Atom_dinf : public CMP4ContainerAtom
{
public:
    CMP4Atom_dinf() : CMP4ContainerAtom(MP4_BUILD_ATOMID('d','i','n','f'))
	{}
};

// atom:      url
// container: dreff
// purpose:   url for content that may be used in the presentation
// note:      m_uiFlags == 0x000001 indicates media data is in the same file as this atom
class CMP4Atom_url : public CMP4VersionedAtom
{
public:
    CMP4Atom_url() : CMP4VersionedAtom(MP4_BUILD_ATOMID('u','r','l',' '))
	{
	    m_uiFlags = 0x000001;
	    m_pszLocation = NULL;
	    m_uiLocationLength = 0;
	}
    ~CMP4Atom_url()
	{
	    HX_VECTOR_DELETE( m_pszLocation );
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + m_uiLocationLength;
	    
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		// If the content is local, this will not be set (but m_uiFlags
		//  will be 0x000001)
		if( m_pszLocation )
		{
		    // This will copy the null terminator
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_pszLocation );
		}
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
private:
    UCHAR* m_pszLocation;
    UINT32 m_uiLocationLength;
};

// atom:      urn
// container: dref
// purpose:   urn for content that may be used in the presentation
// note:      m_uiFlags == 0x000001 indicates media data is in the same file as this atom
class CMP4Atom_urn : public CMP4VersionedAtom
{
public:
    CMP4Atom_urn() : CMP4VersionedAtom(MP4_BUILD_ATOMID('u','r','n',' '))
	{
	    m_pszName = NULL;
	    m_uiNameLength = 0;
	    m_pszLocation = NULL;
	    m_uiLocationLength = 0;
	}
    ~CMP4Atom_urn()
	{
	    HX_VECTOR_DELETE( m_pszName );
	    HX_VECTOR_DELETE( m_pszLocation );
	}
    
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + m_uiNameLength + m_uiLocationLength;
	    
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		if( m_pszName )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_pszName );
		}
		if( m_pszLocation )
		{
		    // This will copy the null terminator
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_pszLocation );
		}
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
private:
    UCHAR* m_pszName;
    UINT32 m_uiNameLength;
    UCHAR* m_pszLocation;
    UINT32 m_uiLocationLength;
};

// atom:      dref
// container: dinf
// purpose:   data reference table; contains multiple URL or URN objects
// notes:     this is both versioned and container 
class CMP4Atom_dref : public CMP4VersionedAtom
{
public:
    CMP4Atom_dref() : CMP4VersionedAtom(MP4_BUILD_ATOMID('d','r','e','f'))
	{
	    m_uiEntryCount = 0;
	}
    STDMETHOD_(UINT32, GetCurrentSize )  (THIS_ BOOL bIncludeChildren = FALSE )
	{
	    UINT32 base = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren);
	    
	    return base + 4;
	}
    
    STDMETHOD( WriteToBuffer )     (THIS_ UCHAR*& pBuffer,
	                            BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );
	    
	    CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiEntryCount );
	    
	    if( bIncludeChildren )
	    {
		retVal = ChildrenWriteToBuffer( pBuffer );
	    }
	    return retVal;
	}
    STDMETHOD( AddChild )         (THIS_ CMP4Atom* pAtom )
	{
	    HX_RESULT retVal;
	    retVal = CMP4VersionedAtom::AddChild( pAtom );
	    if( SUCCEEDED( retVal ) )
	    {
		m_uiEntryCount++;
	    }
	    return retVal;
	}
private:
    UINT32 m_uiEntryCount;
};


// atom:      stbl
// container: minf
// purpose:   sample table box
class CMP4Atom_stbl : public CMP4ContainerAtom
{
public:
    CMP4Atom_stbl() : CMP4ContainerAtom(MP4_BUILD_ATOMID('s','t','b','l'))
	{}
};

// atom:      stts
// container: stbl
// purpose:   sample table box
class CMP4Atom_stts : public CMP4VersionedAtom
{
public:
    CMP4Atom_stts() : CMP4VersionedAtom(MP4_BUILD_ATOMID('s','t','t','s'))
	{
	    m_uiEntryCount = 0;
	}
    STDMETHOD_( UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + (8 * m_uiEntryCount);
	    
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiEntryCount );

		m_blSampleCountsDeltas.DumpToBuffer( pBuffer );
		pBuffer += (8 * m_uiEntryCount);
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    STDMETHOD( AddSamplePair )       (THIS_ UINT32 uiSampleCount, INT32 iSampleDelta )
	{
	    int idx = m_blSampleCountsDeltas.GetSize();
	    m_blSampleCountsDeltas[idx++] = (void*)uiSampleCount;
	    m_blSampleCountsDeltas[idx]   = (void*)iSampleDelta;
	    m_uiEntryCount++;
	    return HXR_OK;
	}
    // Get the timestamp delta for the previous entry; if no previous entry exists, return 0
    STDMETHOD_( INT32, GetLastSampleDelta ) (THIS)
	{
	    int idx = m_blSampleCountsDeltas.GetSize();
	    INT32 val = 0;
	    
	    if( idx )
	    {
		val = (INT32) (m_blSampleCountsDeltas[idx - 1]);
	    }

	    return val;
	}
    // Increment the sample count on the previous entry (XXX non error checked)
    STDMETHOD( IncrementLastSampleCount )   (THIS)
	{
	    int idx = m_blSampleCountsDeltas.GetSize();
	    UINT32 val = (UINT32) (m_blSampleCountsDeltas[idx - 2]);
	    val++;
	    m_blSampleCountsDeltas[idx-2] = (void*) val;
	    return HXR_OK;
	}
    
private:
    UINT32 m_uiEntryCount;

    // Pairs structured as:
    //  UINT32 uiSampleCount
    //  INT32  iSampleDelta
    CBList m_blSampleCountsDeltas;
};

// atom:      stsd
// container: stbl
// purpose:   sample table box
class CMP4Atom_stsd : public CMP4VersionedAtom
{
public:
    CMP4Atom_stsd() : CMP4VersionedAtom(MP4_BUILD_ATOMID('s','t','s','d'))
	{
	    m_uiEntryCount = 0;
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4;
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {	

		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiEntryCount );

		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    STDMETHOD( AddChild )         (THIS_ CMP4Atom* pAtom )
	{
	    HX_RESULT retVal;
	    retVal = CMP4VersionedAtom::AddChild( pAtom );
	    if( SUCCEEDED( retVal ) )
	    {
		m_uiEntryCount++;
	    }
	    return retVal;
	}
private:
    UINT32 m_uiEntryCount;
};

// atom:      mp4a
// container: stsd
// purpose:   audio sample description entry
class CMP4Atom_mp4a : public CMP4ContainerAtom
{
public:
    CMP4Atom_mp4a() : CMP4ContainerAtom(MP4_BUILD_ATOMID('m','p','4','a'))
	{
	    memset( m_reserved1, 0, sizeof(m_reserved1) );
	    m_usDataReferenceIndex = 1;
	    memset( m_reserved2, 0, sizeof(m_reserved2) );
	    m_reserved3 = 2;
	    m_reserved4 = 16;
	    m_reserved5 = 0;
	    m_usTimescale = 0;
	    m_reserved6 = 0;
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4ContainerAtom::GetCurrentSize(bIncludeChildren)
			 + sizeof(m_reserved1) + 2 + sizeof(m_reserved2) + 2 + 2 + 4 + 2 + 2;
	    return len;
	}
    STDMETHOD( WriteToBuffer )       (THIS_ UCHAR*& pBuffer,
	                              BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    
	    if( pBuffer )
	    {
		CMP4ContainerAtom::WriteToBuffer( pBuffer, FALSE );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved1, 6 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_usDataReferenceIndex );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved2, 2 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved3 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved4 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved5 );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_usTimescale );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved6 );

		if( bIncludeChildren )
		{
		    ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    
	    return retVal;
	}
    MP4_ATOM_PROPERTY(Timescale,  UINT16, m_usTimescale);

private:
    UCHAR   m_reserved1[6];
    UINT16  m_usDataReferenceIndex;
    UINT32  m_reserved2[2];
    UINT16  m_reserved3;    // always 2
    UINT16  m_reserved4;    // always 16
    UINT32  m_reserved5;
    UINT16  m_usTimescale;  // from track
    UINT16  m_reserved6;
};

// atom:      esds
// container: mp4a or mp4v
// purpose:   elementary stream descriptor
// note:      the esds is actually a descriptor, so it's expandable, which impacts how field lengths are written.
class CMP4Atom_esds : public CMP4VersionedAtom
{
public:
    CMP4Atom_esds() : CMP4VersionedAtom(MP4_BUILD_ATOMID('e','s','d','s'))
	{
	    memset( (void*) &m_ES_Descriptor, 0, sizeof( m_ES_Descriptor ) );

	    m_ES_Descriptor.m_ucESTag = 0x03;
	    m_ES_Descriptor.m_DC_Descriptor.m_ucDCDTag = 0x04;
	    m_ES_Descriptor.m_DC_Descriptor.m_reserved = 1;

	    m_ES_Descriptor.m_DC_Descriptor.m_ucDSTag = 0x05;
	    
	    m_ucSLConfigTag = 0x06;
	    m_ucPredefined  = 0x02;
	}
    ~CMP4Atom_esds()
	{
	    HX_VECTOR_DELETE( m_ES_Descriptor.m_DC_Descriptor.m_pDecoderSpecificInfo );
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren);

	    // es_descriptor; add in the tag and the length field
	    len += 1 + 4 + GetESDescriptorLen();
	    
	    // dc_descriptor
	    len += 1 + 4 + GetDCDescriptorLen();

	    // slconfig descriptor
	    len += 1 + 4 + 1;
	    
	    return len;
	}

    
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {
		// first fix up all the lengths

		// slconfig: just the predefined byte for mp4
		m_ulSLConfigLength = 1;

		// dc descriptor
		m_ES_Descriptor.m_DC_Descriptor.m_ulLen = GetDCDescriptorLen();
		
		// es_descriptor. we have to incorporate the tag (1) and length (4) fields for our children.
		m_ES_Descriptor.m_ulLen = GetESDescriptorLen() + 5 + m_ulSLConfigLength + 5 + m_ES_Descriptor.m_DC_Descriptor.m_ulLen;

		// now class expand the lengths...
		ConvertLength( m_ulSLConfigLength );
		ConvertLength( m_ES_Descriptor.m_DC_Descriptor.m_ulLen );
		ConvertLength( m_ES_Descriptor.m_ulLen );

		// now write the data
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );
		
		ES_Descriptor_t* pES = &(m_ES_Descriptor);
		CMP4Atom::WriteToBufferAndInc( pBuffer, pES->m_ucESTag );
		CMP4Atom::WriteToBufferAndInc( pBuffer, pES->m_ulLen );
		CMP4Atom::WriteToBufferAndInc( pBuffer, pES->m_usESID );
		UCHAR flags =
		    (pES->m_bStreamDepFlag << 7) |
		    (pES->m_bURLFlag       << 6) |
		    (pES->m_bOCRStreamFlag << 5) |
		    (pES->m_ucStreamPriority   );
		CMP4Atom::WriteToBufferAndInc( pBuffer, flags );
		if( pES->m_bStreamDepFlag )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, pES->m_usDependsOnESID );
		}
		if( pES->m_bOCRStreamFlag )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, pES->m_usOCR_ESID );
		}

		// dc descriptor
		DC_Descriptor_t* pDC = &(pES->m_DC_Descriptor);
		CMP4Atom::WriteToBufferAndInc( pBuffer, pDC->m_ucDCDTag );
		CMP4Atom::WriteToBufferAndInc( pBuffer, pDC->m_ulLen );
		CMP4Atom::WriteToBufferAndInc( pBuffer, pDC->m_ucObjectTypeIndication );

		flags =
		    (pDC->m_ucStreamType << 2) |
		    (pDC->m_bUpStream    << 1) |
		    (pDC->m_reserved);
		CMP4Atom::WriteToBufferAndInc( pBuffer, flags );

		UCHAR ucBufferSizeDB[3];
		ucBufferSizeDB[0] = (UCHAR)((pDC->m_uiBufferSizeDB >> 16) & 0xff);
		ucBufferSizeDB[1] = (UCHAR)((pDC->m_uiBufferSizeDB >>  8) & 0xff);
		ucBufferSizeDB[2] = (UCHAR)((pDC->m_uiBufferSizeDB >>  0) & 0xff);
		CMP4Atom::WriteToBufferAndInc( pBuffer, ucBufferSizeDB, 3 );

		CMP4Atom::WriteToBufferAndInc( pBuffer, pDC->m_uiMaxBitrate );
		CMP4Atom::WriteToBufferAndInc( pBuffer, pDC->m_uiAvgBitrate );
		
		if( pDC->m_uiDecoderSpecificInfoLength )
		{
		    UINT32 uiExpandedLen = pDC->m_uiDecoderSpecificInfoLength;
		    ConvertLength( uiExpandedLen );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, pDC->m_ucDSTag );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, uiExpandedLen );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, pDC->m_pDecoderSpecificInfo, pDC->m_uiDecoderSpecificInfoLength );
		}

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_ucSLConfigTag );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_ulSLConfigLength );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_ucPredefined );
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    
	    return retVal;
	}
    STDMETHOD( SetDecoderSpecificInfo ) ( THIS_ UCHAR* pBuffer, UINT32 len )
	{
	    HX_RESULT retVal = HXR_OUTOFMEMORY;

	    UCHAR* p = new UCHAR[len];

	    if( p )
	    {
		HX_VECTOR_DELETE( m_ES_Descriptor.m_DC_Descriptor.m_pDecoderSpecificInfo );
		memcpy( p, pBuffer, len );
		m_ES_Descriptor.m_DC_Descriptor.m_uiDecoderSpecificInfoLength = len;
		m_ES_Descriptor.m_DC_Descriptor.m_pDecoderSpecificInfo = p;
		retVal = HXR_OK;
	    }
	    return retVal;
	}
    
    
    MP4_ATOM_PROPERTY(ObjectType,   UCHAR,  m_ES_Descriptor.m_DC_Descriptor.m_ucObjectTypeIndication);
    MP4_ATOM_PROPERTY(StreamType,   UCHAR,  m_ES_Descriptor.m_DC_Descriptor.m_ucStreamType);
    MP4_ATOM_PROPERTY(BufferSizeDB, UINT32, m_ES_Descriptor.m_DC_Descriptor.m_uiBufferSizeDB);
    MP4_ATOM_PROPERTY(MaxBitrate,   UINT32, m_ES_Descriptor.m_DC_Descriptor.m_uiMaxBitrate);
    MP4_ATOM_PROPERTY(AvgBitrate,   UINT32, m_ES_Descriptor.m_DC_Descriptor.m_uiAvgBitrate);

    enum ObjectTypeIndicationValues
    {
	OTI_Forbidden         = 0x00,
	OTI_14496_1A          = 0x01,
	OTI_14496_1B          = 0x02,
	OTI_InteractionStream = 0x03,
	/* 0x04 - 0x1F are reserved */
	OTI_Visual_14496_2    = 0x20,
	/* 0x21 - 0x3F are reserved */
	OTI_Audio_14496_3     = 0x40,
	/* 0x41 - 0x5F are reserved */
	OTI_Visual_13818_2_Simple   = 0x60,
	OTI_Visual_13818_2_Main     = 0x61,
	OTI_Visual_13818_2_SNR      = 0x62,
	OTI_Visual_13818_2_Spatial  = 0x63,
	OTI_Visual_13818_2_High     = 0x64,
	OTI_Visual_13818_2_422      = 0x65,
	OTI_Audio_13818_7_Main      = 0x66,
	OTI_Audio_13818_7_Low       = 0x67,
	OTI_Audio_13818_7_Scaleable = 0x68,
	OTI_Audio_13818_3           = 0x69,
	OTI_Visual_11172_2          = 0x6A,
	OTI_Audio_11172_3           = 0x6B,
	OTI_Visual_10918_1          = 0x6C,
	
	OTI_NoObjectType            = 0xFF
    };
    
    enum DecoderConfigStreamTypes
    {
	ST_Forbidden               = 0x00,
	ST_ObjectDescriptorStream  = 0x01,
	ST_ClockReferenceStream    = 0x02,
	ST_SceneDescriptionStream  = 0x03,
	ST_VisualStream            = 0x04,
	ST_AudioStream             = 0x05,
	ST_MPEG7Stream             = 0x06,
	ST_IPMPStream              = 0x07,
	ST_ObjectContentInfoStream = 0x08,
	ST_MPEGJStream             = 0x09,
	ST_InteractionStream       = 0x0a,
    };

    // from 14496 sp1; this needs to be cleaned up
    enum DS_Samplerates
    {
	SR_96000 =  0,
	SR_88200 =  1,
	SR_64000 =  2,
	SR_48000 =  3,
	SR_44100 =  4,
	SR_32000 =  5,
	SR_24000 =  6,
	SR_22050 =  7,
	SR_16000 =  8,
	SR_12000 =  9,
	SR_11025 = 10,
	SR_8000  = 11,
	SR_7350  = 12,
	SR_ESCAPE= 15, // used for non standard SR, not supported
    };
    
private:
    // decoderconfigdescriptor
    typedef struct _DC_Descriptor_s
    {
	UCHAR    m_ucDCDTag;        // always 0x04
	UINT32   m_ulLen;           // class expansion length
	UCHAR    m_ucObjectTypeIndication;
	UCHAR    m_ucStreamType;    // 6 bits
	BOOL     m_bUpStream;
	BOOL     m_reserved;        // always 1
	UINT32   m_uiBufferSizeDB;  // 24 bits
	UINT32   m_uiMaxBitrate;
	UINT32   m_uiAvgBitrate;
	
	// optional
	UCHAR    m_ucDSTag;          // always 0x05
	UINT32   m_uiDecoderSpecificInfoLength;   // class expansion
	UCHAR*   m_pDecoderSpecificInfo;
    } DC_Descriptor_t;
    
    // ES_Descriptor
    typedef struct _ES_Descriptor_s
    {
	UCHAR    m_ucESTag;          // always 0x03
	UINT32   m_ulLen;            // class expansion length
	UINT16   m_usESID;
	BOOL     m_bStreamDepFlag;
	BOOL     m_bURLFlag;         // not supported
	BOOL     m_bOCRStreamFlag;
	UCHAR    m_ucStreamPriority; // 5 bits

	// only written if m_bStreamDepFlag
	UINT16   m_usDependsOnESID;

	// only written if m_bURLFlag
	//UCHAR    m_ucURLLength;      
	//UCHAR    m_URLString;

	// only written if m_bOCRStreamFlag
	UINT16   m_usOCR_ESID;

	DC_Descriptor_t m_DC_Descriptor;
    } ES_Descriptor_t;

    ES_Descriptor_t m_ES_Descriptor;
    
    // profilelevelindicationindexdescriptor 0 .. 255
    
    // slconfigdescriptor
    UCHAR    m_ucSLConfigTag;    // always 0x06
    UINT32   m_ulSLConfigLength; // class expansion
    UCHAR    m_ucPredefined;     // always 0x02

    // ipi_descrpointer   0 .. 1

    // ip_identificationdataset 0 .. 255

    // ipmp_descriptorpointer  0 .. 255

    // languagedescriptor   0 .. 255

    // qos_descriptor  0 .. 1

    // registrationdescriptor 0 .. 1

    // extensiondescriptor    0 .. 255
    UINT32 GetESDescriptorLen()
	{
	    UINT32 len = 2 + 1;
	    if( m_ES_Descriptor.m_bStreamDepFlag )
	    {
		len += 2;
	    }
	    if( m_ES_Descriptor.m_bOCRStreamFlag )
	    {
		len += 2;
	    }
	    return len;
	}
    UINT32 GetDCDescriptorLen()
	{
	    UINT32 len = 1 + 1 + 3 + 4 + 4;
	    if( m_ES_Descriptor.m_DC_Descriptor.m_uiDecoderSpecificInfoLength )
	    {
		len += 1 + 4 + m_ES_Descriptor.m_DC_Descriptor.m_uiDecoderSpecificInfoLength;
	    }
	    return len;
	}
    // This disgusting sequence is defined in 18.3.3 of the mp4 spec (14496-1)
    void ConvertLength( UINT32& len )
	{
	    UINT32 newlen = (len < 0x80 ? len : 0);
	    while( len >= 0x80 )
	    {
		newlen <<= 8;
		newlen |= 0xff;
		len -= 0x7f;
	    }
	    len = newlen | 0x80808000;
	}
};

// atom:      stsz
// container: stbl
// purpose:   sample size
class CMP4Atom_stsz : public CMP4VersionedAtom
{
public:
    CMP4Atom_stsz() : CMP4VersionedAtom(MP4_BUILD_ATOMID('s','t','s','z'))
	{
	    m_bSampleSizeChanged = FALSE;
	    m_uiSampleCount    = 0;
	    m_uiLastSampleSize = 0;
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + 4;
	    if( m_bSampleSizeChanged )
	    {
		len += (4 * m_uiSampleCount);
	    }
	    
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		if( !m_bSampleSizeChanged )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiLastSampleSize );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, (UINT32)0 );
		}
		else
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, (UINT32)0 );
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiSampleCount );

		    m_blEntrySizes.DumpToBuffer( pBuffer );
		    pBuffer += (4 * m_uiSampleCount);
		}
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    STDMETHOD( AddSampleSize )       (THIS_ UINT32 uiSampleSize)
	{
	    m_blEntrySizes[m_uiSampleCount] = (void*)uiSampleSize;
	    m_uiSampleCount++;

	    // Track sample size changes until we hit one
	    if( !m_bSampleSizeChanged )
	    {
		if( !m_uiLastSampleSize )
		{
		    m_uiLastSampleSize = uiSampleSize;
		}
		else if( m_uiLastSampleSize != uiSampleSize )
		{
		    m_bSampleSizeChanged = TRUE;
		}
	    }
	    return HXR_OK;
	}
    
private:
    UINT32  m_uiSampleCount;
    CBList  m_blEntrySizes;

    // XXX this is an optimization, we track sample sizes and if,
    //     at the end, they are all the same size, we can shorten this
    //     atom significantly
    BOOL    m_bSampleSizeChanged;
    UINT32  m_uiLastSampleSize;
};

// atom:      stz2
// container: stbl
// purpose:   compact sample size
class CMP4Atom_stz2 : public CMP4VersionedAtom
{
public:
    CMP4Atom_stz2() : CMP4VersionedAtom(MP4_BUILD_ATOMID('s','t','z','2'))
	{
	    m_uiSampleSize  = 0;
	    m_ucFieldSize   = 0;
	    m_uiSampleCount = 0;
	    m_uiEntrySizes  = NULL;
	}
    ~CMP4Atom_stz2()
	{
	    HX_VECTOR_DELETE( m_uiEntrySizes );
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + 4 + ((m_ucFieldSize * m_uiSampleCount) >> 3);
	    
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		UINT32 sizes = (m_uiSampleSize << 8) | m_ucFieldSize;
		CMP4Atom::WriteToBufferAndInc( pBuffer, sizes );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiSampleCount );

		// Pack the data
		switch( m_ucFieldSize )
		{
		    case 4:
		    {
			// XXX assumption; if an odd number of entries exist,
			//     the entrysizes array is still even in size, and the
			//     last byte is a 0
			for( UINT32 i = 0; i < m_uiSampleCount; i+= 2 )
			{
			    UCHAR val = (UCHAR) ((m_uiEntrySizes[i] | (m_uiEntrySizes[i+1] << 4)) & 0xff);
			    CMP4Atom::WriteToBufferAndInc( pBuffer, val );
			}
			break;
		    }
		    case 8:
		    {
			for( UINT32 i = 0; i < m_uiSampleCount; i++ )
			{
			    UCHAR val = (UCHAR) (m_uiEntrySizes[i] & 0xff);
			    CMP4Atom::WriteToBufferAndInc( pBuffer, val );
			}
			break;
		    }
		    case 16:
		    {
			for( UINT32 i = 0; i < m_uiSampleCount; i++ )
			{
			    UINT16 val = (UINT16) (m_uiEntrySizes[i] & 0xffff);
			    CMP4Atom::WriteToBufferAndInc( pBuffer, val );
			}
			break;
		    }
		    default:
		    {
			HX_ASSERT("This should never happen" && 0);
			break;
		    }
		};
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
private:
    UINT32  m_uiSampleSize;  // 24 bits, sits in the upper 3/4 of a word
    UCHAR   m_ucFieldSize;   // lower 1/4, in bits; 4, 8, or 16
    UINT32  m_uiSampleCount;
    
    UINT32* m_uiEntrySizes;  // regardless of the field size, store each in a UINT32
};

// atom:      stsc
// container: stbl
// purpose:   sample to chunk box
class CMP4Atom_stsc : public CMP4VersionedAtom
{
public:
    CMP4Atom_stsc() : CMP4VersionedAtom(MP4_BUILD_ATOMID('s','t','s','c'))
	{
	    m_uiEntryCount = 0;
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + (12 * m_uiEntryCount);
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiEntryCount );

		m_blEntries.DumpToBuffer( pBuffer );
		pBuffer += (12 * m_uiEntryCount);
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    STDMETHOD( AddNewEntry )        (THIS_ UINT32 uiFirstChunk, UINT32 uiSamplesPerChunk,
	                             UINT32 uiSampleDescriptionIndex )
	{
	    int idx = m_blEntries.GetSize();
	    m_blEntries[idx++]  = (void*) uiFirstChunk;
	    m_blEntries[idx++]  = (void*) uiSamplesPerChunk;
	    m_blEntries[idx]    = (void*) uiSampleDescriptionIndex;
	    m_uiEntryCount++;
	    
	    return HXR_OK;
	}

private:
    UINT32 m_uiEntryCount;
    
    // BList entries structured as follows
    //  UINT32 m_uiFirstChunk
    //  UINT32 m_uiSamplesPerChunk
    //  UINT32 m_uiSampleDescriptionIndex
    CBList m_blEntries;
};

// atom:      stco
// container: stbl
// purpose:   chunk offset box
class CMP4Atom_stco : public CMP4VersionedAtom
{
public:
    CMP4Atom_stco() : CMP4VersionedAtom(MP4_BUILD_ATOMID('s','t','c','o'))
	{
	    m_uiEntryCount = 0;
	}

    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + (4 * m_uiEntryCount);
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiEntryCount );

		m_blChunkOffsets.DumpToBuffer( pBuffer );
		pBuffer += (4 * m_uiEntryCount);
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    STDMETHOD( AddChunkOffset )    (THIS_ UINT32 uiChunkOffset)
	{
	    m_blChunkOffsets[ m_uiEntryCount++ ] = (void*)uiChunkOffset;
	    return HXR_OK;
	}
    
private:

    UINT32 m_uiEntryCount;
    CBList m_blChunkOffsets;
};

// atom:      co64
// container: stbl
// purpose:   64 bit chunk offset box
class CMP4Atom_co64 : public CMP4VersionedAtom
{
public:
    CMP4Atom_co64() : CMP4VersionedAtom(MP4_BUILD_ATOMID('c','o','6','4'))
	{
	    m_uiEntryCount = 0;
	    m_uiChunkOffsets = NULL;
	}
    ~CMP4Atom_co64()
	{
	    HX_VECTOR_DELETE( m_uiChunkOffsets );
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + (8 * m_uiEntryCount);
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiEntryCount );

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiChunkOffsets, m_uiEntryCount );
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
private:

    UINT32 m_uiEntryCount;
    UINT64* m_uiChunkOffsets;
};


// atom:      free
// container: file or moov
// purpose:   free space
class CMP4Atom_free : public CMP4Atom
{
public:
    CMP4Atom_free(UINT32 ulLength = 0) : CMP4Atom(MP4_BUILD_ATOMID('f','r','e','e'))
	{
	    m_uiLength = ulLength;
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    return 8 + m_uiLength;
	}
    // XXX i am not too sure about this one. In the normal case we probably
    // wont allocate more than 10-15k here, but this could potentially be
    // very large, in which case doing a huge allocation might not be cool
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren=FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		memset( pBuffer, 0, GetCurrentSize() );
		CMP4Atom::WriteToBufferAndInc( pBuffer, GetCurrentSize() );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_ulAtomID );
		pBuffer += m_uiLength;
		
		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}

    MP4_ATOM_PROPERTY(FreeSize, UINT32, m_uiLength );

private:
    UINT32 m_uiLength;
};

// atom:      udta
// container: moov or trak
// purpose:   user specified data
// note:      used for m4a metadata
class CMP4Atom_udta : public CMP4ContainerAtom
{
public:
    CMP4Atom_udta() : CMP4ContainerAtom(MP4_BUILD_ATOMID('u','d','t','a'))
	{}
};

// atom:      mdat
// container: file
// purpose:   contains the actual stream data
// note:      we dont write this like a typical atom, since there is no
//            way to retain the streeam data. instead, we have a length
//            field which indicates the total number of bytes we actually
//            wrote, and when queried for our size we give only our actual
//            atom size (non inclusive of the bytes we wrote)
class CMP4Atom_mdat : public CMP4ContainerAtom
{
public:
    CMP4Atom_mdat() : CMP4ContainerAtom(MP4_BUILD_ATOMID('m','d','a','t'))
	{
	    m_uiBytesWritten = 0;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer,
	                             BOOL bIncludeChildren= FALSE)
	{
	    HX_RESULT retVal = HXR_OK;
	    if( pBuffer )
	    {
		CMP4Atom::WriteToBufferAndInc( pBuffer, GetCurrentSize() + m_uiBytesWritten );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_ulAtomID );
	    }
	    // XXX we should never have children, so ignore bIncludeChildren
	    
	    return retVal;
	}
    
    MP4_ATOM_PROPERTY(BytesWritten, UINT32, m_uiBytesWritten);
private:
    UINT32 m_uiBytesWritten;
};

#endif /* _MP4ATOMS_H_ */
