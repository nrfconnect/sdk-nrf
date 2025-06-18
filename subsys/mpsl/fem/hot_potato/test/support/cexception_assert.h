#ifndef CEXCEPTION_ASSERT_H__
#define CEXCEPTION_ASSERT_H__

#include "CException.h"

#define TEST_ASSERT_FAIL_ASSERT(_code_under_test)             \
{                                                             \
    CEXCEPTION_T e;                                           \
    Try \
    {                                                         \
        _code_under_test;                                     \
        TEST_FAIL_MESSAGE("Code under test did not assert");  \
    } Catch(e) {}                                             \
}

#define TEST_ASSERT_PASS_ASSERT(_code_under_test)                 \
{                                                                 \
    CEXCEPTION_T e;                                               \
    Try {                                                         \
        _code_under_test;                                         \
    } Catch(e) {                                                  \
        TEST_FAIL_MESSAGE("Code under test failed an assertion"); \
    }                                                             \
}

#endif // CEXCEPTION_ASSERT_H__
