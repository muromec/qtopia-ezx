/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ff_source.cpp,v 1.7 2007/08/18 00:21:12 dcollins Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

//XXXSMP FIX THE INTERFACES TO STDMETHODIMP

#ifdef _UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#endif // _UNIX

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "ihxpckts.h"
#include "hxplugn.h"

#include "hxbdwdth.h"

#include "hxmap.h"

#include "hxslist.h"
#include "hxstring.h"
#include "plgnhand.h"
#include "hxstrutl.h"

#include "srcerrs.h"
#include "hxprot.h"

#include "hxasm.h"

#include "proc.h"
#include "proc_container.h"
#include "engine.h"
#include "fsys_wrap.h"
#include "base_errmsg.h"

#include "ff_source.h"

FileFormatSource::FileFormatSource(Process* proc, 
                                   IHXFileFormatObject* file_format,
                                   UINT32 mount_point_len,
                                   IUnknown* pFileObject,
                                   IHXRequest* pRequest,
                                   BOOL bIsLive)
{
    m_lNumStreams		= 0;
    m_lRefCount			= 0;
    m_proc			= proc;
    m_is_ready			= FALSE;
    m_file_format 		= file_format;
    m_bFileFormatInitialized = FALSE;
    m_file_format->AddRef();
    m_pSinkControl		= 0;
    m_pSinkPackets		= 0;
    m_mount_point_len           = mount_point_len;
    m_is_live                   = bIsLive;

    m_pFileObject               = pFileObject;
    if (m_pFileObject)
    {
        m_pFileObject->AddRef();
    }

    m_pRequest                  = pRequest;

    if (m_pRequest)
    {
        m_pRequest->AddRef();
        const char* pTemp = 0;
        m_pRequest->GetURL(pTemp);
        m_pURL = pTemp ? new_string (pTemp) : 0;
    }

    m_bwe_values = NULL;
}

FileFormatSource::~FileFormatSource()
{
    delete[] m_pURL;

    if (m_pSinkControl)
    {
        m_pSinkControl->Release();
        m_pSinkControl = NULL;
    }

    if (m_pSinkPackets)
    {
        m_pSinkPackets->Release();
        m_pSinkPackets = NULL;
    }

    if (m_pFileObject)
    {
        m_pFileObject->Release();
        m_pFileObject = NULL;
    }

    if (m_pRequest)
    {
        m_pRequest->Release();
        m_pRequest = NULL;
    }

    if (m_file_format)
    {
        /*
         * Close() must be called before Release() so that the
         * IHXFileFormatObject will properly free its IHXFileObject
         */

        m_file_format->Close();
        m_file_format->Release();
        m_file_format = NULL;
    }

    if(m_bwe_values)
    {
        m_bwe_values->Release();
        m_bwe_values = NULL;
    }
}

STDMETHODIMP 
FileFormatSource::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPSourceControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSourceControl))
    {
        AddRef();
        *ppvObj = (IHXPSourceControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSourcePackets))
    {
        AddRef();
        *ppvObj = (IHXPSourcePackets*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXFormatResponse))
    {
	AddRef();
	*ppvObj = (IHXFormatResponse*)this;
	return HXR_OK;
    }
    else if ( (IsEqualIID(riid, IID_IHXFileFormatObject)) && m_file_format)
    {
	return m_file_format->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXPacketTimeOffsetHandler)) && m_file_format)
    {
	return m_file_format->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXLiveFileFormatInfo)) && m_file_format)
    {
	return m_file_format->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXASMSource)) && m_file_format)
    {
        return m_file_format->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
FileFormatSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
FileFormatSource::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
FileFormatSource::Init(IHXPSinkControl* pSink, IHXValues* values)
{
    m_bwe_values = values;
    m_bwe_values->AddRef();

    return Init(pSink);
}

STDMETHODIMP
FileFormatSource::Done()
{
    if (m_pSinkControl)
    {
        m_pSinkControl->Release();
        m_pSinkControl = NULL;
    }

    if (m_pSinkPackets)
    {
        m_pSinkPackets->Release();
        m_pSinkPackets = NULL;
    }

    if (m_pFileObject)
    {
        m_pFileObject->Release();
        m_pFileObject = NULL;
    }

    if (m_pRequest)
    {
        m_pRequest->Release();
        m_pRequest = NULL;
    }

    if (m_file_format)
    {
        /*
         * Close() must be called before Release() so that the
         * IHXFileFormatObject will properly free its IHXFileObject
         */

        m_file_format->Close();
        m_file_format->Release();
        m_file_format = NULL;
        m_bFileFormatInitialized = FALSE;
    }

    if(m_bwe_values)
    {
        m_bwe_values->Release();
        m_bwe_values = NULL;
    }
    return HXR_OK;
}

HX_RESULT
FileFormatSource::Init(IHXPSinkControl* pSink)
{
    HX_RESULT			h_result = HXR_OK;
    IHXFileObject*             pFileObject;
    IHXBandwidthNegotiator*    pBandwidthNegotiator;

    HX_RELEASE(m_pSinkControl);
    pSink->AddRef();
    m_pSinkControl = pSink;

    AddRef();
    if (HXR_OK == h_result)
    {
        if(HXR_OK == m_pFileObject->QueryInterface(IID_IHXFileObject,
                                                 (void**)&pFileObject))
        {
          
            if(m_bwe_values && 
               (HXR_OK == m_file_format->QueryInterface(
                IID_IHXBandwidthNegotiator,
                (void**)&pBandwidthNegotiator)))
            {
                pBandwidthNegotiator->SetBandwidthInfo(m_bwe_values);
                pBandwidthNegotiator->Release();
                m_bwe_values->Release();
                m_bwe_values = NULL;
            }

            HX_ASSERT(m_file_format != NULL);
            if (m_bFileFormatInitialized)
            {
                InitDone(HXR_OK);
            }
            else
            {
                m_bFileFormatInitialized = TRUE;
                h_result = m_file_format->InitFileFormat(m_pRequest, this,
                                                     pFileObject);
            }

            HX_RELEASE(pFileObject);

            if (HXR_OK != h_result)
            {
                ERRMSG(m_proc->pc->error_handler,
                       "File Format initialization failed\n");
                m_file_format->Close();
                m_file_format->Release();
                m_file_format = 0;
                m_bFileFormatInitialized = FALSE;
            }
        }

    }

    Release();

    return h_result;
}

HX_RESULT
FileFormatSource::Init(IHXPSinkPackets* pSink)
{
    HX_RELEASE(m_pSinkPackets);
    pSink->AddRef();
    m_pSinkPackets = pSink;
    return HXR_OK;
}

HX_RESULT
FileFormatSource::GetFileHeader(IHXPSinkControl* pSink)
{
    ASSERT(pSink == m_pSinkControl);
    HX_RESULT rc;

    rc = m_file_format->GetFileHeader();

    return rc;
}

HX_RESULT
FileFormatSource::GetStreamHeader(IHXPSinkControl* pSink, UINT16 stream_number)
{
    ASSERT(pSink == m_pSinkControl);
    HX_RESULT rc;

    rc =  m_file_format->GetStreamHeader(stream_number);

    return rc;
}

HX_RESULT
FileFormatSource::GetPacket(UINT16 unStreamNumber)
{
    HX_RESULT rc = HXR_OK;
    HX_RESULT ulResult;

    if(m_file_format)
    {
        ulResult = m_file_format->GetPacket(unStreamNumber);

        if (HXR_OK != ulResult)
        {
            StreamDone(unStreamNumber);

            /*
             * XXXSCR
             * I am ifdefing this code because it needs to be revisited.
             * It was put here to disconnect players who have lost
             * vital pieces of the protocol exchange due high load
             * in cloaking, causing PLAY to occur with no stream
             * subscription.  Unfortuneately, we see GetPacket() failing
             * in other places.  This shouldn't happen.  We should
             * be able to treat GetPacket failure as a fatal error
             *
             * XXXJC : 
             * Calling StreamDone should be enough, lets leave this out
             *
             */
#if 0
            if (client)
            {
                client->protocol()->sendAlert(SE_INVALID_PROTOCOL);
            }
#endif	
        }
    }
    return rc;
}

HX_RESULT
FileFormatSource::InitDone(HX_RESULT status)
{
    if (m_pSinkControl)
        m_pSinkControl->InitDone(status);

    return HXR_OK;
}

HX_RESULT
FileFormatSource::FileHeaderReady(HX_RESULT status, IHXValues* header)
{
    HX_ASSERT(header || (status != HXR_OK));

    if (header)
    {
        header->GetPropertyULONG32("StreamCount", m_lNumStreams);
    }

    if (m_pSinkControl)
    {
        m_pSinkControl->FileHeaderReady(status, header);
    }

    return HXR_OK;
}

HX_RESULT
FileFormatSource::StreamHeaderReady(HX_RESULT status, IHXValues* header)
{
    if (header && m_is_live)
    {
        FixHeaderForLive(header);
    }

    if (m_pSinkControl)
    {
        m_pSinkControl->StreamHeaderReady(status, header);
    }

    return HXR_OK;
}

HX_RESULT
FileFormatSource::PacketReady(HX_RESULT status, IHXPacket* packet)
{
    if (status != HXR_OK)
    {
        UINT16 i;
        for (i = 0; i < m_lNumStreams; i++)
        {
            StreamDone(i);
        }
    }
    else
    {
        m_pSinkPackets->PacketReady(status, packet);
    }

    return HXR_OK;
}

HX_RESULT
FileFormatSource::SeekDone(HX_RESULT status)
{
    if (m_pSinkControl)
        m_pSinkControl->SeekDone(status);

    return HXR_OK;
}

HX_RESULT
FileFormatSource::StreamDone(UINT16 stream_number)
{
    if (m_pSinkControl)
        m_pSinkControl->StreamDone(stream_number);

    return HXR_OK;
}

HX_RESULT
FileFormatSource::Seek(UINT32 seek_time)
{
    HX_RESULT ulResult;

    ulResult = m_file_format->Seek(seek_time);

    if (ulResult == HXR_OK)
    {
        return HXR_OK;
    }
    else if (ulResult == HXR_NOTIMPL)
    {
        SeekDone(HXR_OK);
    }
    else
    {
        SeekDone(ulResult);
    }

    return HXR_OK;
}

BOOL
FileFormatSource::IsLive()
{
    return FALSE;
}

void
FileFormatSource::FixHeaderForLive(IHXValues*& pHeader)
{
  pHeader->AddRef();

    // Store the longest stream duration in the file for later use
   //   UINT32 ulValue = 0;
//      if (HXR_OK == pHeader->GetPropertyULONG32("Duration", ulValue))
//      {
//  	if (ulValue > m_ulCurrFileDuration)
//  	{
//  	    m_ulCurrFileDuration = ulValue;
//  	}
//      }

    pHeader->SetPropertyULONG32("LiveStream", 1);
    pHeader->SetPropertyULONG32("Duration", 0);

    // If an EndTime Property is specified, set it to zero. 
    // This property does not apply to live playback
    UINT32 ulEndTime = 0;
    if (HXR_OK == pHeader->GetPropertyULONG32("EndTime", ulEndTime))
    {
        pHeader->SetPropertyULONG32("EndTime", 0);
    }

    /*
     * Need to subscribe to all asm rules.
     */
    UINT32 ulStreamNumber = 0;
    pHeader->GetPropertyULONG32("StreamNumber",
        ulStreamNumber);
   
    /* 
     * See if it even has a rulebook.
     */
    IHXBuffer* pBuffer = 0;
    if(HXR_OK == pHeader->GetPropertyCString("ASMRuleBook", pBuffer))
    {
        IHXASMSource* pASMSource = 0;
        /*
         * See if it implements the necessary interface.
         */
        if(HXR_OK == m_file_format->QueryInterface(IID_IHXASMSource,
                                                   (void **)&pASMSource))
        {
            /*
             * count up the num of rules and subscribe to all.
             */
            UINT32 ulNumRules = 0;
            unsigned char* p = pBuffer->GetBuffer();
            unsigned char* pLast = p;
            while(*p)
            {
                if(*p == ';')
                {
                    ulNumRules++;
                }
                pLast = p;
                p++;
            }
            /*
             * If the rulebook was NULL or the last rule was not ; terminated,
             * then there is one more rule than ;'s
             */
            if(*pLast != ';')
            {
                ulNumRules++;
            }
            UINT32 i;
            for(i = 0; i < ulNumRules; i++)
            {
                pASMSource->Subscribe((UINT16)ulStreamNumber, (UINT16)i);
            }
            HX_RELEASE(pASMSource);
        }
        HX_RELEASE(pBuffer);
    }
    pHeader->Release();
}

