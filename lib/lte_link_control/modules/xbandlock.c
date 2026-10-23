/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>

#include "modules/xbandlock.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* Highest band number that can be given in the <band_mask> bit string. */
#define BAND_MASK_MAX_BAND		88
/* Bit string, plus null terminator. */
#define BAND_MASK_LEN			(BAND_MASK_MAX_BAND + 1)
/* Comma-separated list of bands above BAND_MASK_MAX_BAND, including null terminator. */
#define BAND_LIST_LEN			128

#define XBANDLOCK_OPERATION_REMOVE	0

/* The band lock types are used as the %XBANDLOCK <operation>. */
BUILD_ASSERT(LTE_LC_BAND_LOCK_PERMANENT == 1 && LTE_LC_BAND_LOCK_RUNTIME == 2,
	     "Band lock types must match the %XBANDLOCK operations");

/* +CME ERROR 518: Not allowed in active state. */
#define CME_ERROR_NOT_ALLOWED_IN_ACTIVE_STATE 518

#if defined(CONFIG_LTE_LOCK_BANDS)
BUILD_ASSERT(sizeof(CONFIG_LTE_LOCK_BAND_MASK) > 1 || sizeof(CONFIG_LTE_LOCK_BAND_LIST) > 1,
	     "CONFIG_LTE_LOCK_BAND_MASK or CONFIG_LTE_LOCK_BAND_LIST must be set");
BUILD_ASSERT(sizeof(CONFIG_LTE_LOCK_BAND_MASK) <= BAND_MASK_LEN,
	     "CONFIG_LTE_LOCK_BAND_MASK is too long");
BUILD_ASSERT(sizeof(CONFIG_LTE_LOCK_BAND_LIST) <= BAND_LIST_LEN,
	     "CONFIG_LTE_LOCK_BAND_LIST is too long");

/* Runtime band lock, re-applied when the modem has been powered off. */
static char runtime_mask[BAND_MASK_LEN] = CONFIG_LTE_LOCK_BAND_MASK;
static char runtime_list[BAND_LIST_LEN] = CONFIG_LTE_LOCK_BAND_LIST;
#else
static char runtime_mask[BAND_MASK_LEN];
static char runtime_list[BAND_LIST_LEN];
#endif /* CONFIG_LTE_LOCK_BANDS */

/* Protects the stored runtime band lock, which is also applied from the modem hooks. */
static K_MUTEX_DEFINE(band_lock_mutex);

static bool band_lock_type_is_valid(enum lte_lc_band_lock_type type)
{
	/* The erase-protected band lock, type 3, can only be set in production test firmware. */
	return type == LTE_LC_BAND_LOCK_PERMANENT || type == LTE_LC_BAND_LOCK_RUNTIME;
}

/* Map an error from nrf_modem_at_printf() to an error code. */
static int band_lock_err_map(int err)
{
	if (err < 0) {
		LOG_ERR("Failed to send %%XBANDLOCK, err %d", err);

		return -EFAULT;
	} else if (err > 0) {
		if (nrf_modem_at_err_type(err) == NRF_MODEM_AT_CME_ERROR &&
		    nrf_modem_at_err(err) == CME_ERROR_NOT_ALLOWED_IN_ACTIVE_STATE) {
			LOG_ERR("Band lock can not be changed when the modem is active");

			return -EBUSY;
		}

		LOG_ERR("The modem rejected %%XBANDLOCK, err %d", err);

		return -EBADMSG;
	}

	return 0;
}

/* Returns the error from nrf_modem_at_printf(). */
static int band_lock_send(int operation, const char *mask, const char *list)
{
	if (list[0] == '\0') {
		return nrf_modem_at_printf("AT%%XBANDLOCK=%d,\"%s\"", operation, mask);
	} else if (mask[0] == '\0') {
		return nrf_modem_at_printf("AT%%XBANDLOCK=%d,,\"%s\"", operation, list);
	}

	return nrf_modem_at_printf("AT%%XBANDLOCK=%d,\"%s\",\"%s\"", operation, mask, list);
}

/* Build the <band_mask> bit string from bands up to BAND_MASK_MAX_BAND and the <band_list> from
 * bands above it. The list is only used when needed, because it is not supported by all modem
 * firmware versions.
 */
static int band_lock_format(const uint16_t *bands, size_t band_count, char *mask, char *list)
{
	uint16_t highest_band = 0;
	size_t list_len = 0;

	/* Find the highest band number. */
	for (size_t i = 0; i < band_count; i++) {
		if (bands[i] == 0) {
			LOG_ERR("Invalid band: 0");

			return -EINVAL;
		}

		if (bands[i] <= BAND_MASK_MAX_BAND) {
			highest_band = MAX(highest_band, bands[i]);
		}
	}

	/* Leading zeros are omitted, so the string is as long as the highest band number. */
	memset(mask, '0', highest_band);

	mask[highest_band] = '\0';
	list[0] = '\0';

	for (size_t i = 0; i < band_count; i++) {
		int len;
		bool duplicate = false;

		if (bands[i] <= BAND_MASK_MAX_BAND) {
			/* LSB, the last character, is band 1. */
			mask[highest_band - bands[i]] = '1';

			continue;
		}

		/* Add each band only once to the list. */
		for (size_t j = 0; j < i; j++) {
			if (bands[j] == bands[i]) {
				duplicate = true;
				break;
			}
		}

		if (duplicate) {
			continue;
		}

		len = snprintk(&list[list_len], BAND_LIST_LEN - list_len, "%s%u",
			       list_len > 0 ? "," : "", bands[i]);

		if (len < 0 || len >= BAND_LIST_LEN - list_len) {
			LOG_ERR("Too many bands above band %d", BAND_MASK_MAX_BAND);

			return -E2BIG;
		}

		list_len += len;
	}

	return 0;
}

int lte_lc_band_lock_set(enum lte_lc_band_lock_type type, const uint16_t *bands,
			 size_t band_count)
{
	int err;
	char mask[BAND_MASK_LEN];
	char list[BAND_LIST_LEN];

	if (!band_lock_type_is_valid(type)) {
		LOG_ERR("Invalid band lock type: %d", type);

		return -EINVAL;
	}

	if (bands == NULL || band_count == 0) {
		return -EINVAL;
	}

	err = band_lock_format(bands, band_count, mask, list);
	if (err) {
		return err;
	}

	/* Sending and storing is done under the mutex, so that the modem hooks can not apply the
	 * previous runtime band lock in between.
	 */
	k_mutex_lock(&band_lock_mutex, K_FOREVER);

	err = band_lock_send(type, mask, list);
	if (err == -NRF_EPERM && type == LTE_LC_BAND_LOCK_RUNTIME) {
		/* The modem library is not initialized. The runtime band lock is stored and set
		 * when the modem is initialized.
		 */
		LOG_DBG("Modem library not initialized, band lock is set on modem init");
	} else if (err) {
		err = band_lock_err_map(err);
		goto exit;
	}

	if (type == LTE_LC_BAND_LOCK_RUNTIME) {
		/* The runtime band lock is lost when the modem is powered off. */
		BUILD_ASSERT(sizeof(mask) == sizeof(runtime_mask) &&
			     sizeof(list) == sizeof(runtime_list));
		memcpy(runtime_mask, mask, sizeof(runtime_mask));
		memcpy(runtime_list, list, sizeof(runtime_list));
	}

	err = 0;

exit:
	k_mutex_unlock(&band_lock_mutex);

	return err;
}

int lte_lc_band_lock_remove(void)
{
	int err;

	k_mutex_lock(&band_lock_mutex, K_FOREVER);

	err = nrf_modem_at_printf("AT%%XBANDLOCK=%d", XBANDLOCK_OPERATION_REMOVE);
	if (err) {
		err = band_lock_err_map(err);
	} else {
		runtime_mask[0] = '\0';
		runtime_list[0] = '\0';
	}

	k_mutex_unlock(&band_lock_mutex);

	return err;
}

void xbandlock_runtime_lock_apply(void)
{
	k_mutex_lock(&band_lock_mutex, K_FOREVER);

	if (runtime_mask[0] != '\0' || runtime_list[0] != '\0') {
		(void)band_lock_err_map(band_lock_send(LTE_LC_BAND_LOCK_RUNTIME, runtime_mask,
						       runtime_list));
	}

	k_mutex_unlock(&band_lock_mutex);
}

void xbandlock_runtime_lock_clear(void)
{
	k_mutex_lock(&band_lock_mutex, K_FOREVER);

	runtime_mask[0] = '\0';
	runtime_list[0] = '\0';

	k_mutex_unlock(&band_lock_mutex);
}
