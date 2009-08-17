/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxostrm.h,v 1.4 2005/03/14 19:27:09 bobclark Exp $
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

#ifndef _IHXOSTRM_H
#define _IHXOSTRM_H

#include "hxcom.h"

// These interfaces use HXString as one of the data primitives
// that can be written/read from a stream. Makes life easy for
// implementors of IHXStreamableObj
#include "hxstring.h"

//typedef interface IHXStreamableObj IHXStreamableObj;
typedef _INTERFACE IHXObjOutStream  IHXObjOutStream;
typedef _INTERFACE IHXObjInStream   IHXObjInStream;


// {47533471-BECE-11d1-8F09-0060083BE561}
DEFINE_GUID(IID_IHXStreamableObj, 0x47533471, 0xbece, 0x11d1, 0x8f, 0x9, 0x0, 0x60, 0x8, 0x3b, 0xe5, 0x61);


#undef  INTERFACE
#define INTERFACE IHXStreamableObj

DECLARE_INTERFACE_(IHXStreamableObj, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG32, Release) (THIS) PURE;

	// this method is used to dump a binary
	// representation of itself to the Encoder Object.
    STDMETHOD(WriteObjToBits) (THIS_ IHXObjOutStream* pIOutStream) PURE;

	// this method is used to set the object's internal state
	// from the data in the Decoder Object.
    STDMETHOD(ReadObjFromBits) (THIS_ IHXObjInStream* pIInStream) PURE;

};



// {B3A156D1-BEDD-11d1-8F0A-0060083BE561}
DEFINE_GUID(IID_IHXObjOutStream, 0xb3a156d1, 0xbedd, 0x11d1, 0x8f, 0xa, 0x0, 0x60, 0x8, 0x3b, 0xe5, 0x61);


#undef  INTERFACE
#define INTERFACE IHXObjOutStream

DECLARE_INTERFACE_(IHXObjOutStream, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(UINT32, AddRef) (THIS) PURE;
    STDMETHOD_(UINT32, Release) (THIS) PURE;

	STDMETHOD(Initialize) (THIS) PURE;	// clean out buffers, start fresh!
	
    STDMETHOD_(UINT32, WriteObj) (THIS_ IHXStreamableObj* pObj) PURE;
    STDMETHOD_(UINT32, WriteObj) (THIS_ IHXStreamableObj& pObj) PURE;
	STDMETHOD_(UINT32, WriteUCHAR) (THIS_ UCHAR nValue) PURE;
    STDMETHOD_(UINT32, WriteUINT16) (THIS_ UINT16 nValue) PURE;
    STDMETHOD_(UINT32, WriteUINT32) (THIS_ UINT32 nValue) PURE;
    STDMETHOD_(UINT32, WriteString) (THIS_ const char* szValue) PURE;
    STDMETHOD_(UINT32, WriteStringCat) (THIS_ const char* szValue1,const char* szValue2,const char* szValue3=NULL) PURE;
    STDMETHOD_(UINT32, WriteLargeString) (THIS_ const char* szValue) PURE;
    STDMETHOD_(UINT32, WriteBuffer)  (THIS_ const char* szBuffer, UINT32 nSize) PURE;

	// dump at specific positions (earlier positions than current offset)
    STDMETHOD_(UINT32, WriteUINT16At) (THIS_ UINT32 nOffset, UINT16 nValue) PURE;
    STDMETHOD_(UINT32, WriteUINT32At) (THIS_ UINT32 nOffset, UINT32 nValue) PURE;
	
	// get data and length
	STDMETHOD_(const char*, GetBuffer) (THIS) PURE;
	STDMETHOD_(UINT32, GetLength) (THIS) PURE;

	// get end of raw data buffer - used when dumping bits into the
	// stream and you want to know the current position in the stream
	STDMETHOD_(UINT32, GetOffset) (THIS) PURE;
};




// {B3A156D1-BEDD-11d1-8F0A-0060083BE561}
DEFINE_GUID(IID_IHXObjInStream, 0xb3a156d2, 0xbedd, 0x11d1, 0x8f, 0xa, 0x0, 0x60, 0x8, 0x3b, 0xe5, 0x61);

#undef  INTERFACE
#define INTERFACE IHXObjInStream

DECLARE_INTERFACE_(IHXObjInStream, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG32, Release) (THIS) PURE;

	
	STDMETHOD(Initialize) (THIS) PURE;
	STDMETHOD(Initialize) (THIS_ UCHAR* buf, UINT32 nLen) PURE;
	
    STDMETHOD_(UINT32, ReadObj) (THIS_ IHXStreamableObj* pObj) PURE;
    STDMETHOD_(UINT32, ReadObj) (THIS_ IHXStreamableObj& pObj) PURE;
	
    STDMETHOD_(UINT32, ReadUCHAR)  (THIS_ UCHAR& nValue) PURE;
    STDMETHOD_(UINT32, ReadUINT16) (THIS_ UINT16& nValue) PURE;
    STDMETHOD_(UINT32, ReadUINT32) (THIS_ UINT32& nValue) PURE;
    STDMETHOD_(UINT32, ReadString) (THIS_ CHXString& strValue) PURE;
	STDMETHOD_(UINT32, ReadAndAllocCString) (THIS_ char*& pszValue) PURE;
    STDMETHOD_(UINT32, ReadLargeString) (THIS_ CHXString& strValue) PURE;
    STDMETHOD_(UINT32, ReadAndAllocLargeCString) (THIS_ char*& pszValue) PURE;
    STDMETHOD_(UINT32, ReadBuffer)  (THIS_ char* szBuffer, UINT32 nSize) PURE;

	STDMETHOD_(HXBOOL, IsEndOfData) () PURE;

	// arbitrarily change offset position.
    STDMETHOD(Seek) 		(UINT32 nPos) PURE;
    STDMETHOD(SkipForward) 	(UINT32 nAmount) PURE;
	
	STDMETHOD_(UINT32, GetOffset) (THIS) PURE;

	// get data and length
	STDMETHOD_(const char*, GetBuffer) (THIS) PURE;
	STDMETHOD_(UINT32, GetLength) (THIS) PURE;
};



#endif


