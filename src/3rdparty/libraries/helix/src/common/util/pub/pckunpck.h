/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pckunpck.h,v 1.26 2009/05/20 13:39:50 ehyche Exp $
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

#ifndef PCKUNPCK_H
#define PCKUNPCK_H

/* Unfortunate but necessary includes */
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxstring.h"

#include "ihxfgbuf.h"
#include "ihxpckts.h"
#include "hxengin.h"
#include <stdarg.h> /* for va_arg */

/*
 * forward decls
 */
typedef _INTERFACE IHXBuffer IHXBuffer;
typedef _INTERFACE IHXValues IHXValues;
typedef _INTERFACE IHXFragmentedBuffer IHXFragmentedBuffer;

/*
 * constant for text encoding type
 */
typedef enum	{
	HX_TEXT_ENCODING_TYPE_UNKNOWN = 0,
	HX_TEXT_ENCODING_TYPE_ISO8859_1,
	HX_TEXT_ENCODING_TYPE_UTF16,
	HX_TEXT_ENCODING_TYPE_UTF16BE,
	HX_TEXT_ENCODING_TYPE_UTF16LE,
	HX_TEXT_ENCODING_TYPE_UTF8,
} HX_TEXT_ENCODING_TYPE;
		
/*
 * packing/unpacking functions
 * NOTE: this are Nick Hart's original methods imported
 * directly from chinembed/unix/bufferutils.cpp
 */

#if 0
HX_RESULT PackBuffer(REF(IHXBuffer*) pBuffer,
		     const char*      pFormat,
		     ...);
HX_RESULT PackBufferV(REF(IHXBuffer*) pBuffer,
		      const char*      pFormat,
		      va_list vargs);
int UnpackBuffer(REF(const char*) pBuffer,
		 const char*      pFormat,
		 ...);
int UnpackBufferV(REF(const char*) pBuffer,
		  const char*      pFormat,
		  va_list          vargs);
#ifdef _DEBUG
void TestBufferPacking();
#endif
#endif

/*
 * NOTE: these are methods Eric Hyche added specifically
 * for packing and unpacking IHXValues. They also add two
 * additional options:
 * a) binary packing (as opposed to Nick's text string packing); and 
 * b) if you pass in an IUnknown context, then all your IHXBuffer's
 *    and IHXValues will be created by the common class factory.
 *    If you leave out the IUnknown, then IHXBuffer's will be 
 *    directly created from CHXBuffer's and IHXValues will be created
 *    directly from CHXHeader's.
 */
// GetBinaryPackedSize() returns the number of
// bytes required to binary pack the IHXValues
// that you pass in
UINT32 GetBinaryPackedSize(IHXValues* pValues);
// PackValuesBinary() packs the IHXValues you provide
// in the IHXBuffer you provide in binary form. It assumes
// that pBuffer is at least as big as the the
// number of bytes returned by GetBinaryPackedSize(). If
// pBuffer is not big enough, it will return HXR_FAIL.
HX_RESULT PackValuesBinary(IHXBuffer* pBuffer,
                           IHXValues* pValues);
// CreateBufferCCF() creates an IHXBuffer using
// the IHXCommonClassFactory it QI's the pContext
// to get.
HX_RESULT CreateBufferCCF(REF(IHXBuffer*) rpBuffer,
                          IUnknown*       pContext);
// CreateSizedBufferCCF() creates an IHXBuffer using
// the IHXCommonClassFactory it QI's the pContext
// to get. It also calls IHXBuffer::SetSize() on 
// the buffer and optionally intializes every byte
// in the buffer to ucInitVal.
HX_RESULT CreateSizedBufferCCF(REF(IHXBuffer*) rpBuffer,
                               IUnknown*       pContext,
                               UINT32          ulSize,
                               HXBOOL          bInitialize = FALSE,
                               BYTE            ucInitVal = 0);
// CreateAndSetBufferCCF() creates an IHXBuffer
// using the IHXCommonClassFactory and calls
// IHXBuffer::Set() using the supplied buffer
// pointer and length.
HX_RESULT CreateAndSetBufferCCF(REF(IHXBuffer*) rpBuffer,
                                BYTE*           pBuf,
                                UINT32          ulLen,
                                IUnknown*       pContext);
HX_RESULT CreateAndSetBufferWithoutAllocCCF(REF(IHXBuffer*) rpBuffer,
                                BYTE*           pBuf,
                                UINT32          ulLen,
                                IUnknown*       pContext);
HX_RESULT CreateFragmentedBufferCCF(REF(IHXFragmentedBuffer*) rpBuffer,
				    IUnknown* pContext);
// CreateValuesCCF() creates an IHXValues using the
// IHXCommonClassFactory it QI's pContext to get.
HX_RESULT CreateValuesCCF(REF(IHXValues*) rpValues,
                          IUnknown*        pContext);
// CreateStringBufferCCF() creates an IHXBuffer with the
// string pszStr in it. The length of rpBuffer will
// be strlen(pszStr) + 1. CreateStringBuffer() calls
// CreateBuffer() to create the IHXBuffer.
HX_RESULT CreateStringBufferCCF(REF(IHXBuffer*) rpBuffer,
                                const char*     pszStr,
                                IUnknown*       pContext);
// CreatePacketCCF() creates an IHXPacket using
// the IHXCommonClassFactory it QI's the pContext
// to get.
HX_RESULT CreatePacketCCF(REF(IHXPacket*) rpPacket,
                          IUnknown*       pContext);
// SetCStringPropertyCCF() sets a CString property in
// pValues where pszName is the property name
// and the property value is pszValue. SetCStringProperty()
// calls CreateStringBufferCCF() to create the IHXBuffer
// which holds the property value string.
HX_RESULT SetCStringPropertyCCF(IHXValues* pValues,
                                const char* pszName,
                                const char* pszValue,
                                IUnknown*   pContext,
                                HXBOOL        bSetAsBufferProp = FALSE);
// SetCStringPropertyCCFWithNullTerm() sets a CString property in
// pValues where pszName is the property name
// and the property value is in pBuf and ulLen. It's assumed
// that pBuf and ulLen hold a string, but the string is
// not NULL-terminated. Therefore, SetCStringPropertyWithNT()
// will NULL-terminate the buffer in pBuf (making the
// IHXBuffer it creates ulLen + 1).
HX_RESULT SetCStringPropertyCCFWithNullTerm(IHXValues*  pValues,
                                            const char* pszName,
                                            BYTE*       pBuf,
                                            UINT32      ulLen,
                                            IUnknown*   pContext,
                                            HXBOOL        bSetAsBufferProp = FALSE);
// CreateNullTermBuffer takes pBuf and ulLen, creates a
// buffer of length ulLen+1, copies ulLen bytes
// of pBuf into this new buffer, and then NULL-terminates
// it. It then returns this buffer as an out parameter.
HX_RESULT CreateNullTermBuffer(BYTE*  pBuf,
                               UINT32 ulLen,
                               char** ppNTBuf);
// SetBufferPropetyCCF() sets a Buffer property in pValues.
// First calls CreateBufferCCF() (see above) to create the
// IHXBuffer, then calls IHXBuffer::Set() with the
// provided pBuf and ulLen, and then sets this
// property into the IHXValues.
HX_RESULT SetBufferPropertyCCF(IHXValues*  pValues,
                               const char* pszName,
                               BYTE*       pBuf,
                               UINT32      ulLen,
                               IUnknown*   pContext);
// UnpackPropertyULONG32() unpacks a ULONG32 property
// from binary form, and then sets that ULONG32 property
// into the provided IHXValues. Initially rpBuf should
// be set to the beginning of the packed ULONG32 and 
// pLimit should be set to the end of the entire packing
// buffer (this provides protection against corrupt data
// buffer overruns). Upon return, rpBuf will be set to
// the first byte past the end of the packed ULONG32.
// UnpackPropertyULONG32() is called by UnpackValuesBinary().
HX_RESULT UnpackPropertyULONG32CCF(IHXValues* pValues,
                                   REF(BYTE*)  rpBuf,
                                   BYTE*       pLimit,
                                   IUnknown*   pContext);
// UnpackPropertyCString() unpacks a CString property
// from binary form, and then sets that CString property
// into the provided IHXValues. Initially rpBuf should
// be set to the beginning of the packed CString and 
// pLimit should be set to the end of the entire packing
// buffer (this provides protection against corrupt data
// buffer overruns). Upon return, rpBuf will be set to
// the first byte past the end of the packed CString.
// UnpackPropertyCString() is called by UnpackValuesBinary().
HX_RESULT UnpackPropertyCStringCCF(IHXValues* pValues,
                                   REF(BYTE*)  rpBuf,
                                   BYTE*       pLimit,
                                   IUnknown*   pContext);
// UnpackPropertyBuffer() unpacks a Buffer property
// from binary form, and then sets that Buffer property
// into the provided IHXValues. Initially rpBuf should
// be set to the beginning of the packed CString and 
// pLimit should be set to the end of the entire packing
// buffer (this provides protection against corrupt data
// buffer overruns). Upon return, rpBuf will be set to
// the first byte past the end of the packed Buffer.
// UnpackPropertyBuffer() is called by UnpackValuesBinary().
HX_RESULT UnpackPropertyBufferCCF(IHXValues* pValues,
                                  REF(BYTE*)  rpBuf,
                                  BYTE*       pLimit,
                                  IUnknown*   pContext);
// UnpackValuesBinary() unpacks a binary-packed pBuffer into
// the provided pValues. If pBuffer is not binary-packed, then
// it will return HXR_FAIL. If you pass in an IUnknown context,
// then it will use the common class factory to create the
// necessary IHXBuffer's. Otherwise, it will use "new CHXBuffer()".
HX_RESULT UnpackValuesBinaryCCF(IHXValues* pValues,
                                IHXBuffer* pBuffer,
                                IUnknown*   pContext);
// UnpackValuesBinary() unpacks a binary-packed pBuffer into
// the provided pValues. If pBuffer is not binary-packed, then
// it will return HXR_FAIL. If you pass in an IUnknown context,
// then it will use the common class factory to create the
// necessary IHXBuffer's. Otherwise, it will use "new CHXBuffer()".
HX_RESULT UnpackValuesBinaryCCF(IHXValues* pValues,
                                BYTE*       pBuf,
                                UINT32      ulLen,
                                IUnknown*   pContext);
// PackValues() packs pValues into an IHXBuffer and returns
// the packed buffer into rpBuffer. If bPackBinary is TRUE,
// it will pack the IHXValues as binary; otherwise, it
// will pack the IHXValues as text. If you pass in an IUnknown context,
// then it will use the common class factory to create the
// necessary IHXBuffer's. Otherwise, it will use "new CHXBuffer()".
HX_RESULT PackValuesCCF(REF(IHXBuffer*) rpBuffer,
                        IHXValues*      pValues,
                        HXBOOL          bPackBinary,
                        IUnknown*       pContext);
// UnpackValues() unpacks an IHXBuffer into rpValues. It 
// automatically detects whether pBuffer is binary-packed
// or text-packed (as long as PackValues() was used to pack
// the IHXValues).
HX_RESULT UnpackValuesCCF(REF(IHXValues*) rpValues,
                          IHXBuffer*      pBuffer,
                          IUnknown*       pContext);
// UnpackValues() unpacks an IHXBuffer into rpValues. It 
// automatically detects whether pBuffer is binary-packed
// or text-packed (as long as PackValues() was used to pack
// the IHXValues).
HX_RESULT UnpackValuesCCF(REF(IHXValues*) rpValues,
                          BYTE*            pBuf,
                          UINT32           ulLen,
                          IUnknown*        pContext);
// AreValuesInclusiveIdentical() checks whether all the
// properties in pValues1 are:
// a) in pValues2; AND
// b) identical to the corresponding properties in pValues2.
HXBOOL AreValuesInclusiveIdentical(IHXValues* pValues1,
                                 IHXValues* pValues2);
// AreValuesIdentical() returns
// AreValuesInclusiveIdentical(pValues1, pValues2) &&
// AreValuesInclusiveIdentical(pValues1, pValues2).
// By checking inclusive identity in both directions, this
// absolutely establishes identity between the contents
// of pValues1 and pValues2.
HXBOOL AreValuesIdentical(IHXValues* pValues1,
                        IHXValues* pValues2);
#ifdef _DEBUG
// This tests text and binary packing of IHXValues.
// It is also a good place to look for examples of how
// to call PackValues() and UnpackValues().
HX_RESULT TestValuesPacking(IUnknown* pContext = NULL);
#endif


// Text encoding support
HX_RESULT SetCStringPropertyEx(
				IHXValues* pValues,
				const char* pszName,
				UINT8* pbValue,
				UINT32 ulBytes, 
				IUnknown*	 pContext,
				HX_TEXT_ENCODING_TYPE EncodeType,
                                HXBOOL bSetAsBufferProp = FALSE);

HX_RESULT SetCStringPropertyEx(
				IHXValues* pValues,
				const char* pszName,
				IHXBuffer* pBuffer,
				IUnknown*  pContext,
				HX_TEXT_ENCODING_TYPE EncodeType,
                                HXBOOL bSetAsBufferProp = FALSE);

HX_RESULT GetCStringPropertyEx(
				IHXValues* pValues,
				const char* pszName,
				IHXBuffer* & pBuffer,
				IUnknown*	 pContext,
				HX_TEXT_ENCODING_TYPE & EncodeType,
                                HXBOOL bGetAsBufferProp = FALSE);

HX_RESULT SetCStringPropertyWithNullTermEx(
				IHXValues* pValues,
				const char* pszName,
				UINT8* pbValue,
				UINT32 ulBytes, 
				IUnknown*	 pContext,
				HX_TEXT_ENCODING_TYPE EncodeType,
                                HXBOOL bSetAsBufferProp = FALSE);

IHXValues* CloneHeader(IHXValues* pHeader, IUnknown* pContext);

// Unpack a UINT64 in big-endian format
HX_RESULT UnpackUINT64BE(BYTE* pBuf, UINT32 ulLen, UINT64* pullVal);
// Unpack a UINT64 in big-endian format and advance the buffer parsing
HX_RESULT UnpackUINT64BEInc(BYTE** ppBuf, UINT32* pulLen, UINT64* pullVal);
// Unpack a UINT64 in little-endian format
HX_RESULT UnpackUINT64LE(BYTE* pBuf, UINT32 ulLen, UINT64* pullVal);
// Unpack a UINT64 in little-endian format and advance the buffer parsing
HX_RESULT UnpackUINT64LEInc(BYTE** ppBuf, UINT32* pulLen, UINT64* pullVal);
// Unpack a UINT32 in big-endian format
HX_RESULT UnpackUINT24BE(BYTE* pBuf, UINT32 ulLen, UINT32* pulVal);
// Unpack a UINT32 in big-endian format
HX_RESULT UnpackUINT32BE(BYTE* pBuf, UINT32 ulLen, UINT32* pulVal);
// Unpack a UINT32 in big-endian format and advance the buffer parsing
HX_RESULT UnpackUINT32BEInc(BYTE** ppBuf, UINT32* pulLen, UINT32* pulVal);
// Unpack a UINT32 in little-endian format
HX_RESULT UnpackUINT32LE(BYTE* pBuf, UINT32 ulLen, UINT32* pulVal);
// Unpack a UINT32 in little-endian format and advance the buffer parsing
HX_RESULT UnpackUINT32LEInc(BYTE** ppBuf, UINT32* pulLen, UINT32* pulVal);
// Unpack an unsigned 32-bit valud in big-endian format
HX_RESULT UnpackUINT24BEInc(BYTE** ppBuf, UINT32* pulLen, UINT32* pulVal);
// Unpack a UINT16 in big-endian format
HX_RESULT UnpackUINT16BE(BYTE* pBuf, UINT32 ulLen, UINT16* pusVal);
// Unpack a UINT16 in big-endian format and advance the buffer parsing
HX_RESULT UnpackUINT16BEInc(BYTE** ppBuf, UINT32* pulLen, UINT16* pusVal);
// Unpack a UINT16 in little-endian format
HX_RESULT UnpackUINT16LE(BYTE* pBuf, UINT32 ulLen, UINT16* pusVal);
// Unpack a UINT16 in little-endian format and advance the buffer parsing
HX_RESULT UnpackUINT16LEInc(BYTE** ppBuf, UINT32* pulLen, UINT16* pusVal);
// Unpack a UINT8
HX_RESULT UnpackUINT8(BYTE* pBuf, UINT32 ulLen, UINT8* pucVal);
// Unpack a UINT8 and advance the buffer parsing
HX_RESULT UnpackUINT8Inc(BYTE** ppBuf, UINT32* pulLen, UINT8* pucVal);
// Unpack a variable length (1, 2, 3, or 4 bytes) value in big-endian format
HX_RESULT UnpackVariableBEInc(BYTE** ppBuf, UINT32* pulLen, UINT32 ulNumBytes, UINT32* pulVal);
// Pack a single byte and advance the counters
HX_RESULT PackUINT8Inc(BYTE** ppBuf, UINT32* pulLen, BYTE ucVal);
// Pack a UINT16 in big-endian format
HX_RESULT PackUINT16BE(BYTE* pBuf, UINT32 ulLen, UINT16 usVal);
// Pack a UINT16 in big-endian format and advance the buffer
HX_RESULT PackUINT16BEInc(BYTE** ppBuf, UINT32* pulLen, UINT16 usVal);
// Pack a UINT16 in little-endian format
HX_RESULT PackUINT16LE(BYTE* pBuf, UINT32 ulLen, UINT16 usVal);
// Pack a UINT16 in little-endian format and advance the buffer
HX_RESULT PackUINT16LEInc(BYTE** ppBuf, UINT32* pulLen, UINT16 usVal);
// Pack a UINT32 in big-endian format
HX_RESULT PackUINT24BE(BYTE* pBuf, UINT32 ulLen, UINT32 ulVal);
// Pack a UINT32 in big-endian format and advance the buffer
HX_RESULT PackUINT24BEInc(BYTE** ppBuf, UINT32* pulLen, UINT32 ulVal);
// Pack a UINT32 in big-endian format
HX_RESULT PackUINT32BE(BYTE* pBuf, UINT32 ulLen, UINT32 ulVal);
// Pack a UINT32 in big-endian format and advance the buffer
HX_RESULT PackUINT32BEInc(BYTE** ppBuf, UINT32* pulLen, UINT32 ulVal);
// Pack a UINT32 in little-endian format
HX_RESULT PackUINT32LE(BYTE* pBuf, UINT32 ulLen, UINT32 ulVal);
// Pack a UINT32 in little-endian format and advance the buffer
HX_RESULT PackUINT32LEInc(BYTE** ppBuf, UINT32* pulLen, UINT32 ulVal);
// Pack a UINT64 in big-endian format
HX_RESULT PackUINT64BE(BYTE* pBuf, UINT32 ulLen, UINT64 ulVal);
// Unpack an 8-byte double in big-endian format
HX_RESULT UnpackDoubleBEInc(BYTE** ppBuf, UINT32* pulLen, double* pdVal);
// Pack an 8-byte double in big-endian format
HX_RESULT PackDoubleBEInc(BYTE** ppBuf, UINT32* pulLen, double dVal);
// Unpack a certain number of bits
HX_RESULT UnpackBits(BYTE** ppBuf, UINT32* pulLen, UINT32* pulBitPos, UINT32 ulNumBits, UINT32* pulVal);
// Pack a certain number of bits
HX_RESULT PackBits(BYTE** ppBuf, UINT32* pulLen, UINT32* pulBitPos, UINT32 ulNumBits, UINT32 ulVal);
// create IHXEvent from CCF
HX_RESULT CreateEventCCF(void** ppObject, IUnknown* pContext,
			 const char* pszName, HXBOOL bManualReset);

HX_RESULT CreateInstanceCCF(REFCLSID clsid, void** ppObject, IHXCommonClassFactory* pCCF);

HX_RESULT CreateInstanceCCF_QI(REFCLSID clsid, REFIID iid, void** ppItf, IHXCommonClassFactory* pCCF);

HX_RESULT CreateInstanceCCF(REFCLSID clsid, void** ppObject, IUnknown* pContext);

HX_RESULT CreateInstanceCCF_QI(REFCLSID clsid, REFIID iid, void** ppItf, IUnknown* pContext);

// Pack an array of strings together using
// the supplied separator string and put the
// resulting string into an IHXBuffer
HX_RESULT CatStringsCCF(REF(IHXBuffer*) rpOutStr, const char** ppszInStr,
                        const char* pszSeparator, IUnknown* pContext);

#endif
