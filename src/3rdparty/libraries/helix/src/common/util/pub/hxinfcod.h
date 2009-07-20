/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxinfcod.h,v 1.5 2005/03/14 19:36:40 bobclark Exp $
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

#ifndef _HXINFCOD_H_
#define _HXINFCOD_H_

#include "hxtypes.h"
#include "hxstring.h"
#include "timerep.h"
#include "ihxostrm.h"
#include "unkimp.h"


#define Perplex_BASE 		41
#define Perplex_PER_ULONG32	6
#define Perplex_ALIGNMENT	4


// for dynamic buffer used in Encoder/Decoder classes.
#define	DEFAULT_GROW_SIZE	1024


class CHXInfoEncoder;
class CHXInfoDecoder;
class CHXSimpleBuffer; // crappy little buffer class used by CHXInfoEncoder



/////////////////////////////////////////////////////////////////////
// A simple buffer. There is no implicit access/bounds checking,
// you have to do that yourself or use the 'Safexxxxx()' methods.
// It's actual size is usually not what you set with resizeBuffer(),
// since the size will be rounded up to the nearest 'GrowBy' size.
class CHXSimpleBuffer
{
public:

	CHXSimpleBuffer();
	CHXSimpleBuffer(UINT32 nSize, UINT32 nGrowBy=DEFAULT_GROW_SIZE);

	~CHXSimpleBuffer();


	inline UCHAR*	GetPtr() 				  const	{ return(m_pData); }
	inline UCHAR*	GetPtrAt(UINT32 nOffset)  const { return(m_pData+nOffset); }
	inline UCHAR	GetDataAt(UINT32 nOffset) const { return(*(m_pData+nOffset)); }
	inline UINT32	GetSize() 				  const { return(m_nSize); }

	inline void		SetUCHAR(UINT32 off, UCHAR  x) { *(m_pData+off)=x; }
	inline void		SetUINT16(UINT32 off, UINT16 x) { *((UINT16*)(m_pData+off))=x; }


	// returns TRUE if PTR points inside of buffer.
//	HXBOOL	IsValidPtr(UCHAR* p);
	HXBOOL	IsValidOffset(UINT32 n) const;

	// make sure that Offset is within the buffer.
	// If not, the buffer is automatically resized.
	HXBOOL	EnsureValidOffset(UINT32 n);

	// copy data into buffer, dynamically growing buffer if needed.
	HXBOOL	SafeMemCopy(UINT32 nOffset, const void* data, UINT32 len);

	// no bounds checking here.
	inline void	MemCopy(UINT32 nOffset, const void* data, UINT32 len)
					{ 
					#if _WIN16
					HX_ASSERT(len <= ((INT32)1024 * (INT32)64));
					#endif
					memcpy(m_pData+nOffset, data, (int)len); /* Flawfinder: ignore */
					}

	// grow/shink buffer (does memcpy)
	HXBOOL	Resize(UINT32 nSize);
	void	Free();


protected:
	UINT32	RoundUpToGrowSize(UINT32 nSize);

private:
	UINT32	m_nSize;
	UCHAR*	m_pData;

	UINT32	m_nGrowBy;
};



/////////////////////////////////////////////////////////////////////
class CHXInfoEncoder : public IHXObjOutStream, 
					   public CUnknownIMP
{
    // the IUnknown implementation declaration
    DECLARE_UNKNOWN(CHXInfoEncoder)

public:
	CHXInfoEncoder();
	~CHXInfoEncoder();

	// Next 8 functions used most when you need to encode an object.
	void  EncodeObj(IHXStreamableObj* pObj);
	void  EncodeObj(IHXStreamableObj& Obj) {EncodeObj(&Obj);}

	void  PerplexEncodeObj(IHXStreamableObj* pObj);
	void  PerplexEncodeObj(IHXStreamableObj& Obj) {PerplexEncodeObj(&Obj);}

	void  HexEncodeObj(IHXStreamableObj* pObj);
	void  HexEncodeObj(IHXStreamableObj& Obj) {HexEncodeObj(&Obj);}


	// After using one of these two functions, the data retreived from
	// GetBuffer() will be either perplexed or hexed. Don't bother using these
	// if you're using PerplexEncodeObj() or HexEncodeObj(), since those
	// methods do an implicit 'Perplex()' or 'Hex()' for you.
	// Misuse of the encoder can cause these to fail, so check the return
	// value!
	HXBOOL Perplex(); // returns false if perplexing failed for any reason
	HXBOOL Hex(); // returns false if hex failed for any reason


	//** IHXObjOutStream methods **//
	STDMETHOD(Initialize) (THIS);	// clean out buffers, start fresh!

    STDMETHOD_(UINT32, WriteObj) (THIS_ IHXStreamableObj* pObj);
    STDMETHOD_(UINT32, WriteObj) (THIS_ IHXStreamableObj& Obj) {return this->WriteObj(&Obj); }
	STDMETHOD_(UINT32, WriteUCHAR) (THIS_ UCHAR nValue);
    STDMETHOD_(UINT32, WriteUINT16) (THIS_ UINT16 nValue);
    STDMETHOD_(UINT32, WriteUINT32) (THIS_ UINT32 nValue);
    STDMETHOD_(UINT32, WriteString) (THIS_ const char* szValue);
    STDMETHOD_(UINT32, WriteStringCat) (THIS_ const char* szValue1,const char* szValue2,const char* szValue3=NULL);
    STDMETHOD_(UINT32, WriteLargeString) (THIS_ const char* szValue);
    STDMETHOD_(UINT32, WriteBuffer)  (THIS_ const char* szBuffer, UINT32 nSize);

	// dump at specific positions (earlier positions than current offset)
    STDMETHOD_(UINT32, WriteUINT16At) (THIS_ UINT32 nOffset, UINT16 nValue);
    STDMETHOD_(UINT32, WriteUINT32At) (THIS_ UINT32 nOffset, UINT32 nValue);
	
	// get encoded data and encoded length
	STDMETHOD_(const char*, GetBuffer) (THIS);
	STDMETHOD_(UINT32, GetLength) (THIS);

	// get end of raw data buffer - used when dumping bits into the
	// encoder and you want to know the current pos in the buffer
	STDMETHOD_(UINT32, GetOffset) (THIS) { return(m_nOffset);}
	
	

	// basic routines for hex/perplexing, used when this class doesn't
	// meet your needs.
	static void	DumpToHex(char* hex, UCHAR* Bits, UINT32 nSize);
	static void	DumpToPerplex(char* Perplex, UINT32 ulPerplexSize, UCHAR* Bits, UINT32 nSize);

protected:

	static char	ToHexNibble(UCHAR hexValue);
	static void	DumpToMIMEBase64(char* MIMEBase64, const char* Bits, UINT32 nSize);
	static char MapToPerplex(UCHAR Perplex);
	static void ToPerplex(ULONG32 Input, char* Perplex);
	static char MapToMIMEBase64(UCHAR MIMEBase64);

private:

	CHXSimpleBuffer		m_Buffer;
	UINT32				m_nOffset;		// current end of data in buffer

	UCHAR*				m_FinalBuffer;
	HXBOOL				m_bFinalBufferAllocFlag;
	UINT32				m_nFinalLen;	// length of encoded data
};


/////////////////////////////////////////////////////////////////////
class CHXInfoDecoder : public IHXObjInStream, 
					   public CUnknownIMP

{
    // the IUnknown implementation declaration
    DECLARE_UNKNOWN(CHXInfoDecoder)

public:
	CHXInfoDecoder() : m_Buffer(NULL), m_nDecodedLen(0), m_nOffset(0) {}
	~CHXInfoDecoder() {};

	//**** THESE METHODS USED TO LOAD DATA INTO AN IHXStreamableObj OBJECT.
	HXBOOL  PerplexDecodeObj(const char* szPerplex, IHXStreamableObj* pObj);
	HXBOOL  PerplexDecodeObj(const char* szPerplex, IHXStreamableObj& Obj) {return PerplexDecodeObj(szPerplex, &Obj);}

	HXBOOL  PerplexDecodeObj(const char* szPerplex, UINT32 nSize, IHXStreamableObj* pObj);
	HXBOOL  PerplexDecodeObj(const char* szPerplex, UINT32 nSize, IHXStreamableObj& Obj) {return PerplexDecodeObj(szPerplex, nSize, &Obj);}

	HXBOOL  HexDecodeObj(const char* szHex, IHXStreamableObj* pObj);
	HXBOOL  HexDecodeObj(const char* szHex, IHXStreamableObj& Obj) {return HexDecodeObj(szHex, &Obj);}

	HXBOOL  HexDecodeObj(const char* szHex, UINT32 nSize, IHXStreamableObj* pObj);
	HXBOOL  HexDecodeObj(const char* szHex, UINT32 nSize, IHXStreamableObj& Obj) {return PerplexDecodeObj(szHex, nSize, &Obj);}

	// for raw data...
	HXBOOL  DecodeObj(const UCHAR* buffer, UINT32 nSize, IHXStreamableObj* pObj);
	HXBOOL  DecodeObj(const UCHAR* buffer, UINT32 nSize, IHXStreamableObj& Obj) {return DecodeObj(buffer, nSize, &Obj);}
	HXBOOL  DecodeObj(const char* buffer, UINT32 nSize, IHXStreamableObj* pObj){return DecodeObj((UCHAR*)buffer, nSize, pObj);}
	HXBOOL  DecodeObj(const char* buffer, UINT32 nSize, IHXStreamableObj& Obj) {return DecodeObj((UCHAR*)buffer, nSize, &Obj);}

	
	STDMETHOD(Initialize) (THIS) {m_Buffer=NULL;m_nOffset=0;m_nDecodedLen=0; return HXR_OK;}
	STDMETHOD(Initialize) (THIS_ UCHAR* buf, UINT32 nLen) {m_Buffer=buf;m_nOffset=0;m_nDecodedLen=nLen; return HXR_OK;}
	
    STDMETHOD_(UINT32, ReadObj) (THIS_ IHXStreamableObj* pObj);
    STDMETHOD_(UINT32, ReadObj) (THIS_ IHXStreamableObj& Obj) {return this->ReadObj(&Obj); }
	
    STDMETHOD_(UINT32, ReadUCHAR)  (THIS_ UCHAR& nValue);
    STDMETHOD_(UINT32, ReadUINT16) (THIS_ UINT16& nValue);
    STDMETHOD_(UINT32, ReadUINT32) (THIS_ UINT32& nValue);
    STDMETHOD_(UINT32, ReadString) (THIS_ CHXString& strValue);
	STDMETHOD_(UINT32, ReadAndAllocCString) (THIS_ char*& pszValue);
    STDMETHOD_(UINT32, ReadLargeString) (THIS_ CHXString& strValue);
    STDMETHOD_(UINT32, ReadAndAllocLargeCString) (THIS_ char*& pszValue);
    STDMETHOD_(UINT32, ReadBuffer)  (THIS_ char* szBuffer, UINT32 nSize);
//	STDMETHOD_(UINT32, ReadUTCTime) (THIS_ UTCTimeRep& TimeRep);

	STDMETHOD_(HXBOOL, IsEndOfData) ();

	// arbitrarily change offset position.
    STDMETHOD(Seek) 		(UINT32 nPos);
    STDMETHOD(SkipForward) 	(UINT32 nAmount);
	
	STDMETHOD_(UINT32, GetOffset) (THIS) { return(m_nOffset);}

	// get data and length
	STDMETHOD_(const char*, GetBuffer) (THIS) {return((char*)m_Buffer);}
	STDMETHOD_(UINT32, GetLength) (THIS) {return(m_nDecodedLen);}


	//** FUNCTIONS TO DECODE HEX/PERPLEX
	static UINT32	SetFromHex(const char* hex, UCHAR* Bits, UINT32 nSize);
	static UINT32	SetFromPerplex(const char* Perplex, UCHAR* Bits, UINT32 ulBitsLen, UINT32 nSize);

protected:

	// basic functions used internally for decoding perlpex/hex
	static UCHAR		FromHexNibble(char hexChar);
	static UINT32	SetFromMIMEBase64(const char* MIMEBase64, char* Bits, UINT32 nSize);
	static UCHAR MapFromPerplex(char Perplex);
	static ULONG32 FromPerplex(const char* Perplex);
	static UCHAR MapFromMIMEBase64(char MIMEBase64);

private:

	UCHAR* m_Buffer;	  // Ptr to user-supplied data to load into object.
	UINT32 m_nDecodedLen; // actual end of data in buffer
	UINT32 m_nOffset;     // current position in buffer
};




#endif
