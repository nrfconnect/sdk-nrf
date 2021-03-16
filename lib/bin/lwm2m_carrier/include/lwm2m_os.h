/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
 * @brief Maximum number of work queues that the system must support.
 */
#define LWM2M_OS_MAX_WORK_QS 2

/**
 * @brief Maximum number of timers that the system must support.
 */
#define LWM2M_OS_MAX_TIMER_COUNT (6 + (LWM2M_OS_MAX_WORK_QS * 2))

typedef void lwm2m_os_work_q_t;
typedef void lwm2m_os_timer_t;

/**
 * @brief Maximum number of semaphores that the system must support.
 */
#define LWM2M_OS_MAX_SEM_COUNT (5 + (LWM2M_OS_MAX_WORK_QS * 3))

/* pointer to semaphore object */
typedef void lwm2m_os_sem_t;

#define LWM2M_LOG_LEVEL_NONE 0
#define LWM2M_LOG_LEVEL_ERR  1
#define LWM2M_LOG_LEVEL_WRN  2
#define LWM2M_LOG_LEVEL_INF  3
#define LWM2M_LOG_LEVEL_TRC  4

#define LWM2M_OS_LTE_MODE_NONE   -1
#define LWM2M_OS_LTE_MODE_CAT_M1  6
#define LWM2M_OS_LTE_MODE_CAT_NB1 7

/**
 * @brief Range of the non-volatile storage identifiers used by the library.
 *
 * @note  The application MUST NOT use the values within this range for its
 *        own non-volatile storage management as it could potentially delete
 *        or overwrite entries used by the library.
 */
#define LWM2M_OS_STORAGE_BASE 0xCA00
#define LWM2M_OS_STORAGE_END  0xCAFF

/**
 * @brief Timer callback function.
 */
typedef void (*lwm2m_os_timer_handler_t)(void *timer);

struct lwm2m_os_sms_deliver_address {
	char   *address_str;
	uint8_t length;
};

struct lwm2m_os_sms_udh_app_port {
	bool present;
	uint16_t dest_port;
	uint16_t src_port;
};

struct lwm2m_os_sms_deliver_header {
	struct lwm2m_os_sms_deliver_address originating_address;
	struct lwm2m_os_sms_udh_app_port    app_port;
};

union lwm2m_os_sms_header {
	struct lwm2m_os_sms_deliver_header deliver;
};

/** @brief SMS PDU data. */
struct lwm2m_os_sms_data {
	/** SMS header. */
	union lwm2m_os_sms_header header;
	/** Length of the data in data buffer. */
	int payload_len;
	/** SMS message data. */
	char *payload;
};

/**
 * @brief SMS subscriber callback function.
 */
typedef void (*lwm2m_os_sms_callback_t)(struct lwm2m_os_sms_data *const data, void *context);

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
#define LWM2M_OS_DOWNLOAD_EVT_ERROR    1
#define LWM2M_OS_DOWNLOAD_EVT_DONE     2
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
	int port;
	int pdn_id;
};

/**
 * @brief Download client asynchronous event handler.
 */
typedef int (*lwm2m_os_download_callback_t)(const struct lwm2m_os_download_evt *event);

/** @brief PDN family */
enum lwm2m_os_pdn_fam {
	LWM2M_OS_PDN_FAM_IPV4,
	LWM2M_OS_PDN_FAM_IPV6,
	LWM2M_OS_PDN_FAM_IPV4V6,
};

/** @brief PDN event */
enum lwm2m_os_pdn_event {
	LWM2M_OS_PDN_EVENT_CNEC_ESM,
	LWM2M_OS_PDN_EVENT_ACTIVATED,
	LWM2M_OS_PDN_EVENT_DEACTIVATED,
	LWM2M_OS_PDN_EVENT_IPV6_UP,
	LWM2M_OS_PDN_EVENT_IPV6_DOWN,
};

/**
 * @brief PDN event handler.
 *
 * If assigned during PDP context creation, the event handler will receive
 * status information relative to the Packet Data Network connection,
 * as reported by the AT notifications CNEC and GGEV.
 *
 * This handler is executed by the same context that dispatches AT notifications.
 */
typedef void (*lwm2m_os_pdn_event_handler_t)
	(uint8_t cid, enum lwm2m_os_pdn_event event, int reason);

/**
 * @brief Initialize the PDN functionality.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_init(void);

/**
 * @brief Create a Packet Data Protocol (PDP) context.
 *
 * If a callback is provided via the @c cb parameter,
 * generate events from the CNEC and GGEV AT notifications to report
 * state of the Packet Data Network (PDN) connection.
 *
 * @param[out] cid The ID of the new PDP context.
 * @param cb Optional event handler.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_ctx_create(uint8_t *cid, lwm2m_os_pdn_event_handler_t cb);

/**
 * @brief Configure a Packet Data Protocol context.
 *
 * @param cid The PDP context to configure.
 * @param apn The Access Point Name to configure the PDP context with.
 * @param family The family to configure the PDN context for.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_ctx_configure(uint8_t cid, const char *apn, enum lwm2m_os_pdn_fam family);

/**
 * @brief Destroy a Packet Data Protocol context.
 *
 * @param cid The PDP context to destroy.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_ctx_destroy(uint8_t cid);

/**
 * @brief Activate a Packet Data Network (PDN) connection.
 *
 * @param cid The PDP context ID to activate a connection for.
 * @param[out] esm If provided, the function will block to return
 *		   the ESM error reason.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_activate(uint8_t cid, int *esm);

/**
 * @brief Deactivate a Packet Data Network (PDN) connection.
 *
 * @param cid The PDP context ID.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_deactivate(uint8_t cid);

/**
 * @brief Retrieve the PDN ID for a given PDP Context.
 *
 * The PDN ID can be used to route traffic through a Packet Data Network.
 *
 * @param cid The context ID of the PDN connection.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_id_get(uint8_t cid);

/**
 * @brief Retrieve the default Access Point Name (APN).
 *
 * The default APN is the APN of the default PDP context (zero).
 *
 * @param buf The buffer to copy the APN into.
 * @param len The size of the output buffer.
 * @return int Zero on success or an errno otherwise.
 */
int lwm2m_os_pdn_default_apn_get(char *buf, size_t len);

/**
 * @brief Set a callback for events pertaining to the default PDP context (zero).
 *
 * @param cb The PDN event handler.
 */
int lwm2m_os_pdn_default_callback_set(lwm2m_os_pdn_event_handler_t cb);

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
 * @brief Initialize a semaphore.
 *
 * @param sem Address of the pointer to the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @retval 0 Semaphore created successfully
 * @retval -EINVAL Invalid values
 */
int lwm2m_os_sem_init(lwm2m_os_sem_t **sem, unsigned int initial_count, unsigned int limit);

/**
 * @brief Take a semaphore.
 *
 * @param sem Address of the semaphore.
 * @param timeout Timeout in ms or -1 for forever.
 *
 * @retval 0 Semaphore taken.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int lwm2m_os_sem_take(lwm2m_os_sem_t *sem, int timeout);

/**
 * @brief Give a semaphore.
 *
 * @param sem Address of the semaphore.
 */
void lwm2m_os_sem_give(lwm2m_os_sem_t *sem);

/**
 * @brief Reset a semaphore.
 *
 * @param sem Address of the semaphore.
 */
void lwm2m_os_sem_reset(lwm2m_os_sem_t *sem);

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
 * @brief Start a workqueue.
 *
 * @param index number of the queue.
 * @param name Name of the queue.
 *
 * @retval Workqueue.
 */
lwm2m_os_work_q_t *lwm2m_os_work_q_start(int index, const char *name);

/**
 * @brief Reserve a timer task from the OS.
 *
 * @param handler Function to run for this task.
 *
 * @retval Timer task.
 */
lwm2m_os_timer_t *lwm2m_os_timer_get(lwm2m_os_timer_handler_t handler);

/**
 * @brief Release a timer task.
 */
void lwm2m_os_timer_release(lwm2m_os_timer_t *timer);

/**
 * @brief Start a timer on system work queue.
 *
 * @param timer Timer task.
 * @param msec Delay before submitting the task in ms.
 *
 * @retval  0      If timeout is @c NO_WAIT and work was already on a queue
 * @retval  1      If timeout is @c NO_WAIT and work was not submitted but has now been queued to
 *                 workqueue; or timeout is not @c NO_WAIT and work has been scheduled
 * @retval  2      If delay is @c NO_WAIT and work was running and has been queued to the queue
 *                 that was running it
 * @retval -EINVAL timer not found.
 */
int lwm2m_os_timer_start(lwm2m_os_timer_t *timer, int64_t timeout);

/**
 * @brief Start a timer on a specific queue.
 *
 * @param work_q Workqueue.
 * @param timer Timer task.
 * @param msec Delay before submitting the task in ms.
 *
 * @retval  0      If timeout is @c NO_WAIT and work was already on a queue
 * @retval  1      If timeout is @c NO_WAIT and work was not submitted but has now been queued to
 *                 @p work_q; or timeout is not @c NO_WAIT and work has been scheduled
 * @retval  2      If delay is @c NO_WAIT and work was running and has been queued to the queue
 *                 that was running it
 * @retval -EINVAL Timer or work_q not found.
 */
int lwm2m_os_timer_start_on_q(lwm2m_os_work_q_t *work_q, lwm2m_os_timer_t *timer, int64_t msec);

/**
 * @brief Cancel a timer run.
 *
 * @param timer Timer task.
 */
void lwm2m_os_timer_cancel(lwm2m_os_timer_t *timer);

/**
 * @brief Obtain the time remaining on a timer.
 *
 * @param timer Timer task.
 *
 * @retval Time remaining in ms.
 */
int64_t lwm2m_os_timer_remaining(lwm2m_os_timer_t *timer);

/**
 * @brief Check if a timer task is pending.
 *
 * @param timer Timer task.
 *
 * @retval true if a timer task is pending.
 */
bool lwm2m_os_timer_is_pending(lwm2m_os_timer_t *timer);

/**
 * @brief Create a string copy for a logger subsystem.
 */
const char *lwm2m_os_log_strdup(const char *str);

/**
 * @brief Log a message.
 */
void lwm2m_os_log(int level, const char *fmt, ...);

/**
 * @brief Print a data dump via logger.
 *
 * @param msg  Log message.
 * @param data Data to dump.
 * @param len  Data length.
 */
void lwm2m_os_logdump(int level, const char *msg, const void *data, size_t len);

/**
 * @brief Initialize modem library.
 *
 * @return 0  on success
 * @return -1 on error
 * @return an error from @em nrf_modem_dfu in case of modem DFU.
 */
int lwm2m_os_nrf_modem_init(void);

/**
 * @brief Shutdown the Modem library.
 *
 * @return 0 on success, -1 otherwise.
 */
int lwm2m_os_nrf_modem_shutdown(void);

/**
 * @brief Initialize AT command driver.
 */
int lwm2m_os_at_init(void);

/**
 * @brief Initialize SMS subscriber library.
 */
int lwm2m_os_sms_init(void);

/**
 * @brief Uninitialize SMS subscriber library.
 */
void lwm2m_os_sms_uninit(void);

/**
 * @brief Set AT command global notification handler.
 */
int lwm2m_os_at_notif_register_handler(void *context, lwm2m_os_at_cmd_handler_t handler);

/**
 * @brief Register as an SMS client/listener.
 */
int lwm2m_os_sms_client_register(lwm2m_os_sms_callback_t lib_callback, void *context);

/**
 * @brief degister as an SMS client/listener.
 */
void lwm2m_os_sms_client_deregister(int handle);

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
int lwm2m_os_at_params_list_init(struct lwm2m_os_at_param_list *list, size_t max_params_count);

/**
 * @brief Get a parameter value as an integer number.
 */
int lwm2m_os_at_params_int_get(struct lwm2m_os_at_param_list *list, size_t index, uint32_t *value);

/**
 * @brief Get a parameter value as a short number.
 */
int lwm2m_os_at_params_short_get(struct lwm2m_os_at_param_list *list, size_t index,
				 uint16_t *value);

/**
 * @brief Get a parameter value as a string.
 */
int lwm2m_os_at_params_string_get(struct lwm2m_os_at_param_list *list, size_t index, char *value,
				  size_t *len);

/**
 * @brief Clear/reset all parameter types and values.
 */
int lwm2m_os_at_params_list_clear(struct lwm2m_os_at_param_list *list);

/**
 * @brief Parse AT command or response parameters from a string.
 */
int lwm2m_os_at_parser_params_from_str(const char *at_params_str, char **next_param_str,
				       struct lwm2m_os_at_param_list *const list);

/**
 * @brief Get the number of valid parameters in the list.
 */
int lwm2m_os_at_params_valid_count_get(struct lwm2m_os_at_param_list *list);

/**
 * @brief Establish a connection with the server.
 */
int lwm2m_os_download_connect(const char *host, const struct lwm2m_os_download_cfg *cfg);

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

/**
 * @brief Retrieve size of file being downloaded.
 *
 * @param size Size of the file being downloaded.
 */
int lwm2m_os_download_file_size_get(size_t *size);

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

/*
 * @brief get system mode from modem.
 *
 * @retval LWM2M_OS_LTE_MODE_NONE    Not connected
 * @retval LWM2M_OS_LTE_MODE_CAT_M1  Cat-M1 (LTE-FDD)
 * @retval LWM2M_OS_LTE_MODE_CAT_NB1 Cat-NB1 (NB-IOT)
 */
int32_t lwm2m_os_lte_mode_get(void);

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
 * @param[in]  sec_tag  The tag to search for.
 * @param[out] exists   Whether the credential exists.
 *                      Only valid if the operation is successful.
 * @param[out] perm     The permission flags of the credential.
 *                      Not yet implemented.
 *
 * @retval 0        On success.
 * @retval -ENOBUFS Insufficient memory.
 * @retval -EPERM   Insufficient permissions.
 */
int lwm2m_os_sec_ca_chain_exists(uint32_t sec_tag, bool *exists, uint8_t *perm);

/**
 * @brief Compare a certificate chain.
 *
 * @param[in] sec_tag Security tag associated with the certificate chain.
 * @param[in] buf     Buffer to compare the certificate chain to.
 * @param[in] len     Length of the certificate chain.
 *
 * @retval 0   If the certificate chain match.
 * @retval 1   If the certificate chain do not match.
 * @retval < 0 On error.
 */
int lwm2m_os_sec_ca_chain_cmp(uint32_t sec_tag, const void *buf, size_t len);

/**
 * @brief Provision a certificate chain or update an existing one.
 *
 * @note If used when the LTE link is active, the function will return
 *       an error and the key will not be written.
 *
 * @param[in]  sec_tag  Security tag for this credential.
 * @param[in]  buf      Buffer containing the credential data.
 * @param[in]  len      Length of the buffer.
 *
 * @retval 0        On success.
 * @retval -EINVAL  Invalid parameters.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EACCES  The operation failed because the LTE link is active.
 * @retval -ENOMEM  Not enough memory to store the credential.
 * @retval -ENOENT  The security tag could not be written.
 * @retval -EPERM   Insufficient permissions.
 */
int lwm2m_os_sec_ca_chain_write(uint32_t sec_tag, const void *buf, size_t len);

/**
 * @brief Check if a pre-shared key exists in persistent storage.
 *
 * @param[in]   sec_tag     The tag to search for.
 * @param[out]  exists      Whether the credential exists.
 *                          Only valid if the operation is successful.
 * @param[out]  perm_flags  The permission flags of the credential.
 *                          Only valid if the operation is successful
 *                          and @p exists is @c true.
 *                          Not yet implemented.
 *
 * @retval 0        On success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EPERM   Insufficient permissions.
 * @retval -EIO     Internal error.
 */
int lwm2m_os_sec_psk_exists(uint32_t sec_tag, bool *exists, uint8_t *perm_flags);

/**
 * @brief Provision a new pre-shared key or update an existing one.
 *
 * @note If used when the LTE link is active, the function will return
 *       an error and the key will not be written.
 *
 * @param[in] sec_tag  Security tag for this credential.
 * @param[in] buf      Buffer containing the credential data.
 * @param[in] len      Length of the buffer.
 *
 * @retval 0        On success.
 * @retval -EINVAL  Invalid parameters.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -ENOMEM  Not enough memory to store the credential.
 * @retval -EACCES  The operation failed because the LTE link is active.
 * @retval -ENOENT  The security tag could not be written.
 * @retval -EPERM   Insufficient permissions.
 * @retval -EIO     Internal error.
 */
int lwm2m_os_sec_psk_write(uint32_t sec_tag, const void *buf, uint16_t len);

/**
 * @brief Delete pre-shared key.
 *
 * @note If used when the LTE link is active, the function will return
 *       an error and the key will not be deleted.
 *
 * @param[in] sec_tag  Security tag for this credential.
 *
 * @retval 0        On success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EACCES  The operation failed because the LTE link is active.
 * @retval -ENOENT  No credential associated with the given
 *                  @p sec_tag and @p cred_type.
 * @retval -EPERM   Insufficient permissions.
 */
int lwm2m_os_sec_psk_delete(uint32_t sec_tag);

/**
 * @brief Check if an identity credential exists in persistent storage.
 *
 * @param[in]  sec_tag    The tag to search for.
 * @param[out] exists     Whether the credential exists.
 *                        Only valid if the operation is successful.
 * @param[out] perm_flags The permission flags of the credential.
 *                        Only valid if the operation is successful
 *                        and @p exists is @c true.
 *                        Not yet implemented.
 *
 * @retval 0        On success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EPERM   Insufficient permissions.
 * @retval -EIO     Internal error.
 */
int lwm2m_os_sec_identity_exists(uint32_t sec_tag, bool *exists, uint8_t *perm_flags);

/**
 * @brief Provision a new identity credential or update an existing one.
 *
 * @note If used when the LTE link is active, the function will return
 *       an error and the key will not be written.
 *
 * @param[in] sec_tag  Security tag for this credential.
 * @param[in] buf      Buffer containing the credential data.
 * @param[in] len      Length of the buffer.
 *
 * @retval 0        On success.
 * @retval -EINVAL  Invalid parameters.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -ENOMEM  Not enough memory to store the credential.
 * @retval -EACCES  The operation failed because the LTE link is active.
 * @retval -ENOENT  The security tag could not be written.
 * @retval -EPERM   Insufficient permissions.
 * @retval -EIO     Internal error.
 */
int lwm2m_os_sec_identity_write(uint32_t sec_tag, const void *buf, uint16_t len);

/**
 * @brief Delete identity credential.
 *
 * @note If used when the LTE link is active, the function will return
 *       an error and the key will not be deleted.
 *
 * @param[in] sec_tag Security tag for this credential.
 *
 * @retval 0        On success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EACCES  The operation failed because the LTE link is active.
 * @retval -ENOENT  No credential associated with the given
 *                  @p sec_tag and @p cred_type.
 * @retval -EPERM   Insufficient permissions.
 */
int lwm2m_os_sec_identity_delete(uint32_t sec_tag);

/**@} */

#endif /* LWM2M_OS_H__ */
