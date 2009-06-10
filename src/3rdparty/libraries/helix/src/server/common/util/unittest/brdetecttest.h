#ifndef BRDETECTTEST_H
#define BRDETECTTEST_H


#include "unittest.h"
#include "brdetect.h"


class SetStreamCountTest : public UnitTest
{
public:
    void runTest();

private:
    void init();
    void cleanup();

    BRDetector*     m_pBRD;
};


class OnStreamHeaderTest : public UnitTest
{
public:
    void runTest();

private:
    void init();
    void cleanup();

    BRDetector*     m_pBRD;
    IHXValues*      m_pSHdr;
};


class GetTypeTest : public UnitTest
{
public:
    void runTest();

private:
    void init();
    void cleanup();

    BRDetector*     m_pBRD;
    IHXValues*      m_pSHdr;
    IHXBuffer*      m_pBufA;
    IHXBuffer*      m_pBufB;
};


class FunctionalityTest : public UnitTest
{
public:
    void runTest();

private:
    void init();
    void cleanup();

    BRDetector*     m_pBRD;
    IHXValues*      m_pSHdr;
    IHXBuffer*      m_pBufA;
    IHXBuffer*      m_pBufB;
};


#endif
