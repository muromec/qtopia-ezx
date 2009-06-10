/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qos_diffserv_cfg.cpp,v 1.4 2004/06/02 20:02:02 tmarshall Exp $
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
#include "ihxpckts.h"
#include "hxengin.h"
#include "hxnet.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "hxerror.h"

#include "chxmapstringtoob.h"
#include "timeval.h"
#include "servreg.h"

#include "streamer_container.h"
#include "proc.h"
#include "proc_container.h"
#include "globals.h"
#include "streamer_info.h"
#include "base_errmsg.h"

#include "qos_diffserv_cfg.h"

QoSDiffServConfigurator::QoSDiffServConfigurator (Process* pProc) :
  m_lRefCount (0),
  m_pDSCP (NULL),
  m_pProc (pProc)
{
    m_pDSCP = new UINT8 [HX_QOS_DIFFSERV_CLASS_COUNT];
    memset(m_pDSCP, 0, (HX_QOS_DIFFSERV_CLASS_COUNT) * sizeof(UINT8));
}

QoSDiffServConfigurator::~QoSDiffServConfigurator ()
{
    HX_VECTOR_DELETE(m_pDSCP);
}

/* IHXQoSDiffServConfigurator */
STDMETHODIMP
QoSDiffServConfigurator::ConfigureSocket(IHXSocket* pSock,
                                         HX_QOS_DIFFSERV_CLASS cClass)
{
    if ((pSock == NULL) || (cClass >= HX_QOS_DIFFSERV_CLASS_COUNT))
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RESULT hRes = HXR_OK;

    INT32 lTemp = 0;
    if (SUCCEEDED(m_pProc->pc->registry->GetInt("config.DiffServ.Media", &lTemp, m_pProc)))
    {
        m_pDSCP [HX_QOS_DIFFSERV_CLASS_MEDIA] = (UINT8)lTemp;
    }
    if (SUCCEEDED(m_pProc->pc->registry->GetInt("config.DiffServ.Control", &lTemp, m_pProc)))
    {
        m_pDSCP [HX_QOS_DIFFSERV_CLASS_CONTROL] = (UINT8)lTemp;
    }
    if (SUCCEEDED(m_pProc->pc->registry->GetInt("config.DiffServ.Admin", &lTemp, m_pProc)))
    {
        m_pDSCP [HX_QOS_DIFFSERV_CLASS_ADMIN] = (UINT8)lTemp;
    }
    if (SUCCEEDED(m_pProc->pc->registry->GetInt("config.DiffServ.Dist", &lTemp, m_pProc)))
    {
        m_pDSCP [HX_QOS_DIFFSERV_CLASS_DIST] = (UINT8)lTemp;
    }

    //XXXTDM: only applicable to IPv4; what is desired IPv6 behavior?
    hRes = (m_pDSCP[cClass]) ? pSock->SetOption(HX_SOCKOPT_IN4_TOS, m_pDSCP[cClass]) : HXR_OK;

    if (FAILED(hRes))
    {
        ERRMSG(m_pProc->pc->error_handler, "Failed to set DSCP value %d. Check user permissions.\n", m_pDSCP[cClass]);
    }

    return hRes;
}

/* IHXUnknown methods */
STDMETHODIMP
QoSDiffServConfigurator::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXQoSDiffServConfigurator*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSDiffServConfigurator))
    {
        AddRef();
        *ppvObj = (IHXQoSDiffServConfigurator*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSDiffServConfigurator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSDiffServConfigurator::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}
