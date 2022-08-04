/*
 * Copyright (c) 2019-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file lwm2m_os.h
 *
 * @defgroup lwm2m_carrier_os LWM2M OS layer
 * @{
 */

#ifndef LWM2M_OS_H__
#define LWM2M_OS_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of work queues that the system must support.
 */
#define LWM2M_OS_MAX_WORK_QS 2

/**
 * @brief Maximum number of timers that the system must support.
 */
#define LWM2M_OS_MAX_TIMER_COUNT (5 + (LWM2M_OS_MAX_WORK_QS * 4))

typedef void lwm2m_os_work_q_t;
typedef void lwm2m_os_timer_t;

/**
 * @brief Maximum number of threads that the system must support.
 */
#define LWM2M_OS_MAX_THREAD_COUNT 1

typedef void (*lwm2m_os_thread_entry_t)(void *p1, void *p2, void *p3);

/**
 * @brief Maximum number of semaphores that the system must support.
 */
#define LWM2M_OS_MAX_SEM_COUNT (4 + (LWM2M_OS_MAX_WORK_QS * 2))

/* pointer to semaphore object */
typedef void lwm2m_os_sem_t;

#define LWM2M_OS_LTE_MODE_NONE   -1
/* LTE Rel-13 Cat-M1 HD-FDD == E-UTRAN == LTE-M */
#define LWM2M_OS_LTE_MODE_CAT_M1  6
/* LTE Rel-13 Cat-NB1 HD-FDD || LTE Rel-14 Cat-NB1 and Cat-NB2 HD-FDD == E-UTRAN NB-S1 == NB-IoT */
#define LWM2M_OS_LTE_MODE_CAT_NB1 7

/**
 * @brief Range of the non-volatile storage identifiers used by the library.
 */
#define LWM2M_OS_STORAGE_BASE 0xCA00
#define LWM2M_OS_STORAGE_END  0xCAFF

/**
 * @brief AT command error handler callback function.
 */
typedef void (*lwm2m_os_at_handler_callback_t)(const char *notif);

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
 * @defgroup lwm2m_os_download_evt_id LwM2M OS download events
 * @{
 */
#define LWM2M_OS_DOWNLOAD_EVT_FRAGMENT 0
#define LWM2M_OS_DOWNLOAD_EVT_ERROR    1
#define LWM2M_OS_DOWNLOAD_EVT_DONE     2
/** @} */

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

/** @brief Download client configuration values. */
#define LWM2M_OS_DOWNLOAD_SEC_TAG_NONE -1

/**
 * @brief Download client configuration options.
 */
struct lwm2m_os_download_cfg {
	/** Security tag to be used for TLS. Set to LWM2M_OS_DOWNLOAD_SEC_TAG_NONE if non-secure. */
	int sec_tag;
	/** PDN ID to be used for the download. */
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
 * If assigned during PDP context creation, the event handler will receive status information
 * relative to the Packet Data Network connection, as reported by the AT notifications CNEC and
 * GGEV.
 *
 * This handler is executed by the same context that dispatches AT notifications.
 */
typedef void (*lwm2m_os_pdn_event_handler_t)
	(uint8_t cid, enum lwm2m_os_pdn_event event, int reason);

/**
 * @brief Initialize the PDN functionality.
 *
 * @retval  0      If success.
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
 * @param      cb  Optional event handler.
 *
 * @retval  0      If success.
 */
int lwm2m_os_pdn_ctx_create(uint8_t *cid, lwm2m_os_pdn_event_handler_t cb);

/**
 * @brief Configure a Packet Data Protocol context.
 *
 * @param cid    The PDP context to configure.
 * @param apn    The Access Point Name to configure the PDP context with.
 * @param family The family to configure the PDN context for.
 *
 * @retval  0      If success.
 */
int lwm2m_os_pdn_ctx_configure(uint8_t cid, const char *apn, enum lwm2m_os_pdn_fam family);

/**
 * @brief Destroy a Packet Data Protocol context.
 *
 * @param cid The PDP context to destroy.
 *
 * @retval  0      If success.
 */
int lwm2m_os_pdn_ctx_destroy(uint8_t cid);

/**
 * @brief Activate a Packet Data Network (PDN) connection.
 *
 * @param      cid    The PDP context ID to activate a connection for.
 * @param[out] esm    If provided, the function will block to return the ESM error reason.
 * @param[out] family If provided, the function will block to return PDN_FAM_IPV4 if only IPv4 is
 *                    supported, or PDN_FAM_IPV6 if only IPv6 is supported. Otherwise, this value
 *                    will remain unchanged.
 *
 * @retval  0      If success.
 */
int lwm2m_os_pdn_activate(uint8_t cid, int *esm, enum lwm2m_os_pdn_fam *family);

/**
 * @brief Deactivate a Packet Data Network (PDN) connection.
 *
 * @param cid The PDP context ID.
 *
 * @retval  0      If success.
 */
int lwm2m_os_pdn_deactivate(uint8_t cid);

/**
 * @brief Retrieve the PDN ID for a given PDP Context.
 *
 * The PDN ID can be used to route traffic through a Packet Data Network.
 *
 * @param cid The context ID of the PDN connection.
 *
 * @retval  0      If success.
 */
int lwm2m_os_pdn_id_get(uint8_t cid);

/**
 * @brief Retrieve the default Access Point Name (APN).
 *
 * The default APN is the APN of the default PDP context (zero).
 *
 * @param buf The buffer to copy the APN into.
 * @param len The size of the output buffer.
 *
 * @retval  0      If success.
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
 * @param sem           Address of the pointer to the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit         Maximum permitted semaphore count.
 *
 * @retval  0      Semaphore created successfully.
 * @retval -EINVAL Invalid values.
 */
int lwm2m_os_sem_init(lwm2m_os_sem_t **sem, unsigned int initial_count, unsigned int limit);

/**
 * @brief Take a semaphore.
 *
 * @param sem     Address of the semaphore.
 * @param timeout Timeout in ms or -1 for forever.
 *
 * @retval  0      Semaphore taken.
 * @retval -EBUSY  Returned without waiting.
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
 * @param index Number of the queue.
 * @param name  Name of the queue.
 *
 * @return Workqueue.
 */
lwm2m_os_work_q_t *lwm2m_os_work_q_start(int index, const char *name);

/**
 * @brief Reserve a timer task from the OS.
 *
 * @param handler Function to run for this task.
 *
 * @return Timer task.
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
 * @param delay Delay before submitting the task in ms.
 *
 * @retval  0      Work placed on queue, already on queue or already running.
 * @retval -EINVAL Timer not found.
 */
int lwm2m_os_timer_start(lwm2m_os_timer_t *timer, int64_t delay);

/**
 * @brief Start a timer on a specific queue.
 *
 * @param work_q Workqueue.
 * @param timer  Timer task.
 * @param delay  Delay before submitting the task in ms.
 *
 * @retval  0      Work placed on queue, already on queue or already running.
 * @retval -EINVAL Timer or work_q not found.
 */
int lwm2m_os_timer_start_on_q(lwm2m_os_work_q_t *work_q, lwm2m_os_timer_t *timer, int64_t delay);

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
 * @return Time remaining in ms.
 */
int64_t lwm2m_os_timer_remaining(lwm2m_os_timer_t *timer);

/**
 * @brief Check if a timer task is pending.
 *
 * @param timer Timer task.
 *
 * @retval  true  If a timer task is pending.
 */
bool lwm2m_os_timer_is_pending(lwm2m_os_timer_t *timer);

/**
 * @brief Create and start a new thread.
 *
 * @param index Number of the thread.
 * @param entry Thread entry function.
 * @param name  Name of the thread.
 *
 * @retval  0      Thread created and started.
 * @retval -EINVAL Thread index not valid.
 */
int lwm2m_os_thread_start(int index, lwm2m_os_thread_entry_t entry, const char *name);

/**
 * @brief Initialize modem library.
 *
 * @retval  0      If success.
 * @retval -EIO    If modem initialization failed.
 * @return  A positive number @em nrf_modem_dfu in case of modem firmware update.
 */
int lwm2m_os_nrf_modem_init(void);

/**
 * @brief Shutdown the Modem library.
 *
 * @retval  0      If success.
 * @retval -EIO    If modem shutdown failed.
 */
int lwm2m_os_nrf_modem_shutdown(void);

/**
 * @brief Initialize AT command driver.
 *
 * @retval  0      If success.
 * @retval -EIO    If AT command driver initialization failed.
 */
int lwm2m_os_at_init(lwm2m_os_at_handler_callback_t callback);

/**
 * @brief Initialize SMS subscriber library.
 */
int lwm2m_os_sms_init(void);

/**
 * @brief Uninitialize SMS subscriber library.
 */
void lwm2m_os_sms_uninit(void);

/**
 * @brief Register as an SMS client/listener.
 *
 * @retval  0      If success.
 * @retval -EIO    If unable to register as SMS listener.
 */
int lwm2m_os_sms_client_register(lwm2m_os_sms_callback_t lib_callback, void *context);

/**
 * @brief degister as an SMS client/listener.
 */
void lwm2m_os_sms_client_deregister(int handle);

/**
 * @brief Establish a connection with the server.
 *
 * @retval  0      If success.
 */
int lwm2m_os_download_connect(const char *host, const struct lwm2m_os_download_cfg *cfg);

/**
 * @brief Disconnect from the server.
 *
 * @retval  0      If success.
 */
int lwm2m_os_download_disconnect(void);

/**
 * @brief Initialize the download client.
 *
 * @retval  0      If success.
 */
int lwm2m_os_download_init(lwm2m_os_download_callback_t lib_callback);

/**
 * @brief Download a file.
 *
 * @retval  0      If success.
 */
int lwm2m_os_download_start(const char *file, size_t from);

/**
 * @brief Retrieve size of file being downloaded.
 *
 * @param size Size of the file being downloaded.
 *
 * @retval  0      If success.
 */
int lwm2m_os_download_file_size_get(size_t *size);

/**
 * @brief get enabled system modes from modem.
 *
 * @param modes Array to store the enabled modes.
 *                  LWM2M_OS_LTE_MODE_CAT_M1  Cat-M1 (LTE-FDD)
 *                  LWM2M_OS_LTE_MODE_CAT_NB1 Cat-NB1 (NB-IoT)
 *
 * @return Number of enabled modes:
 */
size_t lwm2m_os_lte_modes_get(int32_t *modes);

/**
 * @brief set preferred bearer in modem.
 *
 * @param prefer LWM2M_OS_LTE_MODE_NONE    for no preference
 *               LWM2M_OS_LTE_MODE_CAT_M1  for Cat-M1 (LTE-FDD)
 *               LWM2M_OS_LTE_MODE_CAT_NB1 for Cat-NB1 (NB-IoT)
 *
 * @return Number of enabled modes:
 */
void lwm2m_os_lte_mode_request(int32_t prefer);

/**
 * @brief Translate the error number.
 */
int lwm2m_os_nrf_errno(void);

/**
 * @brief Check if a pre-shared key exists in persistent storage.
 *
 * @param[in]   sec_tag    The tag to search for.
 * @param[out]  exists     Whether the credential exists. Only valid if the operation is successful.
 *
 * @retval  0       If success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EPERM   Insufficient permissions.
 * @retval -EIO     Internal error.
 */
int lwm2m_os_sec_psk_exists(uint32_t sec_tag, bool *exists);

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
 * @retval  0       On success.
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
 * @retval  0       If success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EACCES  The operation failed because the LTE link is active.
 * @retval -ENOENT  No credential associated with the given @p sec_tag.
 * @retval -EPERM   Insufficient permissions.
 */
int lwm2m_os_sec_psk_delete(uint32_t sec_tag);

/**
 * @brief Check if an identity credential exists in persistent storage.
 *
 * @param[in]  sec_tag    The tag to search for.
 * @param[out] exists     Whether the credential exists. Only valid if the operation is successful.
 *
 * @retval  0       If success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EPERM   Insufficient permissions.
 * @retval -EIO     Internal error.
 */
int lwm2m_os_sec_identity_exists(uint32_t sec_tag, bool *exists);

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
 * @retval  0       If success.
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
 * @retval  0       If success.
 * @retval -ENOBUFS Internal buffer is too small.
 * @retval -EACCES  The operation failed because the LTE link is active.
 * @retval -ENOENT  No credential associated with the given @p sec_tag.
 * @retval -EPERM   Insufficient permissions.
 */
int lwm2m_os_sec_identity_delete(uint32_t sec_tag);

/**
 * @brief Start an application firmware upgrade.
 *
 * @param[in] max_file_size Estimate of the new firmware image to be received. May be greater than
 *                          or equal to the actual image size received by the end.
 *
 * @retval  0       Ready to start a new firmware upgrade.
 * @return  A positive number of bytes written so far, if the previous upgrade was not completed.
 *          In this case, the upgrade will resume from this offset.
 * @retval -EBUSY   Another application firmware upgrade is already ongoing.
 * @retval -ENOMEM  Not enough memory to store the file of the given size.
 * @retval -EIO     Internal error.
 * @retval -ENOTSUP This function is not implemented.
 */
int lwm2m_os_app_fota_start(size_t max_file_size);

/**
 * @brief Receive an application firmware image fragment and validate its CRC.
 *
 * @param[in] buf    Buffer containing the fragment.
 * @param[in] len    Length of the fragment in bytes.
 * @param[in] offset Offset into the buffer, from which to start copying the fragment to flash.
 * @param[in] crc32  Expected IEEE CRC32 value. Must be checked for the whole fragment.
 *
 * @retval  0       Success.
 * @retval -EACCES  lwm2m_os_app_fota_start() was not called beforehand.
 * @retval -ENOMEM  Not enough memory to store the file.
 * @retval -EINVAL  CRC error.
 * @retval -EIO     Internal error.
 * @retval -ENOTSUP Firmware image not recognized, or this function is not implemented.
 */
int lwm2m_os_app_fota_fragment(const char *buf, uint16_t len, uint16_t offset, uint32_t crc32);

/**
 * @brief Finalize the current application firmware upgrade and CRC-validate the image.
 *
 * @param[in] crc32  Expected IEEE CRC32 value. Should be checked for the whole file in flash.
 *
 * @retval  0       Success.
 * @retval -EACCES  lwm2m_os_app_fota_start() was not called beforehand.
 * @retval -EINVAL  CRC error.
 * @retval -EIO     Internal error.
 * @retval -ENOTSUP This function is not implemented.
 */
int lwm2m_os_app_fota_finish(uint32_t crc32);

/**
 * @brief Abort the current application firmware upgrade.
 */
void lwm2m_os_app_fota_abort(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* LWM2M_OS_H__ */
