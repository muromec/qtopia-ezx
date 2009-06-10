/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: network_svc_test.cpp,v 1.3 2007/07/06 21:58:25 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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
#include "network_svc_test.h"

#include "hx_unit_test.h"
#include "hx_ut_debug.h"

#include "minictx.h"
#include "remote_logger.h"

#include "platform/symbian/process_events.h"


HXNetworkServicesTest::HXNetworkServicesTest() :
    m_pContext(0),
    m_pNetSvc(0)
{}

HXNetworkServicesTest::~HXNetworkServicesTest()
{
    Reset();
}

const char* HXNetworkServicesTest::DefaultCommandLine() const
{
    return "HXNetworkServicesTest";
}

bool HXNetworkServicesTest::Init(int argc, char* argv[])
{
    Reset();

    m_pContext = new HXMiniContext;

    if (m_pContext)
	m_pContext->AddRef();

    return (m_pContext != 0);
}

bool HXNetworkServicesTest::Start()
{
    bool bRet = false;

    HXRemoteLogger* pLogger = new HXRemoteLogger;

    if (!pLogger)
    {
	DPRINTF(D_ERROR, ("Failed to create logger\n"));
    }
    else if (HXR_OK != pLogger->Init(m_pContext,
				     "207.188.30.91",
				     1234,
				     "tnetsvc.txt"))
    {
	DPRINTF(D_ERROR, ("Failed to initialize logger\n"));
    }
    else
    {
	for (int i = 0; i < 30; i++)
	{
	    char buf[100];
	    sprintf(buf, "This is a test %d\n", i);
	    
	    pLogger->Log(buf);

	    SymbianProcessEvents(1000);
	}

	bRet = true;
    }

    return bRet;
}

void HXNetworkServicesTest::Reset()
{
    HX_RELEASE(m_pNetSvc);

    if (m_pContext)
    {
	m_pContext->Close();
    }

    HX_RELEASE(m_pContext);
}
