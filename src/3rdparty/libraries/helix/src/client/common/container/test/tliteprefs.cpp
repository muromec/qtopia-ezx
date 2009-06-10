/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tliteprefs.cpp,v 1.3 2007/07/06 21:57:57 jfinnecy Exp $
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

#include "chxliteprefs.h"
#include "hxccf.h"
#include "unkimp.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hlxclib/stdlib.h"


class CHXSimpleFactory : public IHXCommonClassFactory,
			 public IHXScheduler,
			 public CUnknownIMP
{
public:
    CHXSimpleFactory() : m_pCallback(NULL) {}

    DECLARE_UNKNOWN(CHXSimpleFactory);

    STDMETHOD(CreateInstance)		(THIS_
					REFCLSID    /*IN*/  rclsid,
					void**	    /*OUT*/ ppUnknown)
    {
	HX_RESULT res = HXR_NOTIMPL;

	if (IsEqualIID(rclsid, CLSID_IHXBuffer))
	{
	    CHXBuffer* pBuffer = new CHXBuffer;
	    if (pBuffer)
	    {
		pBuffer->AddRef();
		res = pBuffer->QueryInterface(IID_IUnknown, ppUnknown);
		pBuffer->Release();
	    }
	    else
	    {
		res = HXR_OUTOFMEMORY;
	    }
	}

	return res;
    }

    STDMETHOD(CreateInstanceAggregatable)
				    (THIS_
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter)
    {
	return HXR_NOTIMPL;
    }

    static IHXBuffer*	Bufferize(int value)
    {
	char temp[12];
	itoa(value, temp, 10);
	return Bufferize(temp);
    }

    static IHXBuffer*	Bufferize(const char* value)
    {
	IHXBuffer* pBuffer = new CHXBuffer;
	if (pBuffer)
	{
	    pBuffer->AddRef();
	    if (SUCCEEDED(pBuffer->Set((const UCHAR*)value, strlen(value)+1)))
	    {
		return pBuffer;
	    }
	    pBuffer->Release();
	}
	return NULL;
    }

    STDMETHOD_(CallbackHandle,RelativeEnter)	(THIS_
						IHXCallback* pCallback,
						UINT32 ms)
    {
	HX_RELEASE(m_pCallback);
	m_pCallback = pCallback;
	m_pCallback->AddRef();
	return (CallbackHandle)m_pCallback;
    }

    STDMETHOD_(CallbackHandle,AbsoluteEnter)	(THIS_
						IHXCallback* pCallback,
						HXTimeval tVal)
    {
	HX_RELEASE(m_pCallback);
	m_pCallback = pCallback;
	m_pCallback->AddRef();
	return (CallbackHandle)m_pCallback;
    }

    STDMETHOD(Remove)		(THIS_
			    	CallbackHandle Handle)
    {
	if (Handle == (CallbackHandle)m_pCallback)
	{
	    HX_RELEASE(m_pCallback);
	    return HXR_OK;
	}
	return HXR_UNEXPECTED;
    }

    STDMETHOD_(HXTimeval,GetCurrentSchedulerTime)	(THIS)
    {
	HXTimeval t = {0,0};
	return t;
    }

    void    ProcessCallback()
    {
	if (m_pCallback)
	{
	    m_pCallback->Func();
	    HX_RELEASE(m_pCallback);
	}
    }

private:
    IHXCallback*    m_pCallback;
};


BEGIN_INTERFACE_LIST(CHXSimpleFactory)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXCommonClassFactory)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXScheduler)
END_INTERFACE_LIST


struct TestStr
{
    const char* pName;
    const char* pValue;
};

struct TestInt
{
    const char* pName;
    int nValue;
};

const TestStr kTestStr[] =
{
    { "foostr", "foo" },
    { "barstr", "bar" },
    { "foo\\foostr", "foo-foo" },
    { "foo\\barstr", "foo-bar" },
    { "foo\\foobarstr", "foo-foobar" },
    { "bar\\foostr", "bar-foo" },
    { NULL, NULL }
};

const TestInt kTestInt[] =
{
    { "foo", 10 },
    { "foobar", 100 },
    { "foo\\bar", 1000 },
    { "foo\\foobar", 20 },
    { "foo\\foo\\bar", 200 },
    { NULL, 0 }
};


int main(int argc, char* argv[])
{
    printf("starting liteprefs test...\n");

    CHXSimpleFactory* pFactory = new CHXSimpleFactory;
    if (pFactory)
    {
	pFactory->AddRef();

	CHXLitePrefs* pPrefs = CHXLitePrefs::CreateObject();
	if (pPrefs)
	{
	    pPrefs->AddRef();

	    HX_RESULT res = pPrefs->Init(pFactory->GetUnknown());
	    if (FAILED(res))
	    {
		printf("failed to initialize prefs\n");
	    }

	    res = pPrefs->Open("test", "test", 1, 0);
	    if (FAILED(res))
	    {
		printf("failed to open prefs\n");
	    }

	    // stick some strings into the prefs
	    {
		int n = 1;
		const TestStr* pTest = kTestStr;
		while (pTest->pName)
		{
		    IHXBuffer* pBuf = CHXSimpleFactory::Bufferize(pTest->pValue);
		    res = pPrefs->WritePref(pTest->pName, pBuf);
		    if (FAILED(res))
		    {
			printf("WritePref string test %d failed\n", n);
		    }
		    HX_RELEASE(pBuf);
		    pTest++;
		    n++;
		}
	    }

	    // hack our fake scheduler
	    pFactory->ProcessCallback();

	    // stick some ints into the prefs
	    {
		int n = 1;
		const TestInt* pTest = kTestInt;
		while (pTest->pName)
		{
		    IHXBuffer* pBuf = CHXSimpleFactory::Bufferize(pTest->nValue);
		    res = pPrefs->WritePref(pTest->pName, pBuf);
		    if (FAILED(res))
		    {
			printf("WritePref string test %d failed\n", n);
		    }
		    HX_RELEASE(pBuf);
		    pTest++;
		    n++;
		}
	    }

	    // hack our fake scheduler
	    pFactory->ProcessCallback();

	    // check our strings
	    {
		int n = 1;
		const TestStr* pTest = kTestStr;
		while (pTest->pName)
		{
		    IHXBuffer* pBuf = NULL;
		    res = pPrefs->ReadPref(pTest->pName, pBuf);
		    if (FAILED(res) || !pBuf)
		    {
			printf("ReadPref string test %d failed\n", n);
		    }
		    else if (strcmp(pTest->pValue, (const char*)pBuf->GetBuffer()))
		    {
			printf("mismatched pref string test %d\n", n);
		    }
		    HX_RELEASE(pBuf);
		    pTest++;
		    n++;
		}
	    }

	    // check our ints
	    {
		int n = 1;
		const TestInt* pTest = kTestInt;
		while (pTest->pName)
		{
		    IHXBuffer* pBuf = NULL;
		    res = pPrefs->ReadPref(pTest->pName, pBuf);
		    if (FAILED(res) || !pBuf)
		    {
			printf("ReadPref string test %d failed\n", n);
		    }
		    else if (pTest->nValue != atoi((const char*)pBuf->GetBuffer()))
		    {
			printf("mismatched pref string test %d\n", n);
		    }
		    HX_RELEASE(pBuf);
		    pTest++;
		    n++;
		}
	    }


	    HX_RELEASE(pPrefs);
	}
	else
	{
	    printf("out of memory!\n");
	}
	HX_RELEASE(pFactory);
    }
    else
    {
	printf("out of memory!\n");
    }

    printf("done!\n");

    return 0;
}


