/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_NRF_LCS
#include <nrf_lcs/nrf_lcs.h>
#endif

LOG_MODULE_REGISTER(manufacturing_app, LOG_LEVEL_DBG);

#ifdef CONFIG_NRF_LCS
int assembly_and_test_handler(void)
{
	/* In this state Bootloaders are unable to check signatures.
	 * That said, there is no uarantee on the correctness of the BL1 as well
	 * as BL2 validity.
	 * In case of MCUboot as BL2, it is capable of prevalidating the manufacturing app by
	 * checking the image digest.
	 */

	/*
	 * Steps to be performed in a trusted manufacturing environment.
	 */

	/* Check the correctness of the B0 (NSIB). */
	/* Check the correctness of the B1 (MCUboot). */
	/* Check the correctness of the provisioning image (self). */
	/* Check the presence of the application update candidate. */
	/* Check the updateability of the application update candidate. */
	/* Check essential fields of the UICR. */

	/* Provision IKG seed. */
	/* Open connection to the external provisioning tool (if needed). */
	/* Provision secure boot keys. */
	/* Provision ADAC keys. */

	/* Configure erase protection.*/
	/* Set LCS to "PROT Provisioning". */
	nrf_lcs_set(NRF_LCS_PSA_ROT_PROVISIONING);
	/* Check erase protection value .*/
	/* Check the current LCS value. */

	return 0;
}

int psa_rot_provisioning_handler(void)
{
	/* In this state Bootloaders have verified the correctness of both BL2
	 * as well as the manufacturing application correctness.
	 * The bootloaders are not capable of performing DFU at this stage, thus
	 * the provisioning application is booted again.
	 */

	/*
	 * Steps to be performed in an untrusted manufacturing environment.
	 */

	/* Test the product (PCB, connections, sensors, etc.). */
	/* Open a secure connection for provisioning of other assets. */
	/* Set LCS to "Secured". */
	nrf_lcs_set(NRF_LCS_SECURED);
	/* Schedule installation of the application candidate. */
	/* Revoke Manufacturing Tool (provisioning image) verification key. */
	/* Reboot. */

	/* At this point the BL2 (MCUboot) should install the main application.
	 * The update process should overwrite the provisioning image.
	 * As a result, no other actions can or should be performed by this application.
	 */

	 return 0;
}

int secured_handler(void)
{
	/* In this state the BL2 should not start the manufacturing application.
	 * It should've been overwritten by the main application.
	 *
	 * Transition to LCS "Decommissioned" just to test the LCS awareness
	 * inside the bootloaders.
	 */
	nrf_lcs_set(NRF_LCS_DECOMMISSIONED);

	return 0;
}
#endif /* CONFIG_NRF_LCS */

int main(void)
{
#ifdef CONFIG_NRF_LCS
	int ret = 0;

	LOG_INF("LCS-awareness enabled. Current LCS: %x", nrf_lcs_get());

	switch (nrf_lcs_get()) {
	case NRF_LCS_ASSEMBLY_AND_TEST:
		ret = assembly_and_test_handler();
		break;
	case NRF_LCS_PSA_ROT_PROVISIONING:
		ret = psa_rot_provisioning_handler();
		break;
	case NRF_LCS_SECURED:
		/* Boot and update logic is allowed only in these three states. */
		ret = secured_handler();
		break;

	default:
		LOG_ERR("Device in an unbootable state: %x", nrf_lcs_get());
		return -1;
	}

	if (ret) {
		LOG_ERR("Failed to handle LCS, error: %d", ret);
		return -1;
	}
#endif /* CONFIG_NRF_LCS */

	LOG_INF("Success!");

	return 0;
}
