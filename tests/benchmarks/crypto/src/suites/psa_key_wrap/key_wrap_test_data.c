#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "key_wrap_test_logic.h"

const struct suite suite_aes_kw_128 = KEY_WRAP_SUITE_ENTRY("aes_kw", "aes128", PSA_ALG_KW, 128);
const struct suite suite_aes_kw_192 = KEY_WRAP_SUITE_ENTRY("aes_kw", "aes192", PSA_ALG_KW, 192);
const struct suite suite_aes_kw_256 = KEY_WRAP_SUITE_ENTRY("aes_kw", "aes256", PSA_ALG_KW, 256);
const struct suite suite_aes_kwp_128 = KEY_WRAP_SUITE_ENTRY("aes_kwp", "aes128", PSA_ALG_KWP, 128);
