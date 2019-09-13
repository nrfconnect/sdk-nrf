/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <lwm2m_os.h>

#include <stdlib.h>
#include <zephyr.h>
#include <nvs/nvs.h>
#include <logging/log.h>
#include <secure_services.h>
#include <misc/util.h>

/* NVS-related defines */

/* Multiple of FLASH_PAGE_SIZE */
#define NVS_SECTOR_SIZE    DT_FLASH_ERASE_BLOCK_SIZE
/* At least 2 sectors */
#define NVS_SECTOR_COUNT   3
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

int lwm2m_os_sleep(int ms)
{
	return k_sleep(K_MSEC(ms));
}

/* OS functions */

void lwm2m_os_sys_reset(void)
{
	spm_request_system_reboot();
	CODE_UNREACHABLE;
}

uint32_t lwm2m_os_rand_get(void)
{
	return sys_rand32_get();
}

/* Non volatile storage */

int lwm2m_os_storage_delete(uint16_t id)
{
	return nvs_delete(&fs, id);
}

int lwm2m_os_storage_read(uint16_t id, void *data, size_t len)
{
	return nvs_read(&fs, id, data, len);
}

int lwm2m_os_storage_write(uint16_t id, const void *data, size_t len)
{
	return nvs_write(&fs, id, data, len);
}

/* LWM2M timer, built on top of delayed works. */

struct lwm2m_work {
	struct k_delayed_work work_item;
	lwm2m_os_timer_handler_t handler;
};

static struct lwm2m_work lwm2m_works[LWM2M_OS_MAX_TIMER_COUNT];

static void work_handler(struct k_work *work)
{
	struct lwm2m_work *lwm2m_work =
		CONTAINER_OF(work, struct lwm2m_work, work_item);

	lwm2m_work->handler(lwm2m_work);
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

	/* Zephyr's timing subsystem uses signed integers so the
	 * largest tick count that can be represented is 31 bit large.
	 *
	 * max_timeout_sec = int max - 1 / ticks per sec
	 */
	const int max_timeout_sec =
		(INT32_MAX - 1) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	/* Avoid requesting timeouts larger than max_timeout_sec,
	 * or they will expire immediately.
	 * See: https://github.com/zephyrproject-rtos/zephyr/issues/19075
	 *
	 * `timeout` is expressed in milliseconds,
	 * use K_SECONDS to specify the correct unit.
	 */
	timeout = MIN(K_SECONDS(max_timeout_sec), timeout);

	/* FIXME: return an error if the timeout requested is too large. */
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

	return k_delayed_work_remaining_get(&work->work_item);
}

/* LWM2M logs. */

LOG_MODULE_REGISTER(lwm2m, CONFIG_LOG_DEFAULT_LEVEL);

static const u8_t log_level_lut[] = {
	LOG_LEVEL_NONE, /* LWM2M_LOG_LEVEL_NONE */
	LOG_LEVEL_ERR,  /* LWM2M_LOG_LEVEL_ERR */
	LOG_LEVEL_WRN,  /* LWM2M_LOG_LEVEL_WRN */
	LOG_LEVEL_INF,  /* LWM2M_LOG_LEVEL_INF */
	LOG_LEVEL_DBG,  /* LWM2M_LOG_LEVEL_TRC */
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
