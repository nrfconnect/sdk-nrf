/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef LWM2M_OS_H__
#define LWM2M_OS_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**@file lwm2m_os.h
 *
 * @defgroup lwm2m_carrier_os LWM2M OS layer
 * @{
 */

/**
 * @brief Maximum number of timers that the system must support.
 */
#define LWM2M_OS_MAX_TIMER_COUNT 8

#define LWM2M_LOG_LEVEL_NONE 0
#define LWM2M_LOG_LEVEL_ERR 1
#define LWM2M_LOG_LEVEL_WRN 2
#define LWM2M_LOG_LEVEL_INF 3
#define LWM2M_LOG_LEVEL_TRC 4

/**
 * @brief Range of the non-volatile storage identifiers used by the library.
 *
 * @note  The application MUST NOT use the values within this range for its
 *        own non-volatile storage management as it could potentially delete
 *        or overwrite entries used by the library.
 */
#define LWM2M_OS_STORAGE_BASE 0xCA00
#define LWM2M_OS_STORAGE_END 0xCAFF

/**
 * @brief Timer callback function.
 */
typedef void (*lwm2m_os_timer_handler_t)(void *timer);

/**
 * @brief List of AT parameters that compose an AT command or response.
 */
struct lwm2m_os_at_param_list {
	size_t param_count;
	void *params;
};

/**
 * @brief AT Command handler.
 */
typedef void (*lwm2m_os_at_cmd_handler_t)(void *ctx, const char *response);

/**
 * @defgroup lwm2m_os_download_evt_id LwM2M OS download events
 * @{
 */
#define LWM2M_OS_DOWNLOAD_EVT_FRAGMENT 0
#define LWM2M_OS_DOWNLOAD_EVT_ERROR 1
#define LWM2M_OS_DOWNLOAD_EVT_DONE 2
/**@} */

/**
 * @brief Download client event.
 */
struct lwm2m_os_download_evt {
	/** Event ID. */
	int id;
	union {
		/** Error cause. */
		int error;
		/** Fragment data. */
		struct lwm2m_os_fragment {
			const void *buf;
			size_t len;
		} fragment;
	};
};

/**
 * @brief Download client configuration options.
 */
struct lwm2m_os_download_cfg {
	int sec_tag;
	const char *apn;
};

/**
 * @brief Download client asynchronous event handler.
 */
typedef int (*lwm2m_os_download_callback_t)(
	const struct lwm2m_os_download_evt *event);

/**
 * @brief Initialize the LWM2M OS layer.
 */
int lwm2m_os_init(void);

/**
 * @brief Allocate memory.
 */
void *lwm2m_os_malloc(size_t size);

/**
 * @brief Free memory.
 */
void lwm2m_os_free(void *ptr);

/**
 * @brief Get uptime, in milliseconds.
 */
int64_t lwm2m_os_uptime_get(void);

/**
 * @brief Get uptime delta, in milliseconds.
 */
int64_t lwm2m_os_uptime_delta(int64_t *ref);

/**
 * @brief Put a thread to a sleep.
 */
int lwm2m_os_sleep(int ms);

/**
 * @brief Reboot system.
 */
void lwm2m_os_sys_reset(void);

/**
 * @brief Get a random value.
 */
uint32_t lwm2m_os_rand_get(void);

/**
 * @brief Delete a non-volatile storage entry.
 */
int lwm2m_os_storage_delete(uint16_t id);

/**
 * @brief Read an entry from non-volatile storage.
 */
int lwm2m_os_storage_read(uint16_t id, void *data, size_t len);

/**
 * @brief Write an entry to non-volatile storage.
 */
int lwm2m_os_storage_write(uint16_t id, const void *data, size_t len);

/**
 * @brief Request a timer from the OS.
 */
void *lwm2m_os_timer_get(lwm2m_os_timer_handler_t handler);

/**
 * @brief Release a timer.
 */
void lwm2m_os_timer_release(void *timer);

/**
 * @brief Start a timer.
 */
int lwm2m_os_timer_start(void *timer, int32_t timeout);

/**
 * @brief Cancel a timer run.
 */
void lwm2m_os_timer_cancel(void *timer);

/**
 * @brief Obtain the time remaining on a timer.
 */
int32_t lwm2m_os_timer_remaining(void *timer);

/**
 * @brief Create a string copy for a logger subsystem.
 */
const char *lwm2m_os_log_strdup(const char *str);

/**
 * @brief Log a message.
 */
void lwm2m_os_log(int level, const char *fmt, ...);

/**
 * @brief Initialize BSD library.
 *
 * @return 0  on success
 * @return -1 on error
 * @return an error from @ref bsd_modem_dfu in case of modem DFU
 */
int lwm2m_os_bsdlib_init(void);

/**
 * @brief Shutdown BSD library.
 *
 * @return 0 on success, -1 otherwise.
 */
int lwm2m_os_bsdlib_shutdown(void);

/**
 * @brief Initialize AT command driver.
 */
int lwm2m_os_at_init(void);

/**
 * @brief Set AT command global notification handler.
 */
int lwm2m_os_at_notif_register_handler(void *context,
				       lwm2m_os_at_cmd_handler_t handler);

/**
 * @brief Send an AT command and receive response immediately.
 */
int lwm2m_os_at_cmd_write(const char *const cmd, char *buf, size_t buf_len);

/**
 * @brief Free a list of AT parameters.
 */
void lwm2m_os_at_params_list_free(struct lwm2m_os_at_param_list *list);

/**
 * @brief Create a list of AT parameters.
 */
int lwm2m_os_at_params_list_init(struct lwm2m_os_at_param_list *list,
				 size_t max_params_count);

/**
 * @brief Get a parameter value as an integer number.
 */
int lwm2m_os_at_params_int_get(struct lwm2m_os_at_param_list *list,
			       size_t index, uint32_t *value);

/**
 * @brief Get a parameter value as a short number.
 */
int lwm2m_os_at_params_short_get(struct lwm2m_os_at_param_list *list,
				 size_t index, uint16_t *value);

/**
 * @brief Get a parameter value as a string.
 */
int lwm2m_os_at_params_string_get(struct lwm2m_os_at_param_list *list,
				  size_t index, char *value, size_t *len);

/**
 * @brief Clear/reset all parameter types and values.
 */
int lwm2m_os_at_params_list_clear(struct lwm2m_os_at_param_list *list);

/**
 * @brief Parse AT command or response parameters from a string.
 */
int lwm2m_os_at_parser_params_from_str(
	const char *at_params_str, char **next_param_str,
	struct lwm2m_os_at_param_list *const list);

/**
 * @brief Get the number of valid parameters in the list.
 */
int lwm2m_os_at_params_valid_count_get(
	struct lwm2m_os_at_param_list *list);

/**
 * @brief Establish a connection with the server.
 */
int lwm2m_os_download_connect(const char *host,
			      const struct lwm2m_os_download_cfg *cfg);

/**
 * @brief Disconnect from the server.
 */
int lwm2m_os_download_disconnect(void);

/**
 * @brief Initialize the download client.
 */
int lwm2m_os_download_init(lwm2m_os_download_callback_t lib_callback);

/**
 * @brief Download a file.
 */
int lwm2m_os_download_start(const char *file, size_t from);

/*
 * @brief Initialize and make a connection with the modem.
 */
int lwm2m_os_lte_link_up(void);

/*
 * @brief Set the modem to offline mode.
 */
int lwm2m_os_lte_link_down(void);

/*
 * @brief Set the modem to power off mode.
 */
int lwm2m_os_lte_power_down(void);

/**
 * @brief Translate the error number.
 */
int lwm2m_os_errno(void);

/**
 * @brief Return a textual description for the current error.
 */
const char *lwm2m_os_strerror(void);

/**
 * @brief Check if a certificate chain credential exists in persistent storage.
 *
 * @param[in]  sec_tag		The tag to search for.
 * @param[out] exists		Whether the credential exists.
 *				Only valid if the operation is successful.
 * @param[out] perm_flags	The permission flags of the credential.
 *				Only valid if the operation is successful
 *				and @param exists is @c true.
 *				Not yet implemented.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Insufficient memory.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int lwm2m_os_sec_ca_chain_exists(uint32_t sec_tag, bool *exists,
				 uint8_t *perm_flags);

/**
 * @brief Provision a certificate chain or update an existing one.
 *
 * @note If used when the LTE link is active, the function will return
 *	 an error and the key will not be written.
 *
 * @param[in]  sec_tag	Security tag for this credential.
 * @param[in]  buf	Buffer containing the credential data.
 * @param[in]  len	Length of the buffer.
 *
 * @retval 0		On success.
 * @retval -EINVAL	Invalid parameters.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -EACCES	The operation failed because the LTE link is active.
 * @retval -ENOMEM	Not enough memory to store the credential.
 * @retval -ENOENT	The security tag could not be written.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int lwm2m_os_sec_ca_chain_write(uint32_t sec_tag, const void *buf,
				uint16_t len);

/**
 * @brief Check if a pre-shared key exists in persistent storage.
 *
 * @param[in]   sec_tag		The tag to search for.
 * @param[out]  exists		Whether the credential exists.
 *				Only valid if the operation is successful.
 * @param[out]  perm_flags	The permission flags of the credential.
 *				Only valid if the operation is successful
 *				and @param exists is @c true.
 *				Not yet implemented.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int lwm2m_os_sec_psk_exists(uint32_t sec_tag, bool *exists,
			    uint8_t *perm_flags);

/**
 * @brief Provision a new pre-shared key or update an existing one.
 *
 * @note If used when the LTE link is active, the function will return
 *	 an error and the key will not be written.
 *
 * @param[in] sec_tag	Security tag for this credential.
 * @param[in] buf	Buffer containing the credential data.
 * @param[in] len	Length of the buffer.
 *
 * @retval 0		On success.
 * @retval -EINVAL	Invalid parameters.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOMEM	Not enough memory to store the credential.
 * @retval -EACCES	The operation failed because the LTE link is active.
 * @retval -ENOENT	The security tag could not be written.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int lwm2m_os_sec_psk_write(uint32_t sec_tag, const void *buf, uint16_t len);

/**
 * @brief Check if an identity credential exists in persistent storage.
 *
 * @param[in]  sec_tag		The tag to search for.
 * @param[out] exists		Whether the credential exists.
 *				Only valid if the operation is successful.
 * @param[out] perm_flags	The permission flags of the credential.
 *				Only valid if the operation is successful
 *				and @param exists is true.
 *				Not yet implemented.
 *
 * @retval 0		On success.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int lwm2m_os_sec_identity_exists(uint32_t sec_tag, bool *exists,
				 uint8_t *perm_flags);

/**
 * @brief Provision a new identity credential or update an existing one.
 *
 * @note If used when the LTE link is active, the function will return
 *	 an error and the key will not be written.
 *
 * @param[in] sec_tag	Security tag for this credential.
 * @param[in] buf	Buffer containing the credential data.
 * @param[in] len	Length of the buffer.
 *
 * @retval 0		On success.
 * @retval -EINVAL	Invalid parameters.
 * @retval -ENOBUFS	Internal buffer is too small.
 * @retval -ENOMEM	Not enough memory to store the credential.
 * @retval -EACCES	Th operation failed because the LTE link is active.
 * @retval -ENOENT	The security tag could not be written.
 * @retval -EPERM	Insufficient permissions.
 * @retval -EIO		Internal error.
 */
int lwm2m_os_sec_identity_write(uint32_t sec_tag, const void *buf,
				uint16_t len);

/**@} */

#endif /* LWM2M_OS_H__ */
