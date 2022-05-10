/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZZHC_INTERNAL_H_
#define ZZHC_INTERNAL_H_

/**
 * @file zzhc_internal.h
 *
 * @defgroup zzhc China Telecom self-registration (Zi-ZHu-Ce, ZZHC)
 *
 * @{
 *
 * @brief Private APIs for the China Telecom self-registration, not supposed to
 *        be referenced by applications.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/settings/settings.h>
#include <modem/lte_lc.h>

#define AT_RESPONSE_LEN   64        /** Length of AT-command response */
#define HTTP_PKT_LEN      512       /** Length of HTTP request packet */
#define ICCID_LEN         19        /** Length of ICCID */

/**@brief State machine */
enum state {
	STATE_INIT = 0,
	STATE_WAIT_FOR_REG,
	STATE_START,
	STATE_HTTP_REQ_SENT,
	STATE_HTTP_RESP_RECVD,
	STATE_RETRY_TIMEOUT,
	STATE_END,
};

/**@brief Context type of self-registration (zzhc) */
struct zzhc {
	enum state state;               /**< State machine. */

	bool registered;                /**< Whether registered to operator */
	bool sim_ready;                 /**< Whether simcard is ready */
	int  sock_fd;                   /**< Socket file descriptor */
	int  http_pkt_len;              /**< HTTP packet length */
	int  rx_offset;                 /**< RX offset */
	int  delimiter;                 /**< Delimiter buffer */
	bool header_recvd;              /**< Header received */
	int  retry_cnt;                 /**< Retry counter */
	char ueiccid[ICCID_LEN+1];      /**< Locally stored ICCID */
	char at_resp[AT_RESPONSE_LEN];  /**< Rx buffer for AT-cmd response */
	char http_pkt[HTTP_PKT_LEN];    /**< Buffer to compile HTTP packets */

	void *thread;                   /**< Pointer to FSM thread */
	void *sem;                      /**< Pointer to Semophore */
	void *at_list;                  /**< List of AT-parameters */
};

#ifndef LOG_DBG
#define LOG_DBG()
#endif

/**@brief TS27.007 ch.10.1.22, EPS registration status 1,
 *        "registered, home network"
 */
#define NW_REGISTERED_HOME      LTE_LC_NW_REG_REGISTERED_HOME

/**@brief TS27.007 ch.10.1.22, EPS registration status 5,
 *        "registered, roaming"
 */
#define NW_REGISTERED_ROAM      LTE_LC_NW_REG_REGISTERED_ROAMING

/**@brief POSIX sem_init. */
#define zzhc_sem_init(sem, pshared, value)      k_sem_init(sem, value, 1)

/**@brief POSIX sem_wait() */
#define zzhc_sem_wait(sem)                      k_sem_take(sem, K_FOREVER)

/**@brief POSIX sem_post() */
#define zzhc_sem_post(sem)                      k_sem_give(sem)

/**@brief Sleep (seconds). */
#define zzhc_sleep(t)                           k_sleep(K_SECONDS(t))

/**
 * @brief Get pointer to the current thread.
 *
 * @return Pointer to the current thread.
 */
#define zzhc_get_current_thread()               k_current_get()

/**
 * @brief Wake up a sleeping thread.
 *
 * @param thr  Pointer to the thread to wake.
 *
 * @return N/A
 */
#define zzhc_wakeup(thr)                        k_wakeup(thr)

/**@brief Malloc. */
#define zzhc_malloc(size)                       k_malloc(size)

/**@brief Free. */
#define zzhc_free(ptr)                          k_free(ptr)

/**@brief Save ICCID to file.
 *
 * The function saves ICCID to non-volatile storage.
 *
 * @param iccid         Pointer to ICCID.
 * @param len           Length of ICCID.
 *
 * @return 0 on success, non-zero on failure.
 *
 */
#define zzhc_save_iccid(iccid, len)                     \
	settings_save_one("zzhc/iccid", iccid, len)

/**@brief Load ICCID from file.
 *
 * The function loads ICCID from non-volatile storage.
 *
 * @param iccid_buf     Pointer to ICCID buffer.
 * @param buf_len       Length of ICCID buffer.
 *
 * @return 0 on success, non-zero on failure.
 *
 */
int zzhc_load_iccid(char *iccid_buf, int buf_len);

/**@brief Encode a buffer into base64 format.
 *
 * This function is equivalent to bash command "base64 < src.bin > dst.bin"
 *
 * @param dst      destination buffer
 * @param dlen     size of the destination buffer
 * @param src      source buffer
 * @param slen     amount of data to be encoded
 *
 * @return         Number of bytes encoded.
 */
int zzhc_base64(unsigned char *dst, size_t dlen, const unsigned char *src,
	size_t slen);

/**@brief Initialization of external modules
 *
 * This function initializes external modules, which are referred to as platform
 * dependent modules.
 *
 * @param ctx      Pointer to zzhc context.
 *
 * @return 0 on success, non-zero on failure.
 *
 */
int zzhc_ext_init(struct zzhc *ctx);

/**@brief Un-initialization of external modules */
void zzhc_ext_uninit(struct zzhc *ctx);

/**@brief Check HTTP payload.
 *
 * This function parses the return string from zzhc server as json format. If
 * "resultCode" is "0" and "resultDesc" is "Success", then the returned result
 * is regarded as success. In case where server returns something else, then
 * the returned result is regarded as failure.
 *
 * @param ctx      Pointer to zzhc context.
 *
 * @retval true    If "resultCode" is "0" and "resultDesc" is "Success".
 * @retval false   Else.
 */
bool zzhc_check_http_payload(struct zzhc *ctx);

/**@brief Get parameter as short from AT-notifications by index.
 *
 * For example, if the notification is "+CEREG: 5,1,......", then:
 *    - Index 0 returns 5;
 *    - Index 1 returns 1;
 *    - ...etc.
 *
 * @param ctx      Pointer to zzhc context.
 * @param data     Pointer to AT-command notification or response.
 * @param idx      Index of parameter to get.
 *
 * @return 0 on success, non-zero on failure.
 *
 */
int zzhc_get_at_param_short(struct zzhc *ctx, const char *data, int idx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZZHC_INTERNAL_H_ */
