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

#ifndef _CCODEC_H_
#define	_CCODEC_H_ 

/****************************************************************************
 *  Includes
 */
#if defined( _WIN32 ) || defined( _WINDOWS )
#include "hlxclib/windows.h"
#endif
#include <stdlib.h>
#include "racodec.h"

// these are to set the #define for _MAX_PATH

#ifndef _MAX_PATH
#if defined ( _MACINTOSH ) || defined(_MAC_UNIX)
#include "platform/mac/maclibrary.h"
#elif defined (_UNIX)
#include <stdlib.h>
#ifndef _VXWORKS
#include <sys/param.h>
#endif
#define _MAX_PATH       MAXPATHLEN
#endif
#endif // _MAX_PATH

// Use DLLAccess instead of LoadLibrary, FreeLibrary, GetProcAddress
// 07/24/98 JBH
class DLLAccessBridge;

#define CODEC_ID_LENGTH 4
#define _MAX_DLL_NAME_LENGTH 256

// Add Cookie API
typedef void*	RACODEC;

// New Exportable Functions
typedef HX_RESULT (HXEXPORT_PTR RA_OPEN_CODEC) (RACODEC* codecRef); // preview compatibility -  do NOT use
typedef HX_RESULT (HXEXPORT_PTR RA_OPEN_CODEC2) (RACODEC* codecRef, const char* pCodecPath);
typedef HX_RESULT (HXEXPORT_PTR RA_CLOSE_CODEC) (RACODEC codecRef);
typedef HX_RESULT (HXEXPORT_PTR RA_SET_PASSWORD) (RACODEC codecRef, const char* password);
typedef UINT16    (HXEXPORT_PTR RA_GET_NUM_FLAVORS) (RACODEC codecRef); // no error returned. zero flavors means there's a problem.
typedef void*     (HXEXPORT_PTR RA_GET_FLAVOR_PROPERTY) (RACODEC codecRef, UINT16 flavorNum, UINT16 propNum, UINT16* propSize); // use IHXValues?
typedef HX_RESULT (HXEXPORT_PTR RA_SET_FLAVOR) (RACODEC codecRef, UINT16 flavorNum);

typedef HX_RESULT (HXEXPORT_PTR RA_INIT_ENCODER) (RACODEC codecRef, void* params);
typedef HX_RESULT (HXEXPORT_PTR RA_ENCODE) (RACODEC codecRef, UINT16* inBuf, Byte* outBuf);
typedef void (HXEXPORT_PTR RA_FREE_ENCODER) (RACODEC codecRef);

typedef HX_RESULT (HXEXPORT_PTR RA_INIT_DECODER) (RACODEC codecRef, void* params);
typedef HX_RESULT (HXEXPORT_PTR RA_DECODE) (RACODEC codecRef, Byte* inBuf, UINT32 inLength,
					 Byte* outBuf, UINT32* outLength, UINT32 userData);
typedef HX_RESULT (HXEXPORT_PTR RA_FLUSH) (RACODEC codecRef, Byte* outBuf, UINT32* outLength);
typedef void (HXEXPORT_PTR RA_FREE_DECODER) (RACODEC codecRef);

typedef void (HXEXPORT_PTR RA_GET_BACKEND) (RACODEC codecRef, void*** ppFuncList);
typedef void (HXEXPORT_PTR RA_GET_GUID) (UCHAR* pGUID);
typedef void (HXEXPORT_PTR RA_GET_DECODE_GUID) (RACODEC codecRef, UCHAR* pGUID);
typedef HX_RESULT (HXEXPORT_PTR RA_GO_SECURE) (RACODEC codecRef);


class CCodec 
{
public:

  /* class constructor and destructor */
	CCodec(char * pCodecID, 
	       IUnknown* pContext = NULL,
	       HXBOOL bDummy = FALSE);	    // Reserved
	virtual ~CCodec ();

  // Loads codec's DLL
	virtual HX_RESULT   LoadCodec();

  // Unloads codec's DLL
	virtual void       FreeCodec();

    // Encode & Decode functions
	UINT16	    GetNumberOfFlavors (void);
	// GetFlavorProperty returns a point to the property requested and
	// sets size of the property in pPropertySize. The pointer returned
	// is valid until the next GetFlavorProperty is called or until the
	// destructor of the "CODEC" object is called.
	void*	    GetFlavorProperty (UINT16 flavorIndex, UINT16 flavorProperty, UINT16* pPropertySize);
	HX_RESULT   SetFlavor (UINT16 flavorIndex);
	UINT16	    GetCurrentFlavor (void) { return mCurrentFlavor; };

	char*	    GetID (void) { return mCodecID; };
	HX_RESULT   GetLastError (void) { return mLastError; };

    // Encoder Functions
	// platform dependent part of the init function
	HX_RESULT   InitEncoder (RAENCODER_INIT_PARAMS* params);
	// Encodes data from input buffer to output buffer.
	HX_RESULT   Encode (Byte* pInBuffer, Byte* pOutBuffer);
	// platform dependent part of the free function
	void	    FreeEncoder (void);

    // Decoder Functions
	// plaform dependent part of the init function
	HX_RESULT   InitDecoder (RADECODER_INIT_PARAMS* params, HXBOOL bSwitchable);
	// Decodes inLength bytes of data from input buffer, using userData if needed.
	// Returns outLength bytes of data in output buffer.
	virtual HX_RESULT   Decode (Byte* pInBuffer, UINT32 inLength, Byte* pOutBuffer, UINT32* pOutLength, UINT32 userData);
	// Flushes decoder stream and returns data (if available).
	// Called at seek, pause, or end of stream.
	HX_RESULT   Flush (Byte* pOutBuffer, UINT32* pOutLength);
	// platform dependent part of the free function
	virtual void	    FreeDecoder (void);

	HX_RESULT	    InitCodec(HXBOOL bWithVersion = FALSE);

protected:
#ifdef _WINDOWS
	HXBOOL       		IsFileExist(const char * DecDllPath);
	HXBOOL       		VerifyCodec();
#endif
	IUnknown*		m_pContext;
	char*			m_pszFullDllPath;

	DLLAccessBridge*	mpDLLAccess;

#ifdef _MACINTOSH
	short			DllVrefNum;
	LONG32	 		DllParID;
#endif

	HX_RESULT		mLastError;         // last error occured
	HXBOOL       		mEncoderInited;     // Initialization flag
	HXBOOL       		mDecoderInited;     // Initialization flag
	char       		mCodecID[CODEC_ID_LENGTH + 1];/* Flawfinder: ignore */  // codec ID 
	char			mDLLName[_MAX_DLL_NAME_LENGTH];

	Byte * 			curProperty;
	UINT16			mCurrentFlavor;
	RACODEC			mCodecRef;  // cookie

	RA_OPEN_CODEC2		fpOpenCodec;	// only support NEW OpenCodec API w/ codecDir path
	RA_CLOSE_CODEC		fpCloseCodec;
	RA_SET_PASSWORD		fpSetPassword;
	RA_GET_NUM_FLAVORS	fpGetNumFlavors;
	RA_GET_FLAVOR_PROPERTY	fpGetFlavorProperty;
	RA_SET_FLAVOR		fpSetFlavor;
	RA_INIT_ENCODER		fpInitEncoder;
	RA_ENCODE		fpEncode;
	RA_FREE_ENCODER		fpFreeEncoder;
	RA_INIT_DECODER		fpInitDecoder;
	RA_DECODE		fpDecode;
	RA_FLUSH		fpFlush;
	RA_FREE_DECODER		fpFreeDecoder;
};
	
#endif // _CCODEC_H_





