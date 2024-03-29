/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <mocks.h>

static suit_manifest_class_id_t sample_class_id = {{0xca, 0xd8, 0x52, 0x3a, 0xf8, 0x29, 0x5a, 0x9a,
						    0xba, 0x85, 0x2e, 0xa0, 0xb2, 0xf5, 0x77,
						    0xc9}};
static struct zcbor_string valid_manifest_component_id = {
	.value = (const uint8_t *)0x1234,
	.len = 123,
};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suit_platform_devconfig_seq_tests, NULL, NULL, test_before, NULL, NULL);

static suit_plat_err_t
suit_plat_decode_manifest_class_id_correct_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_manifest_component_id, component_id,
		      "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = &sample_class_id;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_invalid_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_manifest_component_id, component_id,
		      "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = NULL;

	return SUIT_PLAT_ERR_CRASH;
}

static int suit_processor_get_manifest_metadata_seq_one_fake_func(
	const uint8_t *envelope_str, size_t envelope_len, bool authenticate,
	struct zcbor_string *manifest_component_id, struct zcbor_string *digest,
	enum suit_cose_alg *alg, unsigned int *seq_num)
{
	zassert_not_equal(
		seq_num, NULL,
		"The API must provide a valid pointer, to read the sequence number of a manifest");
	*seq_num = 1;

	return SUIT_SUCCESS;
}

static int suit_processor_get_manifest_metadata_decoder_busy_fake_func(
	const uint8_t *envelope_str, size_t envelope_len, bool authenticate,
	struct zcbor_string *manifest_component_id, struct zcbor_string *digest,
	enum suit_cose_alg *alg, unsigned int *seq_num)
{
	zassert_not_equal(
		seq_num, NULL,
		"The API must provide a valid pointer, to read the sequence number of a manifest");

	return SUIT_ERR_WAIT;
}

static mci_err_t suit_mci_downgrade_prevention_policy_get_enabled_fake_func(
	const suit_manifest_class_id_t *class_id, suit_downgrade_prevention_policy_t *policy)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");
	zassert_not_equal(
		policy, NULL,
		"The API must provide a valid pointer, to read the downgrade prevention policy");
	*policy = SUIT_DOWNGRADE_PREVENTION_ENABLED;

	return SUIT_PLAT_SUCCESS;
}

static mci_err_t suit_mci_downgrade_prevention_policy_get_disabled_fake_func(
	const suit_manifest_class_id_t *class_id, suit_downgrade_prevention_policy_t *policy)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");
	zassert_not_equal(
		policy, NULL,
		"The API must provide a valid pointer, to read the downgrade prevention policy");
	*policy = SUIT_DOWNGRADE_PREVENTION_DISABLED;

	return SUIT_PLAT_SUCCESS;
}

static mci_err_t suit_mci_downgrade_prevention_policy_get_unsupported_fake_func(
	const suit_manifest_class_id_t *class_id, suit_downgrade_prevention_policy_t *policy)
{
	zassert_equal(class_id, &sample_class_id, "Invalid manifest class ID value");
	zassert_not_equal(
		policy, NULL,
		"The API must provide a valid pointer, to read the downgrade prevention policy");
	*policy = (suit_downgrade_prevention_policy_t)0x1234;

	return SUIT_PLAT_SUCCESS;
}

ZTEST(suit_platform_devconfig_seq_tests, test_null_arg)
{
	/* WHEN platform is asked for authorization of INSTALL sequence from manifest with sequence
	 * number 1 and NULL component ID
	 */
	int ret = suit_plat_authorize_sequence_num(SUIT_SEQ_INSTALL, NULL, 1);

	/* THEN manifest is not authorized... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Invalid manifest class ID authorized");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
	zassert_equal(suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
}

ZTEST(suit_platform_devconfig_seq_tests, test_null_component_id)
{
	struct zcbor_string invalid_manifest_component_id;

	/* GIVEN the manifest component ID points to the NULL. */
	invalid_manifest_component_id.value = NULL;
	invalid_manifest_component_id.len = 123;

	/* WHEN platform is asked for authorization of INSTALL sequence from manifest with sequence
	 * number 1
	 */
	int ret = suit_plat_authorize_sequence_num(SUIT_SEQ_INSTALL, &invalid_manifest_component_id,
						   1);

	/* THEN manifest is not authorized... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Invalid manifest class ID authorized");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
	zassert_equal(suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
}

ZTEST(suit_platform_devconfig_seq_tests, test_invalid_manifest_component_id)
{
	/* GIVEN the manifest component ID contains an invalid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_invalid_fake_func;

	/* WHEN platform is asked for authorization of INSTALL sequence from manifest with sequence
	 * number 1
	 */
	int ret =
		suit_plat_authorize_sequence_num(SUIT_SEQ_INSTALL, &valid_manifest_component_id, 1);

	/* THEN manifest is not authorized... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Invalid manifest class ID authorized");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and other checks were not performed */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
	zassert_equal(suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
}

ZTEST(suit_platform_devconfig_seq_tests, test_unsupported_class_id)
{
	/* GIVEN the manifest component ID contains a valid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	/* ... and the manifest class ID is not supported */
	suit_mci_manifest_class_id_validate_fake.return_val = MCI_ERR_MANIFESTCLASSID;

	/* WHEN platform is asked for authorization of INSTALL sequence from manifest with sequence
	 * number 1
	 */
	int ret =
		suit_plat_authorize_sequence_num(SUIT_SEQ_INSTALL, &valid_manifest_component_id, 1);

	/* THEN manifest is not authorized... */
	zassert_equal(SUIT_ERR_AUTHENTICATION, ret,
		      "Manifest sequence number authorized with unauthorized class ID");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest class ID is validated */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	/* ... and other checks were not performed */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 0,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
	zassert_equal(suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
}

ZTEST(suit_platform_devconfig_seq_tests, test_boot_no_installed_envelope)
{
	enum suit_command_sequence suit_seq[] = {
		SUIT_SEQ_VALIDATE,
		SUIT_SEQ_LOAD,
		SUIT_SEQ_INVOKE,
	};

	for (size_t i = 0; i < ARRAY_SIZE(suit_seq); i++) {
		/* Reset mocks */
		mocks_reset();

		/* Reset common FFF internal structures */
		FFF_RESET_HISTORY();

		/* GIVEN the manifest component ID contains a valid manifest class ID. */
		suit_plat_decode_manifest_class_id_fake.custom_fake =
			suit_plat_decode_manifest_class_id_correct_fake_func;
		/* ... and the manifest class ID is supported */
		suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
		/* ... and the SUIT storage contains a valid envelope with given class ID */
		suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_ERR_NOT_FOUND;
		/* ... and the envelope from SUIT storage is decodeable */
		suit_processor_get_manifest_metadata_fake.custom_fake =
			suit_processor_get_manifest_metadata_seq_one_fake_func;
		/* ... and the downgrade prevention policy is enabled */
		suit_mci_downgrade_prevention_policy_get_fake.custom_fake =
			suit_mci_downgrade_prevention_policy_get_enabled_fake_func;

		/* WHEN platform is asked for authorization of VALIDATE, LOAD or INVOKE sequence
		 * from manifest with sequence number 1
		 */
		int ret = suit_plat_authorize_sequence_num(suit_seq[i],
							   &valid_manifest_component_id, 1);

		/* THEN manifest is not authorized... */
		zassert_equal(SUIT_ERR_AUTHENTICATION, ret,
			      "Manifest sequence number authorized too boot from update area");

		/* ... and manifest class ID is decoded from the manifest component ID */
		zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
			      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
		/* ... and manifest class ID is validated */
		zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
			      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
		/* ... and SUIT storage is searched for manifest with given class ID */
		zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
			      "Incorrect number of suit_storage_installed_envelope_get() calls");
		/* ... and other checks were not performed */
		zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
			      "Incorrect number of suit_processor_get_manifest_metadata() calls");
		zassert_equal(
			suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
			"Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
	}
}

ZTEST(suit_platform_devconfig_seq_tests, test_update_no_installed_envelope)
{
	enum suit_command_sequence suit_seq[] = {
		SUIT_SEQ_PARSE,		SUIT_SEQ_SHARED,  SUIT_SEQ_DEP_RESOLUTION,
		SUIT_SEQ_PAYLOAD_FETCH, SUIT_SEQ_INSTALL,
	};

	for (size_t i = 0; i < ARRAY_SIZE(suit_seq); i++) {
		/* Reset mocks */
		mocks_reset();

		/* Reset common FFF internal structures */
		FFF_RESET_HISTORY();

		/* GIVEN the manifest component ID contains a valid manifest class ID. */
		suit_plat_decode_manifest_class_id_fake.custom_fake =
			suit_plat_decode_manifest_class_id_correct_fake_func;
		/* ... and the manifest class ID is supported */
		suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
		/* ... and the SUIT storage contains an invalid envelope with given class ID */
		suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_ERR_CBOR_DECODING;
		/* ... and the envelope from SUIT storage is decodeable */
		suit_processor_get_manifest_metadata_fake.custom_fake =
			suit_processor_get_manifest_metadata_seq_one_fake_func;
		/* ... and the downgrade prevention policy is enabled */
		suit_mci_downgrade_prevention_policy_get_fake.custom_fake =
			suit_mci_downgrade_prevention_policy_get_enabled_fake_func;

		/* WHEN platform is asked for authorization of PARSE, SHARED, DEP_RESOLUTION,
		 * PAYLOAD_FETCH, INSTALL, sequence from manifest with sequence number 1
		 */
		int ret = suit_plat_authorize_sequence_num(suit_seq[i],
							   &valid_manifest_component_id, 1);

		/* THEN manifest is authorized... */
		zassert_equal(SUIT_SUCCESS, ret, "Manifest sequence number unauthorized");

		/* ... and manifest class ID is decoded from the manifest component ID */
		zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
			      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
		/* ... and manifest class ID is validated */
		zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
			      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
		/* ... and SUIT storage is searched for manifest with given class ID */
		zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
			      "Incorrect number of suit_storage_installed_envelope_get() calls");
		/* ... and other checks were not performed */
		zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
			      "Incorrect number of suit_processor_get_manifest_metadata() calls");
		zassert_equal(
			suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
			"Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
	}
}

ZTEST(suit_platform_devconfig_seq_tests, test_decode_busy)
{
	enum suit_command_sequence suit_seq[] = {
		SUIT_SEQ_PARSE,		SUIT_SEQ_SHARED,  SUIT_SEQ_DEP_RESOLUTION,
		SUIT_SEQ_PAYLOAD_FETCH, SUIT_SEQ_INSTALL, SUIT_SEQ_VALIDATE,
		SUIT_SEQ_LOAD,		SUIT_SEQ_INVOKE,
	};

	for (size_t i = 0; i < ARRAY_SIZE(suit_seq); i++) {
		/* Reset mocks */
		mocks_reset();

		/* Reset common FFF internal structures */
		FFF_RESET_HISTORY();

		/* GIVEN the manifest component ID contains a valid manifest class ID. */
		suit_plat_decode_manifest_class_id_fake.custom_fake =
			suit_plat_decode_manifest_class_id_correct_fake_func;
		/* ... and the manifest class ID is supported */
		suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
		/* ... and the SUIT storage contains a valid envelope with given class ID */
		suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_SUCCESS;
		/* ... and the envelope from SUIT storage is decodeable, but the decoder is busy */
		suit_processor_get_manifest_metadata_fake.custom_fake =
			suit_processor_get_manifest_metadata_decoder_busy_fake_func;

		/* WHEN platform is asked for authorization of any sequence from manifest with
		 * sequence number 1
		 */
		int ret = suit_plat_authorize_sequence_num(suit_seq[i],
							   &valid_manifest_component_id, 1);

		/* THEN manifest is not authorized... */
		zassert_equal(
			SUIT_ERR_AUTHENTICATION, ret,
			"Manifest sequence number authorized, without checking installed manifest");

		/* ... and manifest class ID is decoded from the manifest component ID */
		zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
			      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
		/* ... and manifest class ID is validated */
		zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
			      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
		/* ... and SUIT storage is searched for manifest with given class ID */
		zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
			      "Incorrect number of suit_storage_installed_envelope_get() calls");
		/* ... and class ID is extracted from the envelope stored inside SUIT storage */
		zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 1,
			      "Incorrect number of suit_processor_get_manifest_metadata() calls");
		/* ... and MCI is not asked for the downgrade prevention policy, associated with
		 * given class ID
		 */
		zassert_equal(
			suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
			"Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
	}
}

ZTEST(suit_platform_devconfig_seq_tests, test_manifest_present_no_downgrade_policy)
{
	/* GIVEN the manifest component ID contains a valid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	/* ... and the manifest class ID is supported */
	suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
	/* ... and the SUIT storage contains a valid envelope with given class ID */
	suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_SUCCESS;
	/* ... and the envelope from SUIT storage is decodeable */
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_seq_one_fake_func;
	/* ... and the downgrade prevention policy is unknown */
	suit_mci_downgrade_prevention_policy_get_fake.return_val = MCI_ERR_MANIFESTCLASSID;

	/* WHEN platform is asked for authorization of INSTALL sequence from manifest with sequence
	 * number 1
	 */
	int ret =
		suit_plat_authorize_sequence_num(SUIT_SEQ_INSTALL, &valid_manifest_component_id, 1);

	/* THEN manifest is not authorized... */
	zassert_equal(
		SUIT_ERR_AUTHENTICATION, ret,
		"Manifest sequence number authorized for unknown downgrade prevention policy");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest class ID is validated */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	/* ... and SUIT storage is searched for manifest with given class ID */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	/* ... and class ID is extracted from the envelope stored inside SUIT storage */
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 1,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
	/* ... and MCI is asked for the downgrade prevention policy, associated with given class ID
	 */
	zassert_equal(suit_mci_downgrade_prevention_policy_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
}

ZTEST(suit_platform_devconfig_seq_tests, test_manifest_present_unsupported_downgrade_policy)
{
	/* GIVEN the manifest component ID contains a valid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	/* ... and the manifest class ID is supported */
	suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
	/* ... and the SUIT storage contains a valid envelope with given class ID */
	suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_SUCCESS;
	/* ... and the envelope from SUIT storage is decodeable */
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_seq_one_fake_func;
	/* ... and the downgrade prevention policy is unsupported */
	suit_mci_downgrade_prevention_policy_get_fake.custom_fake =
		suit_mci_downgrade_prevention_policy_get_unsupported_fake_func;

	/* WHEN platform is asked for authorization of INSTALL sequence from manifest with sequence
	 * number 1
	 */
	int ret =
		suit_plat_authorize_sequence_num(SUIT_SEQ_INSTALL, &valid_manifest_component_id, 1);

	/* THEN manifest is not authorized... */
	zassert_equal(
		SUIT_ERR_AUTHENTICATION, ret,
		"Manifest sequence number authorized for unsupported downgrade prevention policy");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest class ID is validated */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	/* ... and SUIT storage is searched for manifest with given class ID */
	zassert_equal(suit_storage_installed_envelope_get_fake.call_count, 1,
		      "Incorrect number of suit_storage_installed_envelope_get() calls");
	/* ... and class ID is extracted from the envelope stored inside SUIT storage */
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 1,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
	/* ... and MCI is asked for the downgrade prevention policy, associated with given class ID
	 */
	zassert_equal(suit_mci_downgrade_prevention_policy_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_downgrade_prevention_policy_get() calls");
}

ZTEST(suit_platform_devconfig_seq_tests, test_update_manifest_present_disabled_downgrade_prevention)
{
	enum suit_command_sequence suit_seq[] = {
		SUIT_SEQ_PARSE,		SUIT_SEQ_SHARED,  SUIT_SEQ_DEP_RESOLUTION,
		SUIT_SEQ_PAYLOAD_FETCH, SUIT_SEQ_INSTALL,
	};

	for (size_t i = 0; i < ARRAY_SIZE(suit_seq); i++) {
		for (uint32_t seq_num = 0; seq_num < 2; seq_num++) {
			/* Reset mocks */
			mocks_reset();

			/* Reset common FFF internal structures */
			FFF_RESET_HISTORY();

			/* GIVEN the manifest component ID contains a valid manifest class ID. */
			suit_plat_decode_manifest_class_id_fake.custom_fake =
				suit_plat_decode_manifest_class_id_correct_fake_func;
			/* ... and the manifest class ID is supported */
			suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
			/* ... and the SUIT storage contains a valid envelope with given class ID */
			suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_SUCCESS;
			/* ... and the envelope from SUIT storage is decodeable */
			suit_processor_get_manifest_metadata_fake.custom_fake =
				suit_processor_get_manifest_metadata_seq_one_fake_func;
			/* ... and the downgrade prevention policy is enabled */
			suit_mci_downgrade_prevention_policy_get_fake.custom_fake =
				suit_mci_downgrade_prevention_policy_get_disabled_fake_func;

			/* WHEN platform is asked for authorization of any sequence from manifest
			 * with any sequence number
			 */
			int ret = suit_plat_authorize_sequence_num(
				suit_seq[i], &valid_manifest_component_id, seq_num);

			/* THEN manifest is authorized... */
			zassert_equal(SUIT_SUCCESS, ret,
				      "Manifest sequence number unauthorized for sequence %d with "
				      "number %d",
				      suit_seq[i], seq_num);

			/* ... and manifest class ID is decoded from the manifest component ID */
			zassert_equal(
				suit_plat_decode_manifest_class_id_fake.call_count, 1,
				"Incorrect number of suit_plat_decode_manifest_class_id() calls");
			/* ... and manifest class ID is validated */
			zassert_equal(
				suit_mci_manifest_class_id_validate_fake.call_count, 1,
				"Incorrect number of suit_mci_manifest_class_id_validate() calls");
			/* ... and SUIT storage is searched for manifest with given class ID */
			zassert_equal(
				suit_storage_installed_envelope_get_fake.call_count, 1,
				"Incorrect number of suit_storage_installed_envelope_get() calls");
			/* ... and class ID is extracted from the envelope stored inside SUIT
			 * storage
			 */
			zassert_equal(
				suit_processor_get_manifest_metadata_fake.call_count, 1,
				"Incorrect number of suit_processor_get_manifest_metadata() calls");
			/* ... and MCI is asked for the downgrade prevention policy, associated with
			 * given class ID
			 */
			zassert_equal(suit_mci_downgrade_prevention_policy_get_fake.call_count, 1,
				      "Incorrect number of "
				      "suit_mci_downgrade_prevention_policy_get() calls");
		}
	}
}

ZTEST(suit_platform_devconfig_seq_tests, test_boot_manifest_present_disabled_downgrade_prevention)
{
	enum suit_command_sequence suit_seq[] = {
		SUIT_SEQ_VALIDATE,
		SUIT_SEQ_LOAD,
		SUIT_SEQ_INVOKE,
	};

	for (size_t i = 0; i < ARRAY_SIZE(suit_seq); i++) {
		for (uint32_t seq_num = 0; seq_num < 2; seq_num++) {
			/* Reset mocks */
			mocks_reset();

			/* Reset common FFF internal structures */
			FFF_RESET_HISTORY();

			/* GIVEN the manifest component ID contains a valid manifest class ID. */
			suit_plat_decode_manifest_class_id_fake.custom_fake =
				suit_plat_decode_manifest_class_id_correct_fake_func;
			/* ... and the manifest class ID is supported */
			suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
			/* ... and the SUIT storage contains a valid envelope with given class ID */
			suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_SUCCESS;
			/* ... and the envelope from SUIT storage is decodeable */
			suit_processor_get_manifest_metadata_fake.custom_fake =
				suit_processor_get_manifest_metadata_seq_one_fake_func;
			/* ... and the downgrade prevention policy is enabled */
			suit_mci_downgrade_prevention_policy_get_fake.custom_fake =
				suit_mci_downgrade_prevention_policy_get_disabled_fake_func;

			/* WHEN platform is asked for authorization of any sequence from manifest
			 * with any sequence number
			 */
			int ret = suit_plat_authorize_sequence_num(
				suit_seq[i], &valid_manifest_component_id, seq_num);

			if (seq_num == 1) {
				/* .. and the sequence number matches the installed envelope */

				/* THEN manifest is authorized... */
				zassert_equal(SUIT_SUCCESS, ret,
					      "Manifest sequence number unauthorized for sequence "
					      "%d with number %d",
					      suit_seq[i], seq_num);

				/* ... and manifest class ID is decoded from the manifest component
				 * ID
				 */
				zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
					      "Incorrect number of "
					      "suit_plat_decode_manifest_class_id() calls");
				/* ... and manifest class ID is validated */
				zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_mci_manifest_class_id_validate() calls");
				/* ... and SUIT storage is searched for manifest with given class ID
				 */
				zassert_equal(suit_storage_installed_envelope_get_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_storage_installed_envelope_get() calls");
				/* ... and class ID is extracted from the envelope stored inside
				 * SUIT storage
				 */
				zassert_equal(suit_processor_get_manifest_metadata_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_processor_get_manifest_metadata() calls");
				/* ... and MCI is not asked for the downgrade prevention policy,
				 * associated with given class ID
				 */
				zassert_equal(
					suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
					"Incorrect number of "
					"suit_mci_downgrade_prevention_policy_get() calls");
			} else {
				/* .. and the sequence number doe not match the installed envelope
				 */

				/* THEN manifest is not authorized... */
				zassert_equal(SUIT_ERR_AUTHENTICATION, ret,
					      "Manifest sequence number authorized for sequence %d "
					      "with number %d",
					      suit_seq[i], seq_num);

				/* ... and manifest class ID is decoded from the manifest component
				 * ID
				 */
				zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
					      "Incorrect number of "
					      "suit_plat_decode_manifest_class_id() calls");
				/* ... and manifest class ID is validated */
				zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_mci_manifest_class_id_validate() calls");
				/* ... and SUIT storage is searched for manifest with given class ID
				 */
				zassert_equal(suit_storage_installed_envelope_get_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_storage_installed_envelope_get() calls");
				/* ... and class ID is extracted from the envelope stored inside
				 * SUIT storage
				 */
				zassert_equal(suit_processor_get_manifest_metadata_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_processor_get_manifest_metadata() calls");
				/* ... and MCI is not asked for the downgrade prevention policy,
				 * associated with given class ID
				 */
				zassert_equal(
					suit_mci_downgrade_prevention_policy_get_fake.call_count, 0,
					"Incorrect number of "
					"suit_mci_downgrade_prevention_policy_get() calls");
			}
		}
	}
}

ZTEST(suit_platform_devconfig_seq_tests, test_update_manifest_present_enabled_downgrade_prevention)
{
	enum suit_command_sequence suit_seq[] = {
		SUIT_SEQ_PARSE,		SUIT_SEQ_SHARED,  SUIT_SEQ_DEP_RESOLUTION,
		SUIT_SEQ_PAYLOAD_FETCH, SUIT_SEQ_INSTALL,
	};

	for (size_t i = 0; i < ARRAY_SIZE(suit_seq); i++) {
		for (uint32_t seq_num = 0; seq_num < 2; seq_num++) {
			/* Reset mocks */
			mocks_reset();

			/* Reset common FFF internal structures */
			FFF_RESET_HISTORY();

			/* GIVEN the manifest component ID contains a valid manifest class ID. */
			suit_plat_decode_manifest_class_id_fake.custom_fake =
				suit_plat_decode_manifest_class_id_correct_fake_func;
			/* ... and the manifest class ID is supported */
			suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
			/* ... and the SUIT storage contains a valid envelope with given class ID */
			suit_storage_installed_envelope_get_fake.return_val = SUIT_PLAT_SUCCESS;
			/* ... and the envelope from SUIT storage is decodeable */
			suit_processor_get_manifest_metadata_fake.custom_fake =
				suit_processor_get_manifest_metadata_seq_one_fake_func;
			/* ... and the downgrade prevention policy is enabled */
			suit_mci_downgrade_prevention_policy_get_fake.custom_fake =
				suit_mci_downgrade_prevention_policy_get_enabled_fake_func;

			/* WHEN platform is asked for authorization of any sequence from manifest
			 * with any sequence number
			 */
			int ret = suit_plat_authorize_sequence_num(
				suit_seq[i], &valid_manifest_component_id, seq_num);

			if (seq_num >= 1) {
				/* .. and the sequence number is not smaller than the installed
				 * envelope sequence
				 */

				/* THEN manifest is authorized... */
				zassert_equal(SUIT_SUCCESS, ret,
					      "Manifest sequence number unauthorized for sequence "
					      "%d with number %d",
					      suit_seq[i], seq_num);

				/* ... and manifest class ID is decoded from the manifest component
				 * ID
				 */
				zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
					      "Incorrect number of "
					      "suit_plat_decode_manifest_class_id() calls");
				/* ... and manifest class ID is validated */
				zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_mci_manifest_class_id_validate() calls");
				/* ... and SUIT storage is searched for manifest with given class ID
				 */
				zassert_equal(suit_storage_installed_envelope_get_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_storage_installed_envelope_get() calls");
				/* ... and class ID is extracted from the envelope stored inside
				 * SUIT storage
				 */
				zassert_equal(suit_processor_get_manifest_metadata_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_processor_get_manifest_metadata() calls");
				/* ... and MCI is not asked for the downgrade prevention policy,
				 * associated with given class ID
				 */
				zassert_equal(
					suit_mci_downgrade_prevention_policy_get_fake.call_count, 1,
					"Incorrect number of "
					"suit_mci_downgrade_prevention_policy_get() calls");
			} else {
				/* .. and the sequence number doe not match the installed envelope
				 */

				/* THEN manifest is not authorized... */
				zassert_equal(SUIT_ERR_AUTHENTICATION, ret,
					      "Manifest sequence number authorized for sequence %d "
					      "with number %d",
					      suit_seq[i], seq_num);

				/* ... and manifest class ID is decoded from the manifest component
				 * ID
				 */
				zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
					      "Incorrect number of "
					      "suit_plat_decode_manifest_class_id() calls");
				/* ... and manifest class ID is validated */
				zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_mci_manifest_class_id_validate() calls");
				/* ... and SUIT storage is searched for manifest with given class ID
				 */
				zassert_equal(suit_storage_installed_envelope_get_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_storage_installed_envelope_get() calls");
				/* ... and class ID is extracted from the envelope stored inside
				 * SUIT storage
				 */
				zassert_equal(suit_processor_get_manifest_metadata_fake.call_count,
					      1,
					      "Incorrect number of "
					      "suit_processor_get_manifest_metadata() calls");
				/* ... and MCI is not asked for the downgrade prevention policy,
				 * associated with given class ID
				 */
				zassert_equal(
					suit_mci_downgrade_prevention_policy_get_fake.call_count, 1,
					"Incorrect number of "
					"suit_mci_downgrade_prevention_policy_get() calls");
			}
		}
	}
}

ZTEST_SUITE(suit_platform_devconfig_completed_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_platform_devconfig_completed_tests, test_null_arg)
{
	/* WHEN sequence is completed in a manifest with unknown component ID */
	int ret = suit_plat_sequence_completed(SUIT_SEQ_PARSE, NULL, NULL, 0);

	/* THEN manifest is not accepted... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest class ID handled");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_storage_install_envelope_fake.call_count, 0,
		      "Incorrect number of suit_storage_install_envelope() calls");
}

ZTEST(suit_platform_devconfig_completed_tests, test_null_component_id)
{
	struct zcbor_string invalid_manifest_component_id;

	/* GIVEN the manifest component ID points to the NULL. */
	invalid_manifest_component_id.value = NULL;
	invalid_manifest_component_id.len = 123;

	/* WHEN sequence is completed in a manifest with component ID set to NULL */
	int ret = suit_plat_sequence_completed(SUIT_SEQ_PARSE, &invalid_manifest_component_id, NULL,
					       0);

	/* THEN manifest is not accepted... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest class ID handled");

	/* ... and other checks were not performed */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_storage_install_envelope_fake.call_count, 0,
		      "Incorrect number of suit_storage_install_envelope() calls");
}

ZTEST(suit_platform_devconfig_completed_tests, test_invalid_manifest_component_id)
{
	/* GIVEN the manifest component ID contains an invalid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_invalid_fake_func;

	/* WHEN sequence is completed */
	int ret =
		suit_plat_sequence_completed(SUIT_SEQ_PARSE, &valid_manifest_component_id, NULL, 0);

	/* THEN manifest is not accepted... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest class ID handled");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and other checks were not performed */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_storage_install_envelope_fake.call_count, 0,
		      "Incorrect number of suit_storage_install_envelope() calls");
}

ZTEST(suit_platform_devconfig_completed_tests, test_unsupported_class_id)
{
	/* GIVEN the manifest component ID contains a valid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	/* ... and the manifest class ID is not supported */
	suit_mci_manifest_class_id_validate_fake.return_val = MCI_ERR_MANIFESTCLASSID;

	/* WHEN sequence is completed */
	int ret =
		suit_plat_sequence_completed(SUIT_SEQ_PARSE, &valid_manifest_component_id, NULL, 0);

	/* THEN manifest is not accepted... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Manifest accepted with unauthorized class ID");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest class ID is validated */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	/* ... and other checks were not performed */
	zassert_equal(suit_storage_install_envelope_fake.call_count, 0,
		      "Incorrect number of suit_storage_install_envelope() calls");
}

ZTEST(suit_platform_devconfig_completed_tests, test_non_install_seq)
{
	enum suit_command_sequence suit_seq[] = {
		SUIT_SEQ_PARSE,	   SUIT_SEQ_SHARED, SUIT_SEQ_DEP_RESOLUTION, SUIT_SEQ_PAYLOAD_FETCH,
		SUIT_SEQ_VALIDATE, SUIT_SEQ_LOAD,   SUIT_SEQ_INVOKE,
	};

	for (size_t i = 0; i < ARRAY_SIZE(suit_seq); i++) {
		/* Reset mocks */
		mocks_reset();

		/* Reset common FFF internal structures */
		FFF_RESET_HISTORY();

		/* GIVEN the manifest component ID contains a valid manifest class ID. */
		suit_plat_decode_manifest_class_id_fake.custom_fake =
			suit_plat_decode_manifest_class_id_correct_fake_func;
		/* ... and the manifest class ID is supported */
		suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;

		/* WHEN sequence is completed */
		int ret = suit_plat_sequence_completed(suit_seq[i], &valid_manifest_component_id,
						       NULL, 0);

		/* THEN manifest is accepted... */
		zassert_equal(SUIT_SUCCESS, ret, "Manifest not accepted for sequence %d",
			      suit_seq[i]);

		/* ... and manifest class ID is decoded from the manifest component ID */
		zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
			      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
		/* ... and manifest class ID is validated */
		zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
			      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
		/* ... and the manifest is not passed to be installed in the SUIT storage */
		zassert_equal(suit_storage_install_envelope_fake.call_count, 0,
			      "Incorrect number of suit_storage_install_envelope() calls");
	}
}

ZTEST(suit_platform_devconfig_completed_tests, test_install_seq_storage_failed)
{
	/* GIVEN the manifest component ID contains a valid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	/* ... and the manifest class ID is supported */
	suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
	/* ... and the SUIT storage is unable to handle the manifest */
	suit_storage_install_envelope_fake.return_val = SUIT_PLAT_ERR_NOT_FOUND;

	/* WHEN sequence is completed */
	int ret = suit_plat_sequence_completed(SUIT_SEQ_INSTALL, &valid_manifest_component_id, NULL,
					       0);

	/* THEN manifest is accepted... */
	zassert_equal(SUIT_ERR_CRASH, ret, "Manifest accepted but not installed");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest class ID is validated */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	/* ... and the manifest is passed to be installed in the SUIT storage */
	zassert_equal(suit_storage_install_envelope_fake.call_count, 1,
		      "Incorrect number of suit_storage_install_envelope() calls");
}

ZTEST(suit_platform_devconfig_completed_tests, test_install_seq_storage_succeed)
{
	/* GIVEN the manifest component ID contains a valid manifest class ID. */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	/* ... and the manifest class ID is supported */
	suit_mci_manifest_class_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;
	/* ... and the SUIT storage is able to handle the manifest */
	suit_storage_install_envelope_fake.return_val = SUIT_PLAT_SUCCESS;

	/* WHEN sequence is completed */
	int ret = suit_plat_sequence_completed(SUIT_SEQ_INSTALL, &valid_manifest_component_id, NULL,
					       0);

	/* THEN manifest is accepted... */
	zassert_equal(SUIT_SUCCESS, ret, "Manifest not accepted");

	/* ... and manifest class ID is decoded from the manifest component ID */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest class ID is validated */
	zassert_equal(suit_mci_manifest_class_id_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	/* ... and the manifest is passed to be installed in the SUIT storage */
	zassert_equal(suit_storage_install_envelope_fake.call_count, 1,
		      "Incorrect number of suit_storage_install_envelope() calls");
}
