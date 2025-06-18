#include "CException.h"

/* assert redefinition that instead of failing throws a CException to be handled by the test framework. */
void __assert_fail(const char * file, int line, const char * func, const char * cond)
{
    (void)file;
    (void)line;
    (void)func;
    (void)cond;
    Throw (0);
}
