#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "xof_test_logic.h"
const struct suite suite_shake128 = XOF_SUITE_ENTRY("shake128", PSA_ALG_SHAKE128);
const struct suite suite_shake256 = XOF_SUITE_ENTRY("shake256", PSA_ALG_SHAKE256);
