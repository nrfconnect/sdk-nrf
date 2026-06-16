/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <app_jwt.h>

#include <psa/crypto.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if IS_ENABLED(CONFIG_BOARD_NRF9251DK_NRF9251_CPUAPP)
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <date_time.h>

static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(time_update_finished, 0, 1);
#endif /* CONFIG_BOARD_NRF9251DK_NRF9251_CPUAPP */

LOG_MODULE_REGISTER(jwt_sample, CONFIG_APPLICATION_JWT_LOG_LEVEL);

#define JWT_AUDIENCE_STR "JSON web token for demonstration"

#define RANDOM_STR_MAX_SIZE 33

static int generate_ecdsa_keypair(uint32_t *user_keypair_id)
{
	int err = -1;

	if (user_keypair_id != NULL) {
		psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
		psa_status_t status;

		/* Initialize PSA Crypto */
		status = psa_crypto_init();
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_crypto_init() failed! (Error: %d)", status);
			return status;
		}

		/* Configure the key attributes */
		psa_set_key_usage_flags(&key_attributes,
					PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
		psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
		psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_type(&key_attributes,
				 PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&key_attributes, 256);

		/**
		 * Generate a random keypair. The keypair is not exposed to the application,
		 * we can use it to sign messages.
		 * You should handle the key purge or destruction by yourself after you are
		 * done using it, in case of a Volatile key this is not needed anyway.
		 */
		status = psa_generate_key(&key_attributes, user_keypair_id);
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_generate_key() failed! (Error: %d)", status);
			return status;
		}

		psa_reset_key_attributes(&key_attributes);

		err = 0;
	}
	return err;
}

/* Output string will contain at most (`output_str_size` - 1) characters */
/* Keep in mind that this function will ALWAYS insert `\0` at the end of the string */
static int get_random_bytes_str(uint8_t *output_str, const size_t output_str_size)
{
	int err = -ENODATA;
	psa_status_t status;
	uint8_t tmp_output_buffer[(RANDOM_STR_MAX_SIZE - 1) / 2] = {0};
	int printed_bytes = 0;

	if ((NULL == output_str) || (output_str_size > (RANDOM_STR_MAX_SIZE))) {
		/* Bad parameter */
		return -EINVAL;
	}

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init() failed! (Error: %d)", status);
		return status;
	}

	status = psa_generate_random(tmp_output_buffer, sizeof(tmp_output_buffer));

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_generate_random() failed! (Error: %d)", status);
		err = -ENOMEM;
	} else {
		err = 0;
	}

	for (uint32_t i = 0; i < ((output_str_size - 1) / 8); i++) {
		printed_bytes += snprintf((output_str + printed_bytes), 9, "%08x",
					  (uint32_t)*((uint32_t *)tmp_output_buffer + i));
	}
	output_str[printed_bytes + 1] = '\0';

	return err;
}

static int jwt_generate_simplified_token(uint32_t exp_time_s, char *out_buffer,
					 const size_t out_buffer_size)
{
	int err = -EINVAL;
	int user_key_id = 0;
	char token_subject_str[APP_JWT_CLAIM_MAX_SIZE] = {0};
	char subject_random_str[RANDOM_STR_MAX_SIZE] = {0};

	if (!out_buffer || !out_buffer_size) {
		return -EINVAL;
	}

	/**
	 * Simplified token requires using a proprietary key for signing.
	 * The key needs to be generated before using its id for signing.
	 * Key needs to be valid for signing messages and verifying message
	 * signatures. It needs to be an ECDSA with a secp256r1 curve.
	 */
	err = generate_ecdsa_keypair(&user_key_id);
	if (err != 0) {
		LOG_ERR("generate_ecdsa_keypair() failed! (Error: %d)", err);
		return -ENOMEM;
	}

	struct app_jwt_data jwt = {.sec_tag = user_key_id,
					.key_type = JWT_KEY_TYPE_CLIENT_PRIV,
					.alg = JWT_ALG_TYPE_ES256,
					/**
					 * Simplified token doesn't need
					 * a `kid` claim in its header
					 */
					.add_keyid_to_header = false,
					.jwt_buf = out_buffer,
					.jwt_sz = out_buffer_size};

	/* Simplified token requires using an expiration claim */
	if (exp_time_s > APP_JWT_VALID_TIME_S_MAX) {
		jwt.validity_s = APP_JWT_VALID_TIME_S_MAX;
	} else if (exp_time_s == 0) {
		jwt.validity_s = APP_JWT_VALID_TIME_S_DEF;
	} else {
		jwt.validity_s = exp_time_s;
	}

	/* Simplified token requires using a subject claim */
	/* Subject: format: <hardware_id>.<16-random_bytes> */

	if (0 == get_random_bytes_str(subject_random_str, RANDOM_STR_MAX_SIZE)) {
		snprintf(token_subject_str, APP_JWT_CLAIM_MAX_SIZE, "%s.%s", CONFIG_BOARD,
			 subject_random_str);
		token_subject_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';
	}

	jwt.subject = token_subject_str;

	/* JWT Claims that are not required are marked with a `0` */
	jwt.json_token_id = 0;
	jwt.audience = 0;
	jwt.issuer = 0;

	/* Simplified token doesn't need a `iat` claim in its header */
	jwt.add_timestamp = false,

	/* Call app_jwt API */
	err = app_jwt_generate(&jwt);

	return err;
}

static int jwt_generate_full_token(uint32_t exp_time_s, char *out_buffer,
				   const size_t out_buffer_size)
{
	int err = -EINVAL;
	char device_uuid_str[APP_JWT_UUID_V4_STR_LEN] = {0};
	char token_issuer_str[APP_JWT_CLAIM_MAX_SIZE] = {0};
	char json_token_id_str[APP_JWT_CLAIM_MAX_SIZE] = {0};
	char jti_random_str[RANDOM_STR_MAX_SIZE] = {0};
	char audience_str[APP_JWT_CLAIM_MAX_SIZE] = {0};

	if (!out_buffer || !out_buffer_size) {
		return -EINVAL;
	}

	/* Full token used IAK key for signing */
	struct app_jwt_data jwt = {.sec_tag = 0,
					.key_type = 0,
					.alg = JWT_ALG_TYPE_ES256,
					/* Full token requires `kid` claim to its header */
					.add_keyid_to_header = true,
					/* Full token requires `iat` claim */
					.add_timestamp = true,
					.jwt_buf = out_buffer,
					.jwt_sz = out_buffer_size};

	/* Full token requires `exp` claim */
	if (exp_time_s > APP_JWT_VALID_TIME_S_MAX) {
		jwt.validity_s = APP_JWT_VALID_TIME_S_MAX;
	} else if (exp_time_s == 0) {
		jwt.validity_s = APP_JWT_VALID_TIME_S_DEF;
	} else {
		jwt.validity_s = exp_time_s;
	}

	/* Full token requires `sub`, `iss`, `jti` and `aud` claims */
	/* Subject: format: "user_defined_string" , we use uuid as subject */

	/* Use app_jwt API for UUID */
	if (0 != app_jwt_get_uuid(device_uuid_str, APP_JWT_UUID_V4_STR_LEN)) {
		return -ENXIO;
	}

	jwt.subject = device_uuid_str;

	/* Issuer: format: <hardware_id>.<sub> */

	snprintf(token_issuer_str, APP_JWT_CLAIM_MAX_SIZE, "%s.%s", CONFIG_BOARD, jwt.subject);
	token_issuer_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';

	jwt.issuer = token_issuer_str;

	/* Json Token ID: format: <hardware_id>.<16-random_bytes> */

	if (0 == get_random_bytes_str(jti_random_str, RANDOM_STR_MAX_SIZE)) {
		snprintf(json_token_id_str, APP_JWT_CLAIM_MAX_SIZE, "%s.%s", CONFIG_BOARD,
			 jti_random_str);
		json_token_id_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';
	}

	jwt.json_token_id = json_token_id_str;

	/* Audience: format: "user_defined_string" */

	snprintf(audience_str, APP_JWT_CLAIM_MAX_SIZE, "%s", JWT_AUDIENCE_STR);
	audience_str[APP_JWT_CLAIM_MAX_SIZE - 1] = '\0';

	jwt.audience = audience_str;

	/* Call app_jwt API */
	err = app_jwt_generate(&jwt);

	return err;
}

#if IS_ENABLED(CONFIG_BOARD_NRF9251DK_NRF9251_CPUAPP)
static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		     (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_INF("Connected to LTE");
			k_sem_give(&lte_connected);
		} else if (evt->nw_reg_status == LTE_LC_NW_REG_UICC_FAIL) {
			LOG_WRN("UICC failure");
			k_sem_reset(&lte_connected);
		}
		break;
	default:
		break;
	}
}

static void date_time_evt_handler(const struct date_time_evt *evt)
{
	k_sem_give(&time_update_finished);
}

static void connect_and_wait_for_time(void)
{
	int err;

	err = nrf_modem_lib_init();
	if (err) {
		LOG_WRN("nrf_modem_lib_init() failed! (Error: %d)", err);
		return;
	}

	lte_lc_register_handler(lte_event_handler);
	date_time_register_handler(date_time_evt_handler);

	LOG_INF("Connecting to LTE...");

	lte_lc_normal();

	err = k_sem_take(&lte_connected, K_MINUTES(5));
	if (err) {
		LOG_WRN("Failed to connect to LTE");
		return;
	}

	LOG_INF("Waiting for current time...");

	/* Wait for an event from the Date Time library. */
	k_sem_take(&time_update_finished, K_MINUTES(1));

	if (date_time_is_valid()) {
		LOG_INF("Got current time");
	} else {
		LOG_WRN("Failed to get current time");
	}
}
#endif /* CONFIG_BOARD_NRF9251DK_NRF9251_CPUAPP */

int main(void)
{
	char jwt_str[APP_JWT_STR_MAX_LEN] = {0};
	int ret = -1;

	LOG_INF("Application JWT sample (%s)", CONFIG_BOARD);

#if IS_ENABLED(CONFIG_BOARD_NRF9251DK_NRF9251_CPUAPP)
	connect_and_wait_for_time();
#endif

	LOG_INF("Generating simplified JWT token");
	ret = jwt_generate_simplified_token(APP_JWT_VALID_TIME_S_MAX, jwt_str,
						APP_JWT_STR_MAX_LEN);

	if (ret == 0) {
		LOG_INF("JWT(length %d):", strlen(jwt_str));
		LOG_INF("%s", jwt_str);

	} else {
		LOG_ERR("jwt_generate_simplified_token() failed! (Error: %d)", ret);
	}

	memset(jwt_str, 0, APP_JWT_STR_MAX_LEN);
	LOG_INF("Generating full JWT token");
	ret = jwt_generate_full_token(APP_JWT_VALID_TIME_S_MAX, jwt_str, APP_JWT_STR_MAX_LEN);

	if (ret == 0) {
		LOG_INF("JWT(length %d):", strlen(jwt_str));
		LOG_INF("%s", jwt_str);

	} else {
		LOG_ERR("jwt_generate_full_token() failed! (Error: %d)", ret);
	}

	return 0;
}
