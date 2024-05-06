/*
 * Copyright (c) 2019-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <lwm2m_os.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <nrf_modem.h>
#include <modem/pdn.h>
#include <modem/sms.h>
#include <net/download_client.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/random/random.h>
#include <zephyr/toolchain.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <nrf_errno.h>
#include <modem/at_monitor.h>

/* NVS-related defines */

/* Divide flash area into NVS sectors */
#define NVS_SECTOR_SIZE     (CONFIG_LWM2M_CARRIER_STORAGE_SECTOR_SIZE)
#define NVS_SECTOR_COUNT    (FLASH_AREA_SIZE(lwm2m_carrier) / NVS_SECTOR_SIZE)
/* Start address of the filesystem in flash */
#define NVS_STORAGE_OFFSET  (FLASH_AREA_OFFSET(lwm2m_carrier))
/* Flash Device runtime structure */
#define NVS_FLASH_DEVICE    (DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller)))

static struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
	.flash_device = NVS_FLASH_DEVICE,
};

K_THREAD_STACK_ARRAY_DEFINE(lwm2m_os_work_q_client_stack, LWM2M_OS_MAX_WORK_QS,
			    CONFIG_LWM2M_CARRIER_WORKQ_STACK_SIZE);
static struct k_work_q lwm2m_os_work_qs[LWM2M_OS_MAX_WORK_QS];

static struct k_sem lwm2m_os_sems[LWM2M_OS_MAX_SEM_COUNT];
static uint8_t lwm2m_os_sems_used;

/* AT monitor for notifications used by the library */
AT_MONITOR(lwm2m_carrier_at_handler, ANY, lwm2m_os_at_handler);

/* LwM2M carrier OS logs. */
LOG_MODULE_REGISTER(lwm2m_os);

/* OS initialization. */

static int lwm2m_os_init(void)
{
	/* Initialize storage */
	if (!device_is_ready(fs.flash_device)) {
		return -ENODEV;
	}

	return nvs_mount(&fs);
}

SYS_INIT(lwm2m_os_init, APPLICATION, 0);

/* Memory management. */

void *lwm2m_os_malloc(size_t size)
{
	return k_malloc(size);
}

void lwm2m_os_free(void *ptr)
{
	k_free(ptr);
}

/* Timing functions. */

int64_t lwm2m_os_uptime_get(void)
{
	return k_uptime_get();
}

int64_t lwm2m_os_uptime_delta(int64_t *ref)
{
	return k_uptime_delta(ref);
}

/* OS functions */

void lwm2m_os_sys_reset(void)
{
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
	CODE_UNREACHABLE;
}

uint32_t lwm2m_os_rand_get(void)
{
	return sys_rand32_get();
}

/* Semaphore functions */

int lwm2m_os_sem_init(lwm2m_os_sem_t **sem, unsigned int initial_count, unsigned int limit)
{
	if (*sem == NULL) {
		__ASSERT(lwm2m_os_sems_used < LWM2M_OS_MAX_SEM_COUNT,
			 "Not enough semaphores in glue layer");

		*sem = (lwm2m_os_sem_t *)&lwm2m_os_sems[lwm2m_os_sems_used++];
	} else {
		__ASSERT(PART_OF_ARRAY(lwm2m_os_sems, (struct k_sem *)*sem),
			 "Uninitialised semaphore");
	}

	return k_sem_init((struct k_sem *)*sem, initial_count, limit);
}

int lwm2m_os_sem_take(lwm2m_os_sem_t *sem, int timeout)
{
	__ASSERT(PART_OF_ARRAY(lwm2m_os_sems, (struct k_sem *)sem), "Uninitialised semaphore");

	return k_sem_take((struct k_sem *)sem, timeout == -1 ? K_FOREVER : K_MSEC(timeout));
}

void lwm2m_os_sem_give(lwm2m_os_sem_t *sem)
{
	__ASSERT(PART_OF_ARRAY(lwm2m_os_sems, (struct k_sem *)sem), "Uninitialised semaphore");

	k_sem_give((struct k_sem *)sem);
}

void lwm2m_os_sem_reset(lwm2m_os_sem_t *sem)
{
	__ASSERT(PART_OF_ARRAY(lwm2m_os_sems, (struct k_sem *)sem), "Uninitialised semaphore");

	k_sem_reset((struct k_sem *)sem);
}

/* Non volatile storage */

int lwm2m_os_storage_delete(uint16_t id)
{
	__ASSERT((id >= LWM2M_OS_STORAGE_BASE) && (id <= LWM2M_OS_STORAGE_END),
		 "Storage ID out of range");

	return nvs_delete(&fs, id);
}

int lwm2m_os_storage_read(uint16_t id, void *data, size_t len)
{
	__ASSERT((id >= LWM2M_OS_STORAGE_BASE) && (id <= LWM2M_OS_STORAGE_END),
		 "Storage ID out of range");

	return nvs_read(&fs, id, data, len);
}

int lwm2m_os_storage_write(uint16_t id, const void *data, size_t len)
{
	__ASSERT((id >= LWM2M_OS_STORAGE_BASE) && (id <= LWM2M_OS_STORAGE_END),
		 "Storage ID out of range");

	return nvs_write(&fs, id, data, len);
}

/* LWM2M timer, built on top of delayed works. */

struct lwm2m_work {
	struct k_work_delayable work_item;
	struct k_work_sync work_sync;
	lwm2m_os_timer_handler_t handler;
};

static struct lwm2m_work lwm2m_works[LWM2M_OS_MAX_TIMER_COUNT];

static void work_handler(struct k_work *work)
{
	struct k_work_delayable *delayed_work = CONTAINER_OF(work, struct k_work_delayable, work);
	struct lwm2m_work *lwm2m_work = CONTAINER_OF(delayed_work, struct lwm2m_work, work_item);

	lwm2m_work->handler((lwm2m_os_timer_t *)lwm2m_work);
}

/* Delayed work queue functions */

lwm2m_os_work_q_t *lwm2m_os_work_q_start(int index, const char *name)
{
	__ASSERT(index < LWM2M_OS_MAX_WORK_QS, "Not enough work queues in glue layer");

	if (index >= LWM2M_OS_MAX_WORK_QS) {
		return NULL;
	}

	lwm2m_os_work_q_t *work_q = (lwm2m_os_work_q_t *)&lwm2m_os_work_qs[index];

	k_work_queue_start(&lwm2m_os_work_qs[index], lwm2m_os_work_q_client_stack[index],
			   K_THREAD_STACK_SIZEOF(lwm2m_os_work_q_client_stack[index]),
			   K_LOWEST_APPLICATION_THREAD_PRIO, NULL);

	k_thread_name_set(&lwm2m_os_work_qs[index].thread, name);

	return work_q;
}

void lwm2m_os_timer_get(lwm2m_os_timer_handler_t handler, lwm2m_os_timer_t **timer)
{
	struct lwm2m_work *work = NULL;

	uint32_t key = irq_lock();

	/* Check whether timer exists */
	if (*timer != NULL) {
		__ASSERT(PART_OF_ARRAY(lwm2m_works, *timer), "get unknown timer");
		return;
	}

	/* Find free delayed work */
	for (int i = 0; i < ARRAY_SIZE(lwm2m_works); i++) {
		if (lwm2m_works[i].handler == NULL) {
			work = &lwm2m_works[i];
			work->handler = handler;
			break;
		}
	}

	irq_unlock(key);

	__ASSERT(work != NULL, "Not enough timers in glue layer");

	if (work != NULL) {
		k_work_init_delayable(&work->work_item, work_handler);
	}

	*timer = (lwm2m_os_timer_t *)work;
}

void lwm2m_os_timer_release(lwm2m_os_timer_t *timer)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "release unknown timer");

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return;
	}

	work->handler = NULL;
}

int lwm2m_os_timer_start(lwm2m_os_timer_t *timer, int64_t delay)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "start unknown timer");

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return -EINVAL;
	}

	k_work_reschedule(&work->work_item, K_MSEC(delay));

	return 0;
}

int lwm2m_os_timer_start_on_q(lwm2m_os_work_q_t *work_q, lwm2m_os_timer_t *timer, int64_t delay)
{
	struct k_work_q *queue = (struct k_work_q *)work_q;
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_os_work_qs, queue), "start timer on unknown queue");
	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "start unknown timer on queue");

	if (!PART_OF_ARRAY(lwm2m_works, work) || !PART_OF_ARRAY(lwm2m_os_work_qs, queue)) {
		return -EINVAL;
	}

	k_work_reschedule_for_queue(queue, &work->work_item, K_MSEC(delay));

	return 0;
}

void lwm2m_os_timer_cancel(lwm2m_os_timer_t *timer, bool sync)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "cancel unknown timer");

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return;
	}

	if (sync) {
		k_work_cancel_delayable_sync(&work->work_item, &work->work_sync);
	} else {
		k_work_cancel_delayable(&work->work_item);
	}
}

int64_t lwm2m_os_timer_remaining(lwm2m_os_timer_t *timer)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "get remaining on unknown timer");

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return 0;
	}

	return k_ticks_to_ms_floor64(k_work_delayable_remaining_get(&work->work_item));
}

bool lwm2m_os_timer_is_pending(lwm2m_os_timer_t *timer)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "get is_pending on unknown timer");

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return 0;
	}

	return k_work_delayable_is_pending(&work->work_item);
}

int lwm2m_os_sleep(int ms)
{
	k_timeout_t timeout = (ms == -1) ? K_FOREVER : K_MSEC(ms);

	return k_sleep(timeout);
}

/* AT command module abstractions. */

static lwm2m_os_at_handler_callback_t at_handler_callback;

int lwm2m_os_at_init(lwm2m_os_at_handler_callback_t callback)
{
	at_handler_callback = callback;

	return 0;
}

static void lwm2m_os_at_handler(const char *notif)
{
	if (at_handler_callback != NULL) {
		at_handler_callback(notif);
	}
}

/* SMS subscriber module abstraction.*/

/** @brief Forward declaration */
static lwm2m_os_sms_callback_t lib_sms_callback;

/**
 * @brief Translate SMS event from the nrf-sdk SMS subscriber module into a LwM2M SMS event.
 *
 * @param[in]  data     SMS Subscriber module sms event data.
 * @param[out] lib_data LwM2M SMS event data.
 */
static void sms_evt_translate(struct sms_data *const data, struct lwm2m_os_sms_data *lib_data)
{
	/* User data and length of user data. */
	lib_data->payload_len = data->payload_len;
	lib_data->payload = data->payload;

	/* App port. */
	lib_data->header.deliver.app_port.present = data->header.deliver.app_port.present;
	lib_data->header.deliver.app_port.dest_port = data->header.deliver.app_port.dest_port;

	/* Originator address. */
	lib_data->header.deliver.originating_address.address_str =
		data->header.deliver.originating_address.address_str,

	lib_data->header.deliver.originating_address.length =
		data->header.deliver.originating_address.length;
}

/**
 * @brief Callback to direct SMS subscriber module callback
 *        into our LwM2M carrier library SMS callback.
 *
 * @param[in] data    SMS Subscriber module sms event data.
 * @param[in] context SMS Subscriber module context.
 */
static void sms_callback(struct sms_data *const data, void *context)
{
	/* Discard non-Deliver events. */
	if (data->type != SMS_TYPE_DELIVER) {
		return;
	}
	struct lwm2m_os_sms_data lib_sms_data;

	sms_evt_translate(data, &lib_sms_data);
	lib_sms_callback(&lib_sms_data, context);
}

int lwm2m_os_sms_client_register(lwm2m_os_sms_callback_t callback, void *context)
{
	int ret;

	lib_sms_callback = callback;
	ret = sms_register_listener(sms_callback, context);
	if (ret < 0) {
		LOG_ERR("Unable to register as SMS listener");
		return -EIO;
	}
	LOG_INF("Registered as SMS listener");
	return 0;
}

void lwm2m_os_sms_client_deregister(int handle)
{
	sms_unregister_listener(handle);
	LOG_INF("Deregistered as SMS listener");
}

/* Download client module abstractions. */

static struct download_client http_downloader;
static lwm2m_os_download_callback_t lwm2m_os_lib_callback;

int lwm2m_os_download_get(const char *host, const struct lwm2m_os_download_cfg *cfg, size_t from)
{
	struct download_client_cfg config = {
		.sec_tag_list = cfg->sec_tag_list,
		.sec_tag_count = cfg->sec_tag_count,
		.pdn_id = cfg->pdn_id,
	};

	if (cfg->family == LWM2M_OS_PDN_FAM_IPV6) {
		config.family = AF_INET6;
	} else if (cfg->family == LWM2M_OS_PDN_FAM_IPV4) {
		config.family = AF_INET;
	}

	return download_client_get(&http_downloader, host, &config, NULL, from);
}

int lwm2m_os_download_disconnect(void)
{
	return download_client_disconnect(&http_downloader);
}

static void download_client_evt_translate(const struct download_client_evt *event,
					  struct lwm2m_os_download_evt *lwm2m_os_event)
{
	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT:
		lwm2m_os_event->id = LWM2M_OS_DOWNLOAD_EVT_FRAGMENT;
		lwm2m_os_event->fragment.buf = event->fragment.buf;
		lwm2m_os_event->fragment.len = event->fragment.len;
		break;
	case DOWNLOAD_CLIENT_EVT_DONE:
		lwm2m_os_event->id = LWM2M_OS_DOWNLOAD_EVT_DONE;
		break;
	case DOWNLOAD_CLIENT_EVT_ERROR:
		lwm2m_os_event->id = LWM2M_OS_DOWNLOAD_EVT_ERROR;
		lwm2m_os_event->error = event->error;
		break;
	case DOWNLOAD_CLIENT_EVT_CLOSED:
		lwm2m_os_event->id = LWM2M_OS_DOWNLOAD_EVT_CLOSED;
		break;
	default:
		break;
	}
}

static int callback(const struct download_client_evt *event)
{
	struct lwm2m_os_download_evt lwm2m_os_event;

	download_client_evt_translate(event, &lwm2m_os_event);

	return lwm2m_os_lib_callback(&lwm2m_os_event);
}

int lwm2m_os_download_init(lwm2m_os_download_callback_t lib_callback)
{
	lwm2m_os_lib_callback = lib_callback;

	return download_client_init(&http_downloader, callback);
}

int lwm2m_os_download_file_size_get(size_t *size)
{
	return download_client_file_size_get(&http_downloader, size);
}

#if defined(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>

/* LTE LC module abstractions. */

size_t lwm2m_os_lte_modes_get(int32_t *modes)
{
	enum lte_lc_system_mode mode;

	(void)lte_lc_system_mode_get(&mode, NULL);

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		modes[0] = LWM2M_OS_LTE_MODE_CAT_M1;
		return 1;
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		modes[0] = LWM2M_OS_LTE_MODE_CAT_NB1;
		return 1;
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS:
		modes[0] = LWM2M_OS_LTE_MODE_CAT_M1;
		modes[1] = LWM2M_OS_LTE_MODE_CAT_NB1;
		return 2;
	default:
		return 0;
	}
}

void lwm2m_os_lte_mode_request(int32_t prefer)
{
	enum lte_lc_system_mode mode;
	enum lte_lc_system_mode_preference preference;
	static enum lte_lc_system_mode_preference application_preference;

	(void)lte_lc_system_mode_get(&mode, &preference);

	switch (prefer) {
	case LWM2M_OS_LTE_MODE_CAT_M1:
		application_preference = preference;
		preference = LTE_LC_SYSTEM_MODE_PREFER_LTEM;
		break;
	case LWM2M_OS_LTE_MODE_CAT_NB1:
		application_preference = preference;
		preference = LTE_LC_SYSTEM_MODE_PREFER_NBIOT;
		break;
	case LWM2M_OS_LTE_MODE_NONE:
		preference = application_preference;
		break;
	}

	(void)lte_lc_system_mode_set(mode, preference);
}
#else
#include <nrf_modem_at.h>

size_t lwm2m_os_lte_modes_get(int32_t *modes)
{
	int err, ltem_mode, nbiot_mode, gps_mode, preference;

	/* It's expected to have all 4 arguments matched */
	err = nrf_modem_at_scanf("AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d",
				 &ltem_mode, &nbiot_mode, &gps_mode, &preference);
	if (err != 4) {
		LOG_ERR("Failed to get system mode, error: %d", err);
		return 0;
	}

	if (ltem_mode && nbiot_mode) {
		modes[0] = LWM2M_OS_LTE_MODE_CAT_M1;
		modes[1] = LWM2M_OS_LTE_MODE_CAT_NB1;
		return 2;
	} else if (ltem_mode) {
		modes[0] = LWM2M_OS_LTE_MODE_CAT_M1;
		return 1;
	} else if (nbiot_mode) {
		modes[0] = LWM2M_OS_LTE_MODE_CAT_NB1;
		return 1;
	}

	return 0;
}

void lwm2m_os_lte_mode_request(int32_t prefer)
{
	int err, ltem_mode, nbiot_mode, gps_mode, preference;
	static int application_preference;

	/* It's expected to have all 4 arguments matched */
	err = nrf_modem_at_scanf("AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d",
				 &ltem_mode, &nbiot_mode, &gps_mode, &preference);
	if (err != 4) {
		LOG_ERR("Failed to get system mode, error: %d", err);
		return;
	}

	switch (prefer) {
	case LWM2M_OS_LTE_MODE_CAT_M1:
		application_preference = preference;
		preference = 1;
		break;
	case LWM2M_OS_LTE_MODE_CAT_NB1:
		application_preference = preference;
		preference = 2;
		break;
	case LWM2M_OS_LTE_MODE_NONE:
		preference = application_preference;
		break;
	}

	err = nrf_modem_at_printf("AT%%XSYSTEMMODE=%d,%d,%d,%d",
				  ltem_mode, nbiot_mode, gps_mode, preference);
	if (err) {
		LOG_ERR("Could not send AT command, error: %d", err);
	}
}
#endif

/* PDN abstractions */

BUILD_ASSERT(
	(int)LWM2M_OS_PDN_FAM_IPV4 == (int)PDN_FAM_IPV4 &&
	(int)LWM2M_OS_PDN_FAM_IPV6 == (int)PDN_FAM_IPV6 &&
	(int)LWM2M_OS_PDN_FAM_IPV4V6 == (int)PDN_FAM_IPV4V6 &&
	(int)LWM2M_OS_PDN_FAM_NONIP == (int)PDN_FAM_NONIP,
	"Incompatible enums"
);
BUILD_ASSERT(
	(int)LWM2M_OS_PDN_EVENT_CNEC_ESM == (int)PDN_EVENT_CNEC_ESM &&
	(int)LWM2M_OS_PDN_EVENT_ACTIVATED == (int)PDN_EVENT_ACTIVATED &&
	(int)LWM2M_OS_PDN_EVENT_DEACTIVATED == (int)PDN_EVENT_DEACTIVATED &&
	(int)LWM2M_OS_PDN_EVENT_IPV6_UP == (int)PDN_EVENT_IPV6_UP &&
	(int)LWM2M_OS_PDN_EVENT_IPV6_DOWN == (int)PDN_EVENT_IPV6_DOWN &&
	(int)LWM2M_OS_PDN_EVENT_NETWORK_DETACHED == (int)PDN_EVENT_NETWORK_DETACH,
	"Incompatible enums"
);

int lwm2m_os_pdn_ctx_create(uint8_t *cid, lwm2m_os_pdn_event_handler_t cb)
{
	return pdn_ctx_create(cid, (pdn_event_handler_t)cb);
}

int lwm2m_os_pdn_ctx_configure(uint8_t cid, const char *apn, enum lwm2m_os_pdn_fam family)
{
	return pdn_ctx_configure(cid, apn, (enum pdn_fam)family, NULL);
}

int lwm2m_os_pdn_ctx_destroy(uint8_t cid)
{
	return pdn_ctx_destroy(cid);
}

int lwm2m_os_pdn_activate(uint8_t cid, int *esm, enum lwm2m_os_pdn_fam *family)
{
	return pdn_activate(cid, esm, (enum pdn_fam *)family);
}

int lwm2m_os_pdn_deactivate(uint8_t cid)
{
	return pdn_deactivate(cid);
}

int lwm2m_os_pdn_id_get(uint8_t cid)
{
	return pdn_id_get(cid);
}

int lwm2m_os_pdn_default_callback_set(lwm2m_os_pdn_event_handler_t cb)
{
	return pdn_default_ctx_cb_reg((pdn_event_handler_t)cb);
}

/* errno handling. */
int lwm2m_os_nrf_errno(void)
{
	/* nrf_errno have the same values as newlibc errno */
	return errno;
}

/* DFU target handling */

#include <dfu/dfu_target.h>

static bool dfu_in_progress;
static bool dfu_crc_validate;
static int dfu_img_type;

#if CONFIG_DFU_TARGET_MCUBOOT
#include <dfu/dfu_target_mcuboot.h>
#include <dfu/dfu_target_stream.h>
#include <pm_config.h>
#include <zephyr/sys/crc.h>
#include <zephyr/dfu/mcuboot.h>

#define MCUBOOT_SECONDARY_ADDR	PM_MCUBOOT_SECONDARY_ADDRESS

#if DT_NODE_EXISTS(PM_MCUBOOT_SECONDARY_DEV)
#define MCUBOOT_SECONDARY_MTD	PM_MCUBOOT_SECONDARY_DEV
#else
#define MCUBOOT_SECONDARY_MTD	DT_NODELABEL(PM_MCUBOOT_SECONDARY_DEV)
#endif

static uint8_t dfu_stream_buf[1024] __aligned(4);

static int crc_validate_fragment(const uint8_t *buf, size_t len, uint32_t crc32)
{
	if ((buf == NULL) || (crc32 != crc32_ieee(buf, len))) {
		return -EINVAL;
	}

	return 0;
}

static int crc_validate_stored_image(size_t len, uint32_t crc32)
{
	uintptr_t image_addr = MCUBOOT_SECONDARY_ADDR;

	/* If the image is stored on the same memory technology device (and address space)
	 * as this code, then it is best to CRC-validate the contents directly.
	 */
	if (DT_SAME_NODE(MCUBOOT_SECONDARY_MTD, DT_CHOSEN(zephyr_flash_controller))) {
		return crc_validate_fragment((const uint8_t *)image_addr, len, crc32);
	}

	/* Otherwise, the image contents may need to be validated progressively,
	 * with the help of a different driver (e.g., when using external flash over SPI).
	 * Repurpose `dfu_stream_buf` to buffer the flash reads.
	 */
	uint32_t actual_crc32 = 0;
	const struct device *image_dev = DEVICE_DT_GET_OR_NULL(MCUBOOT_SECONDARY_MTD);

	if (!device_is_ready(image_dev)) {
		return -EIO;
	}

	while (len > 0) {
		size_t fragment_len = MIN(len, sizeof(dfu_stream_buf));
		int err = flash_read(image_dev, image_addr, dfu_stream_buf, fragment_len);

		if (err) {
			return -EIO;
		}

		actual_crc32 = crc32_ieee_update(actual_crc32, dfu_stream_buf, fragment_len);

		len -= fragment_len;
		image_addr += fragment_len;
	}

	if (crc32 != actual_crc32) {
		return -EINVAL;
	}

	return 0;
}
#else
static int crc_validate_fragment(const uint8_t *buf, size_t len, uint32_t crc32)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(crc32);

	return -EINVAL;
}
#endif /* CONFIG_DFU_TARGET_MCUBOOT */

static void dfu_target_cb(enum dfu_target_evt_id evt_id)
{
	/* This event handler is not in use by LwM2M carrier library. */
	ARG_UNUSED(evt_id);
}

#if CONFIG_LWM2M_CARRIER_SOFTBANK_DIVIDED_FOTA
static struct lwm2m_os_dfu_header dfu_header;

static int dfu_img_header_decode(const uint8_t *const buf, size_t len,
				 struct lwm2m_os_dfu_header **header)
{
	struct lwm2m_os_dfu_header new_header = {};

	if (!header) {
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

	/* Reject if the image file is shorter than the required header length. */
	if (len < (LWM2M_OS_DFU_HEADER_MAGIC_LEN + sizeof(struct lwm2m_os_dfu_header))) {
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

	/* Validate the header magic. */
	if (*((const uint32_t *)buf) != LWM2M_OS_DFU_HEADER_MAGIC) {
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

	memcpy(&new_header, buf + LWM2M_OS_DFU_HEADER_MAGIC_LEN, sizeof(new_header));

	/* Validate image that follows the header if it is the first in the sequence. */
	if ((new_header.number == 1) &&
	    (dfu_target_img_type(buf + LWM2M_OS_DFU_HEADER_LEN, len - LWM2M_OS_DFU_HEADER_LEN) !=
	     DFU_TARGET_IMAGE_TYPE_MCUBOOT)) {
		/* Only MCUBoot upgrades supported. */
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

	/* Reject if the version string of the new header is not null-terminated or length is 0. */
	if (!memchr(new_header.version, '\0', sizeof(new_header.version)) ||
	    (strlen(new_header.version) == 0)) {
		LOG_WRN("DFU header version string is invalid");
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

	/* Validate the sequence. May be the next one or the same if already in progress (retry). */
	if ((dfu_in_progress || new_header.number != dfu_header.number + 1) &&
	    (!dfu_in_progress || new_header.number != dfu_header.number)) {
		/* The file does not follow the sequence. */
		LOG_WRN("DFU header does not follow the sequence");
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

	/* Check if this is the continuation of the current sequence. */
	if ((new_header.number != 1) && (strcmp(dfu_header.version, new_header.version) != 0)) {
		/* The versions do not match. */
		LOG_WRN("DFU header version string does not match");
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

	memcpy(&dfu_header, &new_header, sizeof(dfu_header));
	*header = &dfu_header;

	return LWM2M_OS_DFU_IMG_TYPE_APPLICATION_FILE;
}

static void divided_dfu_reset(void)
{
	/* Divided DFU may have been paused. Reinitialize the target to reset appropriately. */
	dfu_target_init(DFU_TARGET_IMAGE_TYPE_MCUBOOT, 0, 0, dfu_target_cb);
	dfu_in_progress = true;
	lwm2m_os_dfu_reset();
}

static bool divided_dfu_in_progress(void)
{
	return dfu_header.number > 0;
}
#endif

int lwm2m_os_dfu_img_type(const void *const buf, size_t len, struct lwm2m_os_dfu_header **header)
{
	enum dfu_target_image_type type;

	if (!buf) {
		return LWM2M_OS_DFU_IMG_TYPE_NONE;
	}

#if CONFIG_LWM2M_CARRIER_SOFTBANK_DIVIDED_FOTA
	type = dfu_img_header_decode(buf, len, header);
	if (type != LWM2M_OS_DFU_IMG_TYPE_NONE) {
		return type;
	}
#else
	ARG_UNUSED(header);
#endif

	type = dfu_target_img_type(buf, len);

	if (type == DFU_TARGET_IMAGE_TYPE_MCUBOOT) {
		return LWM2M_OS_DFU_IMG_TYPE_APPLICATION;
	} else if (type == DFU_TARGET_IMAGE_TYPE_MODEM_DELTA) {
		return LWM2M_OS_DFU_IMG_TYPE_MODEM_DELTA;
	}

	return LWM2M_OS_DFU_IMG_TYPE_NONE;
}

int lwm2m_os_dfu_start(int img_type, size_t max_file_size, bool crc_validate)
{
	int err;
	size_t offset;
	static enum dfu_target_image_type dfu_target;

	if (dfu_in_progress) {
		return -EBUSY;
	}

#if CONFIG_LWM2M_CARRIER_SOFTBANK_DIVIDED_FOTA
	if (divided_dfu_in_progress() && (img_type != LWM2M_OS_DFU_IMG_TYPE_APPLICATION_FILE)) {
		LOG_WRN("Ongoing divided FOTA interrupted, proceed with the new target");
		divided_dfu_reset();
	}
#endif

	/* Translate image type into the DFU target. */
	switch (img_type) {
	case LWM2M_OS_DFU_IMG_TYPE_APPLICATION:
#if CONFIG_LWM2M_CARRIER_SOFTBANK_DIVIDED_FOTA
	case LWM2M_OS_DFU_IMG_TYPE_APPLICATION_FILE:
#endif
		dfu_target = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
		break;
	case LWM2M_OS_DFU_IMG_TYPE_MODEM_DELTA:
		dfu_target = DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
		break;
	default:
		return -ENOTSUP;
	}

	/* A stream buffer is required if the target is MCUBOOT. */
	if (dfu_target == DFU_TARGET_IMAGE_TYPE_MCUBOOT) {
#if CONFIG_DFU_TARGET_MCUBOOT
		err = dfu_target_mcuboot_set_buf(dfu_stream_buf, sizeof(dfu_stream_buf));
		if (err) {
			return -EIO;
		}
#else
		return -ENOTSUP;
#endif
	}

	/* Mark as in progress regardless of the initialization result. The DFU target must be
	 * reset afterwards to ensure full clean-up.
	 */
	dfu_in_progress = true;
	dfu_img_type = img_type;
	dfu_crc_validate = crc_validate;

	err = dfu_target_init(dfu_target, 0, max_file_size, dfu_target_cb);
	if (err == -EFBIG) {
		/* Insufficient flash storage for the image. */
		return err;
	} else if (err) {
		return -EIO;
	}

	err = dfu_target_offset_get(&offset);
	if (err) {
		return -EIO;
	}

	return offset;
}

int lwm2m_os_dfu_fragment(const char *buf, size_t len, uint32_t crc32)
{
	int err;

	if (!dfu_in_progress) {
		return -EACCES;
	}

	/* CRC-validate whole fragment if required. */
	if (dfu_crc_validate) {
		err = crc_validate_fragment(buf, len, crc32);
		if (err) {
			return err;
		}
	}

	/* Write fragment. */
	err = dfu_target_write(buf, len);
	if ((err == -ENOMEM) || (err == -EINVAL) || (err == -E2BIG)) {
		/* Not enough memory to process the fragment, or integrity check failed. */
		return err;
	} else if (err) {
		return -EIO;
	}

	return 0;
}

int lwm2m_os_dfu_done(bool successful, uint32_t crc32)
{
	int err;

	if (!dfu_in_progress) {
		return -EACCES;
	}

#if CONFIG_DFU_TARGET_MCUBOOT
	if (dfu_crc_validate && (dfu_img_type == DFU_TARGET_IMAGE_TYPE_MCUBOOT)) {
		size_t offset;

		/* Ensure that the whole image is flushed into flash before CRC-validating it. */
		err = dfu_target_stream_done(true);
		if (err) {
			return -EIO;
		}

		err = dfu_target_stream_offset_get(&offset);
		if (err) {
			return -EIO;
		}

		err = crc_validate_stored_image(offset, crc32);
		if (err) {
			return err;
		}
	}
#else
	ARG_UNUSED(crc32);
#endif /* CONFIG_DFU_TARGET_MCUBOOT */

	err = dfu_target_done(successful);
	if (err) {
		return -EIO;
	}

	return 0;
}

int lwm2m_os_dfu_pause(void)
{
	int err;
	size_t offset;

	if (!dfu_in_progress) {
		return -EACCES;
	}

	dfu_in_progress = false;

	err = dfu_target_done(false);
	if (err) {
		return -EIO;
	}

	err = dfu_target_offset_get(&offset);
	if (err) {
		return -EIO;
	}

	return offset;
}

int lwm2m_os_dfu_schedule_update(void)
{
	int err;

	if (!dfu_in_progress) {
		return -EACCES;
	}

	err = dfu_target_schedule_update(0);
	if (err == -EINVAL) {
		return err;
	} else if (err) {
		return -EIO;
	}

	return 0;
}

void lwm2m_os_dfu_reset(void)
{
	if (dfu_in_progress) {
		dfu_target_reset();
		dfu_in_progress = false;
		dfu_img_type = 0;
		dfu_crc_validate = false;
#if CONFIG_LWM2M_CARRIER_SOFTBANK_DIVIDED_FOTA
		memset(&dfu_header, 0, sizeof(dfu_header));
	} else if (divided_dfu_in_progress()) {
		divided_dfu_reset();
#endif
	}
}

__weak bool lwm2m_os_dfu_application_update_validate(void)
{
#if defined(CONFIG_MCUBOOT_IMG_MANAGER)
	int err;
	bool img_confirmed;

	img_confirmed = boot_is_img_confirmed();
	if (!img_confirmed) {
		err = boot_write_img_confirmed();
		if (err == 0) {
			LOG_INF("Marked running image as confirmed");
			return true;
		}

		LOG_WRN("Failed to mark running image as confirmed");
	}
#endif /* CONFIG_MCUBOOT_IMG_MANAGER */

	return false;
}
