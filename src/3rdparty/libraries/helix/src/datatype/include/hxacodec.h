/* ***** BEGIN LICENSE BLOCK *****

 * Source last modified: $Id: hxacodec.h,v 1.7 2006/12/08 01:41:19 ping Exp $
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

#ifndef _HXACODEC_H_
#define _HXACODEC_H_

#ifdef __MWERKS__
#pragma once
#endif

#include "hxtypes.h"
#include "hxcom.h"

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAudioEncoderConfigurator
 *
 *  Purpose:
 *
 *	Configuration Interface for RealMedia audio encoders.
 *
 *  IID_IHXAudioEncoderConfigurator
 *
 *	{B9919B52-54FF-4099-984A-533E75B578B9}
 *
 */

DEFINE_GUID(IID_IHXAudioEncoderConfigurator,
    0xb9919b52, 0x54ff, 0x4099, 0x98, 0x4a, 0x53, 0x3e, 0x75, 0xb5, 0x78, 0xb9);

/* forward declaration of IQueryDecoderUnit */
DECLARE_INTERFACE(IQueryDecoderUnit) ;

#undef  INTERFACE
#define INTERFACE IHXAudioEncoderConfigurator

DECLARE_INTERFACE_(IHXAudioEncoderConfigurator, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXAudioEncoder configuration/encoding methods
     */

    STDMETHOD_(UINT32,GetNumberOfFlavors) (THIS) CONSTMETHOD PURE ;

    enum PROPERTY_LONG {
        FLV_PROP_MIN_SAMPLES_IN, // hand me at least this many. I might take less
        FLV_PROP_MAX_BYTES_OUT,  // that's per decoder unit
        FLV_PROP_PREDATA,
	// the following are obsolete flavours. To maintain backwards compatibility and until
	// we have found a better solution, these are exported even through the new interface
	FLV_PROP_OBSOLETE_BITSPERFRAME, // FLV_PROP_BITSPERFRAME
	FLV_PROP_OBSOLETE_GRANULARITY,  // FLV_PROP_GRANULARITY
	FLV_PROP_OBSOLETE_INTERLEAVE_FACTOR, // FLV_PROP_INTERLEAVE_FACTOR
	FLV_PROP_CORE_SAMPLERATE // currently only supported by HE AAC codec
    } ;

    enum PROPERTY_FLOAT {
        FLV_PROP_BIT_RATE_APPROX // bits/second, rounded. Do not use for calculations!
    } ;

    enum PROPERTY_VOID {
        FLV_PROP_INPUT_AUDIO_FORMAT, // struct AUDIO_FORMAT
        FLV_PROP_BITS_PER_SECOND, // long[2], numerator & denominator
        FLV_PROP_BITS_PER_SAMPLE, // long[2]
        FLV_PROP_FLAVOR_NAME, // was FLV_PROP_NAME
        FLV_PROP_CODEC_NAME, // was FLV_PROP_STATUS_TEXT
        FLV_PROP_FLAVOR_DESCRIPTION, // was FLV_PROP_DESCRIPTION
        FLV_PROP_OPAQUE_DATA //void *
    } ;

    enum {
        /* use this flavor number to inquire about the properties set by SetFlavor() and
           SetOption() */
        FLV_CURRENT = -1
    } ;

    STDMETHOD(GetFlavorProperty) (THIS_
                                  INT32 flvIndex,
                                  PROPERTY_LONG p,
                                  INT32&  l) CONSTMETHOD PURE ;

    STDMETHOD(GetFlavorProperty) (THIS_
                                  INT32 flvIndex,
                                  PROPERTY_FLOAT p,
                                  float& l) CONSTMETHOD PURE ;

    /* set buflen to the amount of memory reserved for v (on input). On output, buflen will
       be changed to the amount of memory actually needed. If the area was to small (or v
       was set to 0), no copying will be done, but buflen will still be changed on output */
    STDMETHOD(GetFlavorProperty) (THIS_
                                  INT32 flvIndex,
                                  PROPERTY_VOID p,
                                  void*  v,
                                  UINT32& buflen) CONSTMETHOD PURE ;

    STDMETHOD(SetFlavor) (THIS_
                          INT32 nFlavor) PURE ;

    /*
     * hardly any of these are supported right now. This just gives you a taste of
     * what I think could possibly go in here.
     */

    enum OPTION_LONG {
        /* OPTION 		 ARGUMENT */
        OPT_2PASS_PASS1, /* outputstream */
        OPT_2PASS_PASS2, /* inputstream */
        OPT_PREDATA,	 /* nbytes, maximum BFM */
        OPT_BITRATE,	 /* bits/s */
        OPT_MAXBYTES_DECODERUNIT,  /* nbytes, max size of decoder units */
        OPT_MAXSAMPLES_DECODERUNIT,/* nbytes, max size of decoder units */
        OPT_QUALITY_LEVEL,
        OPT_ENCODE_TYPE,
		OPT_BITRATE_MANAGED	/* defaulted to on */
    } ;
    
    enum OPTION_ENCODE_TYPE {
		OPT_ENCODE_CBR,
		OPT_ENCODE_VBR_BITRATE,
		OPT_ENCODE_VBR_QUALITY
	};

    STDMETHOD(SetOption) (THIS_
                          OPTION_LONG option, INT32 arg) PURE ;

    STDMETHOD(OpenEncoder)(THIS) PURE ;

    STDMETHOD_(void,CloseEncoder)(THIS) PURE ;
} ;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAudioEncoder
 *
 *  Purpose:
 *
 *	Encoding Interface for RealMedia audio encoders.
 *
 *  IID_IHXAudioEncoder
 *
 *	{56FF66F4-A6C5-42aa-B267-C296DD075DA3}
 *
 */

DEFINE_GUID(IID_IHXAudioEncoder, 
    0x56ff66f4, 0xa6c5, 0x42aa, 0xb2, 0x67, 0xc2, 0x96, 0xdd, 0x7, 0x5d, 0xa3);

#undef  INTERFACE
#define INTERFACE IHXAudioEncoder

DECLARE_INTERFACE_(IHXAudioEncoder, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXAudioEncoder methods
     */
    
    STDMETHOD(EncodeData)(THIS_
                          void *pDataIn, UINT32 nSamplesIn,
			        	  UINT32& nSamplesConsumed,
				          IQueryDecoderUnit* dataOut[], UINT32& nDecoderUnits,
                          INT32 eof) PURE ;

#if 0
    /* submit an event for action at "time" samples out from the last encoded data.
       This function prototype is tentative */
    STDMETHOD(SendEvent) (THIS_
			  int time, IHXAudioEncoderConfigurator::OPTION_LONG option,
			  long arg) PURE ;
#endif
} ;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXCodecOldStyleAuthenticate
 *
 *  Purpose:
 *
 *	Callers authenticate themselves for encoding
 *
 *  IID_IHXCodecOldStyleAuthenticate
 *
 *  {EAE2FEC7-AC4B-4d15-B1DC-0A94549CF2C9}
 *
 */

DEFINE_GUID(IID_IHXCodecOldStyleAuthenticate,
	0xeae2fec7, 0xac4b, 0x4d15, 0xb1, 0xdc, 0xa, 0x94, 0x54, 0x9c, 0xf2, 0xc9);

#undef  INTERFACE
#define INTERFACE IHXCodecOldStyleAuthenticate

DECLARE_INTERFACE_(IHXCodecOldStyleAuthenticate, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXCodecOldStyleAuthenticate methods
     */

	STDMETHOD(SetPassword) (THIS_ const char* password) PURE ;
} ;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IQueryDecoderUnit
 *
 *  Purpose:
 *
 *	Query Interface for Decoder Unit object. Used by encoder client (Producer)
 *
 *  IID_IDecoderUnitQuery
 *
 *	{5720A4DF-7B79-47ae-8494-6B4652F2D4AD}
 *
 */

DEFINE_GUID(IID_IQueryDecoderUnit, 
    0x5720a4df, 0x7b79, 0x47ae, 0x84, 0x94, 0x6b, 0x46, 0x52, 0xf2, 0xd4, 0xad);

#undef  INTERFACE
#define INTERFACE   IQueryDecoderUnit

DECLARE_INTERFACE_(IQueryDecoderUnit, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IQueryDecoderUnit methods
     */

    /* get a pointer to this decoder unit's data, together with its length in bytes and
       the number of PCM samples it represents (when decoded). Use nSamples to do
       timestamping */
    STDMETHOD(getData)	(THIS_
				const char*& data, UINT32& nBytes, UINT32& nSamples) CONSTMETHOD PURE ;
    /* get the buffer fullness that the decoder has when this packet has been decoded. If this
       number is transmitted in the bitstream, startup time could be minimized after a seek() or
       after massive packet loss */
    STDMETHOD_(INT32,getBufferFullness) (THIS_) CONSTMETHOD PURE ;

    /*
     * decoder unit flags. 
     */

    enum FLAGS {
	ENDOFSTREAM = 1
    } ;

    /*
     * return any flags set on this decoder unit
     */
    STDMETHOD_(FLAGS, getFlags) (THIS_) CONSTMETHOD PURE ;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAudioDecoder
 *
 *  Purpose:
 *
 *	Interface for RealMedia audio decoders
 *
 *  IID_IHXAudioDecoder
 *
 *	{26139EDA-98D8-437b-9229-949D3BEF2551}
 *
 */

DEFINE_GUID(IID_IHXAudioDecoder,
    0x26139eda, 0x98d8, 0x437b, 0x92, 0x29, 0x94, 0x9d, 0x3b, 0xef, 0x25, 0x51);

#undef  INTERFACE
#define INTERFACE   IHXAudioDecoder

DECLARE_INTERFACE_(IHXAudioDecoder, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXAudioDecoder methods
     */

	/* configure the decoder. Needs to be called before starting to decode.

		cfgType: The interpretation depends on the decoder. In the case of an AAC
				decoder, for example, it tells the decoder whether it is configured
				with an ADIF header, ADTS header, or MPEG-4 AudioSpecificConfig
		hdr: a pointer to the config data.
		nBytes: the number of bytes in the config data.
	 */

    STDMETHOD(OpenDecoder) (THIS_
							UINT32 cfgType,
							const void* config,
							UINT32 nBytes) PURE ;

	/* reset the decoder to the state just after OpenDecoder(). Use this when seeking or
       when decoding has returned with an error. */
    STDMETHOD(Reset) (THIS) PURE ;

    /* tell the decoder to conceal nSamples samples. The concealed samples will be
		returned with the next Decode() call. */

    STDMETHOD(Conceal) (THIS_
						UINT32 nSamples) PURE ;
	/* Decode up to nbytes bytes of bitstream data. nBytesConsumed will be updated to
		reflect how many bytes have been read by the decoder (and can thus be discarded
		in the caller). The decoder does not necessarily buffer the bitstream; that is
		up to the caller.
		samplesOut should point to an array of INT16s, large enough to hold MaxSamplesOut()
		samples.
		
		nSamplesOut is updated to reflect the number of samples written by the decoder.

		set eof when no more data is available and keep calling the decoder until it does
		not produce any more output.
	*/
    STDMETHOD(Decode)  (THIS_
						const UCHAR* data, UINT32 nBytes, UINT32 &nBytesConsumed, INT16 *samplesOut, UINT32& nSamplesOut, HXBOOL eof) PURE ;
	/*
		Upper limit on the number of samples the decoder will produce per call. A typical
		value would be 2048.
	*/
    STDMETHOD(GetMaxSamplesOut)   (THIS_
								UINT32& nSamples) CONSTMETHOD PURE ;

	/*
		The number of channels of audio in the audio stream.
		This is valid after the OpenDecoder() call.
	*/
	STDMETHOD(GetNChannels) (THIS_
								UINT32& nChannels) CONSTMETHOD PURE ;

	/*
		The sample rate of the audio stream.
		This is valid after the OpenDecoder() call.
	*/
	STDMETHOD(GetSampleRate) (THIS_
								UINT32& sampleRate) CONSTMETHOD PURE ;

	/*
        The codec delay in samples.
        To go from samples to time, divide by (nChannels*sampleRate).
	*/
    STDMETHOD(GetDelay) (THIS_
								UINT32& nSamples) CONSTMETHOD PURE ;
} ;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAudioDecoderRenderer
 *
 *  Purpose:
 *
 *	Interface for HW audio Decoder/Device that requires time information
 *
 *  IID_IHXAudioDecoderRenderer
 *
 *	{26139EDA-98D8-437b-9229-949D3BEF2558}
 *
 */

DEFINE_GUID(IID_IHXAudioDecoderRenderer,
    0x26139eda, 0x98d8, 0x437b, 0x92, 0x29, 0x94, 0x9d, 0x3b, 0xef, 0x25, 0x58);

#undef  INTERFACE
#define INTERFACE   IHXAudioDecoderRenderer

DECLARE_INTERFACE_(IHXAudioDecoderRenderer, IHXAudioDecoder)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXAudioDecoder methods
     */

	/* configure the decoder. Needs to be called before starting to decode.

		cfgType: The interpretation depends on the decoder. In the case of an AAC
				decoder, for example, it tells the decoder whether it is configured
				with an ADIF header, ADTS header, or MPEG-4 AudioSpecificConfig
		hdr: a pointer to the config data.
		nBytes: the number of bytes in the config data.
	 */

    STDMETHOD(OpenDecoder) (THIS_
							UINT32 cfgType,
							const void* config,
							UINT32 nBytes) PURE ;

	/* reset the decoder to the state just after OpenDecoder(). Use this when seeking or
       when decoding has returned with an error. */
    STDMETHOD(Reset) (THIS) PURE ;

    /* tell the decoder to conceal nSamples samples. The concealed samples will be
		returned with the next Decode() call. */

    STDMETHOD(Conceal) (THIS_
						UINT32 nSamples) PURE ;
	/* Decode up to nbytes bytes of bitstream data. nBytesConsumed will be updated to
		reflect how many bytes have been read by the decoder (and can thus be discarded
		in the caller). The decoder does not necessarily buffer the bitstream; that is
		up to the caller.
		samplesOut should point to an array of INT16s, large enough to hold MaxSamplesOut()
		samples.
		
		nSamplesOut is updated to reflect the number of samples written by the decoder.

		set eof when no more data is available and keep calling the decoder until it does
		not produce any more output.
	*/
    STDMETHOD(Decode)  (THIS_
						const UCHAR* data, UINT32 nBytes, UINT32 &nBytesConsumed, INT16 *samplesOut, UINT32& nSamplesOut, HXBOOL eof) PURE ;
	/*
		Upper limit on the number of samples the decoder will produce per call. A typical
		value would be 2048.
	*/
    STDMETHOD(GetMaxSamplesOut)   (THIS_
								UINT32& nSamples) CONSTMETHOD PURE ;

	/*
		The number of channels of audio in the audio stream.
		This is valid after the OpenDecoder() call.
	*/
	STDMETHOD(GetNChannels) (THIS_
								UINT32& nChannels) CONSTMETHOD PURE ;

	/*
		The sample rate of the audio stream.
		This is valid after the OpenDecoder() call.
	*/
	STDMETHOD(GetSampleRate) (THIS_
								UINT32& sampleRate) CONSTMETHOD PURE ;

	/*
        The codec delay in samples.
        To go from samples to time, divide by (nChannels*sampleRate).
	*/
    STDMETHOD(GetDelay) (THIS_
								UINT32& nSamples) CONSTMETHOD PURE ;

    /*
     *	IHXAudioDecoderRenderer methods
     */							
    
    /*
        Set rendering start time
     */
    STDMETHOD(SetStartTime) (THIS_
                                UINT32 ulStartTime) PURE;

    /*
        Set next (audio) frame time
     */
    STDMETHOD(SetNextFrameTime) (THIS_
                                 UINT32 ulFrameTime) PURE ;

} ;


#ifdef __cplusplus
extern "C" {
#endif

/* we will eventually get rid of the distinction between CreateEncoderInstance() and CreateDecoderInstance(),
   merging them into a common CreateInstance(). */

typedef HX_RESULT (HXEXPORT_PTR FPRACreateEncoderInstance) (const CLSID &clsid, IUnknown** ppUnknown) ;
// need this for static linking under Unix
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RACreateEncoderInstance) (const CLSID &clsid, IUnknown** ppUnknown) ;
typedef HX_RESULT (HXEXPORT_PTR FPRACreateDecoderInstance) (const CLSID &clsid, IUnknown** ppUnknown) ;
// need this for static linking under Unix
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RACreateDecoderInstance) (const CLSID &clsid, IUnknown** ppUnknown) ;

#ifdef __cplusplus
}
#endif

#endif // _HXACODEC_H_
