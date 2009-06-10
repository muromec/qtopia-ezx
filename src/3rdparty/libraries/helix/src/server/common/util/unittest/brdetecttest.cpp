#define INITGUID
#include "hxcom.h"
#include "chxpckts.h"
#include "brdetecttest.h"


void
SetStreamCountTest::init()
{
    m_pBRD = new BRDetector;
}

void
SetStreamCountTest::cleanup()
{
    HX_DELETE(m_pBRD);
}

void
SetStreamCountTest::runTest()
{
    HX_RESULT res;

    // 1a. call with stream count 0
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->SetStreamCount(0);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 1b. call with stream count above limit
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->SetStreamCount(100);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 1d. call once with valid arguments
	// expected result: HXR_OK
    // 1e. call twice with proper StreamCount value
	// expected result: HXR_FAIL on second call
    init();
    res = m_pBRD->SetStreamCount(3);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->SetStreamCount(3);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();
}


void
OnStreamHeaderTest::init()
{
    m_pBRD = new BRDetector;
    m_pSHdr = new CHXHeader;
    m_pSHdr->AddRef();
}

void
OnStreamHeaderTest::cleanup()
{
    HX_DELETE(m_pBRD);
    HX_RELEASE(m_pSHdr);
}

void
OnStreamHeaderTest::runTest()
{
    HX_RESULT res;

    // 2a. call with NULL argument
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->OnStreamHeader(NULL);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 2b. call before calling SetStreamCount
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 2c. call with out of range stream number
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->SetStreamCount(2);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 3);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 2d. call twice with same StreamNumber
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 2e. call with stream 0 and 1 with StreamCount set to 2
	// expexted result: HXR_OK
    init();
    res = m_pBRD->SetStreamCount(2);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 1);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    cleanup();
}


void
GetTypeTest::init()
{
    m_pBRD = new BRDetector;
    m_pSHdr = new CHXHeader;
    m_pSHdr->AddRef();
    m_pBufA = new CHXBuffer;
    m_pBufA->AddRef();
    m_pBufB = new CHXBuffer;
    m_pBufB->AddRef();
}

void
GetTypeTest::cleanup()
{
    HX_DELETE(m_pBRD);
    HX_RELEASE(m_pSHdr);
    HX_RELEASE(m_pBufA);
    HX_RELEASE(m_pBufB);
}

void
GetTypeTest::runTest()
{
    HX_RESULT res;
    StreamType type;

    // 3a. call before callinq SetStreamCount
	// expected result: HXR_FAIL
    // 3b. call before calling OnStreamHeader
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->GetType(0, 0, type);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 3c. call without providing mime type
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 0, type);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 3d. call with mime type
	// expected result: HXR_OK
    // 3e. call with in range stream number
	// expected result: HXR_OK
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pBufA->Set((const UCHAR*)"audio/foo", 9);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 0, type);
    UT_ASSERT(res == HXR_OK);
    cleanup();

    // 3f. call with out of range stream number
	// expected result: HXR_FAIL
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pBufA->Set((const UCHAR*)"audio/foo", 9);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(1, 0, type);
    UT_ASSERT(res == HXR_FAIL);
    cleanup();

    // 3g. call with out of range rule
	// expected result: HXR_FAIL
    // 3h. call with in range rule
	// expected result: HXR_OK
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pBufA->Set((const UCHAR*)"audio/foo", 9);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    m_pBufB->Set((const UCHAR*)"a=1,b=2;c=3;", 12);
    m_pSHdr->SetPropertyCString("ASMRuleBook", m_pBufB);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 2, type);
    UT_ASSERT(res == HXR_FAIL);
    res = m_pBRD->GetType(0, 1, type);
    UT_ASSERT(res == HXR_OK);
    cleanup();
}


void
FunctionalityTest::init()
{
    m_pBRD = new BRDetector;
    m_pSHdr = new CHXHeader;
    m_pSHdr->AddRef();
    m_pBufA = new CHXBuffer;
    m_pBufA->AddRef();
    m_pBufB = new CHXBuffer;
    m_pBufB->AddRef();
}

void
FunctionalityTest::cleanup()
{
    HX_DELETE(m_pBRD);
    HX_RELEASE(m_pSHdr);
    HX_RELEASE(m_pBufA);
    HX_RELEASE(m_pBufB);
}

void
FunctionalityTest::runTest()
{
    HX_RESULT res;
    StreamType type;

    // 4a. mime type == realevent
	// expected result: HXR_OK, STRM_INVALID
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pBufA->Set((const UCHAR*)"application/x-pn-realevent", 26);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 0, type);
    UT_ASSERT(res == HXR_OK && type == STRM_INVALID);
    cleanup();

    // 4b. AvgBitRate and MaxBitRate exist in header and are unequal
	// expected result: HXR_OK, STRM_TSD
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pSHdr->SetPropertyULONG32("AvgBitRate", 5000);
    m_pSHdr->SetPropertyULONG32("MaxBitRate", 5000);
    m_pBufA->Set((const UCHAR*)"audio/x-pn-foo", 26);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 0, type);
    UT_ASSERT(res == HXR_OK && type == STRM_TSD);
    cleanup();

    // 4c. AvgBitRate and MaxBitRate exist in header and are equal, 
    // and rulebook doesn't exist
	// expected result: HXR_OK, STRM_TSD
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pSHdr->SetPropertyULONG32("AvgBitRate", 5000);
    m_pSHdr->SetPropertyULONG32("MaxBitRate", 6000);
    m_pBufA->Set((const UCHAR*)"audio/x-pn-foo", 26);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 0, type);
    UT_ASSERT(res == HXR_OK && type == STRM_TSD);
    cleanup();

    // 4d. AvgBitRate and MaxBitRate do not exist in header, and 
    // rulebook doesn't exist
	// expected result: HXR_OK, STRM_TSD
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pBufA->Set((const UCHAR*)"audio/x-pn-foo", 26);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 1, type);
    UT_ASSERT(res == HXR_OK && type == STRM_TSD);
    cleanup();

    // 4e. ASMRuleBook exists, and TimeStampDelivery=t is in rule
	// expected result: HXR_OK, STRM_TSD
    // 4f. ASMRuleBook exists, and TimeStampDelivery=t is not in rule
	// expected result: HXR_OK, STRM_CBR
    init();
    res = m_pBRD->SetStreamCount(1);
    UT_ASSERT(res == HXR_OK);
    m_pSHdr->SetPropertyULONG32("StreamNumber", 0);
    m_pBufA->Set((const UCHAR*)"audio/x-pn-foo", 26);
    m_pSHdr->SetPropertyCString("MimeType", m_pBufA);
    m_pBufB->Set((const UCHAR*)"a=1,TimeStampDelivery=T;c=3,d=4;", 32);
    m_pSHdr->SetPropertyCString("ASMRuleBook", m_pBufB);
    res = m_pBRD->OnStreamHeader(m_pSHdr);
    UT_ASSERT(res == HXR_OK);
    res = m_pBRD->GetType(0, 0, type);
    UT_ASSERT(res == HXR_OK && type == STRM_TSD);
    res = m_pBRD->GetType(0, 1, type);
    UT_ASSERT(res == HXR_OK && type == STRM_CBR);
    cleanup();
}




