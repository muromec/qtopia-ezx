#include <stdio.h>
#include "brdetecttest.h"


int
main()
{
    SetStreamCountTest sscTest;
    sscTest.runTest();

    OnStreamHeaderTest oshTest;
    oshTest.runTest();

    GetTypeTest gtTest;
    gtTest.runTest();

    FunctionalityTest fTest;
    fTest.runTest();

    printf("%d tests completed successfully\n",
        UnitTest::getNumSuccess());
    return 0;
}
