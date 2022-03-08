/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <lwm2m_os.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <zephyr.h>
#include <string.h>
#include <nrf_modem.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <modem/sms.h>
#include <net/download_client.h>
#include <sys/reboot.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <random/rand32.h>
#include <toolchain.h>
#include <fs/nvs.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <nrf_errno.h>
#include <modem/at_monitor.h>

/* NVS-related defines */

/* Multiple of FLASH_PAGE_SIZE */
#define NVS_SECTOR_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)
/* At least 2 sectors */
#define NVS_SECTOR_COUNT 3
/* Start address of the filesystem in flash */
#define NVS_STORAGE_OFFSET DT_REG_ADDR(DT_NODE_BY_FIXED_PARTITION_LABEL(storage))

static struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
	.flash_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller)),
};

K_THREAD_STACK_ARRAY_DEFINE(lwm2m_os_work_q_client_stack, LWM2M_OS_MAX_WORK_QS, 4096);
struct k_work_q lwm2m_os_work_qs[LWM2M_OS_MAX_WORK_QS];

static struct k_sem lwm2m_os_sems[LWM2M_OS_MAX_SEM_COUNT];
static uint8_t lwm2m_os_sems_used;

/* AT monitor for notifications used by the library */
AT_MONITOR(lwm2m_carrier_at_handler, ANY, lwm2m_os_at_handler);

int lwm2m_os_init(void)
{
	/* Initialize storage */
	if (!device_is_ready(fs.flash_device)) {
		return -ENODEV;
	}

	return nvs_mount(&fs);
}

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

int lwm2m_os_sleep(int ms)
{
	return k_sleep(K_MSEC(ms));
}

/* OS functions */

void lwm2m_os_sys_reset(void)
{
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
	if (PART_OF_ARRAY(lwm2m_os_sems, (struct k_sem *)*sem)) {
		goto reinit;
	}

	__ASSERT(lwm2m_os_sems_used < LWM2M_OS_MAX_SEM_COUNT,
		 "Not enough semaphores in glue layer");

	*sem = (lwm2m_os_sem_t *)&lwm2m_os_sems[lwm2m_os_sems_used];

	lwm2m_os_sems_used++;

reinit:
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
	__ASSERT((id >= LWM2M_OS_STORAGE_BASE) || (id <= LWM2M_OS_STORAGE_END),
		 "Storage ID out of range");

	return nvs_delete(&fs, id);
}

int lwm2m_os_storage_read(uint16_t id, void *data, size_t len)
{
	__ASSERT((id >= LWM2M_OS_STORAGE_BASE) || (id <= LWM2M_OS_STORAGE_END),
		 "Storage ID out of range");

	return nvs_read(&fs, id, data, len);
}

int lwm2m_os_storage_write(uint16_t id, const void *data, size_t len)
{
	__ASSERT((id >= LWM2M_OS_STORAGE_BASE) || (id <= LWM2M_OS_STORAGE_END),
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

	lwm2m_work->handler(lwm2m_work);
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

lwm2m_os_timer_t *lwm2m_os_timer_get(lwm2m_os_timer_handler_t handler)
{
	struct lwm2m_work *work = NULL;

	uint32_t key = irq_lock();

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

	return work;
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

void lwm2m_os_timer_cancel(lwm2m_os_timer_t *timer)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "cancel unknown timer");

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return;
	}

	k_work_cancel_delayable_sync(&work->work_item, &work->work_sync);
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

/* LWM2M logs. */

LOG_MODULE_REGISTER(lwm2m_os, LOG_LEVEL_DBG);

int lwm2m_os_nrf_modem_init(void)
{
	int nrf_err;

#if defined CONFIG_LOG_RUNTIME_FILTERING
	log_filter_set(NULL, CONFIG_LOG_DOMAIN_ID, LOG_CURRENT_MODULE_ID(),
		       CONFIG_LOG_DEFAULT_LEVEL);
#endif /* CONFIG_LOG_RUNTIME_FILTERING */

	nrf_err = nrf_modem_lib_init(NORMAL_MODE);

	switch (nrf_err) {
	case MODEM_DFU_RESULT_OK:
		LOG_INF("Modem firmware update successful.");
		LOG_INF("Modem will run the new firmware after reboot.");
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("Modem firmware update failed.");
		LOG_ERR("Modem will run non-updated firmware on reboot.");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("Modem firmware update failed.");
		LOG_ERR("Fatal error.");
		break;
	case -1:
		LOG_ERR("Could not initialize modem library.");
		LOG_ERR("Fatal error.");
		return -EIO;
	default:
		break;
	}

	return nrf_err;
}

int lwm2m_os_nrf_modem_shutdown(void)
{
	int err = nrf_modem_lib_shutdown();

	if (err != 0) {
		LOG_ERR("nrf_modem_lib_shutdown() failed on: %d", err);
		return -EIO;
	}

	return 0;
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

int lwm2m_os_download_connect(const char *host, const struct lwm2m_os_download_cfg *cfg)
{
	struct download_client_cfg config = {
		.sec_tag = cfg->sec_tag,
		.pdn_id = cfg->pdn_id,
	};

	return download_client_connect(&http_downloader, host, &config);
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

int lwm2m_os_download_start(const char *file, size_t from)
{
	return download_client_start(&http_downloader, file, from);
}

int lwm2m_os_download_file_size_get(size_t *size)
{
	return download_client_file_size_get(&http_downloader, size);
}

/* LTE LC module abstractions. */

static void lwm2m_os_lte_event_handler(const struct lte_lc_evt *const evt)
{
	/* This event handler is not in use by LwM2M carrier library. */
}

int lwm2m_os_lte_link_up(void)
{
	int err;
	static bool initialized;

	if (!initialized) {
		initialized = true;

		err = lte_lc_init();
		if (err) {
			return err;
		}
	}

	return lte_lc_connect_async(lwm2m_os_lte_event_handler);
}

int lwm2m_os_lte_link_down(void)
{
	return lte_lc_offline();
}

int lwm2m_os_lte_power_down(void)
{
	return lte_lc_power_off();
}

int32_t lwm2m_os_lte_mode_get(void)
{
	enum lte_lc_system_mode mode;

	(void)lte_lc_system_mode_get(&mode, NULL);

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		return LWM2M_OS_LTE_MODE_CAT_M1;
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		return LWM2M_OS_LTE_MODE_CAT_NB1;
	default:
		return LWM2M_OS_LTE_MODE_NONE;
	}
}

/* PDN abstractions */

BUILD_ASSERT(
	(int)LWM2M_OS_PDN_FAM_IPV4 == (int)PDN_FAM_IPV4 &&
	(int)LWM2M_OS_PDN_FAM_IPV6 == (int)PDN_FAM_IPV6 &&
	(int)LWM2M_OS_PDN_FAM_IPV4V6 == (int)PDN_FAM_IPV4V6,
	"Incompatible enums"
);
BUILD_ASSERT(
	(int)LWM2M_OS_PDN_EVENT_CNEC_ESM == (int)PDN_EVENT_CNEC_ESM &&
	(int)LWM2M_OS_PDN_EVENT_ACTIVATED == (int)PDN_EVENT_ACTIVATED &&
	(int)LWM2M_OS_PDN_EVENT_DEACTIVATED == (int)PDN_EVENT_DEACTIVATED &&
	(int)LWM2M_OS_PDN_EVENT_IPV6_UP == (int)PDN_EVENT_IPV6_UP &&
	(int)LWM2M_OS_PDN_EVENT_IPV6_DOWN == (int)PDN_EVENT_IPV6_DOWN,
	"Incompatible enums"
);

int lwm2m_os_pdn_init(void)
{
	return pdn_init();
}

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

int lwm2m_os_pdn_default_apn_get(char *buf, size_t len)
{
	return pdn_default_apn_get(buf, len);
}

int lwm2m_os_pdn_default_callback_set(lwm2m_os_pdn_event_handler_t cb)
{
	return pdn_default_callback_set((pdn_event_handler_t)cb);
}

/* errno handling. */
int lwm2m_os_nrf_errno(void)
{
	/* nrf_errno have the same values as newlibc errno */
	return errno;
}

int lwm2m_os_sec_psk_exists(uint32_t sec_tag, bool *exists)
{
	return modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, exists);
}

int lwm2m_os_sec_psk_write(uint32_t sec_tag, const void *buf, uint16_t len)
{
	return modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, buf, len);
}

int lwm2m_os_sec_psk_delete(uint32_t sec_tag)
{
	return modem_key_mgmt_delete(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK);
}

int lwm2m_os_sec_identity_exists(uint32_t sec_tag, bool *exists)
{
	return modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, exists);
}

int lwm2m_os_sec_identity_write(uint32_t sec_tag, const void *buf, uint16_t len)
{
	return modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, buf, len);
}

int lwm2m_os_sec_identity_delete(uint32_t sec_tag)
{
	return modem_key_mgmt_delete(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY);
}
