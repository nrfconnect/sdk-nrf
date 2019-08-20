/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>

#include "common_test.h"
#include <mbedtls/hkdf.h>

/**@brief HKDF test vectors can be found in RFC 5869 document.
 *
 * https://tools.ietf.org/html/rfc5869
 */

const char hkdf_ikm_len_22[] = {
	"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"
};
const char hkdf_ikm_len_80[] = {
	"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021222324"
	"25262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40414243444546474849"
	"4a4b4c4d4e4f"
};
const char hkdf_ikm_len_250[] = {
	"d9ffac12ae7a19e13c9e92b21e341bff5e2a949f240a55595d9cbcd77512480b435237341e"
	"dbc831dfc524f35ad8b95d238629d66849d5b66edda67907cbe5556f175a3dedd8f5e5d5ef"
	"12102fa1dde3e279b559130d0b441f1a20c04f5dbeb2bf0912272f29b96c390fba1b36a951"
	"fea808275c8713b9685398bbbb1ba64f069b231f49c3095d2c95471b27df56acd671d7cbe7"
	"817826d107815af721f3f7d262c651f1ebae961979778eb37dac8ce75f1efdb703789764a0"
	"d34600ffc056e331dab60b1d207a5935649fb75e5a8d3ea6b09a20954736fad51a4b031a2e"
	"961efa85b65a7c7b02345c199e90d8be40bb28496ad1ea93c9daeb55"
};
const char hkdf_dummy_okm[] = { "3a8d5dc16eba7ac69b38" };
const char hkdf_salt_len_13[] = { "000102030405060708090a0b0c" };
const char hkdf_salt_len_20[] = { "1911bff47c578781d0609cb563bb7da69c27fd18" };
const char hkdf_salt_len_80[] = {
	"606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f8081828384"
	"85868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9"
	"aaabacadaeaf"
};
const char hkdf_salt_len_250[] = {
	"7d1ca5017a99229aae0e03c5bfb98b1425e8649956123341543edd6e75600a73fad29a00e7"
	"06d68643133758b3602e4522071704eb212dfd613ab48f8826a90c944f626c13f6762d4361"
	"a322d4f322c52d659ec0dbabf70d9e2daca42bb2333f5d9bbea1376a2fd895c5dda48270c1"
	"757100e9083973e2fb0cdbbeadd44e40309887f300ca59a71167347f5d2e5583b4d5525ded"
	"f7f57692397828c7009a834df2f2a711e9c892f6c706d3bf10a8c07db4c42ab74d75f8e249"
	"e879a5e96329f04600e1bf81ee567ce0c06bd2a1f969feca465a75f996e7a79523b89ef8e2"
	"2b85ecf436d7cfe7b4e6f535e0a661398ca5711f3f4513d864381702"
};
const char hkdf_info_len_10[] = { "f0f1f2f3f4f5f6f7f8f9" };
const char hkdf_info_len_20[] = { "3a594a18b699ef8819008ed38c3aa4320581db9d" };
const char hkdf_info_len_80[] = {
	"b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4"
	"d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9"
	"fafbfcfdfeff"
};
const char hkdf_info_len_250[] = {
	"ccfafb6a30475d53b3a2d420825c54d7e026874e913173108ac83c99aae278b1850538123e"
	"c8f9dd00cc18cf0a66d271bf69c8035f1301c11a241eaaf9be56b99b65ef1a596fdb49e46a"
	"acbfb39cd0afcd45516ab3b2f7e9fbf16246642f24ffc1d04d5d5e5694569d207feef75ba9"
	"55cb119fa4f691a9bce51d32281795ffbd41fb157387b911e252a676b9dbf8e94e0e371495"
	"b15602b527ebcfe1ae1773196fad39de7c5351869724f7ef17586ae9d85af304ba132db50f"
	"a1449e297f650325558824291b88cbac38f7d27a24d8c18f0ba6bd0e749079184693e21ffe"
	"9ee3dae068485e8b22663696f2d70a04e61ceceeb2da839bfbb30838"
};

/* HKDF - SHA-256 Custom test case 1 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_c1) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = MBEDTLS_ERR_HKDF_BAD_INPUT_DATA,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA256 Expand and Extract ikm_len=0 "
				      "okm_len=10 salt_len=13 info_len=10"),
	.p_ikm = "",
	.p_okm = hkdf_dummy_okm,
	.p_prk = "",
	.p_salt = hkdf_salt_len_20,
	.p_info = hkdf_info_len_20
};

/* HKDF - SHA-256 Custom test case 2 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_c2) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME(
		"SHA256 Expand and Extract ikm_len=1 okm_len=1 salt_len=1 info_len=1"),
	.p_ikm = "ab",
	.p_okm = "53",
	.p_prk =
		"1f3624af63d5221a80b6d6cbb7d372e595cb512f4ad248643d8d0a74f0be8335",
	.p_salt = "1b",
	.p_info = "6f"
};

/* HKDF - SHA-256 Custom test case 3 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_c3) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME(
		"SHA256 Expand and Extract Invalid ikm_len=20 okm_len=0 "
		"salt_len=13 info_len=10"),
	.p_ikm = hkdf_ikm_len_22,
	.p_okm = "",
	.p_prk =
		"077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
	.p_salt = hkdf_salt_len_13,
	.p_info = hkdf_info_len_10
};

/* HKDF - SHA-256 Custom test case 4 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_c4) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA256 Expand and Extract ikm_len=250 "
				      "okm_len=250 salt_len=250 info_len=250"),
	.p_ikm = hkdf_ikm_len_250,
	.p_okm =
		"825c45e910cfbcdb12d101543cd837bbb654b54e097f45a9092fcb296a61961ff"
		"e5e64c081beb8a143d34824c9c72ba51d58f53bc4a537bb1fbfd6ea0d32651241"
		"a7a4bf724d8b51b026e4f64f4319b873ca8e77256e022911005e362a302930605"
		"659e76c64db566486eaf78fe21fbe0a7caefbccc0ef09d642e9355209c036bbdd"
		"64bdefbabb6b79b750c0ebf3f60e62071ce902d76c9af02a24eddc62cc5670439"
		"802888dca0d7d954be732c57e9aacd405687a2bc072176052c467347f7e8d0191"
		"eb42101ded1a29f1e2bc9e6166f457724f059430d050dc8e5891c7cd37b15b841"
		"371d6c7774c25ec79c6dc71ef52ca20f4619110785325",
	.p_prk =
		"c5b46e0300b99717bf1a005ab63ca09d9478f618efc17a8ef909c0ff9e3c0d84",
	.p_salt = hkdf_salt_len_250,
	.p_info = hkdf_info_len_250
};

/* HKDF - RFC5869 - Test Case 1 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_1) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA256 Expand and Extract ikm_len=22 "
				      "okm_len=42 salt_len=13 info_len=10"),
	.p_ikm = hkdf_ikm_len_22,
	.p_okm = "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf3"
		 "4007208d5b887185865",
	.p_prk =
		"077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
	.p_salt = hkdf_salt_len_13,
	.p_info = hkdf_info_len_10
};

/* HKDF - RFC5869 - Test Case 2 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_2) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA256 Expand and Extract ikm_len=80 "
				      "okm_len=82 salt_len=80 info_len=80"),
	.p_ikm = hkdf_ikm_len_80,
	.p_okm =
		"b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c5"
		"9045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71cc"
		"30c58179ec3e87c14c01d5c1f3434f1d87",
	.p_prk =
		"06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244",
	.p_salt = hkdf_salt_len_80,
	.p_info = hkdf_info_len_80
};

/* HKDF - RFC5869 - Test Case 3 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_256_3) = {
	.digest_type = MBEDTLS_MD_SHA256,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA256 Expand and Extract ikm_len=22 "
				      "okm_len=42 salt_len=0 info_len=0"),
	.p_ikm = hkdf_ikm_len_22,
	.p_okm = "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9"
		 "d201395faa4b61a96c8",
	.p_prk =
		"19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04",
	.p_salt = "",
	.p_info = ""
};

#if defined(CONFIG_MBEDTLS_SHA512_C)

/* HKDF - SHA-512 Custom test case 1 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_512_c1) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_err_code_expand = MBEDTLS_ERR_HKDF_BAD_INPUT_DATA,
	.expected_result = EXPECTED_TO_FAIL,
	.expected_result_expand = EXPECTED_TO_FAIL,
	.p_test_vector_name = TV_NAME("SHA512 Expand and Extract ikm_len=0 "
				      "okm_len=10 salt_len=13 info_len=10"),
	.p_ikm = "",
	.p_okm = hkdf_dummy_okm,
	.p_prk = "",
	.p_salt = hkdf_salt_len_20,
	.p_info = hkdf_info_len_20
};

/* HKDF - SHA-512 Custom test case 2 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_512_c2) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME(
		"SHA512 Expand and Extract ikm_len=1 okm_len=1 salt_len=1 info_len=1"),
	.p_ikm = "ab",
	.p_okm = "51",
	.p_prk =
		"37fc3ce6c9e6515c26b58e36bfcd288ba4cedd03c96d83a71dcfa9d0792f671f2"
		"0dc2e98470057c79e740053e040385696bb303c643d0cefe3471113ef693e76",
	.p_salt = "1b",
	.p_info = "6f"
};

/* HKDF - SHA-512 Custom test case 3 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_512_c3) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME(
		"SHA512 Expand and Extract Invalid "
		"ikm_len=20 okm_len=0 salt_len=1 info_len=1"),
	.p_ikm = hkdf_ikm_len_22,
	.p_okm = "",
	.p_prk =
		"665799823737ded04a88e47e54a5890bb2c3d247c7a4254a8e61350723590a26c"
		"36238127d8661b88cf80ef802d57e2f7cebcf1e00e083848be19929c61b4237",
	.p_salt = hkdf_salt_len_13,
	.p_info = hkdf_info_len_10
};

/* HKDF - SHA-512 Custom test case 4 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_512_c4) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA512 Expand and Extract ikm_len=250 "
				      "okm_len=250 salt_len=250 info_len=250"),
	.p_ikm = hkdf_ikm_len_250,
	.p_okm =
		"43e3f0defa3572efcc26b1c4e4ef47be92cf1fc873e144dc8b2e655c7adc25a7c"
		"2487fa0d6d58efa2106c6ed9a99a6a6639530cf68fb67ec769f0afc729f35bc54"
		"9afb840915c2b3ebc79aedeb94fddff3e81d32e8cb90b0b851d2d6a3436c57b63"
		"154ef0ec026249722538f8d6e1c2b26633e994c1c0ca6886c3348f27551742dce"
		"7f9b03c44564b2b709c39c9a6e99815cc2ccfaf3daab7d39c2687e30d561a1218"
		"640e0ae0e655028268d358723b15dab2a2f1da1c9abbb86416d8926f39a6570a7"
		"7cd3a430855a07462d40d73d20f023da4ed41c8d243c214e040ce9484d0d4ec59"
		"27a496a4154ecdd9395233f2e9e447f69e4422d438a3f",
	.p_prk =
		"f7c9352c4f7a6c451ebd93638975aed3aec3e5127adae77d8074f80844caeac2f"
		"b7aabeb37957cdcf738f2282a95b1468ddc014fb00a2d297e7b48d6e2812ccf",
	.p_salt = hkdf_salt_len_250,
	.p_info = hkdf_info_len_250
};

/* HKDF - Input based on RFC5869, but with SHA-512 - Test Case 1 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_512_1) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA512 Expand and Extract ikm_len=22 "
				      "okm_len=42 salt_len=13 info_len=10"),
	.p_ikm = hkdf_ikm_len_22,
	.p_okm = "832390086cda71fb47625bb5ceb168e4c8e26a1a16ed34d9fc7fe92c148157933"
		 "8da362cb8d9f925d7cb",
	.p_prk =
		"665799823737ded04a88e47e54a5890bb2c3d247c7a4254a8e61350723590a26c"
		"36238127d8661b88cf80ef802d57e2f7cebcf1e00e083848be19929c61b4237",
	.p_salt = hkdf_salt_len_13,
	.p_info = hkdf_info_len_10
};

/* HKDF - Input based on RFC5869, but with SHA-512 - Test Case 2 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_512_2) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA512 Expand and Extract ikm_len=80 "
				      "okm_len=82 salt_len=80 info_len=80"),
	.p_ikm = hkdf_ikm_len_80,
	.p_okm =
		"ce6c97192805b346e6161e821ed165673b84f400a2b514b2fe23d84cd189ddf1b"
		"695b48cbd1c8388441137b3ce28f16aa64ba33ba466b24df6cfcb021ecff235f6"
		"a2056ce3af1de44d572097a8505d9e7a93",
	.p_prk =
		"35672542907d4e142c00e84499e74e1de08be86535f924e022804ad775dde27ec"
		"86cd1e5b7d178c74489bdbeb30712beb82d4f97416c5a94ea81ebdf3e629e4a",
	.p_salt = hkdf_salt_len_80,
	.p_info = hkdf_info_len_80
};

/* HKDF - Input based on RFC5869, but with SHA-512 - Test Case 3 */
ITEM_REGISTER(test_vector_hkdf_data,
	      test_vector_hkdf_t test_vector_hkdf_512_3) = {
	.digest_type = MBEDTLS_MD_SHA512,
	.expected_err_code = 0,
	.expected_err_code_expand = 0,
	.expected_result = EXPECTED_TO_PASS,
	.expected_result_expand = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("SHA512 Expand and Extract ikm_len=22 "
				      "okm_len=42 salt_len=0 info_len=0"),
	.p_ikm = hkdf_ikm_len_22,
	.p_okm = "f5fa02b18298a72a8c23898a8703472c6eb179dc204c03425c970e3b164bf90ff"
		 "f22d04836d0e2343bac",
	.p_prk =
		"fd200c4987ac491313bd4a2a13287121247239e11c9ef82802044b66ef357e5b1"
		"94498d0682611382348572a7b1611de54764094286320578a863f36562b0df6",
	.p_salt = "",
	.p_info = ""
};

#endif /* MBEDTLS_SHA512_C */
