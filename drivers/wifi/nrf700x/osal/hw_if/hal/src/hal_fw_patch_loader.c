/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing patch loader specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "host_rpu_common_if.h"
#include "hal_fw_patch_loader.h"
#include "hal_mem.h"

/* To reduce HEAP maximum usage */
#define MAX_PATCH_CHUNK_SIZE 8192
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

struct patch_contents {
	const char *id_str;
	const void *data;
	unsigned int size;
	unsigned int dest_addr;
};

/* In order to save RAM, divide the patch in to chunks download */
static enum wifi_nrf_status hal_fw_patch_load(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
						enum RPU_PROC_TYPE rpu_proc,
						const char *patch_id_str,
						unsigned int dest_addr,
						const void *fw_patch_data,
						unsigned int fw_patch_size)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	int last_chunk_size = fw_patch_size % MAX_PATCH_CHUNK_SIZE;
	int num_chunks = fw_patch_size / MAX_PATCH_CHUNK_SIZE +
					(last_chunk_size ? 1 : 0);

	for (int chunk = 0; chunk < num_chunks; chunk++) {
		unsigned char *patch_data_ram;
		unsigned int patch_chunk_size =
			((chunk == num_chunks - 1) ? last_chunk_size : MAX_PATCH_CHUNK_SIZE);
		const void *src_patch_offset = (const char *)fw_patch_data +
			chunk * MAX_PATCH_CHUNK_SIZE;
		int dest_chunk_offset = dest_addr + chunk * MAX_PATCH_CHUNK_SIZE;

		patch_data_ram = wifi_nrf_osal_mem_alloc(hal_dev_ctx->hpriv->opriv,
									patch_chunk_size);
		if (!patch_data_ram) {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				"%s: Failed to allocate memory for patch %s-%s: chunk %d/%d, size: %d\n",
				__func__,
				rpu_proc_to_str(rpu_proc),
				patch_id_str,
				chunk + 1,
				num_chunks,
				patch_chunk_size);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		wifi_nrf_osal_mem_cpy(hal_dev_ctx->hpriv->opriv,
							patch_data_ram,
							src_patch_offset,
							patch_chunk_size);


		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
			"%s: Copying patch %s-%s: chunk %d/%d, size: %d\n",
			__func__,
			rpu_proc_to_str(rpu_proc),
			patch_id_str,
			chunk + 1,
			num_chunks,
			patch_chunk_size);

		status = hal_rpu_mem_write(hal_dev_ctx,
					dest_chunk_offset,
					patch_data_ram,
					patch_chunk_size);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				"%s: Copying patch %s-%s: chunk %d/%d, size: %d failed\n",
				__func__,
				rpu_proc_to_str(rpu_proc),
				patch_id_str,
				chunk + 1,
				num_chunks,
				patch_chunk_size);
			goto out;
		}
out:
		if (patch_data_ram)
			wifi_nrf_osal_mem_free(hal_dev_ctx->hpriv->opriv,
				       patch_data_ram);
		if (status != WIFI_NRF_STATUS_SUCCESS)
			break;
	}

	return status;
}

/*
 * Copies the firmware patches to the RPU memory.
 */
enum wifi_nrf_status wifi_nrf_hal_fw_patch_load(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
						enum RPU_PROC_TYPE rpu_proc,
						const void *fw_pri_patch_data,
						unsigned int fw_pri_patch_size,
						const void *fw_sec_patch_data,
						unsigned int fw_sec_patch_size)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int pri_dest_addr = 0;
	unsigned int sec_dest_addr = 0;

	if (!fw_pri_patch_data) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Primary patch missing for RPU (%d)\n",
				      __func__,
				      rpu_proc);
		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}

	if (!fw_sec_patch_data) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Secondary patch missing for RPU (%d)\n",
				      __func__,
				      rpu_proc);
		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}

	/* Set the HAL RPU context to the current required context */
	hal_dev_ctx->curr_proc = rpu_proc;

	switch (rpu_proc) {
	case RPU_PROC_TYPE_MCU_LMAC:
		pri_dest_addr = RPU_MEM_LMAC_PATCH_BIMG;
		sec_dest_addr = RPU_MEM_LMAC_PATCH_BIN;
		break;
	case RPU_PROC_TYPE_MCU_UMAC:
		pri_dest_addr = RPU_MEM_UMAC_PATCH_BIMG;
		sec_dest_addr = RPU_MEM_UMAC_PATCH_BIN;
		break;
	default:
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid RPU processor type[%d]\n",
				      __func__,
				      rpu_proc);

		goto out;
	}

	const struct patch_contents patches[] = {
		{ "bimg", fw_pri_patch_data, fw_pri_patch_size, pri_dest_addr },
		{ "bin", fw_sec_patch_data, fw_sec_patch_size, sec_dest_addr },
	};

	for (int patch = 0; patch < ARRAY_SIZE(patches); patch++) {
		status = hal_fw_patch_load(hal_dev_ctx,
					   rpu_proc,
					   patches[patch].id_str,
					   patches[patch].dest_addr,
					   patches[patch].data,
					   patches[patch].size);
		if (status != WIFI_NRF_STATUS_SUCCESS)
			goto out;
	}
out:
	/* Reset the HAL RPU context to the LMAC context */
	hal_dev_ctx->curr_proc = RPU_PROC_TYPE_MCU_LMAC;

	return status;
}


enum wifi_nrf_status wifi_nrf_hal_fw_patch_boot(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
						enum RPU_PROC_TYPE rpu_proc,
						bool is_patch_present)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int boot_sig_addr = 0;
	unsigned int boot_sig_val = 0;
	unsigned int boot_excp_0_addr = 0;
	unsigned int boot_excp_1_addr = 0;
	unsigned int boot_excp_2_addr = 0;
	unsigned int boot_excp_3_addr = 0;
	unsigned int boot_excp_0_val = 0;
	unsigned int boot_excp_1_val = 0;
	unsigned int boot_excp_2_val = 0;
	unsigned int boot_excp_3_val = 0;
	unsigned int sleepctrl_addr = 0;
	unsigned int sleepctrl_val = 0;
	unsigned int run_addr = 0;

	if (rpu_proc == RPU_PROC_TYPE_MCU_LMAC) {
		boot_sig_addr = RPU_MEM_LMAC_BOOT_SIG;
		run_addr = RPU_REG_MIPS_MCU_CONTROL;
		boot_excp_0_addr = RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_0;
		boot_excp_0_val = NRF_WIFI_LMAC_BOOT_EXCP_VECT_0;
		boot_excp_1_addr = RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_1;
		boot_excp_1_val = NRF_WIFI_LMAC_BOOT_EXCP_VECT_1;
		boot_excp_2_addr = RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_2;
		boot_excp_2_val = NRF_WIFI_LMAC_BOOT_EXCP_VECT_2;
		boot_excp_3_addr = RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_3;
		boot_excp_3_val = NRF_WIFI_LMAC_BOOT_EXCP_VECT_3;
		if (is_patch_present) {
			sleepctrl_addr = RPU_REG_UCC_SLEEP_CTRL_DATA_0;
			sleepctrl_val = NRF_WIFI_LMAC_ROM_PATCH_OFFSET;
		}
	} else if (rpu_proc == RPU_PROC_TYPE_MCU_UMAC) {
		boot_sig_addr = RPU_MEM_UMAC_BOOT_SIG;
		run_addr = RPU_REG_MIPS_MCU2_CONTROL;
		boot_excp_0_addr = RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_0;
		boot_excp_0_val = NRF_WIFI_UMAC_BOOT_EXCP_VECT_0;
		boot_excp_1_addr = RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_1;
		boot_excp_1_val = NRF_WIFI_UMAC_BOOT_EXCP_VECT_1;
		boot_excp_2_addr = RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_2;
		boot_excp_2_val = NRF_WIFI_UMAC_BOOT_EXCP_VECT_2;
		boot_excp_3_addr = RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_3;
		boot_excp_3_val = NRF_WIFI_UMAC_BOOT_EXCP_VECT_3;
		if (is_patch_present) {
			sleepctrl_addr = RPU_REG_UCC_SLEEP_CTRL_DATA_1;
			sleepctrl_val = NRF_WIFI_UMAC_ROM_PATCH_OFFSET;
		}
	} else {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid RPU processor type %d\n",
				      __func__,
				      rpu_proc);
		goto out;
	}

	/* Set the HAL RPU context to the current required context */
	hal_dev_ctx->curr_proc = rpu_proc;

	/* Clear the firmware pass signature location */
	status = hal_rpu_mem_write(hal_dev_ctx,
				   boot_sig_addr,
				   &boot_sig_val,
				   sizeof(boot_sig_val));

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Clearing of FW pass signature failed for RPU(%d)\n",
				      __func__,
				      rpu_proc);

		goto out;
	}

	if (is_patch_present) {
		/* Write to sleep control register */
		status = hal_rpu_reg_write(hal_dev_ctx,
					   sleepctrl_addr,
					   sleepctrl_val);
	}

	/* Write to Boot exception address 0 */
	status = hal_rpu_reg_write(hal_dev_ctx,
				   boot_excp_0_addr,
				   boot_excp_0_val);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Writing to Boot exception 0 reg for RPU processor(%d) failed\n",
				      __func__,
				      rpu_proc);

		goto out;
	}

	/* Write to Boot exception address 1 */
	status = hal_rpu_reg_write(hal_dev_ctx,
				   boot_excp_1_addr,
				   boot_excp_1_val);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Writing to Boot exception 1 reg for RPU processor(%d) failed\n",
				      __func__,
				      rpu_proc);

		goto out;
	}

	/* Write to Boot exception address 2 */
	status = hal_rpu_reg_write(hal_dev_ctx,
				   boot_excp_2_addr,
				   boot_excp_2_val);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Writing to Boot exception 2 reg for RPU processor(%d) failed\n",
				      __func__,
				      rpu_proc);

		goto out;
	}

	/* Write to Boot exception address 3 */
	status = hal_rpu_reg_write(hal_dev_ctx,
				   boot_excp_3_addr,
				   boot_excp_3_val);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Writing to Boot exception 3 reg for RPU processor(%d) failed\n",
				      __func__,
				      rpu_proc);

		goto out;
	}

	/* Perform pulsed soft reset of MIPS - this should now run */
	status = hal_rpu_reg_write(hal_dev_ctx,
				   run_addr,
				   0x1);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU processor(%d) run failed\n",
				      __func__,
				      rpu_proc);

		goto out;
	}
out:
	/* Reset the HAL RPU context to the LMAC context */
	hal_dev_ctx->curr_proc = RPU_PROC_TYPE_MCU_LMAC;

	return status;

}
