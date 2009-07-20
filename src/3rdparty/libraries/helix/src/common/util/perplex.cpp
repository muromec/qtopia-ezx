/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: perplex.cpp,v 1.8 2005/03/14 19:36:39 bobclark Exp $
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

//*******************************************************************
//*************************** INCLUDE FILES *************************
//*******************************************************************

#include "perplex.h"
#include "hxassert.h"
#include "hlxclib/string.h"
// #include "hlxclib/stdio.h"
#include "netbyte.h"
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

//*******************************************************************
//************************* INTERFACE LISTING ***********************
//*******************************************************************

BEGIN_INTERFACE_LIST(CHXPerplex)
    INTERFACE_LIST_ENTRY(IID_IHXPerplex, IHXPerplex)
END_INTERFACE_LIST

//*******************************************************************
//*************************** CONSTRUCTOR ***************************
//*******************************************************************

CHXPerplex::CHXPerplex()
{
;
}

//*******************************************************************
//*************************** DESTRUCTOR ****************************
//*******************************************************************

CHXPerplex::~CHXPerplex()
{
  ;
}

//*******************************************************************
//*************************** Perplex() *****************************
//*******************************************************************

STDMETHODIMP CHXPerplex::Perplex(IHXBuffer *pInBuf, IHXBuffer *pOutBuf)
{
    UINT32 FinalLen;			    // length of encoded data
    CHXPerplexBuffer pInPBuf;		    // Temp buffer to keep code

    // copy the IRMA Input Buffer into the CHXPerplex Input Buffer
    pInPBuf.SafeMemCopy(0, (const void*) pInBuf->GetBuffer(), (UINT32)pInBuf->GetSize());

    // Make sure the input buffer size is a multiple of 4bytes as 
    // needed by Perplex alg.  Padding with '\0' otherwise.

    UINT32 nAlign = (pInBuf->GetSize()) % sizeof(ULONG32);
    UINT32 Offset = (UINT32)pInBuf->GetSize();
    if (nAlign > 0)
    {
	pInPBuf.EnsureValidOffset(Offset+sizeof(ULONG32)-nAlign);
	for (; nAlign < sizeof(ULONG32); nAlign++)
	{
	    pInPBuf.SetUCHAR(Offset++,0);
	}
    }

    FinalLen = Offset * Perplex_PER_ULONG32 / sizeof(ULONG32);	// calc size of the outout (Perplexed) buffer.

    // alloc mem for final perplexed buffer
    // Add one to length 'cause low level perplex adds
    // a '\0' to the resulting string

    pOutBuf->SetSize(FinalLen+1);

    if (pOutBuf->GetBuffer() == NULL) return(HXR_FAIL);
	
    CHXPerplex::DumpToPerplex((char*)(pOutBuf->GetBuffer()), FinalLen+1, pInPBuf.GetPtr(), Offset);

    return(HXR_OK);
}

//*******************************************************************
//*************************** DePerplex() ***************************
//*******************************************************************

//	Parameters:
//
//		const char* Perplex
//		Pointer to a buffer that contains a Perplexidecimal encoding 
//
//		UCHAR* Bits
//		Pointer to a buffer that will contain decoded bytes of information
//
//		int nSize
//		Input size in bytes

STDMETHODIMP CHXPerplex::DePerplex(IHXBuffer *pInBuf, IHXBuffer *pOutBuf)
{
    const char* pPerplex    = (const char *)pInBuf->GetBuffer();
    UCHAR* Bits		    = NULL;
    UINT32 nSize	    = pInBuf->GetSize();
    if (!nSize)
    {
	pOutBuf->SetSize(0);
	return HXR_OK;
    }

    
    nSize--; 	// -1 is for the extra character added in Perplex
    UINT32	ndxBits;
    UINT32	ndxPerplex = 0;
    ULONG32	temp32;
    UINT32      ulOutSize = 2*nSize+100;
    pOutBuf->SetSize(ulOutSize);	    // Make it bigger since the real size isn't known yet
    Bits = (UCHAR *)pOutBuf->GetBuffer();

    for (ndxBits = 0; ndxPerplex < nSize; )
    {
	temp32 = FromPerplex(&pPerplex[ndxPerplex]);
	ndxPerplex+=Perplex_PER_ULONG32;
	if (ndxBits+4 <= ulOutSize) memcpy(&Bits[ndxBits],&temp32,sizeof(temp32)); /* Flawfinder: ignore */
	ndxBits+=sizeof(temp32);
    }
    pOutBuf->SetSize(ndxBits);
    return HXR_OK;
}


//*******************************************************************
//************************* MapFromPerplex **************************
//*******************************************************************

// Converts appropriate characters for Perplex encoding into a ULONG32.

// A copy of this is kept in in pnmisc\pub\win\pnmisc16.h.  If you change 
// this (which is very unlikely), then be sure to change it there.  
static const char zPerplexChars[] =
	{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
	'E'
	};

UCHAR CHXPerplex::MapFromPerplex(char Perplex)
{
    UCHAR temp = sizeof(zPerplexChars)/sizeof(zPerplexChars[0]);
    for (UCHAR n = 0; n < temp; n++)
    {
	if (Perplex == zPerplexChars[n]) return n;
    }
    return 0;
}

//*******************************************************************
//*************************** MapToPerplex **************************
//*******************************************************************

//	Converts a digit ordinal to the Perplex digit...

char CHXPerplex::MapToPerplex(UCHAR cPerplex)
{                                                  
#ifdef _DEBUG
    int size_zPerplexChars  = sizeof(zPerplexChars); 
    int size_zPerplexChars0 = sizeof(zPerplexChars[0]);
    HX_ASSERT(cPerplex < size_zPerplexChars/size_zPerplexChars0);
#endif
    return zPerplexChars[cPerplex];
}

//*******************************************************************
//*************************** FromPerplex ***************************
//*******************************************************************

//	Converts appropriate characters for Perplex encoding into a ULONG32.

ULONG32 CHXPerplex::FromPerplex(const char* Perplex)
{
	ULONG32 value = 0;
	ULONG32 PerplexBase = 1;

	for (int n = 0; n < Perplex_PER_ULONG32; n++)
	{
		value += MapFromPerplex(Perplex[n]) * PerplexBase;
		PerplexBase *= Perplex_BASE;
	}

	// Convert to local byte order!
	value = DwToHost(value);

	return value;
}

//*******************************************************************
//**************************** ToPerplex ****************************
//*******************************************************************

//	Converts a ULONG32 into the appropriate characters for Perplex encoding.

void CHXPerplex::ToPerplex(ULONG32 Input, char* pPerplex)
{
    ULONG32 value;
    UCHAR   charValue;

    value = DwToNet(Input); // Convert to net byte order!

    for (int n=0; n < Perplex_PER_ULONG32; n++)
    {
    	charValue   = (UCHAR)(value % Perplex_BASE);
	pPerplex[n] = MapToPerplex(charValue);
	value = value / Perplex_BASE;
    }
}

//*******************************************************************
//*************************** DumpToPerplex *************************
//*******************************************************************

//	Parameters:
//
//		char* pPerplex
//		Pointer to buffer to be filled with Perplexadecimal 

void CHXPerplex::DumpToPerplex(char* pPerplex, UINT32 ulPerplexSize, UCHAR* Bits, UINT32 nSize)
{
    UINT32	ndxBits;
    UINT32	ndxPerplex = 0;
    ULONG32	temp32;

    for (ndxBits = 0; ndxBits < nSize; )
    {
	if (ndxBits+4 <= nSize) memcpy(&temp32,&Bits[ndxBits],sizeof(temp32)); /* Flawfinder: ignore */
	ndxBits+=sizeof(temp32);
	if (ndxPerplex+Perplex_PER_ULONG32 <= ulPerplexSize) ToPerplex(temp32,&pPerplex[ndxPerplex]);
	ndxPerplex+=Perplex_PER_ULONG32;
    }
    pPerplex[ndxPerplex] = '\0';
}


//*******************************************************************
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*****                     CHXPerplexBuffer                   ******
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*****                                                        ******
//*******************************************************************

//*******************************************************************
//****************** CHXPerplexBuffer CONSTRUCTOR *******************
//*******************************************************************

CHXPerplexBuffer::CHXPerplexBuffer()
	:  m_nSize(0), m_pData(NULL), m_nGrowBy(DEFAULT_GROW_SIZE)
{
}

//*******************************************************************
//****************** CHXPerplexBuffer CONSTRUCTOR *******************
//*******************************************************************

CHXPerplexBuffer::CHXPerplexBuffer(UINT32 nSize, UINT32 nGrowBy)
	:  m_nSize(0), m_pData(NULL), m_nGrowBy(nGrowBy)
{
	Resize(nSize);
}

//*******************************************************************
//******************* CHXPerplexBuffer DESTRUCTOR *******************
//*******************************************************************

CHXPerplexBuffer::~CHXPerplexBuffer()
{
	Free();
}

//*******************************************************************
//****************** CHXPerplexBuffer::IsValidOffset() **************
//*******************************************************************

inline HXBOOL CHXPerplexBuffer::IsValidOffset(UINT32 n) const
{
	if (n<m_nSize)
		return(TRUE);
	else return(FALSE);
}


//*******************************************************************
//*************** CHXPerplexBuffer::EnsureValidOffset() *************
//*******************************************************************

// make sure that pPos is in the buffer.
// If not, the buffer is automatically resized and contents copied.
// pPos must be > pData.

HXBOOL CHXPerplexBuffer::EnsureValidOffset(UINT32 n)
{
    if (IsValidOffset(n)==TRUE)
    {
	return(TRUE);
    }

    return (this->Resize(n));
}


//*******************************************************************
//********************* CHXPerplexBuffer::Resize() ******************
//*******************************************************************

HXBOOL CHXPerplexBuffer::Resize(UINT32 nNewSize)
{
    if (nNewSize==0)
    {
	Free();
	return(TRUE);
    }

    nNewSize = RoundUpToGrowSize(nNewSize);

    UCHAR*	pNewBuffer = new UCHAR[nNewSize];
    if (pNewBuffer==NULL) return(FALSE);


    if (m_pData!=NULL)
    {
	// copy MIN(oldSize,newSize) elements
	memcpy(pNewBuffer,m_pData, (nNewSize<m_nSize) ? (int)nNewSize : (int)m_nSize); /* Flawfinder: ignore */
	delete [] m_pData;
    }

    m_pData = pNewBuffer;
    m_nSize = nNewSize;

    return(TRUE);
}

//*******************************************************************
//**************** CHXPerplexBuffer::RoundUpToGrowSize() ************
//*******************************************************************

UINT32 CHXPerplexBuffer::RoundUpToGrowSize(UINT32 nSize)
{
    // check for div-by-zero errors (as long as DEFAULT!=0)
    if (m_nGrowBy==0)
    {
	m_nGrowBy = DEFAULT_GROW_SIZE;
    }

    return ( (int)(nSize/m_nGrowBy)+1 ) * m_nGrowBy;
}

//*******************************************************************
//****************** CHXPerplexBuffer::SafeMemCopy() ****************
//*******************************************************************

// copy data into the buffer at the given offset, and if the buffer
// is too small we'll resize the buffer first.

HXBOOL CHXPerplexBuffer::SafeMemCopy(UINT32 nOffset, const void* data, UINT32 len)
{
    if (EnsureValidOffset(nOffset+len-1)==TRUE)
    {
    	memcpy( m_pData+nOffset, data, (int)len ); /* Flawfinder: ignore */
	return(TRUE);
    }
    return(FALSE);
}



//*******************************************************************
//********************** CHXPerplexBuffer::Free() *******************
//*******************************************************************

void CHXPerplexBuffer::Free()
{
    if (m_pData!=NULL)
    delete [] m_pData;

    m_pData=NULL;
    m_nSize=0;
}

//*******************************************************************
//************************** FourByteAlign **************************
//*******************************************************************
/*  already in pnmisc.lib
UINT16 	FourByteAlign(UINT16 number)
{
	return( ((number+3)>>2)<<2 );
}
*/

//*******************************************************************
//************************ MapFromMIMEBase64 ************************
//*******************************************************************

//  Converts appropriate characters for MIME-Base64 encoding into a the
//  Base64 ordinal.

// A copy of this is kept in in pnmisc\pub\win\pnmisc16.h.  If you change 
// this (which is very unlikely), then be sure to change it there.  
static const char zMIMEBase64Chars[] =
	{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
	'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '+', '/'
	};						

#define MIME_BASE64_PADDING		'='
#define FROM_MIME_BASE64_BYTES_IN	4
#define FROM_MIME_BASE64_BYTES_OUT	3
#define TO_MIME_BASE64_BYTES_IN		FROM_MIME_BASE64_BYTES_OUT
#define TO_MIME_BASE64_BYTES_OUT	FROM_MIME_BASE64_BYTES_IN

UCHAR CHXPerplex::MapFromMIMEBase64(char MIMEBase64)
{
    for (UCHAR n = 0; n < sizeof(zMIMEBase64Chars)/sizeof(zMIMEBase64Chars[0]); n++)
    {
	if (MIMEBase64 == zMIMEBase64Chars[n])
	{
	    return n;
	}
    }
    HX_ASSERT(FALSE);
    return 0;
}

//*******************************************************************
//************************ MapFromMIMEBase64 ************************
//*******************************************************************

// Converts a digit ordinal to the MIMEBase64 digit...

char CHXPerplex::MapToMIMEBase64(UCHAR MIMEBase64)
{
    HX_ASSERT(MIMEBase64 < sizeof(zMIMEBase64Chars)/sizeof(zMIMEBase64Chars[0]));
    return zMIMEBase64Chars[MIMEBase64];
}

//*******************************************************************
//************************ SetFromMIMEBase64 ************************
//*******************************************************************

//	Return:	int	Size in bytes of decoded information

UINT32 CHXPerplex::SetFromMIMEBase64(const char* MIMEBase64, char* Bits, UINT32 nSize)
{
    UINT32	ndxBits		= 0;
    UINT32	ndxMIMEBase64	= 0;
    HXBOOL	bDone		= FALSE;
    UINT32	ndxTempIn	= 0;
    UCHAR	tempBitsIn[FROM_MIME_BASE64_BYTES_IN];
    UINT32	padding		= 0;

    HX_ASSERT(strlen(MIMEBase64) <= nSize);

    while (!bDone)
    {
	HX_ASSERT(ndxMIMEBase64 <= nSize);
	HX_ASSERT(ndxBits <= nSize);

	// Fill in our "temporary" in buffer with the Base64 characters.
	for (ndxTempIn = 0;
	     ndxTempIn < FROM_MIME_BASE64_BYTES_IN && (padding == 0);
	     ndxTempIn++)
	{
	    HX_ASSERT(ndxMIMEBase64 <= nSize);
	    UCHAR MIMEBase64char = MIMEBase64[ndxMIMEBase64];

	    switch (MIMEBase64char)
	    {
		case MIME_BASE64_PADDING:
		case '\0':
		{
		    tempBitsIn[ndxTempIn] = 0;
		    bDone = TRUE;
		    padding = (FROM_MIME_BASE64_BYTES_IN - ndxTempIn);
		}
		break;

		default:
		{
		    tempBitsIn[ndxTempIn] = MapFromMIMEBase64(MIMEBase64char);
		}
		break;
	    }
	    ndxMIMEBase64++;
	}

	HX_ASSERT(padding == 2 || padding == 1 || padding == 0);

	// Map the Base64 in buffer to the the output buffer...
	// This should map the 6 pertinate bits of each IN byte, to the
	// the correct bits of the OUT byte.
	{
	    Bits[ndxBits] = (tempBitsIn[0] << 2) + (tempBitsIn[1]>>4);
	    ndxBits++;

	    if (padding < 2)
	    {
		Bits[ndxBits] = (tempBitsIn[1] << 4) + (tempBitsIn[2]>>2);
		ndxBits++;
	    }

	    if (padding < 1)
	    {
		Bits[ndxBits] = (tempBitsIn[2] << 6) + (tempBitsIn[3]);
		ndxBits++;
	    }
	}
    }

    Bits[ndxBits] = '\0';
    return ndxBits;
}

//*******************************************************************
//************************ DumpToMIMEBase64 *************************
//*******************************************************************

void CHXPerplex::DumpToMIMEBase64(char* MIMEBase64, const char* Bits, UINT32 nSize)
{
    UINT32	ndxBits			= 0;
    UINT32	ndxMIMEBase64	= 0;
    HXBOOL			bDone			= FALSE;
    UINT32	ndxTempIn		= 0;
    UINT32	ndxTempOut		= 0;
    UCHAR			tempBitsIn [FROM_MIME_BASE64_BYTES_IN];
    UINT32	padding			= 0;

    HX_ASSERT(strlen(Bits) <= nSize);

    while (!bDone)
    {
	HX_ASSERT(ndxMIMEBase64 <= nSize);
	HX_ASSERT(ndxBits <= nSize);

	// Fill in our "temporary" out buffer with the 6 bit chunks of the input.
	for (	ndxTempIn = 0;
		ndxTempIn < TO_MIME_BASE64_BYTES_IN && (padding == 0);
		ndxTempIn++)
	{
	    UCHAR rawChar = Bits[ndxBits];

	    if (rawChar != '\0')
	    {
		switch (ndxTempIn)
		{
		    case 0:
		    {
			tempBitsIn[0] = rawChar >> 2;
			tempBitsIn[1] = (rawChar & 0x3) << 4;
		    }
		    break;

		    case 1:
		    {
			tempBitsIn[1] += rawChar >> 4;
			tempBitsIn[2] = (rawChar & 0xF) << 2;
		    }
		    break;

		    case 2:
		    {
			tempBitsIn[2] += rawChar >> 6;
			tempBitsIn[3] = (rawChar & 0x3F);
		    }
		    break;
		}
	    }
	    else
	    {
		bDone = TRUE;
		padding = (TO_MIME_BASE64_BYTES_IN - ndxTempIn);
	    }
	    ndxBits++;
	}

	HX_ASSERT(padding == 2 || padding == 1 || padding == 0);

	// Map the Base64 in buffer to the output buffer...
	// This should map the 6 pertinate bits of each IN byte, as an
	// entire OUT byte
	for (	ndxTempOut = 0;
		ndxTempOut < TO_MIME_BASE64_BYTES_OUT;
		ndxTempOut++)
	{
	    if (ndxTempOut < (TO_MIME_BASE64_BYTES_OUT-padding))
	    {
		MIMEBase64[ndxMIMEBase64] = MapToMIMEBase64(tempBitsIn[ndxTempOut]);
	    }
	    else
	    {
		MIMEBase64[ndxMIMEBase64] = MIME_BASE64_PADDING;
	    }
	    ndxMIMEBase64++;
	}
    }
    MIMEBase64[ndxMIMEBase64] = '\0';
}
