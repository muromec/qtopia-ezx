/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxinfcod.cpp,v 1.9 2005/04/28 23:40:17 ehyche Exp $
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

/****************************************************************************
 *
 * This file contains implementation of classes that support
 * the encoding of raw bytes of upgrade negotiation messages
 * into a clear text representaion of the raw data.
 *
 */

#include "hxinfcod.h"
#include "hxassert.h"
#include "hlxclib/string.h"
//#include "hlxclib/stdio.h"
#include "netbyte.h"
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////
UINT16 FourByteAlign(UINT16 number)
{
	return( ((number+3)>>2)<<2 );
}


//*******************************************************************
// CHXInfoEncoder
//*******************************************************************

// IUnknown interface listing
BEGIN_INTERFACE_LIST(CHXInfoEncoder)
    INTERFACE_LIST_ENTRY(IID_IHXObjOutStream, IHXObjOutStream)
END_INTERFACE_LIST

/////////////////////////////////////////////////////////////////////
CHXInfoEncoder::CHXInfoEncoder()
{
	m_FinalBuffer = NULL;
	m_bFinalBufferAllocFlag = FALSE;
	m_nFinalLen = 0;

	m_nOffset = 0;
}

/////////////////////////////////////////////////////////////////////
// get ptr to encoded data - either the data buffer that's been Dump()'d
// to, or a secondary buffer where data was further encoded (ie,
// Hex'd or perplex'd).
STDMETHODIMP_(const char*) CHXInfoEncoder::GetBuffer()
{
	if (m_FinalBuffer)
		return((char*)m_FinalBuffer);

	return((char*)m_Buffer.GetPtr());
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::GetLength()
{
	if (m_FinalBuffer)
		return(m_nFinalLen);

	return(m_nOffset);
}


/////////////////////////////////////////////////////////////////////
CHXInfoEncoder::~CHXInfoEncoder()
{
	if ( (m_bFinalBufferAllocFlag==TRUE) && (m_FinalBuffer!=NULL) )
    {
		delete [] m_FinalBuffer;
    }
}


/////////////////////////////////////////////////////////////////////
void CHXInfoEncoder::EncodeObj(IHXStreamableObj* pObj)
{
	this->Initialize();
	this->WriteObj(pObj);
}


/////////////////////////////////////////////////////////////////////
void CHXInfoEncoder::PerplexEncodeObj(IHXStreamableObj* pObj)
{
	this->Initialize();
	this->WriteObj(pObj);
	this->Perplex();
}

/////////////////////////////////////////////////////////////////////
HXBOOL CHXInfoEncoder::Perplex()
{
	// no data to perplex?
	if (m_nOffset==0) return(FALSE);

	// if final buffer already exists, return error.....
	// That means we already did an encoding, and the user should
	// call Initialize() before the encoder is reused.
	if (m_FinalBuffer!=NULL) return(FALSE);

	// Make sure we're 4byte aligned. Needed by Perplex alg.
    UINT32 nAlign =  (m_nOffset) % sizeof(ULONG32);

    if (nAlign > 0)
    {
		m_Buffer.EnsureValidOffset(m_nOffset+sizeof(ULONG32)-nAlign);
		for (; nAlign < sizeof(ULONG32); nAlign++)
		{
			m_Buffer.SetUCHAR(m_nOffset++,0);
		}
    }

	// calc size of Perplexed buffer.
    m_nFinalLen = m_nOffset * Perplex_PER_ULONG32 / sizeof(ULONG32);

	// alloc mem for final perplexed buffer
	// Add one to length 'cause low level perplex adds
	// a '\0' to the resulting string
	m_bFinalBufferAllocFlag = TRUE;
	m_FinalBuffer = new UCHAR[m_nFinalLen+1];
	HX_ASSERT(m_FinalBuffer!=NULL);

    if( m_FinalBuffer==NULL)
    {
		m_nFinalLen=0;
		return(FALSE);
    }
	else
    {
        CHXInfoEncoder::DumpToPerplex((char*)m_FinalBuffer, m_nFinalLen+1, m_Buffer.GetPtr(), m_nOffset);
    }

	return(TRUE);
}


/////////////////////////////////////////////////////////////////////
void CHXInfoEncoder::HexEncodeObj(IHXStreamableObj* pObj)
{
	this->Initialize();
	this->WriteObj(pObj);
	this->Hex();
}

/////////////////////////////////////////////////////////////////////
HXBOOL CHXInfoEncoder::Hex()
{
	// no data to perplex?
	if (m_nOffset==0) return(FALSE);

	// if final buffer already exists, return error.....
	// That means we already did an encoding, and the user should
	// call Initialize() before the encoder is reused.
	if (m_FinalBuffer!=NULL) return(FALSE);

	// calc size of hexed buffer.
    m_nFinalLen = m_nOffset * 2;

	// alloc mem for final perplexed buffer
	m_bFinalBufferAllocFlag = TRUE;
	// Add one to length 'cause low level perplex adds
	// a '\0' to the resulting string
	m_FinalBuffer = new UCHAR[m_nFinalLen+1];
	HX_ASSERT(m_FinalBuffer!=NULL);

    if( m_FinalBuffer==NULL)
    {
		m_nFinalLen=0;
		return(FALSE);
    }
	else
    {
        CHXInfoEncoder::DumpToHex((char*)m_FinalBuffer, m_Buffer.GetPtr(), m_nOffset);
    }

	return(TRUE);
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CHXInfoEncoder::Initialize()
{
	if (m_bFinalBufferAllocFlag==TRUE)
    {
		if (m_FinalBuffer!=NULL)
				delete [] m_FinalBuffer;
    }
	m_bFinalBufferAllocFlag = FALSE;
	m_FinalBuffer = NULL;
	m_nFinalLen    = 0;

	m_Buffer.Free();
	m_nOffset=0;
	
	return(HXR_OK);
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteObj(IHXStreamableObj* pObj) 
{
	UINT32 nLen=this->GetOffset();

	if (pObj->WriteObjToBits(this)==HXR_OK)
		return this->GetOffset()-nLen;

#if 0		
	// reading object
	this->Seek(nLen);
#endif
	return(0);
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteUCHAR(UCHAR nValue)
{
    m_Buffer.SafeMemCopy(m_nOffset,&nValue, sizeof(nValue));
	m_nOffset += sizeof(nValue);
    return sizeof(nValue);
}



/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteUINT16(UINT16 nValue)
{
    UINT16 temp16 = WToNet(nValue);

    m_Buffer.SafeMemCopy(m_nOffset,&temp16, sizeof(temp16));

	m_nOffset += sizeof(temp16);
    return sizeof(temp16);
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteUINT32(UINT32 nValue)
{
    UINT32 temp32 = DwToNet(nValue);

	m_Buffer.SafeMemCopy(m_nOffset, &temp32, sizeof(temp32));

	m_nOffset += sizeof(temp32);
    return sizeof(temp32);
}

/////////////////////////////////////////////////////////////////////
// Dump at a specific position. Doesn't advance offset pointer
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteUINT16At(UINT32 nOffset, UINT16 nValue)
{
    UINT16 temp16 = WToNet(nValue);

	if (m_Buffer.IsValidOffset(nOffset+sizeof(temp16))==TRUE)
    {
    	m_Buffer.MemCopy(nOffset,&temp16, sizeof(temp16));
    }

    return sizeof(temp16);
}

/////////////////////////////////////////////////////////////////////
// Dump at a specific position. Doesn't advance offset pointer
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteUINT32At(UINT32 nOffset, UINT32 nValue)
{
    UINT32 temp32 = DwToNet(nValue);

	if (m_Buffer.IsValidOffset(nOffset+sizeof(temp32))==TRUE)
    {
		m_Buffer.MemCopy(nOffset, &temp32, sizeof(temp32));
    }

    return sizeof(temp32);
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteString(const char* szValue)
{
	if (szValue==NULL)
    {
		szValue="\0";
    }

    UINT32 FieldLen = strlen(szValue);
    HX_ASSERT(FieldLen < 0xFF);

	// ensure the buffer has enough room for us.
	if (m_Buffer.EnsureValidOffset(m_nOffset+FieldLen+1)==TRUE)
    {
		m_Buffer.SetUCHAR(m_nOffset, (UCHAR)FieldLen );
        m_Buffer.MemCopy(m_nOffset + 1, szValue, FieldLen);
    }

    FieldLen ++;	//account for first SIZE byte

	m_nOffset += FieldLen;
    return FieldLen;
}


/////////////////////////////////////////////////////////////////////
// Similar to DumpString, except this one is handy when you need
// to concat 2 or 3 strings and Dump them. Instead of allocating
// a buffer, concating the strings and calling DumpString, use
// this routine instead. It will take 2 or 3 strings as a parameters,
// and dump them concatenated. The original strings are not modified.
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteStringCat(const char* szValue1,const char* szValue2,const char* szValue3)
{
	if (szValue1==NULL) { szValue1="\0"; }
	if (szValue2==NULL) { szValue2="\0"; }
	if (szValue3==NULL) { szValue3="\0"; }

    UINT16 nLen1=strlen(szValue1);
    UINT16 nLen2=strlen(szValue2);
    UINT16 nLen3=strlen(szValue3);

    UINT32 FieldLen = nLen1 + nLen2 + nLen3;

    HX_ASSERT(FieldLen < 0xFF);

	// ensure the buffer has enough room for us.
	if (m_Buffer.EnsureValidOffset(m_nOffset+FieldLen+1)==TRUE)
    {
		m_Buffer.SetUCHAR(m_nOffset, (UCHAR)FieldLen );
		m_nOffset++;

		if (nLen1)
        {
	        m_Buffer.MemCopy(m_nOffset, szValue1, nLen1);
			m_nOffset+=nLen1;
        }

		if (nLen2)
        {
	        m_Buffer.MemCopy(m_nOffset, szValue2, nLen2);
			m_nOffset+=nLen2;
        }

		if (nLen3)
        {
	        m_Buffer.MemCopy(m_nOffset, szValue3, nLen3);
			m_nOffset+=nLen3;
        }
    }

    FieldLen ++;	//account for first SIZE byte
    return FieldLen;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteLargeString(const char* szValue)
{
	if (szValue==NULL)
    {
		szValue="\0";
    }

	// get the length of the string
    UINT32 FieldLen = strlen(szValue);
    HX_ASSERT(FieldLen < 0xFFFF);

	// dump the size (this increments m_nOffset for us too)
	UINT32 nSize = WriteUINT16((UINT16)FieldLen);

    m_Buffer.SafeMemCopy(m_nOffset, szValue, FieldLen);
	m_nOffset += FieldLen;

    return (FieldLen+nSize);
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteBuffer(const char* szBuffer, UINT32 nSize)
{
	HX_ASSERT(szBuffer!=NULL);

	if (szBuffer!=NULL)
    {
    	m_Buffer.SafeMemCopy(m_nOffset, szBuffer, nSize);
    }

	m_nOffset += nSize;
    return (nSize);
}

#if 0
/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoEncoder::WriteUTCTime(UTCTimeRep& TimeRep)
{
	return(this->WriteUINT32( TimeRep.asUTCTimeT() ));

//    return this->WriteString(( TimeRep.asUTCString() );
}
#endif


//*******************************************************************
// CHXInfoDecoder
//*******************************************************************

// IUnknown interface listing
BEGIN_INTERFACE_LIST(CHXInfoDecoder)
    INTERFACE_LIST_ENTRY(IID_IHXObjInStream, IHXObjInStream)
END_INTERFACE_LIST

/////////////////////////////////////////////////////////////////////


HXBOOL  CHXInfoDecoder::DecodeObj(const UCHAR* buffer, UINT32 nSize, IHXStreamableObj* pObj)
{
	HX_ASSERT(buffer!=NULL);
	if (buffer==NULL)
    {
		return(FALSE);
    }

	this->Initialize();

	// Casting away the const, so we must promise not to mess with the
	// contents...
	m_Buffer = (UCHAR *)buffer;
	m_nDecodedLen = nSize;
	
	// ReadObj returns 0 on failure
	return (this->ReadObj(pObj)>0);
}

/////////////////////////////////////////////////////////////////////
HXBOOL  CHXInfoDecoder::PerplexDecodeObj(const char* szPerplex, IHXStreamableObj* pObj)
{
	if (szPerplex)
    {
		return this->PerplexDecodeObj(szPerplex,strlen(szPerplex),pObj);
    }
	return(FALSE);
}

/////////////////////////////////////////////////////////////////////
// We'll unperplex the data into our own buffer, then decode the obj.
HXBOOL  CHXInfoDecoder::PerplexDecodeObj(const char* szPerplex, UINT32 nPlexLen, IHXStreamableObj* pObj)
{
	UINT32 nBitLen  = nPlexLen * sizeof(ULONG32) / Perplex_PER_ULONG32;

	UCHAR* Buffer = new UCHAR[nBitLen + 4];
	HX_ASSERT(Buffer!=NULL);
	if (Buffer==NULL)	return(FALSE);

    UINT32 nBitLenActual = CHXInfoDecoder::SetFromPerplex(szPerplex, Buffer, nBitLen+4, nPlexLen);

	HXBOOL bRetVal = FALSE;
	if (nBitLenActual==nBitLen)
    {
		bRetVal  = this->DecodeObj(Buffer, nBitLen, pObj);
    }

	delete [] Buffer;

    return bRetVal;
}


/////////////////////////////////////////////////////////////////////
HXBOOL  CHXInfoDecoder::HexDecodeObj(const char* szHex, IHXStreamableObj* pObj)
{
	if (szHex)
    {
		return this->HexDecodeObj(szHex,strlen(szHex),pObj);
    }
	return(FALSE);
}

/////////////////////////////////////////////////////////////////////
HXBOOL  CHXInfoDecoder::HexDecodeObj(const char* szHex, UINT32 nHexLen, IHXStreamableObj* pObj)
{
	UINT32 nBitLen  = nHexLen>>1; // unhexed length is 1/2 of hex'd length

	UCHAR* Buffer=new UCHAR[nBitLen];
	HX_ASSERT(Buffer!=NULL);
	if (Buffer==NULL)	return(FALSE);

    UINT32 nBitLenActual = CHXInfoDecoder::SetFromHex(szHex, Buffer, nHexLen);

	HXBOOL bRetVal = FALSE;
	if (nBitLenActual==nBitLen)
    {
		bRetVal  = this->DecodeObj(Buffer,nBitLen,pObj);
    }

	delete [] Buffer;

    return bRetVal;
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadObj(IHXStreamableObj* pObj) 
{
	UINT32 nLen=this->GetOffset();
	if (pObj->ReadObjFromBits(this)==HXR_OK)
		return this->GetOffset()-nLen;

#if 0		
	// reading object
	this->Seek(nLen);
#endif
	return(0);
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadUCHAR(UCHAR& nValue)
{
    UCHAR temp;
    memcpy(&temp, m_Buffer+m_nOffset, sizeof(temp)); /* Flawfinder: ignore */
    nValue = temp;
	m_nOffset += sizeof(temp);
    return sizeof(temp);
}



/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadUINT16(UINT16& nValue)
{
    UINT16 temp16;
    memcpy(&temp16, m_Buffer+m_nOffset, sizeof(temp16)); /* Flawfinder: ignore */
    nValue = WToHost(temp16);

	m_nOffset += sizeof(temp16);
    return sizeof(temp16);
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadUINT32(UINT32& nValue)
{
    UINT32 temp32;
    memcpy(&temp32, m_Buffer+m_nOffset, sizeof(temp32)); /* Flawfinder: ignore */
    nValue = DwToHost(temp32);

	m_nOffset += sizeof(temp32);
    return sizeof(temp32);
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadString(CHXString& strValue)
{
    UINT32 FieldLen = *(m_Buffer+m_nOffset++);

    char* pBuffer = strValue.GetBuffer((int)FieldLen);
    memcpy(pBuffer, m_Buffer+m_nOffset, (int)FieldLen); /* Flawfinder: ignore */
    pBuffer[FieldLen] = '\0';
    strValue.ReleaseBuffer();

	m_nOffset += FieldLen;

    return FieldLen+1;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadAndAllocCString(char*& pszValue)
{
    UINT32 FieldLen = *(m_Buffer+m_nOffset++);

    pszValue = new char[FieldLen+1];
    memcpy(pszValue, m_Buffer+m_nOffset, (int)FieldLen); /* Flawfinder: ignore */
    pszValue[FieldLen] = '\0';

	m_nOffset += FieldLen;

    return FieldLen+1;
}

/////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadLargeString(CHXString& strValue)
{
    UINT16 FieldLen = 0;
    UINT32 nSize = ReadUINT16(FieldLen);

    char* pBuffer = strValue.GetBuffer(FieldLen);
    memcpy(pBuffer, m_Buffer+m_nOffset, FieldLen); /* Flawfinder: ignore */
    pBuffer[FieldLen] = '\0';
    strValue.ReleaseBuffer();

	m_nOffset += FieldLen;

    return nSize+FieldLen;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadAndAllocLargeCString(char*& pszValue)
{
    UINT16 FieldLen = 0;
    UINT32 nSize = ReadUINT16(FieldLen);

    pszValue = new char[FieldLen+1];
    memcpy(pszValue, m_Buffer+m_nOffset, FieldLen); /* Flawfinder: ignore */
    pszValue[FieldLen] = '\0';

	m_nOffset += FieldLen;

    return nSize+FieldLen;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadBuffer(char* szBuffer, UINT32 nSize)
{
    memcpy(szBuffer, m_Buffer+m_nOffset, (int)nSize); /* Flawfinder: ignore */
	m_nOffset += nSize;
    return nSize;
}

#if 0
/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32) CHXInfoDecoder::ReadUTCTime(UTCTimeRep& TimeRep)
{
	UINT32	time, nSize;

	nSize = ReadUINT32(time);

	TimeRep.SetUTCTime(time);
	return(nSize);
}
#endif

/////////////////////////////////////////////////////////////////////
STDMETHODIMP CHXInfoDecoder::Seek(UINT32 nPos)
{
	if (nPos<=m_nDecodedLen)
    {
		m_nOffset = nPos;
    }
	return(HXR_OK);
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP CHXInfoDecoder::SkipForward(UINT32 nAmount)
{
	m_nOffset += nAmount;

	if (m_nOffset>m_nDecodedLen)
    {
		m_nOffset = m_nDecodedLen;
    }
	return(HXR_OK);
}

/////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HXBOOL) CHXInfoDecoder::IsEndOfData()
{
	if (m_nDecodedLen<Perplex_ALIGNMENT)
	{
		return(TRUE);
	}

	if (m_nOffset>(m_nDecodedLen-Perplex_ALIGNMENT))
		return(TRUE);
	else
		return(FALSE);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		FromHexNibble()
//
//	Purpose:
//
//
//	Parameters:
//
//		char hexChar
//		Character representing a hex encoding of a nibble.
//
//	Return:
//
//		UCHAR
//		Actual value of the nibble encoded by hexChar.
//
UCHAR CHXInfoDecoder::FromHexNibble(char hexChar)
{
	UCHAR value = 0;
	if (hexChar >= '0' && hexChar <= '9')
	{
		value = hexChar - '0';
	}
	else if (hexChar >= 'A' && hexChar <= 'F')
	{
		value = hexChar - 'A' + 10;
	}
	else if (hexChar >= 'a' && hexChar <= 'f')
	{
		value = hexChar - 'a' + 10;
	}
#ifdef _DEBUG
	else
	{
		// Bad hex character!
		HX_ASSERT(FALSE);
	}
#endif

	return value;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		HXClientLicense::SetFromHex()
//
//	Purpose:
//
//		Sets items of the license key from a hexidecimal encoding of the
//		license key bits.
//
//	Parameters:
//
//		const char* hex
//		Pointer to a buffer that contains a hexidecimal encoding of the
//		license key.
//
//		UCHAR* Bits
//		Pointer to a buffer that will be filled on output with a bitwise
//		decoding of the license key.
//
//		UINT32 nSize
//		Size of the hex string on input
//
//	Return:
//
//		int
//		Size in bytes of decoded size!
//
//	Note:
//
// 		Hex representations of the License Key consist of the
// 		representation of the bits in human readable form in a
// 		HEX encoding. This basically uses a pointer to a memory
// 		buffer of charcters encoded as hex representing the bits.
// 		Valid characters are 0-9,A-F. Buffers out will be upper
// 		case letters, buffers in will be converted to upper case.
//
// 		These char's are hex encoding. They are never DBCS, the only
//		valid characters are 0-9,A-F
//
UINT32 CHXInfoDecoder::SetFromHex(const char* hex, UCHAR* Bits, UINT32 nSize)
{
	UINT32	ndxBits;
	UINT32	ndxHex = 0;
	UCHAR			nibble1;
	UCHAR			nibble2;
	UCHAR			byte;

	for (ndxBits = 0; ndxHex < nSize; ndxBits++)
	{
		nibble1 = FromHexNibble(hex[ndxHex]);
		nibble1 = nibble1 << 4;
		ndxHex++;
		nibble2 = FromHexNibble(hex[ndxHex]);
		byte = nibble1 | nibble2;
		ndxHex++;
		Bits[ndxBits] = byte;
	}
	return ndxBits;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		ToHexNibble()
//
//	Purpose:
//
//		Converts a nibble into the appropriate character for Hex encoding.
//
//	Parameters:
//
//		UCHAR hexValue
//		Value of the nibble.
//
//	Return:
//
//		char
//		Character representing the hex encoding of the nibble.
//
char CHXInfoEncoder::ToHexNibble(UCHAR hexValue)
{
	char hexChar = '0';

	if (hexValue <= 0x9)
	{
		hexChar = '0' + hexValue;
	}
	else if (hexValue >= 0xA && hexValue <= 0xF)
	{
		hexChar = 'A' + (hexValue - 10);
	}
#ifdef _DEBUG
	else
	{
		// Bad hex character!
		HX_ASSERT(FALSE);
	}
#endif

	return hexChar;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		DumpToHex()
//
//	Purpose:
//
//		Formats the license key in human readable hexadecimal encoding.
//
//	Parameters:
//
//		char* hex
//		Pointer to buffer to be filled with hexadecimal encoding of
//		license key
//
//	Return:
//
//		None.
//
void CHXInfoEncoder::DumpToHex(char* hex, UCHAR* Bits, UINT32	nSize)
{
	UINT32	ndxBits;
	UINT32	ndxHex = 0;
	UCHAR			nibble1;
	UCHAR			nibble2;
	UCHAR			byte;

	for (ndxBits = 0; ndxBits < nSize; ndxBits++)
	{
		byte = Bits[ndxBits];
		nibble1 = (byte & 0xF0) >> 4;
		nibble2 = (byte & 0x0F);

		hex[ndxHex] = ToHexNibble(nibble1);
		ndxHex++;
		hex[ndxHex] = ToHexNibble(nibble2);
		ndxHex++;
	}
	hex[ndxHex] = '\0';
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		MapFromPerplex()
//
//	Purpose:
//
//		Converts appropriate characters for Perplex encoding into a ULONG32.
//
#ifdef _WIN16
extern char* zPerplexChars;
#else
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

static const char zDePerplexMap[] = 
{	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
	0,36,37,38,39,40,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
	25,26,27,28,29,30,31,32,33,34,35,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#endif /* _WIN16 */

UCHAR CHXInfoDecoder::MapFromPerplex(char Perplex)
{
    return zDePerplexMap[Perplex];
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		MapToPerplex()
//
//	Purpose:
//
//		Converts a digit ordinal to the Perplex digit...
//
char CHXInfoEncoder::MapToPerplex(UCHAR Perplex)
{                                                  
	#ifdef _DEBUG
	#ifndef _WIN16
	// On win16, zPerplexChars is a char *, rather than an array, so this
	// doesn't work.
	int size_zPerplexChars  = sizeof(zPerplexChars); 
	int size_zPerplexChars0 = sizeof(zPerplexChars[0]);
	HX_ASSERT(Perplex < size_zPerplexChars/size_zPerplexChars0);
	#endif
	#endif
	return zPerplexChars[Perplex];
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		FromPerplex()
//
//	Purpose:
//
//		Converts appropriate characters for Perplex encoding into a ULONG32.
//
ULONG32 CHXInfoDecoder::FromPerplex(const char* Perplex)
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

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		ToPerplexNibble()
//
//	Purpose:
//
//		Converts a ULONG32 into the appropriate characters for Perplex encoding.
//
void CHXInfoEncoder::ToPerplex(ULONG32 Input, char* Perplex)
{
	ULONG32 value;
	UCHAR	charValue;

	// Convert to net byte order!
	value = DwToNet(Input);

	for (int n = 0; n < Perplex_PER_ULONG32; n++)
	{
		charValue = (UCHAR)(value % Perplex_BASE);
		Perplex[n] = MapToPerplex(charValue);
		value = value / Perplex_BASE;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		HXClientLicense::SetFromPerplex()
//
//	Purpose:
//
//		Sets items of the license key from a Perplexidecimal encoding of the
//		license key bits.
//
//	Parameters:
//
//		const char* Perplex
//		Pointer to a buffer that contains a Perplexidecimal encoding of the
//		license key.
//
//		UCHAR* Bits
//		Pointer to a buffer that will contain decoded bytes of information
//
//              UINT32 ulBitsLen
//              Number of bytes in Bits output buffer
//
//		int nSize
//		Input size in bytes
//
//	Return:
//
//		int
//		Size in bytes of decoded information
//
//	Note:
//
// 		Perplex representations of the License Key consist of the
// 		representation of the bits in human readable form in a
// 		Perplex encoding. This basically uses a pointer to a memory
// 		buffer of charcters encoded as Perplex representing the bits.
// 		Valid characters are 0-9,A-F. Buffers out will be upper
// 		case letters, buffers in will be converted to upper case.
//
// 		These char's are Perplex encoding. They are never DBCS, the only
//		valid characters are 0-9,A-F
//
UINT32 CHXInfoDecoder::SetFromPerplex(const char* Perplex, UCHAR* Bits, UINT32 ulBitsLen, UINT32 nSize)
{
	UINT32	ndxBits;
	UINT32	ndxPerplex = 0;
	ULONG32			temp32;

	for (ndxBits = 0; ndxPerplex < nSize; )
	{
		temp32 = FromPerplex(&Perplex[ndxPerplex]);
		ndxPerplex+=Perplex_PER_ULONG32;
		if (ndxBits+4 <= ulBitsLen) memcpy(&Bits[ndxBits],&temp32,sizeof(temp32)); /* Flawfinder: ignore */
		ndxBits+=sizeof(temp32);
	}
	return ndxBits;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		DumpToPerplex()
//
//	Purpose:
//
//		Formats the license key in human readable Perplexadecimal encoding.
//
//	Parameters:
//
//		char* Perplex
//		Pointer to buffer to be filled with Perplexadecimal encoding of
//		license key
//
//	Return:
//
//		None.
//
void CHXInfoEncoder::DumpToPerplex(char* Perplex, UINT32 ulPerplexSize, UCHAR* Bits, UINT32 nSize)
{
	UINT32	ndxBits;
	UINT32	ndxPerplex = 0;
	ULONG32			temp32;

	for (ndxBits = 0; ndxBits < nSize; )
	{
		if (ndxBits+4 <= nSize) memcpy(&temp32,&Bits[ndxBits],sizeof(temp32)); /* Flawfinder: ignore */
                ndxBits+=sizeof(temp32);
		if (ndxPerplex+Perplex_PER_ULONG32 <= ulPerplexSize) ToPerplex(temp32,&Perplex[ndxPerplex]);
		ndxPerplex+=Perplex_PER_ULONG32;
	}
	Perplex[ndxPerplex] = '\0';
}



/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		MapFromMIMEBase64()
//
//	Purpose:
//
//		Converts appropriate characters for MIME-Base64 encoding into a the
//		Base64 ordinal.
//
#ifdef _WIN16
extern char* zMIMEBase64Chars;
#else
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
#endif /* _WIN16 */

#define MIME_BASE64_PADDING		'='
#define FROM_MIME_BASE64_BYTES_IN	4
#define FROM_MIME_BASE64_BYTES_OUT	3
#define TO_MIME_BASE64_BYTES_IN		FROM_MIME_BASE64_BYTES_OUT
#define TO_MIME_BASE64_BYTES_OUT	FROM_MIME_BASE64_BYTES_IN

UCHAR CHXInfoDecoder::MapFromMIMEBase64(char MIMEBase64)
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

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		MapToMIMEBase64()
//
//	Purpose:
//
//		Converts a digit ordinal to the MIMEBase64 digit...
//
char CHXInfoEncoder::MapToMIMEBase64(UCHAR MIMEBase64)
{
	HX_ASSERT( MIMEBase64 < sizeof(zMIMEBase64Chars)/sizeof(zMIMEBase64Chars[0]));
	return zMIMEBase64Chars[MIMEBase64];
}

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		SetFromMIMEBase64()
//
//	Purpose:
//
//		...
//
//	Parameters:
//
//		...
//
//	Return:
//
//		int
//		Size in bytes of decoded information
//
UINT32 CHXInfoDecoder::SetFromMIMEBase64(const char* MIMEBase64, char* Bits, UINT32 nSize)
{
	UINT32	ndxBits			= 0;
	UINT32	ndxMIMEBase64	= 0;
	HXBOOL			bDone			= FALSE;
	UINT32	ndxTempIn		= 0;
	UCHAR			tempBitsIn	[FROM_MIME_BASE64_BYTES_IN];
	UINT32	padding			= 0;

	HX_ASSERT(strlen(MIMEBase64) <= nSize);

	while (!bDone)
	{
		HX_ASSERT(ndxMIMEBase64 <= nSize);
		HX_ASSERT(ndxBits <= nSize);

		// Fill in our "temporary" in buffer with the Base64 characters.
		for (	ndxTempIn = 0;
				ndxTempIn < FROM_MIME_BASE64_BYTES_IN && (padding == 0);
				ndxTempIn++
			)
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

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		DumpToMIMEBase64()
//
//	Purpose:
//
//		...
//
//	Parameters:
//
//		...
//
//	Return:
//
//		None.
//
void CHXInfoEncoder::DumpToMIMEBase64(char* MIMEBase64, const char* Bits, UINT32 nSize)
{
	UINT32	ndxBits			= 0;
	UINT32	ndxMIMEBase64	= 0;
	HXBOOL			bDone			= FALSE;
	UINT32	ndxTempIn		= 0;
	UINT32	ndxTempOut		= 0;
	UCHAR			tempBitsIn [TO_MIME_BASE64_BYTES_IN];
	UINT32	padding			= 0;

	HX_ASSERT(strlen(Bits) <= nSize);

	while (!bDone)
	{
		HX_ASSERT(ndxMIMEBase64 <= nSize);
		HX_ASSERT(ndxBits <= nSize);

		// Fill in our "temporary" out buffer with the 6 bit chunks of the input.
		for (	ndxTempIn = 0;
				ndxTempIn < TO_MIME_BASE64_BYTES_IN && (padding == 0);
				ndxTempIn++
			)
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
				ndxTempOut++
			)
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



//*******************************************************************
// 	CHXSimpleBuffer
//*******************************************************************


/////////////////////////////////////////////////////////////////////
CHXSimpleBuffer::CHXSimpleBuffer()
	:  m_nSize(0), m_pData(NULL), m_nGrowBy(DEFAULT_GROW_SIZE)
{
}

/////////////////////////////////////////////////////////////////////
CHXSimpleBuffer::CHXSimpleBuffer(UINT32 nSize, UINT32 nGrowBy)
	:  m_nSize(0), m_pData(NULL), m_nGrowBy(nGrowBy)
{
	Resize(nSize);
}

/////////////////////////////////////////////////////////////////////
CHXSimpleBuffer::~CHXSimpleBuffer()
{
	Free();
}


/////////////////////////////////////////////////////////////////////
HXBOOL CHXSimpleBuffer::IsValidOffset(UINT32 n) const
{
	if (n<m_nSize)
		return(TRUE);
	else return(FALSE);
}


/////////////////////////////////////////////////////////////////////
// make sure that pPos is in the buffer.
// If not, the buffer is automatically resized and contents copied.
// pPos must be > pData.
HXBOOL CHXSimpleBuffer::EnsureValidOffset(UINT32 n)
{
	if (IsValidOffset(n)==TRUE)
    {
		return(TRUE);
    }

	return (this->Resize(n));
}


/////////////////////////////////////////////////////////////////////
// copy data into the buffer at the given offset, and if the buffer
// is too small we'll resize the buffer first.
HXBOOL CHXSimpleBuffer::SafeMemCopy(UINT32 nOffset, const void* data, UINT32 len)
{
    if ((len > 0) && (EnsureValidOffset(nOffset+len-1)==TRUE))
    {
    	memcpy( m_pData+nOffset, data, (int)len ); /* Flawfinder: ignore */
	return(TRUE);
    }

    return(FALSE);
}


/////////////////////////////////////////////////////////////////////
HXBOOL CHXSimpleBuffer::Resize(UINT32 nNewSize)
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


/////////////////////////////////////////////////////////////////////
UINT32 CHXSimpleBuffer::RoundUpToGrowSize(UINT32 nSize)
{
	// check for div-by-zero errors (as long as DEFAULT!=0)
	if (m_nGrowBy==0)
	{
		m_nGrowBy = DEFAULT_GROW_SIZE;
	}

	return ( (int)(nSize/m_nGrowBy)+1 ) * m_nGrowBy;
}


/////////////////////////////////////////////////////////////////////
void CHXSimpleBuffer::Free()
{
	if (m_pData!=NULL)
		delete [] m_pData;

	m_pData=NULL;
	m_nSize=0;
}


