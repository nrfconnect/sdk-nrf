/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief TFM IOCTL API header.
 */


#ifndef TFM_IOCTL_API_H__
#define TFM_IOCTL_API_H__

/**
 * @defgroup tfm_ioctl_api TFM IOCTL API
 * @{
 *
 */

#include <limits.h>
#include <stdint.h>
#include <tfm_platform_api.h>
#include <hal/nrf_gpio.h>

/* Include core IOCTL services */
#include <tfm_ioctl_core_api.h>

#include <zephyr/autoconf.h>

#if CONFIG_FW_INFO
#include <fw_info_bare.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Board specific IOCTL services can be added here */
enum tfm_platform_ioctl_reqest_types_t {
	TFM_PLATFORM_IOCTL_FW_INFO = TFM_PLATFORM_IOCTL_CORE_LAST,
	TFM_PLATFORM_IOCTL_NS_FAULT,
};

#if CONFIG_FW_INFO

/** @brief Argument list for each platform firmware info service.
 */
struct tfm_fw_info_args_t {
	void *fw_address;
	struct fw_info *info;
};

/** @brief Output list for each platform firmware info service
 */
struct tfm_fw_info_out_t {
	uint32_t result;
};

/** Search for the fw_info structure in firmware image located at address.
 *
 * @param[in]   fw_address  Address where firmware image is stored.
 * @param[out]  info        Pointer to where found info is to be written.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If info is NULL or if no info is found.
 * @retval -EPERM   If the TF-M platform service request failed.
 */
int tfm_platform_firmware_info(uint32_t fw_address, struct fw_info *info);

/** Check if S0 is the active B1 slot.
 *
 * @param[in]   s0_address Address of s0 slot.
 * @param[in]   s1_address Address of s1 slot.
 * @param[out]  s0_active  Set to 'true' if s0 is active slot, 'false' otherwise
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If info for both slots could not be found.
 * @retval -EPERM   If the TF-M platform service request failed.
 */
int tfm_platform_s0_active(uint32_t s0_address, uint32_t s1_address,
			   bool *s0_active);

#endif /* CONFIG_FW_INFO */

/** @brief Bitmask of SPU events */
enum tfm_spu_events {
	TFM_SPU_EVENT_RAMACCERR = 1 << 0,
	TFM_SPU_EVENT_FLASHACCERR = 1 << 1,
	TFM_SPU_EVENT_PERIPHACCERR = 1 << 2,
};

/** @brief Copy of exception frame on stack. */
struct tfm_ns_fault_service_handler_context_frame {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
};

/** @brief Copy of callee saved registers. */
struct tfm_ns_fault_service_handler_context_registers {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
};

/** @brief Additional fault status information */
struct tfm_ns_fault_service_handler_context_status {
	uint32_t msp;
	uint32_t psp;
	uint32_t exc_return;
	uint32_t control;
	uint32_t cfsr;
	uint32_t hfsr;
	uint32_t sfsr;
	uint32_t bfar;
	uint32_t mmfar;
	uint32_t sfar;
	uint32_t spu_events;
	uint32_t vectactive;
};

/** @brief Non-secure fault service callback context argument. */
struct tfm_ns_fault_service_handler_context {
	bool valid;
	struct tfm_ns_fault_service_handler_context_registers registers;
	struct tfm_ns_fault_service_handler_context_frame frame;
	struct tfm_ns_fault_service_handler_context_status status;
};

/** @brief Non-secure fault service callback type. */
typedef void (*tfm_ns_fault_service_handler_callback)(void);

/** @brief Non-secure fault service arguments.*/
struct tfm_ns_fault_service_args {
	struct tfm_ns_fault_service_handler_context  *context;
	tfm_ns_fault_service_handler_callback callback;
};

/** @brief Output list for each nonsecure_fault platform service
 */
struct tfm_ns_fault_service_out {
	uint32_t result;
};

/** Search for the fw_info structure in firmware image located at address.
 *
 * @param[in] context  Pointer to callback context information, stored in non-secure memory.
 * @param[in] callback Callback to non-secure function to be called from secure fault handler.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If input arguments are invalid.
 */
int tfm_platform_ns_fault_set_handler(struct tfm_ns_fault_service_handler_context *context,
				      tfm_ns_fault_service_handler_callback callback);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* TFM_IOCTL_API_H__ */
