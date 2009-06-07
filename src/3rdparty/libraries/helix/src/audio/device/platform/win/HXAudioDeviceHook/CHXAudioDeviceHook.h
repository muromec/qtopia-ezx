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

#ifndef __CHXAudioDeviceHook_H_
#define __CHXAudioDeviceHook_H_

#include "resource.h"
#include <uuids.h>

#include "HXAudioDeviceHook.h"
#include "hxausvc.h"

/////////////////////////////////////////////////////////////////////////////
// CHXAudioDeviceHook
class ATL_NO_VTABLE CHXAudioDeviceHookBase : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IMediaObjectImpl<CHXAudioDeviceHookBase,1,1>,		// DMO Template (1 input stream & 1 output stream)
    public IMediaObjectInPlace,
    public IHXAudioDeviceHookDMO
{
friend class IMediaObjectImpl<CHXAudioDeviceHookBase,1,1>;
friend class LockIt;

public:
    CHXAudioDeviceHookBase();	// Constructor
    ~CHXAudioDeviceHookBase();	// Destructor

DECLARE_REGISTRY_RESOURCEID(IDR_HXAUDIODEVICEHOOK)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHXAudioDeviceHookBase)
    COM_INTERFACE_ENTRY_IID(IID_IMediaObject, IMediaObject)
    COM_INTERFACE_ENTRY(IMediaObjectInPlace)
    COM_INTERFACE_ENTRY_IID(IID_IHXAudioDeviceHookDMO, IHXAudioDeviceHookDMO)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
	return CoCreateFreeThreadedMarshaler(
		GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
	FreeStreamingResources();  // In case client does not call this.
	m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

public:
    // IMediaObjectInPlace Methods
    STDMETHODIMP STDMETHODCALLTYPE Process(ULONG ulSize, BYTE *pData, REFERENCE_TIME refTimeStart,DWORD dwFlags);
    STDMETHODIMP STDMETHODCALLTYPE Clone(IMediaObjectInPlace **ppMediaObject);
    STDMETHODIMP STDMETHODCALLTYPE GetLatency(REFERENCE_TIME *pLatencyTime);

    // IHXAudioDeviceHookDMO Methods
    STDMETHODIMP Init(IUnknown* pContext);

protected:
    //IMediaObjectImpl Methods   
    STDMETHODIMP InternalAllocateStreamingResources(void);
    STDMETHODIMP InternalDiscontinuity(DWORD dwInputStreamIndex);
    STDMETHODIMP InternalFlush(void);
    STDMETHODIMP InternalFreeStreamingResources(void);
    STDMETHODIMP InternalGetInputMaxLatency(DWORD dwInputStreamIndex, REFERENCE_TIME *prtMaxLatency);
    STDMETHODIMP InternalSetInputMaxLatency(DWORD dwInputStreamIndex, REFERENCE_TIME rtMaxLatency);
    STDMETHODIMP InternalGetInputSizeInfo(DWORD dwInputStreamIndex, DWORD *pcbSize, DWORD *pcbMaxLookahead, DWORD *pcbAlignment);
    STDMETHODIMP InternalGetOutputSizeInfo(DWORD dwOutputStreamIndex, DWORD *pcbSize, DWORD *pcbAlignment);
    STDMETHODIMP InternalGetInputStreamInfo(DWORD dwInputStreamIndex, DWORD *pdwFlags);
    STDMETHODIMP InternalGetOutputStreamInfo(DWORD dwOutputStreamIndex, DWORD *pdwFlags);
    STDMETHODIMP InternalGetInputType(DWORD dwInputStreamIndex, DWORD dwTypeIndex, DMO_MEDIA_TYPE *pmt);
    STDMETHODIMP InternalGetOutputType(DWORD dwOutputStreamIndex, DWORD dwTypeIndex, DMO_MEDIA_TYPE *pmt);
    STDMETHODIMP InternalProcessInput(DWORD dwInputStreamIndex, IMediaBuffer *pBuffer, DWORD dwFlags, REFERENCE_TIME rtTimestamp, REFERENCE_TIME rtTimelength);
    STDMETHODIMP InternalProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, DMO_OUTPUT_DATA_BUFFER *pOutputBuffers, DWORD *pdwStatus);

    // IMediaObjectImpl Required overides
    STDMETHODIMP InternalAcceptingInput(DWORD dwInputStreamIndex);
    STDMETHODIMP InternalCheckInputType(DWORD dwInputStreamIndex, const DMO_MEDIA_TYPE *pmt);
    STDMETHODIMP InternalCheckOutputType(DWORD dwOutputStreamIndex, const DMO_MEDIA_TYPE *pmt);
    void Lock(void);
    void Unlock(void);

    // Private functions
    HRESULT UpdateStatesInternal();

    virtual HRESULT DoProcess(BYTE *pbData, const BYTE *pbInputData, DWORD dwQuanta);

    WAVEFORMATEX* WaveFormat()
    {
	_ASSERTE(InputType(0)->formattype == FORMAT_WaveFormatEx);
	return reinterpret_cast<WAVEFORMATEX*>(InputType(0)->pbFormat);
    }

    // States of the DMO
    bool	m_fDirty;
    bool	m_fInitialized;

    // States of the input type
    WORD	m_nChannels;
    WORD	m_wBitsPerSample; 
    DWORD	m_nSamplesPerSec;

    CComPtr<IMediaBuffer>	m_pBuffer;		// Pointer to the input buffer
    BYTE*			m_pbInputData;		// Pointer to the data in the input buffer
    DWORD			m_cbInputLength;	// Length of the data
    REFERENCE_TIME		m_rtTimestamp;		// Most recent timestamp
    bool			m_bValidTime;		// Controls whether timestamp is valid or not

    IUnknown*			m_pContext;
    IHXAudioDeviceHookManager*	m_pHook;

};

#endif //__CHXAudioDeviceHook_H_

