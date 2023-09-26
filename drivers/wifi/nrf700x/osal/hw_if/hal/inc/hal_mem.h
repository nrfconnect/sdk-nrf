/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing memoy read/write specific declarations for the
 * HAL Layer of the Wi-Fi driver.
 */

#ifndef __HAL_MEM_H__
#define __HAL_MEM_H__

#include "hal_api.h"

enum HAL_RPU_MEM_TYPE {
	HAL_RPU_MEM_TYPE_GRAM,
	HAL_RPU_MEM_TYPE_PKTRAM,
	HAL_RPU_MEM_TYPE_CORE_ROM,
	HAL_RPU_MEM_TYPE_CORE_RET,
	HAL_RPU_MEM_TYPE_CORE_SCRATCH,
	HAL_RPU_MEM_TYPE_MAX
};


/**
 * hal_rpu_mem_read() - Read from the RPU memory.
 * @hpriv: Pointer to HAL context.
 * @host_addr: Pointer to the host memory where the contents read from the RPU
 *             memory are to be copied.
 * @rpu_mem_addr: Absolute value of the RPU memory address from which the
 *                contents are to be read.
 * @len: The length (in bytes) of the contents to be read from the RPU memory.
 *
 * This function reads len bytes of contents from the RPU memory and copies them
 * to the host memory pointed to by the host_addr parameter.
 *
 * Return: Status
 *		Pass : %NRF_WIFI_STATUS_SUCCESS
 *		Error: %NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status hal_rpu_mem_read(struct nrf_wifi_hal_dev_ctx *hal_ctx,
				      void *host_addr,
				      unsigned int rpu_mem_addr,
				      unsigned int len);

/**
 * hal_rpu_mem_write() - Write to the RPU memory.
 * @hpriv: Pointer to HAL context.
 * @rpu_mem_addr: Absolute value of the RPU memory address where the contents
 *                are to be written.
 * @host_addr: Pointer to the host memory from where the contents are to be
 *             copied to the RPU memory.
 * @len: The length (in bytes) of the contents to be copied to the RPU memory.
 *
 * This function writes len bytes of contents to the RPU memory from the host
 * memory pointed to by the host_addr parameter.
 *
 * Return: Status
 *		Pass : %NRF_WIFI_STATUS_SUCCESS
 *		Error: %NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status hal_rpu_mem_write(struct nrf_wifi_hal_dev_ctx *hal_ctx,
				       unsigned int rpu_mem_addr,
				       void *host_addr,
				       unsigned int len);


/**
 * hal_rpu_mem_clr() - Clear contents of RPU memory.
 * @hal_ctx: Pointer to HAL context.
 * @mem_type: The type of the RPU memory to be cleared.
 *
 * This function fills the RPU memory with 0's.
 *
 * Return: Status
 *		Pass : %NRF_WIFI_STATUS_SUCCESS
 *		Error: %NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status hal_rpu_mem_clr(struct nrf_wifi_hal_dev_ctx *hal_ctx,
				     enum RPU_PROC_TYPE rpu_proc,
				     enum HAL_RPU_MEM_TYPE mem_type);
#endif /* __HAL_MEM_H__ */
