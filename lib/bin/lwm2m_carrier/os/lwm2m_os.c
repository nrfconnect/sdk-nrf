/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <lwm2m_os.h>

#include <stdlib.h>
#include <stdbool.h>
#include <zephyr.h>
#include <string.h>
#include <modem/at_cmd.h>
#include <at_notif.h>
#include <at_cmd_parser/at_cmd_parser.h>
#include <at_cmd_parser/at_params.h>
#include <bsd.h>
#include <lte_lc.h>
#include <net/bsdlib.h>
#include <net/download_client.h>
#include <power/reboot.h>
#include <sys/util.h>
#include <toolchain.h>
#include <fs/nvs.h>
#include <logging/log.h>
#include <errno.h>
#include <nrf_errno.h>
#include <modem_key_mgmt.h>

/* NVS-related defines */

/* Multiple of FLASH_PAGE_SIZE */
#define NVS_SECTOR_SIZE DT_FLASH_ERASE_BLOCK_SIZE
/* At least 2 sectors */
#define NVS_SECTOR_COUNT 3
/* Start address of the filesystem in flash */
#define NVS_STORAGE_OFFSET DT_FLASH_AREA_STORAGE_OFFSET

static struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
};

int lwm2m_os_init(void)
{
	/* Initialize storage */
	return nvs_init(&fs, DT_FLASH_DEV_NAME);
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
	struct k_delayed_work work_item;
	lwm2m_os_timer_handler_t handler;
	int32_t remaining_timeout_ms;
};

static struct lwm2m_work lwm2m_works[LWM2M_OS_MAX_TIMER_COUNT];

static int32_t get_timeout_value(int32_t timeout,
				 struct lwm2m_work *lwm2m_work)
{
	/* Zephyr's timing subsystem uses positive integers so the
	 * largest tick count that can be represented is 31 bit large.
	 *
	 * max_timeout_ms = (int max - 1 / ticks per sec) * 1000
	 */
	static const int32_t max_timeout_ms =
		K_SECONDS((INT32_MAX - 1) / CONFIG_SYS_CLOCK_TICKS_PER_SEC);

	/* Avoid requesting timeouts larger than max_timeout_ms,
	 * or they will expire immediately.
	 * See: https://github.com/zephyrproject-rtos/zephyr/issues/19075
	 */
	if (timeout > max_timeout_ms) {
		lwm2m_work->remaining_timeout_ms = timeout - max_timeout_ms;
		timeout = max_timeout_ms;
	} else {
		lwm2m_work->remaining_timeout_ms = 0;
	}

	return timeout;
}

static void work_handler(struct k_work *work)
{
	struct lwm2m_work *lwm2m_work =
		CONTAINER_OF(work, struct lwm2m_work, work_item);

	if (lwm2m_work->remaining_timeout_ms > 0) {
		int32_t timeout = lwm2m_work->remaining_timeout_ms;

		timeout = get_timeout_value(timeout, lwm2m_work);

		/* FIXME: handle error return from k_delayed_work_submit(). */
		(void)k_delayed_work_submit(&lwm2m_work->work_item,
					    K_MSEC(timeout));
	} else {
		lwm2m_work->handler(lwm2m_work);
	}
}

void *lwm2m_os_timer_get(lwm2m_os_timer_handler_t handler)
{
	struct lwm2m_work *work = NULL;

	u32_t key = irq_lock();

	/* Find free delayed work */
	for (int i = 0; i < ARRAY_SIZE(lwm2m_works); i++) {
		if (lwm2m_works[i].handler == NULL) {
			work = &lwm2m_works[i];
			work->handler = handler;
			break;
		}
	}

	irq_unlock(key);

	if (work != NULL) {
		k_delayed_work_init(&work->work_item, work_handler);
	}

	return work;
}

void lwm2m_os_timer_release(void *timer)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return;
	}

	work->handler = NULL;
}

int lwm2m_os_timer_start(void *timer, int32_t timeout)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return -EINVAL;
	}

	timeout = get_timeout_value(timeout, work);

	return k_delayed_work_submit(&work->work_item, K_MSEC(timeout));
}

void lwm2m_os_timer_cancel(void *timer)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return;
	}

	k_delayed_work_cancel(&work->work_item);
}

int32_t lwm2m_os_timer_remaining(void *timer)
{
	struct lwm2m_work *work = (struct lwm2m_work *)timer;

	if (!PART_OF_ARRAY(lwm2m_works, work)) {
		return 0;
	}

	return k_delayed_work_remaining_get(&work->work_item) +
	       work->remaining_timeout_ms;
}

/* LWM2M logs. */

LOG_MODULE_REGISTER(lwm2m, CONFIG_LOG_DEFAULT_LEVEL);

static const u8_t log_level_lut[] = {
	LOG_LEVEL_NONE, /* LWM2M_LOG_LEVEL_NONE */
	LOG_LEVEL_ERR, /* LWM2M_LOG_LEVEL_ERR */
	LOG_LEVEL_WRN, /* LWM2M_LOG_LEVEL_WRN */
	LOG_LEVEL_INF, /* LWM2M_LOG_LEVEL_INF */
	LOG_LEVEL_DBG, /* LWM2M_LOG_LEVEL_TRC */
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
		log_generic(src_level, fmt, ap);
		va_end(ap);
	}
}

int lwm2m_os_bsdlib_init(void)
{
	int err;

	err = bsdlib_init();

	switch (err) {
	case MODEM_DFU_RESULT_OK:
		lwm2m_os_log(LOG_LEVEL_INF,
			     "Modem firmware update successful.");
		lwm2m_os_log(LOG_LEVEL_INF,
			     "Modem will run the new firmware after reboot.");
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		lwm2m_os_log(LOG_LEVEL_ERR, "Modem firmware update failed.");
		lwm2m_os_log(LOG_LEVEL_ERR,
			     "Modem will run non-updated firmware on reboot.");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		lwm2m_os_log(LOG_LEVEL_ERR, "Modem firmware update failed.");
		lwm2m_os_log(LOG_LEVEL_ERR, "Fatal error.");
		break;
	case -1:
		lwm2m_os_log(LOG_LEVEL_ERR, "Could not initialize bsdlib.");
		lwm2m_os_log(LOG_LEVEL_ERR, "Fatal error.");
		break;
	default:
		break;
	}

	return err;
}

int lwm2m_os_bsdlib_shutdown(void)
{
	return bsdlib_shutdown();
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

int lwm2m_os_at_notif_register_handler(void *context,
				       lwm2m_os_at_cmd_handler_t handler)
{
	return at_notif_register_handler(context, handler);
}

int lwm2m_os_at_cmd_write(const char *const cmd, char *buf, size_t buf_len)
{
	return at_cmd_write(cmd, buf, buf_len, (enum at_cmd_state *)NULL);
}

static void at_params_list_get(struct at_param_list *dst,
			struct lwm2m_os_at_param_list *src)
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
		case AT_PARAM_TYPE_NUM_SHORT:
		case AT_PARAM_TYPE_NUM_INT:
			dst->params[i].value.int_val =
				src_param[i].value.int_val;
			break;
		case AT_PARAM_TYPE_STRING:
			dst->params[i].value.str_val =
				src_param[i].value.str_val;
			break;
		case AT_PARAM_TYPE_ARRAY:
			dst->params[i].value.array_val =
				src_param[i].value.array_val;
			break;
		default:
			break;
		}
	}
}

static void at_params_list_translate(struct lwm2m_os_at_param_list *dst,
			struct at_param_list *src)
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
		case AT_PARAM_TYPE_NUM_SHORT:
		case AT_PARAM_TYPE_NUM_INT:
			dst_param[i].value.int_val =
				src->params[i].value.int_val;
			break;
		case AT_PARAM_TYPE_STRING:
			dst_param[i].value.str_val =
				src->params[i].value.str_val;
			/* Detach this pointer from the source list
			 * so that it's not freed when we free at_param_list.
			 */
			src->params[i].value.str_val = NULL;
			break;
		case AT_PARAM_TYPE_ARRAY:
			/* Detach this pointer from the source list
			 * so that it's not freed when we free at_param_list.
			 */
			dst_param[i].value.array_val =
				src->params[i].value.array_val;
			src->params[i].value.array_val = NULL;
			break;
		default:
			break;
		}
	}
}

int lwm2m_os_at_params_list_init(struct lwm2m_os_at_param_list *list,
				 size_t max_params_count)
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

int lwm2m_os_at_params_int_get(struct lwm2m_os_at_param_list *list,
			       size_t index, uint32_t *value)
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

int lwm2m_os_at_params_short_get(struct lwm2m_os_at_param_list *list,
				 size_t index, uint16_t *value)
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

int lwm2m_os_at_params_string_get(struct lwm2m_os_at_param_list *list,
				  size_t index, char *value, size_t *len)
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

int lwm2m_os_at_parser_params_from_str(
	const char *at_params_str, char **next_param_str,
	struct lwm2m_os_at_param_list *const list)
{
	int err;
	struct at_param_list tmp_list;

	err = at_params_list_init(&tmp_list, list->param_count);
	if (err != 0) {
		return err;
	}

	err = at_parser_params_from_str(at_params_str, next_param_str,
					&tmp_list);

	at_params_list_translate(list, &tmp_list);

	at_params_list_free(&tmp_list);

	return err;
}

int lwm2m_os_at_params_valid_count_get(
	struct lwm2m_os_at_param_list *list)
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

/* Download client module abstractions. */

static struct download_client http_downloader;
static lwm2m_os_download_callback_t lwm2m_os_lib_callback;

int lwm2m_os_download_connect(const char *host,
			      const struct lwm2m_os_download_cfg *cfg)
{
	struct download_client_cfg config = {
		.sec_tag = cfg->sec_tag,
		.apn = cfg->apn,
	};

	return download_client_connect(&http_downloader, host, &config);
}

int lwm2m_os_download_disconnect(void)
{
	return download_client_disconnect(&http_downloader);
}

static void
download_client_evt_translate(const struct download_client_evt *event,
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

/* LTE LC module abstractions. */

int lwm2m_os_lte_link_up(void)
{
	return lte_lc_init_and_connect();
}

int lwm2m_os_lte_link_down(void)
{
	return lte_lc_offline();
}

int lwm2m_os_lte_power_down(void)
{
	return lte_lc_power_off();
}

#ifndef ENOKEY
#define ENOKEY 2001
#endif

#ifndef EKEYEXPIRED
#define EKEYEXPIRED 2002
#endif

#ifndef EKEYREVOKED
#define EKEYREVOKED 2003
#endif

#ifndef EKEYREJECTED
#define EKEYREJECTED 2004
#endif

/* errno handling. */
int lwm2m_os_errno(void)
{
	switch (errno) {
	case EPERM:
		return NRF_EPERM;
	case ENOENT:
		return NRF_ENOENT;
	case EIO:
		return NRF_EIO;
	case ENOEXEC:
		return NRF_ENOEXEC;
	case EBADF:
		return NRF_EBADF;
	case ENOMEM:
		return NRF_ENOMEM;
	case EACCES:
		return NRF_EACCES;
	case EFAULT:
		return NRF_EFAULT;
	case EINVAL:
		return NRF_EINVAL;
	case EMFILE:
		return NRF_EMFILE;
	case EAGAIN:
		return NRF_EAGAIN;
	case EPROTOTYPE:
		return NRF_EPROTOTYPE;
	case ENOPROTOOPT:
		return NRF_ENOPROTOOPT;
	case EPROTONOSUPPORT:
		return NRF_EPROTONOSUPPORT;
	case ESOCKTNOSUPPORT:
		return NRF_ESOCKTNOSUPPORT;
	case EOPNOTSUPP:
		return NRF_EOPNOTSUPP;
	case EAFNOSUPPORT:
		return NRF_EAFNOSUPPORT;
	case EADDRINUSE:
		return NRF_EADDRINUSE;
	case ENETDOWN:
		return NRF_ENETDOWN;
	case ENETUNREACH:
		return NRF_ENETUNREACH;
	case ENETRESET:
		return NRF_ENETRESET;
	case ECONNRESET:
		return NRF_ECONNRESET;
	case EISCONN:
		return NRF_EISCONN;
	case ENOTCONN:
		return NRF_ENOTCONN;
	case ETIMEDOUT:
		return NRF_ETIMEDOUT;
	case ENOBUFS:
		return NRF_ENOBUFS;
	case EHOSTDOWN:
		return NRF_EHOSTDOWN;
	case EINPROGRESS:
		return NRF_EINPROGRESS;
	case ECANCELED:
		return NRF_ECANCELED;
	case ENOKEY:
		return NRF_ENOKEY;
	case EKEYEXPIRED:
		return NRF_EKEYEXPIRED;
	case EKEYREVOKED:
		return NRF_EKEYREVOKED;
	case EKEYREJECTED:
		return NRF_EKEYREJECTED;
	case EMSGSIZE:
		return NRF_EMSGSIZE;
	default:
		__ASSERT(false, "Untranslated errno %d", errno);
		return 0xDEADBEEF;
	}
}

const char *lwm2m_os_strerror(void)
{
	return strerror(errno);
}

int lwm2m_os_sec_ca_chain_exists(uint32_t sec_tag, bool *exists,
				 uint8_t *perm_flags)
{
	return modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				     exists, perm_flags);
}

int lwm2m_os_sec_ca_chain_write(uint32_t sec_tag, const void *buf, uint16_t len)
{
	return modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    buf, len);
}

int lwm2m_os_sec_psk_exists(uint32_t sec_tag, bool *exists, uint8_t *perm_flags)
{
	return modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK,
				     exists, perm_flags);
}

int lwm2m_os_sec_psk_write(uint32_t sec_tag, const void *buf, uint16_t len)
{
	return modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, buf,
				    len);
}

int lwm2m_os_sec_identity_exists(uint32_t sec_tag, bool *exists,
				 uint8_t *perm_flags)
{
	return modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY,
				     exists, perm_flags);
}

int lwm2m_os_sec_identity_write(uint32_t sec_tag, const void *buf, uint16_t len)
{
	return modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY,
				    buf, len);
}
