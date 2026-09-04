#include "../../crypto_benchmarks.h"
#include "../suites.h"
#include "mac_test_logic.h"

const struct suite suite_hmac_sha1 = MAC_SUITE_ENTRY(
	"hmac", "sha1", PSA_ALG_HMAC(PSA_ALG_SHA_1), PSA_KEY_TYPE_HMAC, 160);
const struct suite suite_hmac_sha224 = MAC_SUITE_ENTRY(
	"hmac", "sha224", PSA_ALG_HMAC(PSA_ALG_SHA_224), PSA_KEY_TYPE_HMAC, 224);
const struct suite suite_hmac = MAC_SUITE_ENTRY(
	"hmac", "sha256", PSA_ALG_HMAC(PSA_ALG_SHA_256), PSA_KEY_TYPE_HMAC, 256);
const struct suite suite_hmac_sha384 = MAC_SUITE_ENTRY(
	"hmac", "sha384", PSA_ALG_HMAC(PSA_ALG_SHA_384), PSA_KEY_TYPE_HMAC, 384);
const struct suite suite_hmac_sha512 = MAC_SUITE_ENTRY(
	"hmac", "sha512", PSA_ALG_HMAC(PSA_ALG_SHA_512), PSA_KEY_TYPE_HMAC, 512);

const struct suite suite_cmac = MAC_SUITE_ENTRY(
	"cmac", "aes128", PSA_ALG_CMAC, PSA_KEY_TYPE_AES, 128);
const struct suite suite_cmac_aes192 = MAC_SUITE_ENTRY(
	"cmac", "aes192", PSA_ALG_CMAC, PSA_KEY_TYPE_AES, 192);
const struct suite suite_cmac_aes256 = MAC_SUITE_ENTRY(
	"cmac", "aes256", PSA_ALG_CMAC, PSA_KEY_TYPE_AES, 256);
