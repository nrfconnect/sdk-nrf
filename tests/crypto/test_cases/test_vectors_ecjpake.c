/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "common_test.h"
#include <mbedtls/ecjpake.h>

/*
 * Test data as used by ARMmbed: https://github.com/ARMmbed/mbed-crypto/blob/master/library/ecjpake.c
 */

static const unsigned char ecjpake_password[] =
	"7468726561646a70616b6574657374";
static const unsigned char ecjpake_x1[] =
	"0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f21";
static const unsigned char ecjpake_x2[] =
	"6162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f81";
static const unsigned char ecjpake_x3[] =
	"6162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f81";
static const unsigned char ecjpake_x4[] =
	"c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe1";
static const unsigned char ecjpake_round_msg_cli_1[] =
	"4104accf0106ef858fa2d919331346805a78b58bbad0b844e5c7892879146187dd2666ada7"
	"81bb7f111372251a8910621f634df128ac48e381fd6ef9060731f694a441041dd0bd5d4566"
	"c9bed9ce7de701b5e82e08e84b730466018ab903c79eb982172236c0c1728ae4bf73610d34"
	"de44246ef3d9c05a2236fb66a6583d7449308babce2072fe16662992e9235c25002f11b150"
	"87b82738e03c945bf7a2995dda1e98345841047ea6e3a4487037a9e0dbd79262b2cc273e77"
	"9930fc18409ac5361c5fe669d702e147790aeb4ce7fd6575ab0f6c7fd1c335939aa863ba37"
	"ec91b7e32bb013bb2b4104a49558d32ed1ebfc1816af4ff09b55fcb4ca47b2a02d1e7caf11"
	"79ea3fe1395b22b861964016fabaf72c975695d93d4df0e5197fe9f040634ed59764937787"
	"be20bc4deebbf9b8d60a335f046ca3aa941e45864c7cadef9cf75b3d8b010e443ef0";
static const unsigned char ecjpake_round_msg_cli_2[] =
	"410469d54ee85e90ce3f1246742de507e939e81d1dc1c5cb988b58c310c9fdd9524d93720b"
	"45541c83ee8841191da7ced86e3312d43623c1d63e74989aba4affd1ee4104077e8c31e20e"
	"6bedb760c13593e69f15be85c27d68cd09ccb8c4183608917c5c3d409fac39fefee82f7292"
	"d36f0d23e055913f45a52b85dd8a2052e9e129bb4d200f011f19483535a6e89a580c9b0003"
	"baf21462ece91a82cc38dbdcae60d9c54c";
static const unsigned char ecjpake_round_msg_srv_1[] =
	"41047ea6e3a4487037a9e0dbd79262b2cc273e779930fc18409ac5361c5fe669d702e14779"
	"0aeb4ce7fd6575ab0f6c7fd1c335939aa863ba37ec91b7e32bb013bb2b410409f85b3d20eb"
	"d7885ce464c08d056d6428fe4dd9287aa365f131f4360ff386d846898bc4b41583c2a5197f"
	"65d78742746c12a5ec0a4ffe2f270a750a1d8fb51620934d74eb43e54df424fd96306c0117"
	"bf131afabf90a9d33d1198d905193735144104190a07700ffa4be6ae1d79ee0f06aeb544cd"
	"5addaabedf70f8623321332c54f355f0fbfec783ed359e5d0bf7377a0fc4ea7ace473c9c11"
	"2b41ccd41ac56a56124104360a1cea33fce641156458e0a4eac219e96831e6aebc88b3f375"
	"2f93a0281d1bf1fb106051db9694a8d6e862a5ef1324a3d9e27894f1ee4f7c59199965a8dd"
	"4a2091847d2d22df3ee55faa2a3fb33fd2d1e055a07a7c61ecfb8d80ec00c2c9eb12";
static const unsigned char ecjpake_round_msg_srv_2[] =
	"03001741040fb22b1d5d1123e0ef9feb9d8a2e590a1f4d7ced2c2b06586e8f2a16d4eb2fda"
	"4328a20b07d8fd667654ca18c54e32a333a0845451e926ee8804fd7af0aaa7a641045516ea"
	"3e54a0d5d8b2ce786b38d383370029a5dbe4459c9dd601b408a24ae6465c8ac905b9eb03b5"
	"d3691c139ef83f1cd4200f6c9cd4ec392218a59ed243d3c820ff724a9a70b88cb86f20b434"
	"c6865aa1cd7906dd7c9bce3525f508276f26836c";
static const unsigned char ecjpake_ss[] =
	"f3d47f599844db92a569bbe7981e39d931fd743bf22e98f9b438f719d3c4f351";

/*
 * Uses empty initial data on both sides and deterministic rng.
 * Derive a secret for both client and server.
 * Should verify:
 *      Derived secrets same length.
 *      Derived secrets equal data.
 */
ITEM_REGISTER(
	test_vector_ecjpake_random_data,
	test_vector_ecjpake_t test_vector_ecjpake_trivial_random_handshake) = {
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_password = ecjpake_password,
	.p_test_vector_name = TV_NAME("Trivial random handshake")
};

/*
 * Uses pre-made private keys to generate public keys.
 * Thus only ECJPAKE reads are done, not writes.
 * Messages are also pre-defined.
 * Should verify:
 *      Derived secret client same length as pre-made secret.
 *      Derived secret server same length as pre-made secret.
 *      Derived secret client equal data in pre-made secret.
 *      Derived secret server equal data in pre-made secret.
 */
ITEM_REGISTER(test_vector_ecjpake_given_data,
	      test_vector_ecjpake_t test_vector_ecjpake_given_data_001) = {
	.expected_err_code = 0,
	.expected_result = EXPECTED_TO_PASS,
	.p_test_vector_name = TV_NAME("Predefined private keys"),
	.p_password = ecjpake_password,
	.p_priv_key_client_1 = ecjpake_x1,
	.p_priv_key_client_2 = ecjpake_x2,
	.p_priv_key_server_1 = ecjpake_x3,
	.p_priv_key_server_2 = ecjpake_x4,
	.p_round_message_client_1 = ecjpake_round_msg_cli_1,
	.p_round_message_client_2 = ecjpake_round_msg_cli_2,
	.p_round_message_server_1 = ecjpake_round_msg_srv_1,
	.p_round_message_server_2 = ecjpake_round_msg_srv_2,
	.p_expected_shared_secret = ecjpake_ss,
};
