/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: perplex.h,v 1.5 2005/03/14 19:36:41 bobclark Exp $
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

#ifndef _HXPERPLEX_H_
#define _HXPERPLEX_H_

//****************************************************************************
//****************************** INCLUDE FILES *******************************
//****************************************************************************

#include "hxtypes.h"
#include "hxstring.h"
#include "timerep.h"
#include "ihxperplex.h"
#include "unkimp.h"

//****************************************************************************
//************************** CONSTANT DEFINITIONS ****************************
//****************************************************************************

#define Perplex_BASE 		41
#define Perplex_PER_ULONG32	6
#define Perplex_ALIGNMENT	4


#define	DEFAULT_GROW_SIZE	1024	// for dynamic buffer perplex classes

//****************************************************************************
//************************** FORWARD DECLARATIONS ****************************
//****************************************************************************

class CHXPerplex;
class CHXSimpleBuffer; // crappy little buffer class used by CHXInfoEncoder



//****************************************************************************
//****************************** CHXPerplex **********************************
//****************************************************************************

class CHXPerplex :	public IHXPerplex, 
			public CUnknownIMP
{
    // the IUnknown implementation declaration
    DECLARE_UNKNOWN(CHXPerplex)

public:
    CHXPerplex();
    ~CHXPerplex();

    STDMETHOD(Perplex)	    (THIS_ IHXBuffer *pInBuf, IHXBuffer *pOutBuf);
    STDMETHOD(DePerplex)    (THIS_ IHXBuffer *pInBuf, IHXBuffer *pOutBuf);	
	

    // basic routines for hex/perplexing, used when this class doesn't
    // meet your needs.
    static void	    DumpToPerplex(char* Perplex, UINT32 ulPerplexSize, UCHAR* Bits, UINT32 nSize);
    static UINT32   SetFromPerplex(const char* Perplex, UCHAR* Bits, UINT32 nSize);

protected:

    static void	    DumpToMIMEBase64(char* MIMEBase64, const char* Bits, UINT32 nSize);
    static UINT32   SetFromMIMEBase64(const char* MIMEBase64, char* Bits, UINT32 nSize);

    static char	    MapToPerplex(UCHAR Perplex);
    static UCHAR    MapFromPerplex(char Perplex);

    static void	    ToPerplex(ULONG32 Input, char* Perplex);
    static ULONG32  FromPerplex(const char* Perplex);

    static char	    MapToMIMEBase64(UCHAR MIMEBase64);
    static UCHAR    MapFromMIMEBase64(char MIMEBase64);
};

//****************************************************************************
//**************************** CHXPerplexBuffer ******************************
//****************************************************************************

    // A simple buffer. There is no implicit access/bounds checking,
    // you have to do that yourself or use the 'Safexxxxx()' methods.
    // It's actual size is usually not what you set with resizeBuffer(),
    // since the size will be rounded up to the nearest 'GrowBy' size.

class CHXPerplexBuffer
{
public:

    CHXPerplexBuffer();
    CHXPerplexBuffer(UINT32 nSize, UINT32 nGrowBy=DEFAULT_GROW_SIZE);
    ~CHXPerplexBuffer();

    inline UCHAR*	GetPtr() 		    const { return(m_pData); }
    inline UCHAR*	GetPtrAt(UINT32 nOffset)    const { return(m_pData+nOffset); }
    inline UCHAR	GetDataAt(UINT32 nOffset)   const { return(*(m_pData+nOffset)); }
    inline UINT32	GetSize() 		    const { return(m_nSize); }

    inline void		SetUCHAR(UINT32 off, UCHAR  x)	{ *(m_pData+off)=x; }
    inline void		SetUINT16(UINT32 off, UINT16 x) { *((UINT16*)(m_pData+off))=x; }

    inline HXBOOL	IsValidOffset(UINT32 n) const;

    // make sure that Offset is within the buffer.
    // If not, the buffer is automatically resized.
    HXBOOL    EnsureValidOffset(UINT32 n);

    // copy data into buffer, dynamically growing buffer if needed.
    HXBOOL    SafeMemCopy(UINT32 nOffset, const void* data, UINT32 len);

    // no bounds checking here.
    inline void	MemCopy(UINT32 nOffset, const void* data, UINT32 len)
    { 
	memcpy(m_pData+nOffset, data, (int)len); /* Flawfinder: ignore */
    }

    // grow/shink buffer (does memcpy)
    HXBOOL	Resize(UINT32 nSize);
    void	Free();

protected:
    UINT32	RoundUpToGrowSize(UINT32 nSize);
    UINT32	m_nSize;
    UCHAR*	m_pData;
    UINT32	m_nGrowBy;
};

#endif
