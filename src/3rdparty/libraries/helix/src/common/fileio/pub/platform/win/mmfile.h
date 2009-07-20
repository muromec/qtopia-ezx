/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmfile.h,v 1.6 2007/07/06 20:35:22 jfinnecy Exp $
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

#ifndef __CWin32MMFile__
#define __CWin32MMFile__

#include "hlxclib/fcntl.h"

#include "unkimp.h"
#include "chxfgbuf.h"

class _CBufferWinMemMapped 
    : public IHXBuffer
    , public CUnknownIMP
{
    DECLARE_UNKNOWN(_CBufferWinMemMapped);
public:
    _CBufferWinMemMapped()
	: m_pData(NULL)
	, m_ulLength(0)
	, m_ulOffset(0)
    {}
    ~_CBufferWinMemMapped()
    {_UnMap();}

    //IHXBuffer
    //
    STDMETHOD(Get)
    (
	THIS_
	REF(UCHAR*)	    pData, 
	REF(ULONG32)    ulLength
    )
    {
	pData = m_pData+m_ulOffset; 
	ulLength = m_ulLength; 
	return HXR_OK;
    }

    STDMETHOD(Set)
    (
	THIS_
	const UCHAR*    pData, 
	ULONG32	    ulLength
    )
    {
	return E_NOTIMPL;
    }

    STDMETHOD(SetSize)
    (
	THIS_
	ULONG32	    ulLength
    )
    {
	return E_NOTIMPL;
    }

    STDMETHOD_(ULONG32,GetSize)
    (
	THIS
    )
    {
	return m_ulLength;
    }

    STDMETHOD_(UCHAR*,GetBuffer)
    (
	THIS
    )
    {
	return m_pData+m_ulOffset;
    }

private:
    static UINT32 _AllocGran();
    void _UnMap();
    HX_RESULT _SetMapping
    (
	HANDLE hfmoFile, 
	UINT32 ulOffsetHigh, 
	UINT32 ulOffsetLow , 
	UINT32 ulLength
    );

    friend class CWin32MMFile;

    UCHAR* m_pData;
    UINT32 m_ulLength;
    UINT32 m_ulOffset;
};

class CWin32MMFile : public CHXDataFile
{
public:
    CWin32MMFile(IUnknown* pContext)
	: CHXDataFile(pContext)
	, m_hfmoFile(0)
	, m_hFile(INVALID_HANDLE_VALUE)
	, m_ulPositionHigh(0)
	, m_ulPositionLow(0)
	, m_ulSizeHigh(0)
	, m_ulSizeLow(0)
    {
    }
    ~CWin32MMFile()
    {Close();}

/*  Create creates a file using the specified mode */
    virtual HX_RESULT	Create
    (
	const char *filename, 
	UINT16 mode, 
	HXBOOL textflag
    );

/*  Open will open a file with the specified permissions */
    virtual HX_RESULT	Open
    (
	const char *filename, 
	UINT16 mode, 
	HXBOOL textflag
    );

/*  Close closes a file */
    virtual HX_RESULT	Close		(void);
    
/*  Seek moves the current file position to the offset from the fromWhere 
    specifier */
    virtual HX_RESULT	Seek		(ULONG32 offset, UINT16 fromWhere);

/*  Tell returns the current file position in the ra file */
    virtual ULONG32	Tell		(void);

/*  Read reads up to count bytes of data into buf.
    returns the number of bytes read, EOF, or -1 if the read failed */
    virtual ULONG32	Read		
    (
	char* buf, 
	ULONG32 count
    );

/*  Read reads up to ulCount bytes of data into ppbufOut.
    returns a result code */
    virtual HX_RESULT	ReadToBuffer	
    (
	ULONG32 ulCount, 
	IHXBuffer** ppbufOut
    );

/*  Write writes up to count bytes of data from buf.
    returns the number of bytes written, or -1 if the write failed */
    virtual ULONG32	Write
    (
	const char *buf, 
	ULONG32 count
    );

/*  Rewinds the file to the start of the file */
    virtual HX_RESULT	Rewind		(void);

/*  Return the file descriptor                      */
    virtual INT16	GetFd		(void);

/*  Delete deletes a file */
    virtual HX_RESULT	Delete
    (
	const char *filename
    );

private:
    HANDLE m_hfmoFile;
    HANDLE m_hFile;
    UINT32 m_ulPositionHigh;
    UINT32 m_ulPositionLow;
    UINT32 m_ulSizeHigh;
    UINT32 m_ulSizeLow;
};

#endif //!__CWin32MMFile__
