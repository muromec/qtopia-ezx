/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved. 
 *	
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.	You may not use this file except in 
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

#include "hlxclib/windows.h"
#include "stdafx.h"

#include "resource.h"

#include "CHXAudioDeviceHook.h"
#include "hxausvc.h"
#include "hxbuffer.h"
#include "pckunpck.h"

#include <mmreg.h>

/////////////////////////////////////////////////////////////////////////////
// CHXAudioDeviceHookBase

///////////////////////
//
// CHXAudioDeviceHookBase::CHXAudioDeviceHookBase
//
//  Constructor for CHXAudioDeviceHookBase.
//
CHXAudioDeviceHookBase::CHXAudioDeviceHookBase()
{
    // Initialization
    m_fDirty = TRUE;
    m_fInitialized = FALSE;

    // These variables will get reintialized when input type is set
    m_nChannels = 0;
    m_wBitsPerSample = 0;
    m_nSamplesPerSec = 0;

    m_pContext = NULL;
    m_pHook = NULL;
};


///////////////////////
//
// CHXAudioDeviceHookBase::~CHXAudioDeviceHookBase
//
//  Destructor for CHXAudioDeviceHookBase.
//
CHXAudioDeviceHookBase::~CHXAudioDeviceHookBase()
{
    // Free streaming resources in case it hasn't been done by the client
    InternalFreeStreamingResources();

    HX_RELEASE( m_pContext );
    HX_RELEASE( m_pHook );
};


///////////////////////
//
// CHXAudioDeviceHookBase::UpdateStatesInternal
//
//  Update internal states
//
HRESULT CHXAudioDeviceHookBase::UpdateStatesInternal()
{
    HRESULT hr = S_OK;

// TODO: Update DMO states here

    return hr;
}


///////////////////////////////
//
// IMediaObjectInPlace Methods
//

///////////////////////
//
// IMediaObjectInPlace::Process
//
//  The Process method processes a block of data. The application supplies a
//  pointer to a block of input data. The DMO processes the data in place.
//
//  Parameters
//
//      ulSize
//          [in] Size of the data, in bytes.
//
//      pData
//          [in, out] Pointer to a buffer of size ulSize. On input, the buffer
//          holds the input data. If the method returns successfully, the
//          buffer contains the output data.
//
//      refTimeStart
//          [in] Start time of the data.
//
//      dwFlags
//          [in] Either DMO_INPLACE_NORMAL or DMO_INPLACE_ZERO. See Remarks
//          for more information.
//
//  Return Value
//      S_FALSE Success. There is still data to process.
//      S_TRUE Success. There is no remaining data to process.
//      E_FAIL Failure.
//
//  If the method fails, the buffer might contain garbage. The application
//  should not use the contents of the buffer.
//
//  The DMO might produce output data beyond the length of the input data. This
//  is called an effect tail. For example, a reverb effect continues after the
//  input reaches silence. If the DMO has an effect tail, this method returns
//  S_FALSE.
//
//  While the application has input data for processing, call the Process
//  method with the dwFlags parameter set to DMO_INPLACE_NORMAL. If the last
//  such call returns S_FALSE, call Process again, this time with a zeroed input
//  buffer and the DMO_INPLACE_ZERO flag. The DMO will now fill the zeroed buffer
//  with the effect tail. Continue calling Process in this way until the return
//  value is S_TRUE, indicating that the DMO has finished processing the effect
//  tail.
//
//  If the DMO has no effect tail, this method always returns S_TRUE (or an error code).
//
HRESULT CHXAudioDeviceHookBase::Process(ULONG ulSize, BYTE *pData, REFERENCE_TIME refTimeStart, DWORD dwFlags)
{

// TODO: Modify implementation of Process() if necessary

    HRESULT hr = S_OK;

    if (!m_fInitialized)
    {
        hr = AllocateStreamingResources();
    }

    if (SUCCEEDED(hr))
    {
        // Process the data
        hr = DoProcess(pData, pData, ulSize / WaveFormat()->nBlockAlign);
    }
    return hr;
}


/////////////////////
//
//  IMediaObjectInPlace::Clone
//
//  The Clone method creates a copy of the DMO in its current state.
//
//  Parameters
//
//      ppMediaObject
//          [out] Address of a pointer to receive the new DMO's
//          IMediaObjectInPlace interface.
//
//  Return Value
//      Returns S_OK if successful. Otherwise, returns an HRESULT value
//      indicating the cause of the error.
//
//  If the method succeeds, the IMediaObjectInPlace interface that it returns
//  has an outstanding reference count. Be sure to release the interface when
//  you are finished using it.
//
HRESULT CHXAudioDeviceHookBase::Clone(IMediaObjectInPlace **ppMediaObject)
{
    // Check the input pointer
    if (!ppMediaObject)
    {
        return E_POINTER;
    }

    // This will be cleaned up when client releases the newly created object
    // or if there's some error along the way
    CHXAudioDeviceHookBase * pNewHXAudioDeviceHook = new CComObject<CHXAudioDeviceHookBase>;
    if( !pNewHXAudioDeviceHook )
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;

    hr = pNewHXAudioDeviceHook->UpdateStatesInternal();

    IMediaObject * pCloned = NULL;
    if( SUCCEEDED( hr ) )
    {
        IUnknown *pUnk;
        hr = pNewHXAudioDeviceHook->QueryInterface( IID_IUnknown, (void **) &pUnk );
        if( SUCCEEDED( hr ) )
        {
            hr = pUnk->QueryInterface( IID_IMediaObject, (void **) &pCloned );
            HX_RELEASE(pUnk);
        }
    }


    // Copy the input and output types
    if (SUCCEEDED(hr))
    {
        DMO_MEDIA_TYPE mt;
        DWORD cInputStreams = 0;
        DWORD cOutputStreams = 0;
        GetStreamCount(&cInputStreams, &cOutputStreams);

        for (DWORD i = 0; i < cInputStreams && SUCCEEDED(hr); ++i)
        {
            hr = GetInputCurrentType(i, &mt);
            if (hr == DMO_E_TYPE_NOT_SET)
            {
                hr = S_OK; // great, don't need to set the cloned DMO
            }
            else if (SUCCEEDED(hr))
            {
                hr = pCloned->SetInputType(i, &mt, 0);
                MoFreeMediaType( &mt );
            }
        }

        for (DWORD i = 0; i < cOutputStreams && SUCCEEDED(hr); ++i)
        {
            hr = GetOutputCurrentType(i, &mt);
            if (hr == DMO_E_TYPE_NOT_SET)
            {
                hr = S_OK; // great, don't need to set the cloned DMO
            }
            else if (SUCCEEDED(hr))
            {
                hr = pCloned->SetOutputType(i, &mt, 0);
                MoFreeMediaType( &mt );
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pCloned->QueryInterface(IID_IMediaObjectInPlace, (void**)ppMediaObject);
        }

        // Release the object's original ref.  If clone succeeded (made it through QI) then returned pointer
        // has one ref.  If we failed, refs drop to zero, freeing the object.
        HX_RELEASE(pCloned);
    }

    // Something went wrong, clean up for client
    if (FAILED(hr))
    {
        delete pNewHXAudioDeviceHook;
    }

    return hr;
}


////////////////////////////
//
//  IMediaObjectInPlace::GetLatency
//
//  The GetLatency method retrieves the latency introduced by this DMO.
//
//  Parameters
//
//      pLatencyTime
//          [out] Pointer to a variable that receives the latency, in
//          100-nanosecond units.
//
//  Return Value
//      Returns S_OK if successful. Otherwise, returns an HRESULT value
//      indicating the cause of the error.
//
//  This method returns the average time required to process each buffer.
//  This value usually depends on factors in the run-time environment, such
//  as the processor speed and the CPU load. One possible way to implement
//  this method is for the DMO to keep a running average based on historical
//  data.
//
HRESULT CHXAudioDeviceHookBase::GetLatency(REFERENCE_TIME *pLatencyTime)
{
    // Check the input arguments;
    if (!pLatencyTime)
    {
        return E_POINTER;
    }

// TODO: Calculate some reasonable average latency if this DMO is going to
//       be used with DirectShow. For now, 5 is used as the default latency.

    *pLatencyTime= static_cast<REFERENCE_TIME>(5);

    return S_OK;
}


////////////////////////////////
//
// IMediaObjectImpl Methods
//

///////////////////////////////
//
//  IMediaObjectImpl::InternalAllocateStreamingResources
//
//  *** Called by AllocateStreamingResources, description below ***
//
//  The AllocateStreamingResources method allocates any resources needed by
//  the DMO. Calling this method is always optional.
//
//  An application can call this method as a streaming optimization. It gives
//  the DMO an opportunity to perform any time-consuming initializations
//  before streaming begins. If you call this method, do so after you set
//  the media types on the DMO, but before you make the first calls to
//  ProcessInput or ProcessOutput.
//
//  This method is optional in the following sense:
//
//  *  If the DMO does not support this method, the method returns S_OK.
//
//  *  If the application never calls this method, the DMO allocates resources
//     within a call to ProcessInput or ProcessOutput or Process.
//
//  If the DMO supports this method, it should also support the
//  FreeStreamingResources method.
//
//  Note:
//
//  The template keeps a private flag that indicates whether this method has
//  been called. If the method is called when the flag is already TRUE, it
//  returns S_OK without calling the InternalAllocateStreamingResources
//  method. The FreeStreamingResources method resets the flag to FALSE.
//
HRESULT CHXAudioDeviceHookBase::InternalAllocateStreamingResources(void)
{
    HRESULT hr = S_OK;

// TODO: Our initialization should be done here.
// TODO: If necessary, allocate resources needed to process the current input type

    if( SUCCEEDED(hr) )
    {
        hr = UpdateStatesInternal();
        m_fInitialized = TRUE;
    }

    return hr;
}


////////////////////////////////
//
// IMediaObjectImpl::InternalDiscontinuity
//
//  *** Called by Discontinuity, description below ***
//
// The Discontinuity method signals a discontinuity on the specified input
// stream.
//
// Possible Return Values:
//  S_OK                        Success
//  DMO_E_INVALIDSTREAMINDEX    Invalid streamindex
//
// A discontinuity represents a break in the input. A discontinuity might
// occur because no more data is expected, the format is changing, or there
// is a gap in the data. After a discontinuity, the DMO does not accept further
// input on that stream until all pending data has been processed. The
// application should call the ProcessOutput method until none of the streams
// returns the DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE flag.
//
HRESULT CHXAudioDeviceHookBase::InternalDiscontinuity(DWORD dwInputStreamIndex)
{

// TODO: Implement Discontinuity if necessary

    return S_OK;
}


/////////////////////////
//
//  IMediaObjectImpl::InternalFlush
//
//  *** Called by Flush, description below ***
//
//  The Flush method flushes all internally buffered data.
//
// Return Value:
// Returns S_OK if successful. Otherwise, returns an HRESULT value indicating
// the cause of the error.
//
//  The DMO performs the following actions when this method is called:
//  *  Releases any IMediaBuffer references it holds.
//
//  *  Discards any values that specify the time stamp or sample length for a
//     media buffer.
//
//  *  Reinitializes any internal states that depend on the contents of a
//     media sample.
//
//  Media types, maximum latency, and locked state do not change.
//
//  When the method returns, every input stream accepts data. Output streams
//  cannot produce any data until the application calls the ProcessInput method
//  on at least one input stream.
//
//  Note:
//
//  The template keeps a private flag that indicates the object's flushed
//  state. The Flush method sets the flag to TRUE, and the ProcessInput method
//  resets it to FALSE. If Flush is called when the flag is already TRUE, the
//  method returns S_OK without calling the InternalFlush method.
//
HRESULT CHXAudioDeviceHookBase::InternalFlush(void)
{

// TODO: Change implementation if necessary

    // Right now, just clear out the buffer
    m_pBuffer = NULL;

    return S_OK;
}


////////////////////
//
// IMediaObjectImpl::InternalFreeStreamingResources
//
//  *** Called by FreeStreamingResources, description below ***
//
// The FreeStreamingResources method frees resources allocated by the DMO.
// Calling this method is always optional.
//
// Return Value
//
// Returns S_OK if successful. Otherwise, returns an HRESULT value indicating
// the cause of the error.
//
// This method releases any resources that the AllocateStreamingResources
// method initializes.
//
// If the DMO does not support this method, the method returns S_OK. If you
// call this method during streaming, the method fails and the DMO does not
// release any resources.
//
// Regardless of whether the method fails or succeeds, the application can
// continue to call other methods on the DMO. The DMO might need to
// re-initialize resources that were previously freed.
//
HRESULT CHXAudioDeviceHookBase::InternalFreeStreamingResources(void)
{

// TODO: Implement this function if InternalAllocateStreamingResources() was implemented

    return S_OK;
}


////////////////////////////////////////
//
//  IMediaObjectImpl::InternalGetInputMaxLatency
//
//  *** Called by GetInputMaxLatency, description below ***
//
//  The GetInputMaxLatency method retrieves the maximum latency on a specified
//   input stream.
//
//  Parameters
//   dwInputStreamIndex:    Zero-based index of an input stream on the DMO.
//   prtMaxLatency:         [out] Pointer to a variable that receives the maximum latency.
//
// Return Value
// Returns an HRESULT value. The following are possible return values.
//  S_OK                        Success
//  DMO_E_INVALIDSTREAMINDEX    Invalid stream index
//  E_FAIL                      Failure
//  E_NOTIMPL                   Not implemented. Assume zero latency
//
// The latency is the difference between a time stamp on the input stream and
// the corresponding time stamp on the output stream. The maximum latency is
// the largest possible difference in the time stamps. For a DMO, determine the
// maximum latency as follows:
//
// *  Process input buffers until the DMO can produce output.
//
// *  Process as many output buffers as possible.
//
// *  The maximum latency is the largest delta between input time stamps and
//    output time stamps (taken as an absolute value).
//
//    Under this definition, latency does not include the time that it takes
//    to process samples. Nor does it include any latency introduced by the
//    size of the input buffer.
//
// For the special case where a DMO processes exactly one sample at a time,
// the maximum latency is simply the difference in time stamps.
//
// Latency is defined only when samples have time stamps, and the time stamps
// increase or decrease monotonically. Maximum latency might depend on the
// media types for the input and output streams.
//

HRESULT CHXAudioDeviceHookBase::InternalGetInputMaxLatency(DWORD dwInputStreamIndex, REFERENCE_TIME *prtMaxLatency)
{

// TODO: Provide a good max latency if this DMO is going to be used with DirectShow.
//       For now, 30 is used as the default max latency.

    *prtMaxLatency = 30;

    return S_OK;
}


/////////////////////////////////////////
//
//  IMediaObjectImpl::InternalSetInputMaxLatency
//
//  *** Called by SetInputMaxLatency, description below ***
//
//  The SetInputMaxLatency method sets the maximum latency on a specified input
//  stream. For the definition of maximum latency, see GetInputMaxLatency.
//
//  Parameters
//
//      dwInputStreamIndex
//          Zero-based index of an input stream on the DMO.
//
//      rtMaxLatency
//          Maximum latency.
//
//  Return Value
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      E_FAIL Failure
//      E_NOTIMPL Not implemented
//
HRESULT CHXAudioDeviceHookBase::InternalSetInputMaxLatency(DWORD dwInputStreamIndex, REFERENCE_TIME rtMaxLatency)
{
    // Method not implemented
    return E_NOTIMPL;
}


////////////////////////////////////
//
//  IMediaObjectImpl::InternalGetInputSizeInfo
//
//  *** Called by GetInputSizeInfo, description below ***
//
//  The GetInputSizeInfo method retrieves the buffer requirements for a
//  specified input stream.
//
//  Parameters
//
//  dwInputStreamIndex:     Zero-based index of an input stream on the DMO.
//
//  pcbSize:                [out] Pointer to a variable that receives
//      the minimum size of an input buffer for this stream, in bytes.
//
//  pulSizeMaxLookahead:        [out] Pointer to a variable that receives the
//      maximum amount of data that the DMO will hold for lookahead, in bytes.
//      If the DMO does not perform lookahead on the stream, the value is zero.
//
//  pulSizeAlignment            [out] Pointer to a variable that receives the
//      required buffer alignment, in bytes. If the input stream has no
//      alignment requirement, the value is 1.
//
//  Return Value
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      DMO_E_TYPE_NOT_SET Media type was not set
//
//  The buffer requirements may depend on the media types of the various
//  streams. Before calling this method, set the media type of each stream
//  by calling the SetInputType and SetOutputType methods. If the media types
//  have not been set, this method might return an error.
//
//  If the DMO performs lookahead on the input stream, it returns the
//  DMO_INPUT_STREAMF_HOLDS_BUFFERS flag in the GetInputStreamInfo method.
//  During processing, the DMO holds up to the number of bytes indicated by the
//  pulSizeMaxLookahead parameter. The application must allocate enough buffers for
//  the DMO to hold this much data.
//
//  A buffer is aligned if the buffer's start address is a multiple of
//  *pulSizeAlignment. The alignment must be a power of two. Depending on the
//  microprocessor, reads and writes to an aligned buffer might be faster than
//  to an unaligned buffer. Also, some microprocessors do not support unaligned
//  reads and writes.
//
//  Note:
//
//  GetInputSizeInfo returns DMO_E_TYPE_NOT_SET unless all of the non-optional
//  streams have media types. Therefore, in the derived class, the internal
//  methods can assume that all of the non-optional streams have media types.
//
HRESULT CHXAudioDeviceHookBase::InternalGetInputSizeInfo(DWORD dwInputStreamIndex, DWORD *pcbSize, DWORD *pulSizeMaxLookahead, DWORD *pulSizeAlignment)
{
    // We don't have to do any validation, because it is all done in the base class

    HRESULT hr = S_OK;
    const DMO_MEDIA_TYPE* pmt;
    pmt = InputType(0);
    const WAVEFORMATEX* pwfx = reinterpret_cast<const WAVEFORMATEX*>(pmt->pbFormat);
    *pcbSize = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    *pulSizeMaxLookahead = 0;   // no look ahead
    *pulSizeAlignment = 1;      // no alignment requirement

    return hr;
}


//////////////////////////////////////
//
//  IMediaObjectImpl::InternalGetOutputSizeInfo
//
//  *** Called by GetOutputSizeInfo, description below ***
//
//  The GetOutputSizeInfo method retrieves the buffer requirements for a
//  specified output stream.
//
//  Parameters
//
//      dwOutputStreamIndex
//          Zero-based index of an output stream on the DMO.
//
//      pcbSize
//          [out] Pointer to a variable that receives the minimum size of an
//          output buffer for this stream, in bytes.
//
//      pulSizeAlignment
//          [out] Pointer to a variable that receives the required buffer
//          alignment, in bytes. If the output stream has no alignment
//          requirement, the value is 1.
//
//  Return Value
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      DMO_E_TYPE_NOT_SET Media type was not set
//
//  The buffer requirements may depend on the media types set for each of the
//  streams.
//
//  Before calling this method, set the media type of each stream by calling
//  the SetInputType and SetOutputType methods. If the media types have not
//  been set, this method might return an error. However, if a stream is
//  optional, and the application will not use the stream, you do not have to
//  set the media type for the stream.
//
//  A buffer is aligned if the buffer's start address is a multiple of
//  *pulSizeAlignment. Depending on the architecture of the microprocessor, it is
//  faster to read and write to an aligned buffer than to an unaligned buffer.
//  On some microprocessors, reading and writing to an unaligned buffer is not
//  supported and can cause the program to crash. Zero is not a valid alignment.
//
//  Note:
//
//  GetOutputSizeInfo returns DMO_E_TYPE_NOT_SET unless all of the non-optional
//  streams have media types. Therefore, in the derived class, the internal
//  methods can assume that all of the non-optional streams have media types.
//
HRESULT CHXAudioDeviceHookBase::InternalGetOutputSizeInfo(DWORD dwOutputStreamIndex, DWORD *pcbSize, DWORD *pulSizeAlignment)
{
    // We don't have to do any validation, because it is all done in the base class
    HRESULT hr = S_OK;
    const DMO_MEDIA_TYPE* pmt;
    pmt = OutputType(0);
    const WAVEFORMATEX* pwfx = reinterpret_cast<const WAVEFORMATEX*>(pmt->pbFormat);
    *pcbSize = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    *pulSizeAlignment = 1;

    return hr;
}


////////////////////////////////////////
//
//  IMediaObjectImpl::InternalGetInputStreamInfo
//
//  *** Called by GetInputStreamInfo, description below ***
//
//  The GetInputStreamInfo method retrieves information about an input stream,
//  such as any restrictions on the number of samples per buffer, and whether
//  the stream performs lookahead on the input data. This information never
//  changes.
//
//  Parameters
//      dwInputStreamIndex:
//          Zero-based index of an input stream on the DMO.
//
//      pdwFlags:
//          [out] Pointer to a variable that receives a bitwise combination of
//          zero or more DMO_INPUT_STREAM_INFO_FLAGS flags.
//
//  Return Value
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      E_POINTER NULL pointer argument
//
//  The DMO_INPUT_STREAMF_HOLDS_BUFFERS flag indicates that the DMO performs
//  lookahead on the incoming data.
//
//  The application must be sure to allocate sufficient buffers for the DMO
//  to process the input. Call the GetInputSizeInfo method to determine the
//  buffer requirements.
//
HRESULT CHXAudioDeviceHookBase::InternalGetInputStreamInfo(DWORD dwInputStreamIndex, DWORD *pdwFlags)
{
    *pdwFlags = DMO_OUTPUT_STREAMF_WHOLE_SAMPLES;
    *pdwFlags |= DMO_OUTPUT_STREAMF_FIXED_SAMPLE_SIZE;

    return S_OK;
}


//////////////////////////////////////////
//
//  IMediaObjectImpl::InternalGetOutputStreamInfo
//
//  *** Called by GetOutputStreamInfo, description below ***
//
//  The GetOutputStreamInfo method retrieves information about an output
//  stream; for example, whether the stream is discardable, and whether
//  it uses a fixed sample size. This information never changes.
//
//  Parameters
//      dwOutputStreamIndex
//          Zero-based index of an output stream on the DMO.
//
//      pdwFlags
//          [out] Pointer to a variable that receives a bitwise combination
//          of zero or more DMO_OUTPUT_STREAM_INFO_FLAGS flags.
//
//  Return Value
//
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      E_POINTER NULL pointer argument
//
HRESULT CHXAudioDeviceHookBase::InternalGetOutputStreamInfo(DWORD dwOutputStreamIndex, DWORD *pdwFlags)
{
    *pdwFlags = DMO_OUTPUT_STREAMF_WHOLE_SAMPLES;
    *pdwFlags |= DMO_OUTPUT_STREAMF_FIXED_SAMPLE_SIZE;

    return S_OK;
}


/////////////////////////////////
//
//  IMediaObjectImpl::InternalGetInputType
//
//  *** Called by GetInputType, description below ***
//
//  The GetInputType method retrieves a preferred media type for a specified
//  input stream.
//
//  Parameters
//
//      dwInputStreamIndex
//          Zero-based index of an input stream on the DMO.
//
//      dwTypeIndex
//          Zero-based index on the set of acceptable media types.
//
//      pmt
//          [out] Pointer to a DMO_MEDIA_TYPE structure allocated by the caller.
//          The method fills the structure with the media type. The format block
//          might be NULL, in which case the format type GUID is GUID_NULL.
//
//  Return Value
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      DMO_E_NO_MORE_ITEMS Type index is out of range
//      E_OUTOFMEMORY Insufficient memory
//      E_POINTER NULL pointer argument
//
//  Call this method to enumerate an input stream's preferred media types. The
//  DMO assigns each media type an index value in order of preference. The most
//  preferred type has an index of zero. To enumerate all the types, make
//  successive calls while incrementing the type index until the method returns
//  DMO_E_NO_MORE_ITEMS.
//
//  If the method succeeds, call MoFreeMediaType to free the format block.
//
//  To set the media type, call the SetInputType method. Setting the media type
//  on one stream can change another stream's preferred types. In fact, a
//  stream might not have a preferred type until the type is set on another
//  stream. For example, a decoder might not have a preferred output type until
//  the input type is set. However, the DMO is not required to update its
//  preferred types dynamically in this fashion. Thus, the types returned by
//  this method are not guaranteed to be valid; they might fail when used in the
//  SetInputType method. Conversely, the DMO is not guaranteed to enumerate every
//  media type that it supports. To test whether a particular media type is
//  acceptable, call SetInputType with the DMO_SET_TYPEF_TEST_ONLY flag.
//
HRESULT CHXAudioDeviceHookBase::InternalGetInputType(DWORD dwInputStreamIndex, DWORD dwTypeIndex, DMO_MEDIA_TYPE *pmt)
{
    // This function resembles InternalGetOutputType() since the input and output types must
    // be consistent for DirectSound

    HRESULT hr = S_OK;

    if (dwTypeIndex > 0)
    {
        return DMO_E_NO_MORE_ITEMS;
    }

    // If pmt is NULL, and the type index is in range, we return S_OK
    if (pmt == NULL)
    {
        return S_OK;
    }

    // If the output type is set, we prefer to use that one
    if (OutputTypeSet(0))
    {
        return MoCopyMediaType(pmt, OutputType(0));
    }

    hr = MoInitMediaType(pmt, sizeof(WAVEFORMATEX));

    if (SUCCEEDED(hr))
    {
        pmt->majortype  = MEDIATYPE_Audio;
        pmt->subtype    = MEDIASUBTYPE_PCM;         // We take PCM format!
        pmt->formattype = FORMAT_None;
    }

    return hr;
}


///////////////////////////////////
//
//  IMediaObjectImpl::InternalGetOutputType
//
//  *** Called by GetOutputType, description below ***
//
//  The GetOutputType method retrieves a preferred media type for a specified
//  output stream.
//
//  Parameters
//
//      dwOutputStreamIndex
//          Zero-based index of an output stream on the DMO.
//
//      dwTypeIndex
//          Zero-based index on the set of acceptable media types.
//
//      pmt
//          [out] Pointer to a DMO_MEDIA_TYPE structure allocated by the
//          caller. The method fills the structure with the media type. The
//          format block might be NULL, in which case the format type GUID is GUID_NULL.
//
//  Return Value
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      DMO_E_NO_MORE_ITEMS Type index is out of range
//      E_OUTOFMEMORY Insufficient memory
//      E_POINTER NULL pointer argument
//
//  Call this method to enumerate an output stream's preferred media types. The
//  DMO assigns each media type an index value, in order of preference. The
//  most preferred type has an index of zero. To enumerate all the types, make
//  successive calls while incrementing the type index, until the method returns
//  DMO_E_NO_MORE_ITEMS.
//
//  If the method succeeds, call MoFreeMediaType to free the format block.
//
//  To set the media type, call the SetOutputType method. Setting the media type
//  on one stream can change another stream's preferred types. In fact, a stream
//  might not have a preferred type until the type is set on another stream. For
//  example, a decoder might not have a preferred output type until the input
//  type is set. However, the DMO is not required to update its preferred types
//  dynamically in this fashion. Thus, the types returned by this method are not
//  guaranteed to be valid; they might fail when used in the SetOutputType method.
//  Conversely, the DMO is not guaranteed to enumerate every media type that it
//  supports. To test whether a particular media type is acceptable, call
//  SetOutputType with the DMO_SET_TYPEF_TEST_ONLY flag.
//
//
HRESULT CHXAudioDeviceHookBase::InternalGetOutputType(DWORD dwOutputStreamIndex, DWORD dwTypeIndex, DMO_MEDIA_TYPE *pmt)
{
    // This function resembles InternalGetInputType() since the input and output types must
    // be consistent for DirectSound

    HRESULT hr = S_OK;

    if (dwTypeIndex > 0)
    {
        return DMO_E_NO_MORE_ITEMS;
    }

    // If pmt is NULL, and the type index is in range, we return S_OK
    if (pmt == NULL)
    {
        return S_OK;
    }

    // If the input type is set, we prefer to use that one
    if (InputTypeSet(0))
    {
        return MoCopyMediaType(pmt, InputType(0));
    }

    hr = MoInitMediaType(pmt, sizeof(WAVEFORMATEX));

    if (SUCCEEDED(hr))
    {
        pmt->majortype  = MEDIATYPE_Audio;
        pmt->subtype    = MEDIASUBTYPE_PCM;         // We take PCM format!
        pmt->formattype = FORMAT_None;
    }

    return hr;
}


///////////////////////////////////////
//
//  IMediaObjectImpl::InternalProcessInput
//
//  *** Called by ProcessInput, description below ***
//
//  The ProcessInput method delivers a buffer to the specified input stream.
//
//  Parameters
//      dwInputStreamIndex
//          Zero-based index of an input stream on the DMO.
//
//      pBuffer
//          Pointer to the buffer's IMediaBuffer interface.
//
//      dwFlags
//          Bitwise combination of zero or more flags from the
//          DMO_INPUT_DATA_BUFFER_FLAGS enumeration.
//
//      rtTimestamp
//          Time stamp that specifies the start time of the data in the buffer.
//          If the buffer has a valid time stamp, set the
//          DMO_INPUT_DATA_BUFFERF_TIME flag in the dwFlags parameter.
//          Otherwise, the DMO ignores this value.
//
//      rtTimelength
//          Reference time specifying the duration of the data in the buffer.
//          If this value is valid, set the DMO_INPUT_DATA_BUFFERF_TIMELENGTH
//          flag in the dwFlags parameter. Otherwise, the DMO ignores this value.
//
//  Return Value
//      S_FALSE No output to process
//      S_OK Success
//      DMO_E_INVALIDSTREAMINDEX Invalid stream index
//      DMO_E_NOTACCEPTING Data cannot be accepted
//
//  If the DMO does not process all the data in the buffer, it keeps a
//  reference count on the buffer. It releases the buffer once it has
//  generated all the output, unless it needs to perform lookahead on the data.
//  (To determine whether a DMO performs lookahead, call the GetInputStreamInfo
//  method.)
//
//  If this method returns DMO_E_NOTACCEPTING, call the ProcessOutput method
//  until the input stream can accept more data. To determine whether the stream
//  can accept more data, call the GetInputStatus method.
//
//  If the method returns S_FALSE, no output was generated from this input and the
//  application does not need to call ProcessOutput. However, a DMO is not required
//  to return S_FALSE in this situation; it might return S_OK.
//
//  Note:
//
//  Before this method calls InternalProcessInput, it calls
//  AllocateStreamingResources and InternalAcceptingInput. Therefore, the
//  implementation of InternalProcessInput can assume the following:
//
//  * All resources have been allocated.
//  * The input stream can accept data.
//
HRESULT CHXAudioDeviceHookBase::InternalProcessInput(DWORD dwInputStreamIndex, IMediaBuffer *pBuffer, DWORD dwFlags, REFERENCE_TIME rtTimestamp, REFERENCE_TIME rtTimelength)
{

// TODO: Complete or modify implementation of InternalProcessInput() if necessary

    HRESULT hr = S_OK;

    if (!pBuffer)
    {
        return E_POINTER;
    }

    // Get the size of the input buffer
    hr = pBuffer->GetBufferAndLength(&m_pbInputData, &m_cbInputLength);
    if (SUCCEEDED(hr))
    {
        if (m_cbInputLength <= 0)
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_pBuffer = pBuffer;

        if (dwFlags & DMO_INPUT_DATA_BUFFERF_TIME)
        {
            m_bValidTime = true;
            m_rtTimestamp = rtTimestamp;
        }
        else
        {
            m_bValidTime = false;
        }
    }

    return hr;
}


///////////////////////////////////
//
//  IMediaObjectImpl::InternalProcessOutput
//
//  *** Called by ProcessOutput, description below ***
//
//  The ProcessOutput method generates output from the current input data.
//
//  Parameters
//
//      dwFlags
//          Bitwise combination of zero or more flags from the
//          DMO_PROCESS_OUTPUT_FLAGS enumeration.
//
//      cOutputBufferCount
//          Number of output buffers.
//
//      pOutputBuffers
//          [in, out] Pointer to an array of DMO_OUTPUT_DATA_BUFFER structures
//          containing the output buffers. Specify the size of the array in the
//          cOutputBufferCount parameter.
//
//      pdwStatus
//          [out] Pointer to a variable that receives a reserved value (zero).
//          The application should ignore this value.
//
//  Return Value
//      S_FALSE No output was generated
//      S_OK Success
//      E_FAIL Failure
//      E_INVALIDARG Invalid argument
//      E_POINTER NULL pointer argument
//
//  The pOutputBuffers parameter points to an array of DMO_OUTPUT_DATA_BUFFER
//  structures. The application must allocate one structure for each output
//  stream. To determine the number of output streams, call the GetStreamCount
//  method. Set the cOutputBufferCount parameter to this number.
//
//  Each DMO_OUTPUT_DATA_BUFFER structure contains a pointer to a buffer's
//  IMediaBuffer interface. The application allocates these buffers. The other
//  members of the structure are status fields. The DMO sets these fields if
//  the method succeeds. If the method fails, their values are undefined.
//
//  When the application calls ProcessOutput, the DMO processes as much input
//  data as possible. It writes the output data to the output buffers, starting
//  from the end of the data in each buffer. (To find the end of the data, call
//  the IMediaBuffer::GetBufferAndLength method.) The DMO never holds a
//  reference count on an output buffer.
//
//  If the DMO fills an entire output buffer and still has input data to
//  process, the DMO returns the DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE flag in the
//  DMO_OUTPUT_DATA_BUFFER structure. The application should check for this
//  flag by testing the dwStatus member of each structure.
//
//  If the method returns S_FALSE, no output was generated. However, a DMO is
//  not required to return S_FALSE in this situation; it might return S_OK.
//
//  Discarding data:
//
//  You can discard data from a stream by setting the
//  DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER flag in the dwFlags parameter.
//  For each stream that you want to discard, set the pBuffer member of the
//  DMO_OUTPUT_DATA_BUFFER structure to NULL.
//
//  For each stream in which pBuffer is NULL:
//
//  If the DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER flag is set, and the
//  stream is discardable or optional, the DMO discards the data.
//
//  If the flag is set but the stream is neither discardable nor optional, the
//  DMO discards the data if possible. It is not guaranteed to discard the
//  data.
//
//  If the flag is not set, the DMO does not produce output data for that
//  stream, but does not discard the data.
//
//  To check whether a stream is discardable or optional, call the
//  GetOutputStreamInfo method.
//
//  Note:
//
//  Before this method calls InternalProcessOutput, it calls
//  AllocateStreamingResources. Therefore, the implementation of
//  InternalProcessOutput can assume that all resources have been allocated.
//
HRESULT CHXAudioDeviceHookBase::InternalProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, DMO_OUTPUT_DATA_BUFFER *pOutputBuffers, DWORD *pdwStatus)
{

// TODO: Complete or modify implementation of InternalProcessOutput() if necessary

    HRESULT         hr = S_OK;
    BYTE            *pbData = NULL;
    DWORD           cbData = 0;
    DWORD           cbOutputLength = 0;
    DWORD           cbBytesProcessed = 0;
    bool            bComplete = false;
    const DWORD     UNITS = 10000000;  // 1 sec = 100 * UNITS ns

    CComPtr<IMediaBuffer> pOutputBuffer = pOutputBuffers[0].pBuffer;

    if (!m_pBuffer || !pOutputBuffer)
    {
        return S_FALSE;  // Did not produce output
    }

    // Get the size of the output buffer
    hr = pOutputBuffer->GetBufferAndLength(&pbData, &cbData);

    if (SUCCEEDED(hr))
    {
        hr = pOutputBuffer->GetMaxLength(&cbOutputLength);
    }

    if (SUCCEEDED(hr))
    {
        // Skip past any valid data in the output buffer
        pbData += cbData;
        cbOutputLength -= cbData;

        // Calculate how many quanta we can process
        if (m_cbInputLength > cbOutputLength)
        {
            cbBytesProcessed = cbOutputLength;
        }
        else
        {
            cbBytesProcessed = m_cbInputLength;
            bComplete = true;
        }

        // Process the data
        hr = DoProcess(pbData, m_pbInputData, cbBytesProcessed / WaveFormat()->nBlockAlign);
    }

    if (SUCCEEDED(hr))
    {
        hr = pOutputBuffer->SetLength(cbBytesProcessed + cbData);
    }

    if (SUCCEEDED(hr))
    {
        if (m_bValidTime)
        {
            pOutputBuffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIME;
            pOutputBuffers[0].rtTimestamp = m_rtTimestamp;

            // Estimate how far along we are
            pOutputBuffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
            double dTime = (double)(cbBytesProcessed) / WaveFormat()->nAvgBytesPerSec;
            pOutputBuffers[0].rtTimelength = (REFERENCE_TIME)(dTime * UNITS);
        }

        if (bComplete)
        {
            m_pBuffer = NULL;   // Release input buffer
        }
        else
        {
            pOutputBuffers[0].dwStatus |= DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;
            m_cbInputLength -= cbBytesProcessed;
            m_pbInputData += cbBytesProcessed;
            m_rtTimestamp += pOutputBuffers[0].rtTimelength;
        }
    }
    return hr;
}


///////////////////////////////////
//
//  DoProcess
//
//  *** Called by Process and ProcessOutput ***
//
//  The DoProcess method processes the sound data.
//
//  Parameters
//
//      pbData
//          Pointer to the output buffer
//
//      pbInputData
//          Pointer to the input buffer
//
//      dwQuanta
//          Number of quanta to process
//
//  Return Value
//      S_OK Success
//
HRESULT CHXAudioDeviceHookBase::DoProcess(BYTE *pbData, const BYTE *pbInputData, DWORD dwQuanta)
{
    if( m_pHook )
    {
	DWORD dwInputBufSize = dwQuanta * WaveFormat()->nBlockAlign;
	IHXBuffer* pBuffer = NULL;
	if (HXR_OK == CreateAndSetBufferWithoutAllocCCF(pBuffer,
							(UCHAR*) pbInputData,
							dwInputBufSize,
							m_pContext))
	{
	    HXBOOL bChanged = FALSE;
	    if( SUCCEEDED( m_pHook->ProcessAudioDeviceHooks( pBuffer, bChanged )) && bChanged )
	    {
		memcpy( pbData, pBuffer->GetBuffer(), HX_MIN( dwInputBufSize, pBuffer->GetSize()) );
	    }

	    HX_RELEASE( pBuffer );
	}
    }

    return S_OK;
}




////////////////////////////////////////
//
//  IMediaObjectImpl::InternalAcceptingInput
//
//  Queries whether an input stream can accept more input. The derived class
//  must declare and implement this method.
//
//  Parameters
//
//      dwInputStreamIndex
//          Index of an input stream.
//
//  Return Value
//
//      Returns S_OK if the input stream can accept input, or S_FALSE otherwise.
//
//  Note:
//
//  Called by IMediaObject::GetInputStatus
//
HRESULT CHXAudioDeviceHookBase::InternalAcceptingInput(DWORD dwInputStreamIndex)
{

// TODO: Change implementation if necessary

    // Do not accept input if there is already input data to process
    return (m_pBuffer ? S_FALSE : S_OK);
}


////////////////////////////////////
//
// IMediaObjectImpl::InternalCheckInputType
//
// Queries whether an input stream can accept a given media type.
// The derived class must declare and implement this method.
//
//  Parameters
//
//      dwInputStreamIndex
//          Index of an input stream.
//
//      pmt
//          Pointer to a DMO_MEDIA_TYPE structure that describes the media type.
//
//  Return Value
//
//       Returns S_OK if the media type is valid, or DMO_E_INVALIDTYPE otherwise.
//
//  Note:
//
//  Called by IMediaObject::SetInputType
//
HRESULT CHXAudioDeviceHookBase::InternalCheckInputType(DWORD dwInputStreamIndex, const DMO_MEDIA_TYPE *pmt)
{
    WAVEFORMATEX* pWave = (WAVEFORMATEX*)pmt->pbFormat;

    HRESULT hr = S_OK;

    // Check that we're PCM or float
    if ((NULL                       == pmt) ||
        (MEDIATYPE_Audio            != pmt->majortype) ||
        (MEDIASUBTYPE_PCM           != pmt->subtype) ||
        (FORMAT_WaveFormatEx        != pmt->formattype &&
         FORMAT_None                != pmt->formattype) ||
        (pmt->cbFormat              <  sizeof(WAVEFORMATEX)) ||
        (NULL                       == pmt->pbFormat))
    {
        hr = DMO_E_INVALIDTYPE;
    }

    // If other type set, accept only if identical to that.  Otherwise accept
    // any standard PCM/float audio.
    if (SUCCEEDED(hr))
    {
        if (OutputTypeSet(0))
        {
            const DMO_MEDIA_TYPE* pmtOutput;
            pmtOutput = OutputType(0);
            if (memcmp(pmt->pbFormat, pmtOutput->pbFormat, sizeof(WAVEFORMATEX)))
            {
                hr = DMO_E_INVALIDTYPE;
            }
        }
        else
        {
            WAVEFORMATEX* pWave = (WAVEFORMATEX*)pmt->pbFormat;
            if ((WAVE_FORMAT_PCM != pWave->wFormatTag) ||
                ((8 != pWave->wBitsPerSample) && (16 != pWave->wBitsPerSample)) ||
                ((1 != pWave->nChannels) && (2 != pWave->nChannels)) ||
                ( // Supported sample rates:
                 (96000 != pWave->nSamplesPerSec) &&
                 (48000 != pWave->nSamplesPerSec) &&
                 (44100 != pWave->nSamplesPerSec) &&
                 (32000 != pWave->nSamplesPerSec) &&
                 (22050 != pWave->nSamplesPerSec) &&
                 (16000 != pWave->nSamplesPerSec) &&
                 (11025 != pWave->nSamplesPerSec) &&
                 (8000 != pWave->nSamplesPerSec) &&
                 TRUE   // You may delete && TRUE
                ) ||
                (pWave->nBlockAlign != pWave->nChannels * pWave->wBitsPerSample / 8) ||
                (pWave->nAvgBytesPerSec != pWave->nSamplesPerSec * pWave->nBlockAlign))
            {
                hr = DMO_E_INVALIDTYPE;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // We will remember the number of channels, bps, and sample rate of the input type
        m_nChannels = pWave->nChannels;
        m_wBitsPerSample = pWave->wBitsPerSample;
        m_nSamplesPerSec = pWave->nSamplesPerSec;
    }

    return hr;
}


////////////////////////////////////////////
//
//  IMediaObjectImpl::InternalCheckOutputType
//
//  Queries whether an output stream can accept a given media type. The derived
//  class must declare and implement this method.
//
//  Parameters
//
//      dwOutputStreamIndex
//          Index of an output stream.
//
//      pmt
//          Pointer to a DMO_MEDIA_TYPE structure that describes the media type.
//
//  Return Value
//
//      Returns S_OK if the media type is valid, or DMO_E_INVALIDTYPE otherwise.
//
//  Note:
//
//  Called by IMediaObject::SetOutputType
//
HRESULT CHXAudioDeviceHookBase::InternalCheckOutputType(DWORD dwOutputStreamIndex,const DMO_MEDIA_TYPE *pmt)
{
    // Check that we're PCM or float
    HRESULT hr = S_OK;

    if ((NULL                       == pmt) ||
        (MEDIATYPE_Audio            != pmt->majortype) ||
        (MEDIASUBTYPE_PCM           != pmt->subtype) ||
        (FORMAT_WaveFormatEx        != pmt->formattype &&
         FORMAT_None                != pmt->formattype) ||
        (pmt->cbFormat              <  sizeof(WAVEFORMATEX)) ||
        (NULL                       == pmt->pbFormat))
    {
        hr = DMO_E_INVALIDTYPE;
    }

    // If other type set, accept only if identical to that.  Otherwise accept
    // any standard PCM/float audio.
    if (SUCCEEDED(hr))
    {
        if (InputTypeSet(0))
        {
            const DMO_MEDIA_TYPE* pmtInput;
            pmtInput = InputType(0);
            if (memcmp(pmt->pbFormat, pmtInput->pbFormat, sizeof(WAVEFORMATEX)))
            {
                hr = DMO_E_INVALIDTYPE;
            }
        }
        else
        {
            WAVEFORMATEX* pWave = (WAVEFORMATEX*)pmt->pbFormat;
            if ((WAVE_FORMAT_PCM != pWave->wFormatTag) ||
                ((8 != pWave->wBitsPerSample) && (16 != pWave->wBitsPerSample)) ||
                ((1 != pWave->nChannels) && (2 != pWave->nChannels)) ||
                ( // Supported sample rates:
                 (96000 != pWave->nSamplesPerSec) &&
                 (48000 != pWave->nSamplesPerSec) &&
                 (44100 != pWave->nSamplesPerSec) &&
                 (32000 != pWave->nSamplesPerSec) &&
                 (22050 != pWave->nSamplesPerSec) &&
                 (16000 != pWave->nSamplesPerSec) &&
                 (11025 != pWave->nSamplesPerSec) &&
                 (8000 != pWave->nSamplesPerSec) &&
                 TRUE   // You may delete && TRUE
                ) ||
                (pWave->nBlockAlign != pWave->nChannels * pWave->wBitsPerSample / 8) ||
                (pWave->nAvgBytesPerSec != pWave->nSamplesPerSec * pWave->nBlockAlign))
            {
                hr = DMO_E_INVALIDTYPE;
            }
        }
    }

    return hr;
}


///////////////
//
//  IMediaObjectImpl::Lock
//
// Locks the object. The derived class must declare and implement this method.
//
// If you implement your derived class using the Active Template Library (ATL),
// you can use ATL's default implementation of this method.
//
void CHXAudioDeviceHookBase::Lock(void)
{
    CComObjectRootEx<CComMultiThreadModel>::Lock();
}


///////////////
//
// IMediaObjectImpl::Unlock
//
// Unlocks the object. The derived class must declare and implement this method.
//
// If you implement your derived class using the Active Template Library (ATL),
// you can use ATL's default implementation of this method.
//
void CHXAudioDeviceHookBase::Unlock(void)
{
    CComObjectRootEx<CComMultiThreadModel>::Unlock();
}

///////////////////////////////
//
// IHXAudioDeviceHookDMO Methods
//

HRESULT CHXAudioDeviceHookBase::Init(IUnknown* pContext)
{
    m_pContext = pContext;
    if(m_pContext)
    {
	m_pContext->AddRef();
	m_pContext->QueryInterface(IID_IHXAudioDeviceHookManager, (void **)&m_pHook);
    }

    return S_OK;
}
