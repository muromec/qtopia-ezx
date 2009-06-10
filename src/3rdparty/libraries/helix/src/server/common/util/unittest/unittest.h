#ifndef UNITTEST_H
#define UNITTEST_H


#define UT_ASSERT(condition) \
    assertTrue(condition,__FILE__,__LINE__,#condition)


class UnitTest
{
public:
    virtual ~UnitTest() {}
    virtual void runTest() = 0;
    static int getNumSuccess()
    {
        return num_test_success;
    }

protected:
    void assertTrue(bool condition, const char* file,
        int line, const char* msg);
    static int num_test_success;
};


#endif
