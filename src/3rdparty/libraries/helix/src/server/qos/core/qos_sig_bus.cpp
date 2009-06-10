/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qos_sig_bus.cpp,v 1.28 2008/03/14 12:57:52 yphadke Exp $
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
#include "hxcom.h"
#include "hxpiids.h"
#include "ihxpckts.h"
#include "hxqosinfo.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "hxassert.h"
#include "hxerror.h"
#include "servreg.h"
#include "safestring.h"
#include "proc.h"
#include "server_context.h"

#include "servlist.h"
#include "mutex.h"

#include "qos_sig_bus.h"

/* QoS Signal Bus*/
QoSSignalBus::QoSSignalBus(Process* pProc) :
    m_lRefCount(0),
    m_pProc (pProc),
    m_pQosProfConfigurator(NULL),
    m_pFilterTable(NULL),
    m_pTxRateRange(NULL),
    m_ulRRFrequency(0),
    m_ulRTT(0),
    m_ulPacketLoss(0),
    m_ulReceivedThroughput(0),
    m_ulPacketsSent(0),
    m_ulBytesSent(0),
    m_ulEstimatedPlayerBufferUnderruns(0),
    m_ulEstimatedPlayerBufferOverruns(0),
    m_ulBufferDepthTime(0),
    m_ulBufferDepthBytes(0),
    m_ulTotalBitrateAdaptations(0),
    m_ulCurrentBitrate(0),
    m_ulTotalUpshifts(0),
    m_ulTotalDownshifts(0),
    m_ulASMSubscribes(0),
    m_ulASMUnsubscribes(0),
    m_ulSuccessfulResends(0),
    m_ulFailedResends(0)
{
    m_pProc->pc->server_context->QueryInterface(IID_IHXQoSProfileConfigurator, (void**)&m_pQosProfConfigurator);
}

QoSSignalBus::~QoSSignalBus()
{
    Close();

    HX_RELEASE(m_pTxRateRange);

    HX_RELEASE(m_pQosProfConfigurator);
}

/* IHXQoSSignalBus */
STDMETHODIMP
QoSSignalBus::Init(IHXBuffer* pSessionId)
{
    if (pSessionId == NULL)
    {
	return HXR_INVALID_PARAMETER;
    }

    m_pFilterTable = new FilterTableNode*** [HX_QOS_SIGNAL_LAYER_COUNT+1];

    for (UINT8 i = 0; i < HX_QOS_SIGNAL_LAYER_COUNT+1; i++)
    {
	m_pFilterTable [i] = new FilterTableNode** [HX_QOS_SIGNAL_RELEVANCE_COUNT+1];

	for (UINT8 j = 0; j < HX_QOS_SIGNAL_RELEVANCE_COUNT+1; j++)
	{
	    m_pFilterTable [i][j] = new FilterTableNode* [HX_QOS_SIGNAL_COUNT+1];

	    for (UINT16 k = 0; k < HX_QOS_SIGNAL_COUNT+1; k++)
	    {
		m_pFilterTable[i][j][k] = new FilterTableNode(pSessionId);
	    }
	}
    }

    return HXR_OK;
}

STDMETHODIMP
QoSSignalBus::Close()
{
    if (!m_pFilterTable)
    {
	return HXR_OK;
    }

    for (UINT8 i = 0; i < HX_QOS_SIGNAL_LAYER_COUNT+1; i++)
    {
	for (UINT8 j = 0; j < HX_QOS_SIGNAL_RELEVANCE_COUNT+1; j++)
	{
	    for (UINT16 k = 0; k < HX_QOS_SIGNAL_COUNT+1; k++)
	    {
		HX_DELETE(m_pFilterTable[i][j][k]);
	    }
	    HX_VECTOR_DELETE(m_pFilterTable [i][j]);
	}
	HX_VECTOR_DELETE(m_pFilterTable [i]);
    }

    HX_VECTOR_DELETE(m_pFilterTable);

    return HXR_OK;
}

STDMETHODIMP
QoSSignalBus::Send(IHXQoSSignal* pSignal)
{
    if (!pSignal)
    {
	return HXR_INVALID_PARAMETER;
    }

    if (!m_pFilterTable)
    {
	return HXR_NOT_INITIALIZED;
    }

    HX_QOS_SIGNAL unId = 0;

    pSignal->GetId(unId);

    /* send to all interested sinks from most specific filter to least specific */
    (m_pFilterTable
     [((unId&HX_QOS_SIGNAL_LAYER_MASK)>>HX_QOS_SIGNAL_LAYER_OFFSET)]
     [((unId&HX_QOS_SIGNAL_RELEVANCE_MASK)>>HX_QOS_SIGNAL_RELEVANCE_OFFSET)]
     [(unId&HX_QOS_SIGNAL_ID_MASK)])->Signal(pSignal);

    (m_pFilterTable
     [((unId&HX_QOS_SIGNAL_LAYER_MASK)>>HX_QOS_SIGNAL_LAYER_OFFSET)]
     [((unId&HX_QOS_SIGNAL_RELEVANCE_MASK)>>HX_QOS_SIGNAL_RELEVANCE_OFFSET)][0])->Signal(pSignal);

    (m_pFilterTable
     [((unId&HX_QOS_SIGNAL_LAYER_MASK)>>HX_QOS_SIGNAL_LAYER_OFFSET)][0][0])->Signal(pSignal);

    return HXR_OK;
}

STDMETHODIMP
QoSSignalBus::AttachListener (HX_QOS_SIGNAL unId, IHXQoSSignalSink* pListener)
{
    if (!pListener)
    {
	return HXR_INVALID_PARAMETER;
    }

    if (!m_pFilterTable)
    {
	return HXR_NOT_INITIALIZED;
    }

    ((FilterTableNode*)(m_pFilterTable
			[((unId&HX_QOS_SIGNAL_LAYER_MASK)>>HX_QOS_SIGNAL_LAYER_OFFSET)]
			[((unId&HX_QOS_SIGNAL_RELEVANCE_MASK)>>HX_QOS_SIGNAL_RELEVANCE_OFFSET)]
			[(unId&HX_QOS_SIGNAL_ID_MASK)]))->Attach(pListener);

    return HXR_OK;
}

STDMETHODIMP
QoSSignalBus::DettachListener (HX_QOS_SIGNAL unId, IHXQoSSignalSink* pListener)
{
    if (!pListener)
    {
	return HXR_INVALID_PARAMETER;
    }

    if (!m_pFilterTable)
    {
	return HXR_NOT_INITIALIZED;
    }

    (m_pFilterTable
     [((unId&HX_QOS_SIGNAL_LAYER_MASK)>>HX_QOS_SIGNAL_LAYER_OFFSET)]
     [((unId&HX_QOS_SIGNAL_RELEVANCE_MASK)>>HX_QOS_SIGNAL_RELEVANCE_OFFSET)]
     [(unId&HX_QOS_SIGNAL_ID_MASK)])->Dettach(pListener);

    return HXR_OK;
}

/* IUnknown */
STDMETHODIMP
QoSSignalBus::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXQoSSignalBus*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSSignalBus))
    {
	AddRef();
	*ppvObj = (IHXQoSSignalBus*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSTransportAdaptationInfo))
    {
	AddRef();
	*ppvObj = (IHXQoSTransportAdaptationInfo*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSSessionAdaptationInfo))
    {
	AddRef();
	*ppvObj = (IHXQoSSessionAdaptationInfo*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSApplicationAdaptationInfo))
    {
	AddRef();
	*ppvObj = (IHXQoSApplicationAdaptationInfo*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSProfileConfigurator))
    {
        //QoSSignalBus does not implement IHXQoSProfileConfigurator interface.
        //Instead it encapsulates an instance of QoSProfileConfigurator which
        //implements IHXQoSProfileConfigurator interface. This allows us in sharing
        //one instance of IHXQoSProfileConfigurator between all QoS related classes
        //for a single client connection. Currently all QoS related classes do QI on
        //QoSSignalBus whenever they need a IHXQoSProfileConfigurator.
        //This is a temporary fix. The real fix would be to remove all instances where
        //QoSSignalBus is being QI'd for IHXQoSProfileConfigurator and implement a
        //way in the context for getting IHXQoSProfileConfigurator for a particular
        //client session.
        *ppvObj = m_pQosProfConfigurator;
        HX_ADDREF(m_pQosProfConfigurator);
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSSignalBus::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetRRFrequency()
{
    return m_ulRRFrequency;
}

STDMETHODIMP
QoSSignalBus::SetRRFrequency(UINT32 ulRRFrequency)
{
    m_ulRRFrequency = ulRRFrequency;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetRTT()
{
    return m_ulRTT;
}

STDMETHODIMP
QoSSignalBus::SetRTT(UINT32 ulRTT)
{
    m_ulRTT = ulRTT;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetPacketLoss()
{
    return m_ulPacketLoss;
}

STDMETHODIMP
QoSSignalBus::SetPacketLoss(UINT32 ulPacketLoss)
{
    m_ulPacketLoss = ulPacketLoss;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetReceivedThroughput()
{
    return m_ulReceivedThroughput;
}

STDMETHODIMP
QoSSignalBus::SetReceivedThroughput(UINT32 ulReceivedThroughput)
{
    m_ulReceivedThroughput = ulReceivedThroughput;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetSuccessfulResends()
{
    return m_ulSuccessfulResends;
}

STDMETHODIMP
QoSSignalBus::SetSuccessfulResends(UINT32 ulSuccessfulResends)
{
    m_ulSuccessfulResends = ulSuccessfulResends;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetFailedResends()
{
    return m_ulFailedResends;
}

STDMETHODIMP
QoSSignalBus::SetFailedResends(UINT32 ulFailedResends)
{
    m_ulFailedResends = ulFailedResends;
    return HXR_OK;
}

STDMETHODIMP_(IHXBuffer*)
QoSSignalBus::GetTxRateRange()
{
    if (m_pTxRateRange)
    {
        m_pTxRateRange->AddRef();
    }
    return m_pTxRateRange;
}

STDMETHODIMP
QoSSignalBus::SetTxRateRange(IHXBuffer* pTxRateRange)
{
    IHXBuffer* pTemp = m_pTxRateRange;
    m_pTxRateRange = pTxRateRange;
    if (m_pTxRateRange)
    {
        m_pTxRateRange->AddRef();
    }

    HX_RELEASE(pTemp);
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetPacketsSent()
{
    return m_ulPacketsSent;
}



STDMETHODIMP
QoSSignalBus::SetPacketsSent(UINT32 ulPacketsSent)
{
    m_ulPacketsSent = ulPacketsSent;
    return HXR_OK;
}


STDMETHODIMP_(UINT64)
QoSSignalBus::GetBytesSent()
{
    return m_ulBytesSent;
}


STDMETHODIMP
QoSSignalBus::SetBytesSent(UINT64 ulBytesSent)
{
    m_ulBytesSent = ulBytesSent;
    return HXR_OK;
}



STDMETHODIMP_(UINT32)
QoSSignalBus::GetEstimatedPlayerBufferUnderruns()
{
    return m_ulEstimatedPlayerBufferUnderruns;
}


STDMETHODIMP
QoSSignalBus::SetEstimatedPlayerBufferUnderruns(UINT32 ulEstimatedPlayerBufferUnderruns)
{
    m_ulEstimatedPlayerBufferUnderruns = ulEstimatedPlayerBufferUnderruns;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetEstimatedPlayerBufferOverruns()
{
    return m_ulEstimatedPlayerBufferOverruns;
}


STDMETHODIMP
QoSSignalBus::SetEstimatedPlayerBufferOverruns(UINT32 ulEstimatedPlayerBufferOverruns)
{
    m_ulEstimatedPlayerBufferOverruns = ulEstimatedPlayerBufferOverruns;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetBufferDepthTime()
{
    return m_ulBufferDepthTime;
}

STDMETHODIMP
QoSSignalBus::SetBufferDepthTime(UINT32 ulBufferDepthTime)
{
    m_ulBufferDepthTime = ulBufferDepthTime;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetBufferDepthBytes()
{
    return m_ulBufferDepthBytes;
}

STDMETHODIMP
QoSSignalBus::SetBufferDepthBytes(UINT32 ulBufferDepthBytes)
{
    m_ulBufferDepthBytes = ulBufferDepthBytes;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetTotalBitrateAdaptations()
{
    return m_ulTotalBitrateAdaptations;
}

STDMETHODIMP
QoSSignalBus::SetTotalBitrateAdaptations(UINT32 ulTotalBitrateAdaptations)
{
    m_ulTotalBitrateAdaptations = ulTotalBitrateAdaptations;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetCurrentBitrate()
{
    return m_ulCurrentBitrate;
}

STDMETHODIMP
QoSSignalBus::SetCurrentBitrate(UINT32 ulCurrentBitrate)
{
    m_ulCurrentBitrate = ulCurrentBitrate;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetTotalUpshifts()
{
    return m_ulTotalUpshifts;
}

STDMETHODIMP
QoSSignalBus::SetTotalUpshifts(UINT32 ulTotalUpshifts)
{
    m_ulTotalUpshifts = ulTotalUpshifts;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetTotalDownshifts()
{
    return m_ulTotalDownshifts;
}

STDMETHODIMP
QoSSignalBus::SetTotalDownshifts(UINT32 ulTotalDownshifts)
{
    m_ulTotalDownshifts = ulTotalDownshifts;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetASMSubscribes()
{
    return m_ulASMSubscribes;
}

STDMETHODIMP
QoSSignalBus::SetASMSubscribes(UINT32 ulASMSubscribes)
{
    m_ulASMSubscribes = ulASMSubscribes;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
QoSSignalBus::GetASMUnsubscribes()
{
    return m_ulASMUnsubscribes;
}

STDMETHODIMP
QoSSignalBus::SetASMUnsubscribes(UINT32 ulASMUnsubscribes)
{
    m_ulASMUnsubscribes = ulASMUnsubscribes;
    return HXR_OK;
}



/* node for filter table */
FilterTableNode::FilterTableNode(IHXBuffer* pSessionId) :
    m_pSessionId(pSessionId)
{
    HX_ASSERT(m_pSessionId);

    if (m_pSessionId)
    {
	m_pSessionId->AddRef();
    }

    m_NodeLock = HXCreateMutex();
}

FilterTableNode::~FilterTableNode()
{
    SinkListElem* pElem = NULL;

    HXMutexLock(m_NodeLock, TRUE);
    HXList_iterator j(&m_Sinks);

    for (; *j != 0; ++j)
    {
	pElem = (SinkListElem*)(*j);
	m_Sinks.remove(pElem);
        HXMutexUnlock(m_NodeLock);
    if (pElem && pElem->m_pSink)
	{
	    pElem->m_pSink->ChannelClosed(m_pSessionId);
	}

	HX_DELETE(pElem);
	HXMutexLock(m_NodeLock, TRUE);
    }
    HX_RELEASE(m_pSessionId);
    HXMutexUnlock(m_NodeLock);
    HXDestroyMutex(m_NodeLock);
}

void
FilterTableNode::Signal(IHXQoSSignal* pSignal)
{
    SinkListElem* pElem = NULL;

    HXMutexLock(m_NodeLock, TRUE);
    HXList_iterator j(&m_Sinks);

    for (; *j != 0; ++j)
    {
	pElem = (SinkListElem*)(*j);
        HXMutexUnlock(m_NodeLock);
	if (pElem->m_pSink)
	{
	    pElem->m_pSink->Signal(pSignal, m_pSessionId);
	}
        HXMutexLock(m_NodeLock, TRUE);
    }
    HXMutexUnlock(m_NodeLock);
}

void
FilterTableNode::Attach(IHXQoSSignalSink* pSink)
{
    HXMutexLock(m_NodeLock, TRUE);
    m_Sinks.insert(new SinkListElem(pSink));
    HXMutexUnlock(m_NodeLock);
}

void
FilterTableNode::Dettach(IHXQoSSignalSink* pSink)
{
    SinkListElem* pElem;

    HXMutexLock(m_NodeLock, TRUE);
    HXList_iterator i(&m_Sinks);

    for (; *i != 0; ++i)
    {
	pElem = (SinkListElem*)(*i);
	if (pElem->m_pSink == pSink)
	{
	    m_Sinks.remove(pElem);
	    HX_DELETE(pElem);
	    break;
	}
    }
    HXMutexUnlock(m_NodeLock);
}

/* QoS Signal Filter Sink List Node */
SinkListElem::SinkListElem(IHXQoSSignalSink* pSink):
    m_pSink(pSink)
{
    HX_ASSERT(m_pSink);

    if (m_pSink)
    {
	m_pSink->AddRef();
    }
}

SinkListElem::~SinkListElem()
{
    HX_RELEASE(m_pSink);
}

