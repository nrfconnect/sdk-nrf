/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
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
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <modem/sms.h>
#include <net/download_client.h>
#include <sys/reboot.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <toolchain.h>
#include <fs/nvs.h>
#include <logging/log.h>
#include <nrf_errno.h>

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
};

K_THREAD_STACK_ARRAY_DEFINE(lwm2m_os_work_q_client_stack, LWM2M_OS_MAX_WORK_QS, 2048);
struct k_work_q lwm2m_os_work_qs[LWM2M_OS_MAX_WORK_QS];

static struct k_sem lwm2m_os_sems[LWM2M_OS_MAX_SEM_COUNT];
static uint8_t lwm2m_os_sems_used;

int lwm2m_os_init(void)
{
	/* Initialize storage */
	return nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
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
	return k_cycle_get_32();
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

	if ((id == LWM2M_OS_STORAGE_END - 16) ||
	    (id == LWM2M_OS_STORAGE_END - 18) ||
	    (id == LWM2M_OS_STORAGE_END - 19)) {
		return 0;
	}

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

	/* Workaround. The work is being reused by the library without ever reassigning
	 * the handler, hence the timer cannot be released. Instead, only cancel the work.
	 */
	k_work_cancel_delayable_sync(&work->work_item, &work->work_sync);
}

int lwm2m_os_timer_start(lwm2m_os_timer_t *timer, int64_t msec)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "start unknown timer");

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return -EINVAL;
	}

	k_work_reschedule(&work->work_item, K_MSEC(msec));

	return 0;
}

int lwm2m_os_timer_start_on_q(lwm2m_os_work_q_t *work_q, lwm2m_os_timer_t *timer, int64_t msec)
{
	struct k_work_q *queue = (struct k_work_q *)work_q;
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	__ASSERT(PART_OF_ARRAY(lwm2m_os_work_qs, queue), "start timer on unknown queue");
	__ASSERT(PART_OF_ARRAY(lwm2m_works, work), "start unknown timer on queue");

	if (!PART_OF_ARRAY(lwm2m_works, work) || !PART_OF_ARRAY(lwm2m_os_work_qs, queue)) {
		return -EINVAL;
	}

	k_work_reschedule_for_queue(queue, &work->work_item, K_MSEC(msec));

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

LOG_MODULE_REGISTER(lwm2m, CONFIG_LOG_DEFAULT_LEVEL);

static const uint8_t log_level_lut[] = {
	LOG_LEVEL_NONE, /* LWM2M_LOG_LEVEL_NONE */
	LOG_LEVEL_ERR,	/* LWM2M_LOG_LEVEL_ERR */
	LOG_LEVEL_WRN,	/* LWM2M_LOG_LEVEL_WRN */
	LOG_LEVEL_INF,	/* LWM2M_LOG_LEVEL_INF */
	LOG_LEVEL_DBG,	/* LWM2M_LOG_LEVEL_TRC */
};

const char *lwm2m_os_log_strdup(const char *str)
{
	if (IS_ENABLED(CONFIG_LOG)) {
		return log_strdup(str);
	}

	return str;
}

void lwm2m_os_log(int level, const char *fmt, ...)
{
	if (IS_ENABLED(CONFIG_LOG)) {
		struct log_msg_ids src_level = {
			.level = log_level_lut[level],
			.domain_id = CONFIG_LOG_DOMAIN_ID,
			.source_id = LOG_CURRENT_MODULE_ID()
		};

		va_list ap;

		va_start(ap, fmt);
		log_generic(src_level, fmt, ap, LOG_STRDUP_SKIP);
		va_end(ap);
	}
}

void lwm2m_os_logdump(int level, const char *str, const void *data, size_t len)
{
	if (IS_ENABLED(CONFIG_LOG)) {
		struct log_msg_ids src_level = {
			.level = log_level_lut[level],
			.domain_id = CONFIG_LOG_DOMAIN_ID,
			.source_id = LOG_CURRENT_MODULE_ID()
		};
		log_hexdump(str, data, len, src_level);
	}
}

int lwm2m_os_nrf_modem_init(void)
{
	int err;

	err = nrf_modem_lib_init(NORMAL_MODE);

	switch (err) {
	case MODEM_DFU_RESULT_OK:
		lwm2m_os_log(LOG_LEVEL_INF, "Modem firmware update successful.");
		lwm2m_os_log(LOG_LEVEL_INF, "Modem will run the new firmware after reboot.");
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		lwm2m_os_log(LOG_LEVEL_ERR, "Modem firmware update failed.");
		lwm2m_os_log(LOG_LEVEL_ERR, "Modem will run non-updated firmware on reboot.");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		lwm2m_os_log(LOG_LEVEL_ERR, "Modem firmware update failed.");
		lwm2m_os_log(LOG_LEVEL_ERR, "Fatal error.");
		break;
	case -1:
		lwm2m_os_log(LOG_LEVEL_ERR, "Could not initialize modem library.");
		lwm2m_os_log(LOG_LEVEL_ERR, "Fatal error.");
		break;
	default:
		break;
	}

	return err;
}

int lwm2m_os_nrf_modem_shutdown(void)
{
	return nrf_modem_lib_shutdown();
}

/* AT command module abstractions. */

int lwm2m_os_at_init(void)
{
	int err;

	err = at_cmd_init();
	if (err) {
		return -1;
	}

	err = at_notif_init();
	if (err) {
		return -1;
	}

	return 0;
}

int lwm2m_os_at_notif_register_handler(void *context, lwm2m_os_at_cmd_handler_t handler)
{
	return at_notif_register_handler(context, handler);
}

int lwm2m_os_at_cmd_write(const char *const cmd, char *buf, size_t buf_len)
{
	return at_cmd_write(cmd, buf, buf_len, (enum at_cmd_state *)NULL);
}

static void at_params_list_get(struct at_param_list *dst, struct lwm2m_os_at_param_list *src)
{
	struct at_param *src_param = src->params;

	dst->param_count = src->param_count;
	for (size_t i = 0; i < src->param_count; i++) {
		dst->params[i].size = src_param[i].size;
		dst->params[i].type = src_param[i].type;
		switch (src_param[i].type) {
		case AT_PARAM_TYPE_INVALID:
		case AT_PARAM_TYPE_EMPTY:
			break;
		case AT_PARAM_TYPE_NUM_INT:
			dst->params[i].value.int_val = src_param[i].value.int_val;
			break;
		case AT_PARAM_TYPE_STRING:
			dst->params[i].value.str_val = src_param[i].value.str_val;
			break;
		case AT_PARAM_TYPE_ARRAY:
			dst->params[i].value.array_val = src_param[i].value.array_val;
			break;
		default:
			break;
		}
	}
}

static void at_params_list_translate(struct lwm2m_os_at_param_list *dst, struct at_param_list *src)
{
	struct at_param *dst_param = dst->params;

	dst->param_count = src->param_count;
	for (size_t i = 0; i < src->param_count; i++) {
		dst_param[i].size = src->params[i].size;
		dst_param[i].type = src->params[i].type;
		switch (src->params[i].type) {
		case AT_PARAM_TYPE_INVALID:
		case AT_PARAM_TYPE_EMPTY:
			break;
		case AT_PARAM_TYPE_NUM_INT:
			dst_param[i].value.int_val = src->params[i].value.int_val;
			break;
		case AT_PARAM_TYPE_STRING:
			dst_param[i].value.str_val = src->params[i].value.str_val;
			/* Detach this pointer from the source list
			 * so that it's not freed when we free at_param_list.
			 */
			src->params[i].value.str_val = NULL;
			break;
		case AT_PARAM_TYPE_ARRAY:
			/* Detach this pointer from the source list
			 * so that it's not freed when we free at_param_list.
			 */
			dst_param[i].value.array_val = src->params[i].value.array_val;
			src->params[i].value.array_val = NULL;
			break;
		default:
			break;
		}
	}
}

int lwm2m_os_at_params_list_init(struct lwm2m_os_at_param_list *list, size_t max_params_count)
{
	if (list == NULL) {
		return -EINVAL;
	}

	/* Array initialized with empty parameters. */
	list->params = k_calloc(max_params_count, sizeof(struct at_param));
	if (list->params == NULL) {
		return -ENOMEM;
	}

	list->param_count = max_params_count;

	return 0;
}

void lwm2m_os_at_params_list_free(struct lwm2m_os_at_param_list *list)
{
	struct at_param_list tmp_list = {
		.param_count = list->param_count,
		.params = (struct at_param *)list->params,
	};

	at_params_list_free(&tmp_list);
}

int lwm2m_os_at_params_int_get(struct lwm2m_os_at_param_list *list, size_t index, uint32_t *value)
{
	int err;
	struct at_param_list tmp_list;

	err = at_params_list_init(&tmp_list, list->param_count);
	if (err != 0) {
		return err;
	}

	at_params_list_get(&tmp_list, list);
	err = at_params_int_get(&tmp_list, index, value);
	at_params_list_translate(list, &tmp_list);

	at_params_list_free(&tmp_list);

	return err;
}

int lwm2m_os_at_params_short_get(struct lwm2m_os_at_param_list *list, size_t index, uint16_t *value)
{
	int err;
	struct at_param_list tmp_list;

	err = at_params_list_init(&tmp_list, list->param_count);
	if (err != 0) {
		return err;
	}

	at_params_list_get(&tmp_list, list);
	err = at_params_short_get(&tmp_list, index, value);
	at_params_list_translate(list, &tmp_list);

	at_params_list_free(&tmp_list);

	return err;
}

int lwm2m_os_at_params_string_get(struct lwm2m_os_at_param_list *list, size_t index, char *value,
				  size_t *len)
{
	int err;
	struct at_param_list tmp_list;

	err = at_params_list_init(&tmp_list, list->param_count);
	if (err != 0) {
		return err;
	}

	at_params_list_get(&tmp_list, list);
	err = at_params_string_get(&tmp_list, index, value, len);
	at_params_list_translate(list, &tmp_list);

	at_params_list_free(&tmp_list);

	return err;
}

int lwm2m_os_at_parser_params_from_str(const char *at_params_str, char **next_param_str,
				       struct lwm2m_os_at_param_list *const list)
{
	int err;
	struct at_param_list tmp_list;

	err = at_params_list_init(&tmp_list, list->param_count);
	if (err != 0) {
		return err;
	}

	err = at_parser_params_from_str(at_params_str, next_param_str, &tmp_list);

	at_params_list_translate(list, &tmp_list);

	at_params_list_free(&tmp_list);

	return err;
}

int lwm2m_os_at_params_valid_count_get(struct lwm2m_os_at_param_list *list)
{
	int err;
	struct at_param_list tmp_list;

	err = at_params_list_init(&tmp_list, list->param_count);
	if (err != 0) {
		return err;
	}

	at_params_list_get(&tmp_list, list);
	err = at_params_valid_count_get(&tmp_list);
	at_params_list_translate(list, &tmp_list);

	at_params_list_free(&tmp_list);

	return err;
}

/* SMS subscriber module abstraction.*/

/**@brief Forward declaration */
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
		lwm2m_os_log(LOG_LEVEL_ERR, "Unable to register as SMS listener");
		return -1;
	}
	lwm2m_os_log(LOG_LEVEL_INF, "Registered as SMS listener");
	return ret;
}

void lwm2m_os_sms_client_deregister(int handle)
{
	sms_unregister_listener(handle);
	lwm2m_os_log(LOG_LEVEL_INF, "Deregistered as SMS listener");
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

	return lte_lc_connect();
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

int lwm2m_os_pdn_activate(uint8_t cid, int *esm)
{
	return pdn_activate(cid, esm, NULL);
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
int lwm2m_os_errno(void)
{
	/* nrf_errno have the same values as newlibc errno */
	return errno;
}

const char *lwm2m_os_strerror(void)
{
	return strerror(errno);
}

int lwm2m_os_sec_ca_chain_exists(uint32_t sec_tag, bool *exists, uint8_t *flags)
{
	return modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, exists);
}

int lwm2m_os_sec_ca_chain_cmp(uint32_t sec_tag, const void *buf, size_t len)
{
	return modem_key_mgmt_cmp(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, buf, len);
}

int lwm2m_os_sec_ca_chain_write(uint32_t sec_tag, const void *buf, size_t len)
{
	return modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, buf, len);
}

int lwm2m_os_sec_psk_exists(uint32_t sec_tag, bool *exists, uint8_t *perm_flags)
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

int lwm2m_os_sec_identity_exists(uint32_t sec_tag, bool *exists, uint8_t *perm_flags)
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
