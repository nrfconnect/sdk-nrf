/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_check_image_match_domain_specific.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw/sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

LOG_MODULE_REGISTER(suit_plat_check_image_match_app, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
static int suit_plat_check_image_match_ssf(struct zcbor_string *component_id,
					   enum suit_cose_alg alg_id, struct zcbor_string *digest)
{
	suit_plat_mreg_t component_id_mreg = {
		.mem = component_id->value,
		.size = component_id->len,
	};

	suit_plat_mreg_t digest_mreg = {
		.mem = digest->value,
		.size = digest->len,
	};

	suit_ssf_err_t ret = suit_check_installed_component_digest(&component_id_mreg, (int)alg_id,
								   &digest_mreg);

	switch (ret) {
	case SUIT_PLAT_SUCCESS:
		return SUIT_SUCCESS;
	case SUIT_SSF_FAIL_CONDITION:
		return SUIT_FAIL_CONDITION;
	case SUIT_SSF_MISSING_COMPONENT:
		return SUIT_FAIL_CONDITION;
	case SUIT_PLAT_ERR_UNSUPPORTED:
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	default:
		break;
	}

	return SUIT_ERR_CRASH;
}
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

int suit_plat_check_image_match_domain_specific(suit_component_t component,
						enum suit_cose_alg alg_id,
						struct zcbor_string *digest,
						struct zcbor_string *component_id,
						suit_component_type_t component_type)
{
	int err = SUIT_SUCCESS;

	switch (component_type) {
	case SUIT_COMPONENT_TYPE_INSTLD_MFST:
	case SUIT_COMPONENT_TYPE_MEM:
	case SUIT_COMPONENT_TYPE_SOC_SPEC: {
#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
		err = suit_plat_check_image_match_ssf(component_id, alg_id, digest);
#else
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */
		break;
	}
	default:
		/* Should never get here */
		err = SUIT_ERR_TAMP;
		break;
	}

	return err;
}
