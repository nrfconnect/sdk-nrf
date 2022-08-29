/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Secure Services header.
 */

#ifndef SECURE_SERVICES_H__
#define SECURE_SERVICES_H__

/**
 * @defgroup secure_services Secure Services
 * @{
 * @brief Secure services available to the Non-Secure Firmware.
 *
 * The Secure Services provide access to functionality controlled by the
 * Secure Firmware.
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <fw_info.h>
#include <../arch/arm/include/aarch32/cortex_m/tz_ns.h>

#ifdef __cplusplus
extern "C" {
#endif

void before_nse(void);
void after_nse(void);

/** Implement a wrapper function around a secure_service.
 *
 * This function must reside in the non-secure binary. It makes the secure
 * service thread safe by locking the scheduler while the service is running.
 * The scheduler locking is done via TZ_THREAD_SAFE_NONSECURE_ENTRY_FUNC().
 *
 * The macro implements of the wrapper function. The wrapper function has the
 * same function signature as the secure service.
 *
 * @param ret   The return type of the secure service and the wrapper function.
 * @param name  The name of the wrapper function. The secure service is assumed
 *              to be named the same, but with the suffix '_nse'. E.g. the
 *              wrapper function foo() wraps the secure service foo_nse().
 * @param ...   The arguments of the secure service and the wrapper function, as
 *              they would appear in a function signature, i.e. type and name.
 */
#define NRF_NSE(ret, name, ...) \
ret name ## _nse(__VA_ARGS__); \
ret __attribute__((naked)) name(__VA_ARGS__) \
{ \
	__TZ_WRAP_FUNC(before_nse, name ## _nse, after_nse); \
}

/** Request a system reboot from the Secure Firmware.
 *
 * Rebooting is not available from the Non-Secure Firmware.
 */
__deprecated void spm_request_system_reboot(void);

/** Request a random number from the Secure Firmware.
 *
 * This provides a CTR_DRBG number from the CC3XX platform libraries.
 *
 * @param[out] output  The CTR_DRBG number. Must be at least @c len long.
 * @param[in]  len     The length of the output array.
 * @param[out] olen    The length of the random number provided.
 *
 * @return non-negative on success, negative errno code on fail
 */
__deprecated int spm_request_random_number(uint8_t *output, size_t len, size_t *olen);

/** Request a read operation to be executed from Secure Firmware.
 *
 * @param[out] destination Pointer to destination array where the content is
 *                         to be copied.
 * @param[in]  addr        Address to be copied from.
 * @param[in]  len         Number of bytes to copy.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If destination is NULL, or if len is <= 0.
 * @retval -EPERM   If source is outside of allowed ranges.
 */
__deprecated int spm_request_read(void *destination, uint32_t addr, size_t len);

/** Check if S0 is the active B1 slot.
 *
 * @param[in]   s0_address Address of s0 slot.
 * @param[in]   s1_address Address of s1 slot.
 * @param[out]  s0_active  Set to 'true' if s0 is active slot, 'false' otherwise
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If info for both slots are NULL.
 */
__deprecated int spm_s0_active(uint32_t s0_address, uint32_t s1_address, bool *s0_active);

/** Search for the fw_info structure in firmware image located at address.
 *
 * @param[in]   fw_address  Address where firmware image is stored.
 * @param[out]  info        Pointer to where found info is stored.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If info is NULL.
 * @retval -EFAULT  If no info is found.
 */
__deprecated int spm_firmware_info(uint32_t fw_address, struct fw_info *info);

/** Prevalidate a B1 update
 *
 * This is performed by the B0 bootloader.
 *
 * @param[in]  dst_addr  Target location for the upgrade. This will typically
 *                       be the start address of either S0 or S1.
 * @param[in]  src_addr  Current location of the upgrade.
 *
 * @retval 1         If the upgrade is valid.
 * @retval 0         If the upgrade is invalid.
 * @retval -ENOTSUP  If the functionality is unavailable.
 */
__deprecated int spm_prevalidate_b1_upgrade(uint32_t dst_addr, uint32_t src_addr);

/** Busy wait in secure mode (debug function)
 *
 * This function is for writing tests that require the execution to be in
 * secure mode
 *
 * @param[in]  busy_wait_us  The number of microseconds to wait for.
 */
__deprecated void spm_busy_wait(uint32_t busy_wait_us);

/** @brief Prototype of the function that is called in non-secure context from
 *	   secure fault handler context.
 *
 * Function can be used to print pending logging data before reboot.
 */
typedef void (*spm_ns_on_fatal_error_t)(void);

/** @brief Set handler which is called by the SPM fault handler.
 *
 * Handler can be used to print out any pending log data before reset.
 *
 * @note It is only for debugging purposes!
 *
 * @param handler Handler.
 *
 * @retval -ENOTSUP if feature is disabled.
 * @retval 0 on success.
 */
__deprecated int spm_set_ns_fatal_error_handler(spm_ns_on_fatal_error_t handler);

/** @brief Call non-secure fatal error handler.
 *
 * Must be called from fatal error handler.
 */
__deprecated void z_spm_ns_fatal_error_handler(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* SECURE_SERVICES_H__ */
