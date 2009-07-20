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

#ifndef __HXCODECEX_H__
#define __HXCODECEX_H__

///////////////////////////////////////////////////////////////////////////////
//
//	DESCRIPTION:
//		Class description for RV codec extensions
//
////////////////////
//
//	HISTORY:
//
// $Id: hxcodecex.h,v 1.3 2005/03/14 19:15:56 bobclark Exp $
//

#define IID_IHX20StreamEx	0x00000003 // IID_IHX20Stream + 1

typedef struct tag_HXStreamPreview
{
	UCHAR * pPreviewImage;
	ULONG32 timestamp;
	UCHAR lastPacket;
} HXSTREAM_PREVIEW_OUTPARAMS;

typedef HX_RESULT (HXEXPORT_PTR FP_STREAM_PREVIEW)(HXSTREAM streamRef, 
	HXSTREAM fromStreamRef, HXSTREAM_PREVIEW_OUTPARAMS *pOutParams);
typedef HX_RESULT (HXEXPORT_PTR FP_STREAM_SETBOOL)(HXSTREAM streamRef, 
	const CHAR* key, HXBOOL value);
typedef HX_RESULT (HXEXPORT_PTR FP_STREAM_SETUINT)(HXSTREAM streamRef, 
	const CHAR* key, UINT32 value);
typedef HX_RESULT (HXEXPORT_PTR FP_STREAM_SETDOUBLE)(HXSTREAM streamRef, 
	const CHAR* key, double value);
typedef HX_RESULT (HXEXPORT_PTR FP_STREAM_SETSTRING)(HXSTREAM streamRef, 
	const CHAR* key, const CHAR* value);


class IHX20StreamEx : public IHX20Stream
{
public:
	virtual HX_RESULT HXStream_SetPreviewCallback(HXSTREAM callbackRef, HXMEMORY memoryRef,
		FP_STREAM_PREVIEW fpDataCallback) = 0;
	virtual HX_RESULT HXStream_SetPropertyBool(const CHAR* key, HXBOOL value) = 0;
	virtual HX_RESULT HXStream_SetPropertyUint(const CHAR* key, UINT32 value) = 0;
	virtual HX_RESULT HXStream_SetPropertyDouble(const CHAR* key, double value) = 0;
	virtual HX_RESULT HXStream_SetPropertyString(const CHAR* key, const CHAR* value) = 0;
};

#define IID_IHXCodecLoadAdjustment	0x00000005  // need to move all guids to one file to avoid collision as in above
class IHXCodecLoadAdjustment : public IHXUnknown
{
public:
	virtual HX_RESULT GetLoadLevel(UINT32 *puLoadLevel) = 0;
	virtual HX_RESULT SetLoadLevel(UINT32 uLoadLevel) = 0;
};

#endif 
// __RV30EX_H__
