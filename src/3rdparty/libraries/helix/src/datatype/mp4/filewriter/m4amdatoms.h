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
// m4amdatoms.h: atoms specific to m4a metadata

#ifndef _M4AMDATOMS_H_
#define _M4AMDATOMS_H_

#include "mp4atoms.h"

// meta; this is the top level atom inside the udta, and all
//       metadata atoms live inside it
class CM4AAtom_meta : public CMP4VersionedAtom
{
public:
    CM4AAtom_meta() : CMP4VersionedAtom(MP4_BUILD_ATOMID('m','e','t','a'))
	{}
    STDMETHOD( WriteToBuffer )    (THIS_ UCHAR*& pBuffer, BOOL bIncludeChildren = FALSE )
	{
	    HX_RESULT retVal = HXR_OK;
	    
	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
};

class CM4AAtom_ilst : public CMP4ContainerAtom
{
public:
    CM4AAtom_ilst() : CMP4ContainerAtom(MP4_BUILD_ATOMID('i','l','s','t'))
	{}
};

class CM4AAtom_hdlr : public CMP4VersionedAtom
{
public:
    CM4AAtom_hdlr() : CMP4VersionedAtom(MP4_BUILD_ATOMID('h','d','l','r'))
	{
	    m_uiHdlrType = 0;
	    m_uiHdlrSubType = MP4_BUILD_ATOMID('m','d','i','r');
	    m_uiHdlrManufacturer = MP4_BUILD_ATOMID('a','p','p','l');
	    m_uiComponentFlags = 0;
	    m_uiComponentFlagsMask = 0;
	    m_pszName = NULL;
	    m_uiNameLength = 2;
	}
    ~CM4AAtom_hdlr()
	{
	    HX_VECTOR_DELETE( m_pszName );
	}
    STDMETHOD_( UINT32, GetCurrentSize )            ( THIS_ BOOL bIncludeChildren = FALSE )
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize( bIncludeChildren )
			 + 4 + 4 + 4 + 4 + 4;
	    len += m_uiNameLength;
	    return len;
	}
    STDMETHOD( WriteToBuffer )                 (THIS_ UCHAR*& pBuffer, BOOL bIncludeChildren = FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );
		
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiHdlrType );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiHdlrSubType );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiHdlrManufacturer );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiComponentFlags );
		CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiComponentFlagsMask );

		if( m_pszName )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_pszName, m_uiNameLength );
		} else
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, (UCHAR*)"\0Z", 2 );
		}
		

		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}
    
private:
    UINT32 m_uiHdlrType;
    UINT32 m_uiHdlrSubType;
    UINT32 m_uiHdlrManufacturer;
    UINT32 m_uiComponentFlags;
    UINT32 m_uiComponentFlagsMask;
    UCHAR* m_pszName;
    UINT32 m_uiNameLength;
};

class CM4AAtom_data : public CMP4VersionedAtom
{
public:
    CM4AAtom_data() : CMP4VersionedAtom(MP4_BUILD_ATOMID('d','a','t','a'))
	{
	    m_reserved = 0;
	    m_ucBinStrBuf = NULL;
	    m_uiIntBuf = NULL;
	    m_uiDataLength = 0;
	}
    ~CM4AAtom_data()
	{
	    HX_VECTOR_DELETE( m_ucBinStrBuf );
	    HX_VECTOR_DELETE( m_uiIntBuf );
	}
    STDMETHOD_(UINT32, GetCurrentSize )     (THIS_ BOOL bIncludeChildren = FALSE)
	{
	    UINT32 len = CMP4VersionedAtom::GetCurrentSize(bIncludeChildren)
			 + 4 + m_uiDataLength;
	    return len;
	}
    STDMETHOD( WriteToBuffer )      (THIS_ UCHAR*& pBuffer, BOOL bIncludeChildren = FALSE)
	{
	    HX_RESULT retVal = HXR_OK;

	    if( pBuffer )
	    {
		CMP4VersionedAtom::WriteAtomAndVersionInfo( pBuffer );

		CMP4Atom::WriteToBufferAndInc( pBuffer, m_reserved );

		if( m_uiFlags == _DataFlags::INT )
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_uiIntBuf, m_uiDataLength );
		}
		else
		{
		    CMP4Atom::WriteToBufferAndInc( pBuffer, m_ucBinStrBuf, m_uiDataLength );
		}

		if( bIncludeChildren )
		{
		    retVal = ChildrenWriteToBuffer( pBuffer );
		}
	    }
	    return retVal;
	}

    /*
     * Local routines
     */
    STDMETHOD( SetString )          (THIS_ const UCHAR* pszString)
	{
	    HX_RESULT retVal = HXR_INVALID_PARAMETER;
	    
	    if( pszString )
	    {
		UINT32 len = strlen((const char*)pszString);
		m_ucBinStrBuf = new UCHAR[ len ];
		retVal = HXR_OUTOFMEMORY;
		
		if( m_ucBinStrBuf )
		{
		    memcpy( m_ucBinStrBuf, pszString, len );
		    m_uiDataLength = len;
		    CMP4VersionedAtom::m_uiFlags = _DataFlags::STRING;
		    
		    retVal = HXR_OK;
		}
	    }
	    return retVal;
	}
    STDMETHOD( SetBinary )          (THIS_ const UCHAR* pBinary, UINT32 len )
	{
	    HX_RESULT retVal = HXR_INVALID_PARAMETER;
	    
	    if( pBinary )
	    {
		m_ucBinStrBuf = new UCHAR[ len ];
		retVal = HXR_OUTOFMEMORY;
		
		if( m_ucBinStrBuf )
		{
		    memcpy( m_ucBinStrBuf, pBinary, len );
		    CMP4VersionedAtom::m_uiFlags = _DataFlags::BINARY;
		    retVal = HXR_OK;
		}
	    }
	    return retVal;
	}
    STDMETHOD( SetInt )             (THIS_ const UINT32* pInt, UINT32 len )
	{
	    HX_RESULT retVal = HXR_INVALID_PARAMETER;
	    
	    if( pInt )
	    {
		m_uiIntBuf = new UINT32[ len ];
		retVal = HXR_OUTOFMEMORY;
		
		if( m_uiIntBuf )
		{
		    memcpy( m_uiIntBuf, pInt, len << 2 );
		    CMP4VersionedAtom::m_uiFlags = _DataFlags::INT;
		    retVal = HXR_OK;
		}
	    }
	    return retVal;
	}
    STDMETHOD( SetByte )            (THIS_ const UCHAR* pByte, UINT32 len )
	{
	    HX_RESULT retVal = SetBinary( pByte, len );
	    if( SUCCEEDED( retVal ) )
	    {
		CMP4VersionedAtom::m_uiFlags = _DataFlags::BYTE;
	    }
	    return retVal;
	}

protected:
    // XXX Probably not the best naming ever. The distinctions here
    //     aren't very clear
    enum _DataFlags
    {
	INT=0x00,
	STRING=0x01,
	BINARY=0x0D,
	BYTE=0x15,
    };
private:
    UINT32 m_reserved;
    UCHAR* m_ucBinStrBuf;  // used for STRING, BINARY, BYTE
    UINT32* m_uiIntBuf;    // used for INT
    UINT32 m_uiDataLength;
};

// track name
// string
class CM4AAtom_nam : public CMP4ContainerAtom
{
public:
    CM4AAtom_nam() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xA9,'n','a','m'))
	{}
};

// track artist
// string
class CM4AAtom_art : public CMP4ContainerAtom
{
public:
    CM4AAtom_art() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xA9,'A','R','T'))
	{}
};

// album name
// string
class CM4AAtom_alb : public CMP4ContainerAtom
{
public:
    CM4AAtom_alb() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xA9,'a','l','b'))
	{}
};

// genre name
// string
class CM4AAtom_gen : public CMP4ContainerAtom
{
public:
    CM4AAtom_gen() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xA9,'g','e','n'))
	{}
};

// track number
// int array, 1-2 items
class CM4AAtom_trkn: public CMP4ContainerAtom
{
public:
    CM4AAtom_trkn() : CMP4ContainerAtom(MP4_BUILD_ATOMID('t','r','k','n'))
	{}
};

// disk number
// int array, 1-2 items
class CM4AAtom_disk : public CMP4ContainerAtom
{
public:
    CM4AAtom_disk() : CMP4ContainerAtom(MP4_BUILD_ATOMID('d','i','s','k'))
	{}
};

// day (year)
// string
class CM4AAtom_day : public CMP4ContainerAtom
{
public:
    CM4AAtom_day() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xA9,'d','a','y'))
	{}
};

// comment
// string
class CM4AAtom_cmt : public CMP4ContainerAtom
{
public:
    CM4AAtom_cmt() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xA9,'c','m','t'))
	{}
};

// composer
// string
class CM4AAtom_wrt : public CMP4ContainerAtom
{
public:
    CM4AAtom_wrt() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xA9,'w','r','t'))
	{}
};

// cover art
// binary
class CM4AAtom_covr : public CMP4ContainerAtom
{
public:
    CM4AAtom_covr() : CMP4ContainerAtom(MP4_BUILD_ATOMID('c','o','v','r'))
	{}
};

// tempo
// byte (2)
class CM4AAtom_tmpo : public CMP4ContainerAtom
{
public:
    CM4AAtom_tmpo() : CMP4ContainerAtom(MP4_BUILD_ATOMID('t','m','p','o'))
	{}
};

// compilation
// byte (1)
class CM4AAtom_cmpl : public CMP4ContainerAtom
{
public:
    CM4AAtom_cmpl() : CMP4ContainerAtom(MP4_BUILD_ATOMID('c','m','p','l'))
	{}
};

// tools
// string
class CM4AAtom_too : public CMP4ContainerAtom
{
public:
    CM4AAtom_too() : CMP4ContainerAtom(MP4_BUILD_ATOMID(0xa9,'t','o','o'))
	{}
};

#endif  /* _M4AMDATOMS_H_ */
