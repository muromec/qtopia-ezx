/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvviddec.cpp,v 1.2 2004/07/09 18:32:07 hubbe Exp $
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

/**********************************************************************
 *	VvVidDec.CPP   
 *
 *
 *	PUBLIC:
 *
 *		CreateInstance					- creates an object instance of this class
 *		Receive							- receives input from the input pin,
 *										  sends decompressed output to output pin
 *		CheckInputType					- called to verify media type supported by input pin
 *		CheckTransform					- called to verify media type supported by output pin
 *		DecideBufferSize				- called by output pin to set output sample buffer size
 *		StartStreaming					- called by input pin when video streaming starts
 *		StopStreaming					- called by input pin when video streaming ends
 *		GetMediaType					- returns media types supported by output pin
 *		EndOfStream						- called by output pin when end-of-stream is reached
 *		EndFlush						- called by input pin when flushing stream
 *
 *		VvVideoDecoderFilter			- constructor function
 *		~VvVideoDecoderFilter			- destructor function
 *
 *	PROTECTED:
 *
 *	PRIVATE:
 *
 *		Init							- initializes all member variables of this class
 *
 *	FILE DESCRIPTION:
 *
 *		This file contains the source code for the Vivo Video 
 *		Decompression filter class, VvVideoDecoderFilter.  This filter
 *		is a Helix decoder that is used to decompress
 *		H.263 video format data.. THIS Abstraction of the LAYER does not make any sense??
 **********************************************************************/

#include "hvviddec.h"
#include "hxcinst.h"
#include "h263codecf.h"



//*** begin VvVideoDecoderFilter class ***

/**********************************************************************
 *
 *	VvVideoDecoderFilter
 *
 *	PARAMETERS:		
 *
 *		pFilterName:	the string name for this transform filter
 *		lpUnknown:
 *		pHResult:
 *
 *	RETURNS:		None
 *
 *	DESCRIPTION:	Constructor function
 *
 *********************************************************************/
VvVideoDecoderFilter::VvVideoDecoderFilter( HX_RESULT& ps )
{	
	Init();	// initialize the member variables of this class
	ps=HXR_OK;
}


/**********************************************************************
 *
 *	CreateInstance
 *
 *	PARAMETERS:		
 *
 *		lpUnknown:
 *		pHResult:
 *
 *	RETURNS:		
 *
 *		CUnknown*:
 *
 *	DESCRIPTION:	
 *
 *		This function is used to create an object instance of this
 *		class that will be registered in the system registry.
 *
 *********************************************************************/
VvVideoDecoderFilter*
VvVideoDecoderFilter::CreateInstance(HX_RESULT& ps)
{
	VvVideoDecoderFilter	*pcVideoFilter	= NULL;	// an instance of this transform filter

    // try to create an instance of this transform filter class
	pcVideoFilter	= new VvVideoDecoderFilter(ps);

	return( pcVideoFilter );
}

/**********************************************************************
 *
 *	~VvVideoDecoderFilter
 *
 *	PARAMETERS:		None
 *
 *	RETURNS:		None
 *
 *	DESCRIPTION:	Destructor function
 *
 *********************************************************************/
VvVideoDecoderFilter::~VvVideoDecoderFilter( void )
{
	if ( NULL != m_pcVideoCodec )	// if the video data stream was created 
	{
		delete m_pcVideoCodec;		// delete the audio data stream
		m_pcVideoCodec = NULL;		// clear its pointer
	}
}


/**********************************************************************
 *
 *	Init
 *
 *	PARAMETERS:		None
 *
 *	RETURNS:		None
 *
 *	DESCRIPTION:	
 *
 *		This function is used to initialize all the member variables
 *		of this class.
 *
 *********************************************************************/
void 
VvVideoDecoderFilter::Init( void )
{
	m_pcVideoCodec		= NULL; 
	m_bStartDecompress	= FALSE;
}

/**********************************************************************
 *
 *	Receive
 *
 *	PARAMETERS:		
 *		
 *		piInSample:	the input sample that is passed into the input pin
 *
 *	RETURNS:		
 *
 *		HRESULT:	Returns NOERROR if this function is successful,
 *					otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function gets called from the input pin whenever any
 *		sample data gets sent to the pin.  The sample data is then
 *		processed and delivered to the output pin.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::Receive(
     const HXVA_Image &src,
           HXVA_Image &dst
)
{
// this call is made to lock the critical section guarding the receiving 
// thread; critical sections need to be locked in order to protect data 
// during multithreading operations
//CAutoLock	cReceiveLock( &m_csReceive );
	HX_RESULT		lResult	= HXR_OK;		// checks the return value of called functions

	if ( NULL == m_pcVideoCodec )			// if the video codec instance is invalid
		return( HXR_FAIL );					// exit this function 

	// if the video decompression process has not been started yet
	if ( m_bStartDecompress == FALSE )
	{
		// try to start the video decompression process
		lResult = m_pcVideoCodec->DecompressStart( src.format, dst.format );
		if ( lResult != HXR_OK )		// if the decompression process could not be started
			return( HXR_FAIL );			// return an error

		m_bStartDecompress = TRUE;		// indicate that the video decompression has been started
	}

	// try to decompress the current video frame
	lResult = m_pcVideoCodec->DecompressConvert( src, dst );
	if ( lResult != HXR_OK )			// if the video frame could not be decompressed
		return( HXR_FAIL );				// return an error

	//piOutSample->SetTime( (REFERENCE_TIME*)&tStart,
	//					  (REFERENCE_TIME*)&tStop );

	return( HXR_OK );
} 


/**********************************************************************
 *
 *	DecideBufferSize
 *
 *	PARAMETERS:		
 *
 *		piAllocator:	the allocater used to allocate the required 
 *						output sample buffer memory
 *		psProperties:	contains the actual buffer sizes required by
 *						the output sample
 *
 *	RETURNS:		
 *
 *		HRESULT:	Returns NOERROR if this function is successful,
 *					otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function is called from the output pin to allocate the
 *		memory required to hold the output pin data stream.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::DecideBufferSize(void)
{
	return( HXR_OK );
}


/**********************************************************************
 *
 *	StartStreaming
 *
 *	PARAMETERS:		None
 *
 *	RETURNS:		
 *
 *		HRESULT:	Returns NOERROR if this function is successful,
 *					otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function gets called by the base filter classes whenever
 *		this filter is in the process of switching to active mode,
 *		paused or running.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::StartStreaming(
	const HXVA_Image_Format &src_fmt,
    const HXVA_Image_Format &dst_fmt
	) 
{
// this call is made to lock the critical section guarding this filter 
// critical sections need to be locked in order to protect data during 
// multithreading operations
//CAutoLock	cFilterLock( &m_csFilter );

	HX_RESULT	lResult	= HXR_OK;	// checks the return of called functions

	// try to load the video codec into global memory
	lResult	= TheCodec().Load();
	if ( lResult != HXR_OK )				// if it could not be loaded
		return( HXR_FAIL );			// return an error

	TheCodec().Enable();			// enable the video codec for use	

	// GNEEL: SEND DOWN FID
	m_pcVideoCodec = TheCodec().OpenInstance( );

	if ( NULL == m_pcVideoCodec )	// if the video codec instance could not be created
		return( HXR_OUTOFMEMORY );	// return an out of memory error

	m_input_format = src_fmt;
	m_output_format = dst_fmt;
	return( HXR_OK );
}


/**********************************************************************
 *
 *	StopStreaming
 *
 *	PARAMETERS:		None
 *
 *	RETURNS:		
 *
 *		HRESULT:	Returns NOERROR if this function is successful,
 *					otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function gets called by the base filter classes whenever
 *		this transform filter is in the process of leaving active mode
 *		and entering stopped mode.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::StopStreaming( void )
{
// this call is made to lock the critical sections guarding this filter 
// and receiving thread; critical sections need to be locked in order to
// protect data during multithreading operations
//CAutoLock	cFilterLock( &m_csFilter ),
//			cReceiveLock( &m_csReceive );

	if ( NULL == m_pcVideoCodec )
		return( HXR_FAIL );

	//end compression before deleting the pointer, otherwise codec stays open
	HX_RESULT	lResult	= HXR_OK;	// checks the return of called functions
	if ( m_bStartDecompress == TRUE )	{
		lResult = m_pcVideoCodec->DecompressEnd();
		if ( lResult == HXR_OK )	{
			m_bStartDecompress = FALSE;
		}
	}

	// terminate the video codec instance to remove it from memory
	TheCodec().CloseInstance( (HXH263CodecInstance*) m_pcVideoCodec );
	m_pcVideoCodec	= NULL;

	TheCodec().Disable();

	TheCodec().Free();

	return( HXR_OK );
}


/**********************************************************************
 *
 *	EndOfStream
 *
 *	PARAMETERS:		None
 *
 *	RETURNS:		
 *
 *		HRESULT:	Returns NOERROR if this function is successful,
 *					otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function gets called from the output pin whenever
 *		the output data has reached the end of the stream.  If this
 *		function is overridden, the end-of-stream notification must
 *		be passed to the next filter.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::EndOfStream( void )
{
// this call is made to lock the critical section guarding the receiving 
// thread; critical sections need to be locked in order to protect data 
// during multithreading operations
//CAutoLock cReceiveLock(&m_csReceive);

    if ( NULL == m_pcVideoCodec ) 
	{
        return HXR_FAIL;
    }

	// return the end-of-stream notification to the next filter that
	// is connected to this filter
    return HXR_OK;
}


/**********************************************************************
 *
 *	EndFlush
 *
 *	PARAMETERS:		None
 *
 *	RETURNS:		
 *
 *		HRESULT:	Returns NOERROR if this function is successful,
 *					otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function gets called from the input pin whenever the
 *		pin is leaving the flushing state.  If this function is 
 *		overridden, the notification must be passed to the next filter.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::EndFlush( void )
{
// this call is made to lock the critical section guarding the receiving 
// thread; critical sections need to be locked in order to protect data 
// during multithreading operations
//CAutoLock cReceiveLock(&m_csReceive);

	// return the flushing notification to the next filter that is
	// connected to this filter
    return HXR_OK;
}

/**********************************************************************
 *
 *	GetMediaType
 *
 *	PARAMETERS:		
 *
 *		nPosition:	the position of the media type in the media type list
 *		pcMediaType:the returned output pin media type
 *
 *	RETURNS:		
 *
 *		HRESULT:	Returns NOERROR if this function is successful,
 *					otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function is called by the base filter classes to supply
 *		one of the media types that the output pin supports based on
 *		the position of the media type, nPosition, in the media type list.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::GetMediaType( void )
{
    return( HXR_OK );
}


/**********************************************************************
 *
 *	CheckInputType
 *
 *	PARAMETERS:		
 *
 *		pcInMediaType:	the media type associated with the input pin
 *
 *	RETURNS:		
 *
 *		HRESULT:		Returns NOERROR if this function is successful,
 *						otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function gets called by the base filter classes to 
 *		verify that the input pin supports the input media type
 *		and to propose an output pin media type based on the 
 *		input pin media type.
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::CheckInputType( void )
{
	return HXR_OK;
}


/**********************************************************************
 *
 *	CheckTransform
 *
 *	PARAMETERS:		
 *
 *		pcInMediaType:	the media type associated with the input pin
 *		pcOutMediaType:	the media type associated with the output pin
 *
 *	RETURNS:		
 *
 *		HRESULT:		Returns NOERROR if this function is successful,
 *						otherwise returns the error that occurred.
 *
 *	DESCRIPTION:	
 *
 *		This function gets called by the base filter classes to verify
 *		that th input and output pins support their media types
 *
 *********************************************************************/
HX_RESULT
VvVideoDecoderFilter::CheckTransform( void )
{
    return HXR_OK;
}
//*** end VvVideoDecoderFilter class ***

