/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <nrf_modem.h>
#include <zephyr/drivers/flash.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>
#include <dfu/dfu_target_mcuboot.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/lwm2m.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <net/fota_download.h>
#include <lwm2m_util.h>
#include <net/lwm2m_client_utils.h>
#include <zephyr/settings/settings.h>
/* Firmware update needs access to internal functions as well */
#include <lwm2m_engine.h>

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
#include <dfu/dfu_target_full_modem.h>
#include <nrf_modem_bootloader.h>
#include <dfu/fmfu_fdev.h>
#include <string.h>
#endif

#include <modem/modem_info.h>
#include <ncs_version.h>
#include <pm_config.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_firmware, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define BYTE_PROGRESS_STEP (1024 * 10)

#define LWM2M_FIRM_PREFIX "lwm2m:fir"

#define FOTA_PULL_SUPPORTED_COUNT 4
#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
#define FOTA_INSTANCE_COUNT 2
#define ENABLED_LWM2M_FIRMWARE_OBJECT LWM2M_OBJECT_ADV_FIRMWARE_ID
#else
#define FOTA_INSTANCE_COUNT 1
#define ENABLED_LWM2M_FIRMWARE_OBJECT LWM2M_OBJECT_FIRMWARE_ID
static struct lwm2m_engine_res_inst pull_protocol_buff[FOTA_PULL_SUPPORTED_COUNT];
#endif
/* FOTA resource LWM2M_FOTA_UPDATE_PROTO_SUPPORT_ID  definition */
#define FOTA_PULL_COAP 0
#define FOTA_PULL_COAPS 1
/* Only supported when Server root certicate SEC tag is available*/
#define FOTA_PULL_HTTP 2
#define FOTA_PULL_HTTPS 3

static lwm2m_firmware_event_cb_t firmware_fota_event_cb;
static uint8_t firmware_buf[CONFIG_LWM2M_COAP_BLOCK_SIZE];

/* Supported Protocols */
static uint8_t pull_protocol_support[FOTA_PULL_SUPPORTED_COUNT] = { FOTA_PULL_COAP, FOTA_PULL_HTTP,
								    FOTA_PULL_COAPS,
								    FOTA_PULL_HTTPS };

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
static uint8_t apply_fmfu_from_ext_flash(void);
static int full_modem_stream_params_init(void);
static int configure_full_modem_update(void);
static uint8_t fmfu_buf[1024];
static const struct device *flash_dev = DEVICE_DT_GET_ONE(jedec_spi_nor);
#endif

#ifdef CONFIG_DFU_TARGET_MCUBOOT
static uint8_t mcuboot_buf[CONFIG_LWM2M_CLIENT_UTILS_MCUBOOT_FLASH_BUF_SIZE] __aligned(4);
#endif
#define UNUSED_OBJ_ID 0xffff
#define PENDING_DELAY K_MSEC(10)
static uint16_t ongoing_obj_id;
static char *fota_path;
static char *fota_host;
static int fota_sec_tag;
static uint8_t percent_downloaded;
static uint32_t bytes_downloaded;
static int application_obj_id;
static int modem_obj_id;
static int target_image_type[FOTA_INSTANCE_COUNT];

static void dfu_target_cb(enum dfu_target_evt_id evt);
static void start_fota_download(struct k_work *work);
static void start_pending_fota_download(struct k_work *work);
static void firmware_update_check_linked_instances(int instance_id);
static int target_image_type_get(uint16_t instance);
static uint8_t get_state(uint16_t id);
static void set_result(uint16_t id, uint8_t result);
static K_WORK_DEFINE(download_work, start_fota_download);
static K_WORK_DELAYABLE_DEFINE(pending_download_work, start_pending_fota_download);
static struct update_data {
	struct k_work_delayable work;
	enum {APP, MODEM_DELTA, MODEM_FULL} type;
	uint16_t object_instance;
	bool update_accepted;
} update_data;

void client_acknowledge(void);

NRF_MODEM_LIB_ON_INIT(lwm2m_firmware_init_hook,
		      on_modem_lib_init, NULL);

/* Initialized to value different than success (0) */
static int modem_lib_init_result = -1;

#ifdef CONFIG_ZTEST
/* Only for unit test for emulate modem lib init hook */
void lwm2m_firmware_emulate_modem_lib_init(int modem_init_ret_val)
{
	modem_lib_init_result = modem_init_ret_val;
}
#endif

static int fota_event_download_start(uint16_t obj_inst_id)
{
	struct lwm2m_fota_event event;

	if (!firmware_fota_event_cb) {
		return 0;
	}

	event.id = LWM2M_FOTA_DOWNLOAD_START;
	event.download_start.obj_inst_id = obj_inst_id;
	return firmware_fota_event_cb(&event);
}

static int fota_event_download_ready(uint16_t obj_inst_id)
{
	struct lwm2m_fota_event event;

	if (!firmware_fota_event_cb) {
		return 0;
	}

	event.id = LWM2M_FOTA_DOWNLOAD_FINISHED;
	event.download_ready.obj_inst_id = obj_inst_id;
	event.download_ready.dfu_type = target_image_type_get(obj_inst_id);
	return firmware_fota_event_cb(&event);
}

static int fota_event_update_request(uint16_t obj_inst_id)
{
	struct lwm2m_fota_event event;

	if (!firmware_fota_event_cb) {
		return 0;
	}

	event.id = LWM2M_FOTA_UPDATE_IMAGE_REQ;
	event.update_req.obj_inst_id = obj_inst_id;
	event.update_req.dfu_type = target_image_type_get(obj_inst_id);
	return firmware_fota_event_cb(&event);
}

static int fota_event_update_failure(uint16_t obj_inst_id, uint8_t failure_code)
{
	struct lwm2m_fota_event event;

	if (!firmware_fota_event_cb) {
		return 0;
	}

	event.id = LWM2M_FOTA_UPDATE_ERROR;
	event.failure.obj_inst_id = obj_inst_id;
	event.failure.update_failure = failure_code;
	return firmware_fota_event_cb(&event);
}

static int firmware_request_linked_update(int instance_id)
{
	int postpone_update = 0;
#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
	struct lwm2m_obj_path path;
	struct lwm2m_objlnk object_link;

	path = LWM2M_OBJ(ENABLED_LWM2M_FIRMWARE_OBJECT, instance_id,
			 LWM2M_ADV_FOTA_LINKED_INSTANCES_ID);

	if (lwm2m_get_objlnk(&path, &object_link)) {
		return 0;
	}

	if (object_link.obj_id != LWM2M_OBJECT_ADV_FIRMWARE_ID) {
		return 0;
	}

	if (get_state(object_link.obj_inst) != STATE_DOWNLOADED) {
		return 0;
	}

	postpone_update = fota_event_update_request(object_link.obj_inst);

	if (postpone_update < 0) {
		uint16_t temp_ongoing_id = ongoing_obj_id;

		ongoing_obj_id = object_link.obj_inst;
		/* Application deferred update */
		set_result(object_link.obj_inst, RESULT_ADV_FOTA_DEFERRED);
		ongoing_obj_id = temp_ongoing_id;
	}

#endif
	return postpone_update;
}

static void on_modem_lib_init(int ret, void *ctx)
{
	modem_lib_init_result = ret;
}

static int *target_image_type_buffer_get(uint16_t instance)
{
	if (instance >= FOTA_INSTANCE_COUNT) {
		return NULL;
	}

	return &target_image_type[instance];
}

static int target_image_type_get(uint16_t instance)
{
	if (instance >= FOTA_INSTANCE_COUNT) {
		return DFU_TARGET_IMAGE_TYPE_NONE;
	}

	return target_image_type[instance];
}

static int set(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	uint16_t len;
	void *buf;
	struct lwm2m_obj_path path;
	int ret;

	if (!key) {
		return -ENOENT;
	}

	LOG_DBG("Loading \"%s\"", key);

	ret = lwm2m_string_to_path(key, &path, '/');
	if (ret) {
		return ret;
	}

	if (path.level == LWM2M_PATH_LEVEL_RESOURCE) {
		if (lwm2m_get_res_buf(&path, &buf, &len, NULL, NULL) != 0) {
			return -ENOENT;
		}
	} else if (path.level == LWM2M_PATH_LEVEL_OBJECT_INST) {
		/* Image Type */
		buf = target_image_type_buffer_get(path.obj_inst_id);
		if (!buf) {
			return -ENOENT;
		}
		len = sizeof(int);
	} else {
		return -ENOENT;
	}

	len = read_cb(cb_arg, buf, len);
	if (len <= 0) {
		LOG_ERR("Failed to read data");
		return -ENOENT;
	}

	if (path.level == LWM2M_PATH_LEVEL_RESOURCE) {
		lwm2m_set_res_data_len(&path, len);
	}

	return 0;
}

static struct settings_handler lwm2m_firm_settings = {
	.name = LWM2M_FIRM_PREFIX,
	.h_set = set,
};

static int write_resource_to_settings(int inst, int res, uint8_t *data, uint16_t data_len)
{
	char path[sizeof(LWM2M_FIRM_PREFIX "/65535/0/10")];

	if ((inst < 0 || inst > 9) || (res < 0 || res > 99)) {
		return -EINVAL;
	}

	snprintk(path, sizeof(path), LWM2M_FIRM_PREFIX "/%d/%d/%d", ENABLED_LWM2M_FIRMWARE_OBJECT,
		 inst, res);
	if (settings_save_one(path, data, data_len)) {
		LOG_ERR("Failed to store %s", path);
	}
	LOG_DBG("Permanently stored %s", path);
	return 0;
}

static int write_image_type_to_settings(int inst, int img_type)
{
	char path[sizeof(LWM2M_FIRM_PREFIX "/65632/0")];

	snprintk(path, sizeof(path), LWM2M_FIRM_PREFIX "/%d/%d", ENABLED_LWM2M_FIRMWARE_OBJECT,
		 inst);
	if (settings_save_one(path, &img_type, sizeof(int))) {
		LOG_ERR("Failed to store %s", path);
	}
	LOG_DBG("Permanently stored %s", path);
	return 0;
}

static void target_image_type_store(uint16_t instance, int img_type)
{
	if (instance >= FOTA_INSTANCE_COUNT) {
		LOG_WRN("Image type storage failure");
		return;
	}
	target_image_type[instance] = img_type;
	write_image_type_to_settings(instance, img_type);
}

static void reboot_work_handler(void)
{
	if (!IS_ENABLED(CONFIG_ZTEST)) {
		/* Call reboot execute */
		struct lwm2m_engine_res *dev_res;
		struct lwm2m_obj_path path;
		uint8_t reboot_src = REBOOT_SOURCE_FOTA_OBJ;

		path.level = LWM2M_PATH_LEVEL_RESOURCE;
		path.obj_id = LWM2M_OBJECT_DEVICE_ID;
		path.obj_inst_id = 0;
		path.res_id = 4;
		dev_res = lwm2m_engine_get_res(&path);

		if (dev_res && dev_res->execute_cb) {
			dev_res->execute_cb(0, &reboot_src, 1);
		} else {
			LOG_PANIC();
			sys_reboot(SYS_REBOOT_COLD);
			LOG_WRN("Rebooting");
		}
	}
}
/************** Wrappers between normal FOTA object and Advanced FOTA object ********/
static uint8_t get_state(uint16_t id)
{
	if (id >= FOTA_INSTANCE_COUNT) {
		LOG_WRN("Get state blocked by id, state");
		return 0;
	}
	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
		return lwm2m_adv_firmware_get_update_state(id);
	}
#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
	return lwm2m_firmware_get_update_state_inst(id);
#else
	return 0;
#endif
}

static void set_state(uint16_t id, uint8_t state)
{
	if (id >= FOTA_INSTANCE_COUNT) {
		LOG_WRN("Set state blocked by id, state %d", state);
		return;
	}
	lwm2m_registry_lock();
	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
		lwm2m_adv_firmware_set_update_state(id, state);
	} else {
#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
		lwm2m_firmware_set_update_state_inst(id, state);
#endif
	}
	lwm2m_registry_unlock();
}

static void set_result(uint16_t id, uint8_t result)
{
	if (id >= FOTA_INSTANCE_COUNT) {
		LOG_WRN("Set result blocked by id,  result %d", result);
		return;
	}
	lwm2m_registry_lock();
	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
		lwm2m_adv_firmware_set_update_result(id, result);
	} else {
#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
		lwm2m_firmware_set_update_result_inst(id, result);
#endif
	}
	lwm2m_registry_unlock();
}

static int firmware_target_prepare(int dfu_image_type)
{
	int ret;

	switch (dfu_image_type) {
	case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
		/* Set the required buffer for MCUboot targets */
		ret = dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
		break;

	case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
		ret = 0;
		break;
#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	case DFU_TARGET_IMAGE_TYPE_FULL_MODEM:
		ret = configure_full_modem_update();
		break;
#endif
	default:
		LOG_ERR("Unsupported Image type %d", dfu_image_type);
		ret = -EACCES;
		break;
	}
	return ret;
}

static int firmware_target_pre_init(int dfu_image_type)
{
	int ret;

	ret = firmware_target_prepare(dfu_image_type);
	if (ret) {
		return ret;
	}

	return dfu_target_init(dfu_image_type, 0, 2048, dfu_target_cb);
}

static int firmware_target_reset(uint16_t obj_inst_id)
{
	int ret;
	int dfu_image_type;

	dfu_image_type = target_image_type_get(obj_inst_id);
	ret = firmware_target_pre_init(dfu_image_type);
	if (ret) {
		return ret;
	}

	return dfu_target_reset();
}

static int set_firmware_update_type(int dfu_image_type)
{
	int ret = 0;

	switch (dfu_image_type) {
	case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
		update_data.type = APP;
		break;

	case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
		update_data.type = MODEM_DELTA;
		break;
#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	case DFU_TARGET_IMAGE_TYPE_FULL_MODEM:
		update_data.type = MODEM_FULL;
		break;
#endif
	default:
		ret = -EACCES;
		break;
	}
	return ret;
}

static int firmware_target_schedule_update(int obj_inst_id, int dfu_image_type)
{
	int ret;

	if (IS_ENABLED(CONFIG_FOTA_CLIENT_AUTOSCHEDULE_UPDATE)) {
		/* target is already sheduled */
		return 0;
	}

	ret = firmware_target_pre_init(dfu_image_type);
	if (ret) {
		return ret;
	}
	return dfu_target_schedule_update(0);
}

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
static uint8_t apply_fmfu_from_ext_flash(void)
{
	int ret;

	LOG_INF("Applying full modem firmware update from external flash\n");

	ret = nrf_modem_lib_shutdown();
	if (ret != 0) {
		LOG_ERR("nrf_modem_lib_shutdown() failed: %d\n", ret);
		return RESULT_UPDATE_FAILED;
	}

	ret = nrf_modem_lib_bootloader_init();
	if (ret != 0) {
		LOG_ERR("nrf_modem_lib_bootloader_init() failed: %d\n", ret);
		return RESULT_UPDATE_FAILED;
	}

	ret = fmfu_fdev_load(fmfu_buf, sizeof(fmfu_buf), flash_dev, 0);
	if (ret != 0) {
		LOG_ERR("fmfu_fdev_load failed: %d\n", ret);
		return RESULT_UPDATE_FAILED;
	}
	LOG_INF("Modem firmware update completed\n");
	return RESULT_SUCCESS;
}

static int full_modem_stream_params_init(void)
{
	int ret = 0;

	const struct dfu_target_full_modem_params params = {
		.buf = fmfu_buf,
		.len = sizeof(fmfu_buf),
		.dev = &(struct dfu_target_fmfu_fdev){ .dev = flash_dev, .offset = 0, .size = 0 }
	};

	ret = dfu_target_full_modem_cfg(&params);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("dfu_target_full_modem_cfg failed: %d\n", ret);
	} else {
		ret = 0;
	}

	return ret;
}

static int configure_full_modem_update(void)
{
	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Flash device not ready: %s\n", flash_dev->name);
	}

	return full_modem_stream_params_init();
}
#endif

static uint8_t apply_firmware_delta_modem_update(void)
{
	int ret;
	uint8_t result;

	lte_lc_deinit();
	nrf_modem_lib_shutdown();
	ret = nrf_modem_lib_init();

	ret = modem_lib_init_result;
	switch (ret) {
	case NRF_MODEM_DFU_RESULT_OK:
		LOG_INF("MODEM UPDATE OK. Will run new firmware");
		result = RESULT_SUCCESS;
		break;
	default:
		LOG_INF("MODEM UPDATE fail %d", ret);
		result = RESULT_UPDATE_FAILED;
		break;
	}

	return result;
}

static void update_work_handler(struct k_work *work)
{
	int postpone_update;
	uint8_t result;
	uint16_t instance_id = update_data.object_instance;

	if (instance_id == UNUSED_OBJ_ID) {
		return;
	}

	if (!update_data.update_accepted) {

		postpone_update = fota_event_update_request(instance_id);
		if (postpone_update > 0) {
			LOG_INF("Delay %d s Update instance %d ", postpone_update, instance_id);
			k_work_schedule(&update_data.work, K_SECONDS(postpone_update));
			return;
		} else if (postpone_update < 0) {
			/* Application deferred update */
			set_result(instance_id, RESULT_ADV_FOTA_DEFERRED);
			return;
		}

		/* Poll also possible linked */
		postpone_update = firmware_request_linked_update(instance_id);

		if (postpone_update > 0) {
			k_work_schedule(&update_data.work, K_SECONDS(postpone_update));
			return;
		}
	}

	lwm2m_registry_lock();
	if (update_data.type == MODEM_DELTA) {
		result = apply_firmware_delta_modem_update();
		if (result == RESULT_SUCCESS) {
			ongoing_obj_id = UNUSED_OBJ_ID;
		}
		set_result(instance_id, result);
	} else if (update_data.type == MODEM_FULL) {
#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
		result = apply_fmfu_from_ext_flash();
		if (result == RESULT_SUCCESS) {
			ongoing_obj_id = UNUSED_OBJ_ID;
		}
		/* Set result */
		set_result(instance_id, result);
#endif
	}

	firmware_update_check_linked_instances(instance_id);
	lwm2m_registry_unlock();
	reboot_work_handler();
}

static int firmware_instance_schedule(uint16_t obj_inst_id)
{
	int dfu_image_type;

	dfu_image_type = target_image_type_get(obj_inst_id);

	if (set_firmware_update_type(dfu_image_type)) {
		LOG_ERR("Update for image type not supported %d", dfu_image_type);
		return -EACCES;
	}

	if (firmware_target_schedule_update(obj_inst_id, dfu_image_type)) {
		LOG_ERR("DFU shedule fail");
		return -EACCES;
	}

	return 0;
}

static int firmware_update_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	int postpone_update;
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	if (update_data.object_instance != UNUSED_OBJ_ID) {
		if (update_data.object_instance == obj_inst_id) {
			return 0;
		} else {
			return -EPERM;
		}
	}

	if (firmware_instance_schedule(obj_inst_id)) {
		return -EACCES;
	}

	update_data.object_instance = obj_inst_id;

	postpone_update = fota_event_update_request(obj_inst_id);
	if (postpone_update > 0) {
		LOG_INF("Delay %d s Update instance %d ", postpone_update, obj_inst_id);
		update_data.update_accepted = false;
		k_work_schedule(&update_data.work, K_SECONDS(postpone_update));
		return 0;
	} else if (postpone_update < 0) {
		/* Application deferred update */
		return -EPERM;
	}

	/* Poll also possible linked */
	postpone_update = firmware_request_linked_update(obj_inst_id);

	if (postpone_update > 0) {
		k_work_schedule(&update_data.work, K_SECONDS(postpone_update));
		return 0;
	}

	k_work_schedule(&update_data.work, K_SECONDS(5));
	update_data.update_accepted = true;

	return 0;
}

static void *firmware_get_buf(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			      size_t *data_len)
{
	*data_len = sizeof(firmware_buf);
	return firmware_buf;
}

static int firmware_update_result(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				  uint8_t *data, uint16_t data_len, bool last_block,
				  size_t total_size)
{
	/* Store state to pernament memory */
	write_resource_to_settings(obj_inst_id, res_id, data, data_len);

	if (*data != RESULT_SUCCESS && *data != RESULT_DEFAULT) {
		fota_event_update_failure(obj_inst_id, *data);
	}
	return 0;
}

static void init_firmware_variables(void)
{
	int ret;

	if (fota_host) {
		k_free(fota_host);
		fota_host = NULL;
	}
	fota_path = NULL;
	percent_downloaded = 0;
	bytes_downloaded = 0;
	ongoing_obj_id = UNUSED_OBJ_ID;
	ret = k_work_schedule(&pending_download_work, PENDING_DELAY);
	if (ret < 0) {
		for (int i = 0; i < FOTA_INSTANCE_COUNT; i++) {
			/* Find a pending instance which is which Downloading */
			if (get_state(i) != STATE_DOWNLOADING) {
				continue;
			}
			LOG_ERR("FOTA Download start fail for %d instance", i);
			set_result(i, RESULT_UPDATE_FAILED);
		}
	}
}

static int firmware_update_state(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				 uint8_t *data, uint16_t data_len, bool last_block,
				 size_t total_size)
{
	int ret;

	/* Store state to pernament memory */
	write_resource_to_settings(obj_inst_id, res_id, data, data_len);

	if (*data == STATE_IDLE) {
		/* Cancel Only object is same than ongoing update */
		if (obj_inst_id == ongoing_obj_id) {
			ongoing_obj_id = UNUSED_OBJ_ID;
			fota_download_cancel();
			ret = firmware_target_reset(obj_inst_id);
			if (ret < 0) {
				LOG_ERR("Failed to reset DFU target, err: %d", ret);
			}
			init_firmware_variables();
		}
		if (update_data.object_instance == obj_inst_id) {
			update_data.object_instance = UNUSED_OBJ_ID;
			update_data.update_accepted = false;
		}
	} else if (*data == STATE_DOWNLOADED) {
		if (obj_inst_id == ongoing_obj_id) {
			fota_event_download_ready(obj_inst_id);
			init_firmware_variables();
		}
	} else if (*data == STATE_DOWNLOADING) {
		fota_event_download_start(obj_inst_id);
	}

	return 0;
}

static void dfu_target_cb(enum dfu_target_evt_id evt)
{
	ARG_UNUSED(evt);
}

static int firmware_block_received_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				      uint8_t *data, uint16_t data_len, bool last_block,
				      size_t total_size)
{
	uint8_t curent_percent;
	uint32_t current_bytes;
	size_t offset;
	size_t skip = 0;
	int ret = 0;
	int image_type;

	if (!data_len) {
		return -EINVAL;
	}

	if (bytes_downloaded == 0) {
		if (ongoing_obj_id != UNUSED_OBJ_ID) {
			LOG_INF("DFU is allocated already");
			return -EAGAIN;
		}

		ongoing_obj_id = obj_inst_id;
		client_acknowledge();

		image_type = dfu_target_img_type(data, data_len);
		if (image_type == DFU_TARGET_IMAGE_TYPE_NONE) {
			ret = -ENOMSG; /* Translates to unsupported image type */
			goto cleanup;
		}
		LOG_INF("Image type %d", image_type);
		ret = firmware_target_prepare(image_type);
		if (ret) {
			goto cleanup;
		}

		/* Store Started DFU type */
		target_image_type_store(obj_inst_id, image_type);
		ret = dfu_target_init(image_type, 0, total_size, dfu_target_cb);
		if (ret < 0) {
			LOG_ERR("Failed to init DFU target, err: %d", ret);
			goto cleanup;
		}

		LOG_INF("%s firmware download started.",
			image_type == DFU_TARGET_IMAGE_TYPE_MODEM_DELTA ||
					image_type == DFU_TARGET_IMAGE_TYPE_FULL_MODEM ?
				"Modem" :
				"Application");
	}

	ret = dfu_target_offset_get(&offset);
	if (ret < 0) {
		LOG_ERR("Failed to obtain current offset, err: %d", ret);
		goto cleanup;
	}

	/* Display a % downloaded or byte progress, if no total size was
	 * provided (this can happen in PULL mode FOTA)
	 */
	if (total_size > 0) {
		curent_percent = bytes_downloaded * 100 / total_size;
		if (curent_percent > percent_downloaded) {
			percent_downloaded = curent_percent;
			LOG_INF("Downloaded %d%%", percent_downloaded);
		}
	} else {
		current_bytes = bytes_downloaded + data_len;
		if (current_bytes / BYTE_PROGRESS_STEP > bytes_downloaded / BYTE_PROGRESS_STEP) {
			LOG_INF("Downloaded %d kB", current_bytes / 1024);
		}
	}

	if (bytes_downloaded < offset) {
		skip = MIN(data_len, offset - bytes_downloaded);

		LOG_INF("Skipping bytes %d-%d, already written.", bytes_downloaded,
			bytes_downloaded + skip);
	}

	bytes_downloaded += data_len;

	if (skip == data_len) {
		/* Nothing to do. */
		return 0;
	}

	ret = dfu_target_write(data + skip, data_len - skip);
	if (ret < 0) {
		LOG_ERR("dfu_target_write error, err %d", ret);
		goto cleanup;
	}

	if (!last_block) {
		/* Keep going */
		return 0;
	}
	/* Last write to flash should be flush write */
	ret = dfu_target_done(true);
	if (ret == 0 && IS_ENABLED(CONFIG_FOTA_CLIENT_AUTOSCHEDULE_UPDATE)) {
		ret = dfu_target_schedule_update(0);
	}

	if (ret < 0) {
		LOG_ERR("dfu_target_done error, err %d", ret);
		goto cleanup;
	}
	LOG_INF("Firmware downloaded, %d bytes in total", bytes_downloaded);

	if (total_size && (bytes_downloaded != total_size)) {
		LOG_ERR("Early last block, downloaded %d, expecting %d", bytes_downloaded,
			total_size);
		ret = -EIO;
	}

cleanup:
	if (ret < 0) {
		if (dfu_target_reset() < 0) {
			LOG_ERR("Failed to reset DFU target");
		}
	}

	return ret;
}

static void fota_download_callback(const struct fota_download_evt *evt)
{
	int dfu_image_type;

	if (ongoing_obj_id == UNUSED_OBJ_ID) {
		return;
	}

	switch (evt->id) {
	/* These two cases return immediately */
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		/* Fetch instance image type */
		dfu_image_type = target_image_type_get(ongoing_obj_id);
		if (dfu_image_type == DFU_TARGET_IMAGE_TYPE_NONE) {
			dfu_image_type = fota_download_target();
			LOG_INF("FOTA download started, target %d", dfu_image_type);
			target_image_type_store(ongoing_obj_id, dfu_image_type);
		}
		return;
	default:
		return;

	/* Following cases mark end of FOTA download */
	case FOTA_DOWNLOAD_EVT_CANCELLED:
		LOG_ERR("FOTA_DOWNLOAD_EVT_CANCELLED");
		set_result(ongoing_obj_id, RESULT_CONNECTION_LOST);
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("FOTA_DOWNLOAD_EVT_ERROR");

		dfu_image_type = fota_download_target();
		LOG_INF("FOTA download failed, target %d", dfu_image_type);
		target_image_type_store(ongoing_obj_id, dfu_image_type);
		switch (evt->cause) {
		/* No error, used when event ID is not FOTA_DOWNLOAD_EVT_ERROR. */
		case FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR:
			set_result(ongoing_obj_id, RESULT_CONNECTION_LOST);
			break;
		/* Downloading the update failed. The download may be retried. */
		case FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED:
			set_result(ongoing_obj_id, RESULT_CONNECTION_LOST);
			break;
		/* The update is invalid and was rejected. Retry will not help. */
		case FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE:
			/* FALLTHROUGH */
		/* Actual firmware type does not match expected. Retry will not help. */
		case FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH:
			set_result(ongoing_obj_id, RESULT_UNSUP_FW);
			break;
		default:
			set_result(ongoing_obj_id, RESULT_UPDATE_FAILED);
			break;
		}
		break;

	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download finished");
		dfu_image_type = fota_download_target();
		target_image_type_store(ongoing_obj_id, dfu_image_type);
		set_state(ongoing_obj_id, STATE_DOWNLOADED);
		break;
	}
}

static void start_fota_download(struct k_work *work)
{
	int ret;
	enum dfu_target_image_type type;

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
		if (ongoing_obj_id == modem_obj_id) {
			type = DFU_TARGET_IMAGE_TYPE_ANY_MODEM;
		} else {
			type = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
		}
	} else {
		type = DFU_TARGET_IMAGE_TYPE_ANY;
	}

	ret = fota_download_start_with_image_type(fota_host, fota_path, fota_sec_tag, 0, 0, type);
	if (ret) {
		LOG_ERR("fota_download_start() failed, return code %d", ret);
		set_result(ongoing_obj_id, RESULT_CONNECTION_LOST);
	}

	return;
}

static int init_start_download(char *uri)
{
	int ret;

	ret = fota_download_init(fota_download_callback);
	if (ret != 0) {
		LOG_ERR("fota_download_init() returned %d", ret);
		return -EBUSY;
	}

	bool is_tls = strncmp(uri, "https://", 8) == 0 || strncmp(uri, "coaps://", 8) == 0;
	if (is_tls) {
		fota_sec_tag = CONFIG_LWM2M_CLIENT_UTILS_DOWNLOADER_SEC_TAG;
	} else {
		fota_sec_tag = -1;
	}

	/* Find the end of protocol marker https:// or coap:// */
	char *s = strstr(uri, "://");

	if (!s) {
		LOG_ERR("Host not found");
		return -EINVAL;
	}
	s += strlen("://");

	/* Find the end of host name, which is start of path */
	char *e = strchr(s, '/');

	if (!e) {
		LOG_ERR("Path not found");
		return -EINVAL;
	}

	/* Path can point to a string, which is kept in LwM2M engine's memory */
	fota_path = e + 1; /* Skip the '/' from path */
	int len = e - uri;

	/* For host, I need to allocate space, as I need to copy the substring */
	fota_host = k_malloc(len + 1);
	if (!fota_host) {
		LOG_ERR("Failed to allocate memory");
		return -ENOMEM;
	}
	strncpy(fota_host, uri, len);
	fota_host[len] = 0;

	k_work_submit(&download_work);

	return 0;
}

static void lwm2m_start_download_image(uint8_t *data, uint16_t obj_instance)
{
	int ret;
	char *package_uri = (char *)data;

	ongoing_obj_id = obj_instance;
	/* Clear stored Image type */
	target_image_type_store(obj_instance, DFU_TARGET_IMAGE_TYPE_NONE);
	ret = init_start_download(package_uri);
	switch (ret) {
	case 0:
		/* OK */
		break;
	case -EINVAL:
		set_result(obj_instance, RESULT_INVALID_URI);
		break;
	case -EBUSY:
		/* Failed to init MCUBoot or download client */
		set_result(obj_instance, RESULT_NO_STORAGE);
		break;
	default: /* Remaining errors from init_start_download() are mostly
		  * reflected by OUT OF MEMORY situations
		  */
		set_result(obj_instance, RESULT_OUT_OF_MEM);
	}
}

static void start_pending_fota_download(struct k_work *work)
{
	struct lwm2m_engine_res_inst *res_inst;
	struct lwm2m_obj_path path;

	if (ongoing_obj_id != UNUSED_OBJ_ID) {
		return;
	}

	for (int i = 0; i < FOTA_INSTANCE_COUNT; i++) {
		/* Find a pending instance which is which Downloading */
		if (get_state(i) != STATE_DOWNLOADING) {
			continue;
		}

		path.level = LWM2M_PATH_LEVEL_RESOURCE_INST;
		path.obj_id = LWM2M_OBJECT_ADV_FIRMWARE_ID;
		path.obj_inst_id = i;
		path.res_id = 1;
		path.res_inst_id = 0;

		res_inst = lwm2m_engine_get_res_inst(&path);

		if (!res_inst) {
			continue;
		}
		LOG_INF("Trigger Pending instance %d", i);
		ongoing_obj_id = i;
		lwm2m_start_download_image(res_inst->data_ptr, i);
		return;
	}
}

static int write_dl_uri(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			uint16_t data_len, bool last_block, size_t total_size)
{
	uint8_t state;
	char *package_uri = (char *)data;


	LOG_DBG("write URI: %s", package_uri);

	state = get_state(obj_inst_id);

	if (state == STATE_IDLE && data_len > 0) {
		set_state(obj_inst_id, STATE_DOWNLOADING);

		if (ongoing_obj_id == UNUSED_OBJ_ID) {
			lwm2m_start_download_image(data, obj_inst_id);
		} else {
			if (!IS_ENABLED(
				    CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
				set_result(obj_inst_id, RESULT_ADV_CONFLICT_STATE);
			}
		}
	} else if (data_len == 0) {
		if (ongoing_obj_id == UNUSED_OBJ_ID || ongoing_obj_id == obj_inst_id) {
			ongoing_obj_id = obj_inst_id;
			/* reset to state idle and result default */
			/* Init DFU state */
			set_result(obj_inst_id, RESULT_DEFAULT);
			ongoing_obj_id = UNUSED_OBJ_ID;
		}
	}
	return 0;
}

static void lwm2m_firmware_load_from_settings(int instance_id)
{
	int ret;
	char path[sizeof(LWM2M_FIRM_PREFIX "65632/0/10")];

	/* Read Object Spesific data */
	snprintk(path, sizeof(path), LWM2M_FIRM_PREFIX "/%d/%d", ENABLED_LWM2M_FIRMWARE_OBJECT,
		 instance_id);
	ret = settings_load_subtree(path);
	if (ret) {
		LOG_ERR("Failed to load settings, %d", ret);
	}
}


static void lwm2m_firmware_object_pull_protocol_init(int instance_id)
{
#ifndef CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT
	struct lwm2m_engine_res *res;

	res = lwm2m_engine_get_res(&LWM2M_OBJ(ENABLED_LWM2M_FIRMWARE_OBJECT, instance_id,
					      LWM2M_FOTA_UPDATE_PROTO_SUPPORT_ID));
	if (res && res->multi_res_inst && res->res_inst_count < FOTA_PULL_SUPPORTED_COUNT) {
		/* Update resource instance count and buffer's */
		res->res_instances = pull_protocol_buff;
		res->res_inst_count = FOTA_PULL_SUPPORTED_COUNT;
		for (int i = 0; i < FOTA_PULL_SUPPORTED_COUNT; i++) {
			pull_protocol_buff[i].res_inst_id = RES_INSTANCE_NOT_CREATED;
			pull_protocol_buff[i].data_len = 0;
			pull_protocol_buff[i].max_data_len = 0;
			pull_protocol_buff[i].data_ptr = NULL;
			pull_protocol_buff[i].data_flags = 0;
		}
	}
#endif
}

static bool modem_has_credentials(int sec_tag)
{
	bool exist;
	int ret;

	ret = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exist);
	if (ret < 0) {
		return false;
	}
	return exist;
}

static void lwm2m_firware_pull_protocol_support_resource_init(int instance_id)
{
	struct lwm2m_obj_path path;
	int ret;
	int supported_protocol_count;

	if (!IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
		lwm2m_firmware_object_pull_protocol_init(instance_id);
	}

	if (modem_has_credentials(CONFIG_LWM2M_CLIENT_UTILS_DOWNLOADER_SEC_TAG)) {
		/* Enable non-security &  Security protocols for download client */
		supported_protocol_count = 4;
	} else {
		/* Enable non-security protocols for download client */
		supported_protocol_count = 2;
	}

	for (int i = 0; i < supported_protocol_count; i++) {
		path = LWM2M_OBJ(ENABLED_LWM2M_FIRMWARE_OBJECT, instance_id,
				 LWM2M_FOTA_UPDATE_PROTO_SUPPORT_ID, i);

		ret = lwm2m_create_res_inst(&path);
		if (ret) {
			return;
		}

		ret = lwm2m_set_res_buf(&path, &pull_protocol_support[i],
					sizeof(uint8_t), sizeof(uint8_t), LWM2M_RES_DATA_FLAG_RO);
		if (ret) {
			lwm2m_delete_res_inst(&path);
			return;
		}
	}
}

static void lwm2m_firmware_register_write_callbacks(int instance_id)
{
	struct lwm2m_obj_path path = LWM2M_OBJ(ENABLED_LWM2M_FIRMWARE_OBJECT, instance_id,
					       LWM2M_FOTA_PACKAGE_ID);

	lwm2m_register_pre_write_callback(&path, firmware_get_buf);

	path.res_id = LWM2M_FOTA_PACKAGE_URI_ID;
	lwm2m_register_post_write_callback(&path, write_dl_uri);
	/* State */
	path.res_id = LWM2M_FOTA_STATE_ID;
	lwm2m_register_post_write_callback(&path, firmware_update_state);
	/* Result */
	path.res_id = LWM2M_FOTA_UPDATE_RESULT_ID;
	lwm2m_register_post_write_callback(&path, firmware_update_result);
}

static void firmware_update_check_linked_instances(int instance_id)
{
#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
	/* TODO: When Advanced Firmware object supports more than two
	 * resources, this needs to be changed so that it checks all linked
	 * instances. Now we just assume that there could be only one link
	 */
	struct lwm2m_obj_path path;
	struct lwm2m_objlnk object_link;
	uint8_t result;

	path = LWM2M_OBJ(ENABLED_LWM2M_FIRMWARE_OBJECT, instance_id,
			 LWM2M_ADV_FOTA_LINKED_INSTANCES_ID, 0);

	if (lwm2m_get_objlnk(&path, &object_link)) {
		return;
	}

	if (object_link.obj_id != LWM2M_OBJECT_ADV_FIRMWARE_ID) {
		return;
	}

	if (get_state(object_link.obj_inst) == STATE_DOWNLOADED) {
		LOG_INF("Updating linked instance %d", object_link.obj_inst);
		ongoing_obj_id = object_link.obj_inst;
		set_state(object_link.obj_inst, STATE_UPDATING);
		firmware_instance_schedule(object_link.obj_inst);
		if (update_data.type == MODEM_DELTA) {
			result = apply_firmware_delta_modem_update();
			if (result == RESULT_SUCCESS) {
				ongoing_obj_id = UNUSED_OBJ_ID;
			}
			set_result(modem_obj_id, result);
		} else if (update_data.type == MODEM_FULL) {
#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
			result = apply_fmfu_from_ext_flash();
			if (result == RESULT_SUCCESS) {
				ongoing_obj_id = UNUSED_OBJ_ID;
			}
			/* Set result */
			set_result(modem_obj_id, result);
#endif
		}
	}
	/* Remove link */
	lwm2m_delete_res_inst(&path);
#endif
}


static void firmware_object_state_check(void)
{
#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
	uint8_t app_state, modem_state;

	app_state = get_state(application_obj_id);
	modem_state = get_state(modem_obj_id);

	if (app_state == STATE_DOWNLOADING) {
		ongoing_obj_id = application_obj_id;
		/* reset to state idle and result default */
		/* Init DFU state */
		set_result(application_obj_id, RESULT_DEFAULT);
	}

	if (modem_state == STATE_DOWNLOADING) {
		ongoing_obj_id = modem_obj_id;
		/* reset to state idle and result default */
		/* Init DFU state */
		set_result(modem_obj_id, RESULT_DEFAULT);
	} else if (modem_state == STATE_UPDATING) {
		ongoing_obj_id = modem_obj_id;
		set_result(modem_obj_id, RESULT_UPDATE_FAILED);
	}
#else
	uint8_t object_state;
	int dfu_image_type;

	object_state = get_state(application_obj_id);
	dfu_image_type = target_image_type_get(application_obj_id);

	if (object_state == STATE_DOWNLOADING) {
		ongoing_obj_id = application_obj_id;
		/* reset to state idle and result default */
		/* Init DFU state */
		set_result(application_obj_id, RESULT_DEFAULT);
	} else if (object_state == STATE_UPDATING &&
		   (dfu_image_type & DFU_TARGET_IMAGE_TYPE_ANY_MODEM)) {
		ongoing_obj_id = application_obj_id;
		set_result(application_obj_id, RESULT_UPDATE_FAILED);
	}
#endif
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
static void lwm2m_adv_firmware_versions_set(void)
{
	char buf[255];
	struct lwm2m_obj_path path;
	struct mcuboot_img_header header;

	path = LWM2M_OBJ(ENABLED_LWM2M_FIRMWARE_OBJECT, modem_obj_id,
			 LWM2M_ADV_FOTA_CURRENT_VERSION_ID);
	modem_info_string_get(MODEM_INFO_FW_VERSION, buf, sizeof(buf));
	lwm2m_set_string(&path, buf);

	path.obj_inst_id = application_obj_id;
	path.res_id = LWM2M_ADV_FOTA_CURRENT_VERSION_ID;
	boot_read_bank_header(PM_MCUBOOT_PRIMARY_ID, &header, sizeof(header));
	snprintk(buf, sizeof(buf), "%d.%d.%d-%d", header.h.v1.sem_ver.major,
		 header.h.v1.sem_ver.minor, header.h.v1.sem_ver.revision,
		 header.h.v1.sem_ver.build_num);
	lwm2m_set_string(&path, buf);
}
#endif

int lwm2m_init_firmware_cb(lwm2m_firmware_event_cb_t cb)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Failed to initialize settings subsystem, %d", ret);
		return ret;
	}

	ret = settings_register(&lwm2m_firm_settings);
	if (ret) {
		LOG_ERR("Failed to register settings, %d", ret);
		return ret;
	}

	for (int i = 0; i < FOTA_INSTANCE_COUNT; i++) {
		target_image_type[i] = DFU_TARGET_IMAGE_TYPE_NONE;
	}

	k_work_init_delayable(&update_data.work, update_work_handler);
	update_data.object_instance = UNUSED_OBJ_ID;
	update_data.update_accepted = false;
	ongoing_obj_id = UNUSED_OBJ_ID;
	firmware_fota_event_cb = cb;

	/* Init stream targets */
#ifdef CONFIG_DFU_TARGET_MCUBOOT
	/* Set the required buffer for MCUboot targets */
	ret = dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
	if (ret) {
		LOG_ERR("Failed to set MCUboot flash buffer %d", ret);
	}
#endif

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	ret = full_modem_stream_params_init();
	if (ret) {
		LOG_ERR("Failed to init Full modem Stream Target %d", ret);
	}
#endif

	/* setup data buffer for block-wise transfer */
#if defined(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT)
	application_obj_id = lwm2m_adv_firmware_create_inst(
		"application", firmware_block_received_cb, firmware_update_cb);
	lwm2m_firmware_load_from_settings(application_obj_id);
	lwm2m_firware_pull_protocol_support_resource_init(application_obj_id);
	lwm2m_firmware_register_write_callbacks(application_obj_id);

	modem_obj_id = lwm2m_adv_firmware_create_inst(
		"modem:" CONFIG_SOC, firmware_block_received_cb, firmware_update_cb);
	lwm2m_firmware_load_from_settings(modem_obj_id);
	lwm2m_firware_pull_protocol_support_resource_init(modem_obj_id);
	lwm2m_firmware_register_write_callbacks(modem_obj_id);
	lwm2m_adv_firmware_versions_set();
#else
	application_obj_id = modem_obj_id = 0;
	lwm2m_firmware_load_from_settings(application_obj_id);
	lwm2m_firware_pull_protocol_support_resource_init(application_obj_id);
	lwm2m_firmware_register_write_callbacks(application_obj_id);
	lwm2m_firmware_set_update_cb(firmware_update_cb);
	lwm2m_firmware_set_write_cb(firmware_block_received_cb);
#endif
	firmware_object_state_check();
	return 0;
}

int lwm2m_init_firmware(void)
{
	return lwm2m_init_firmware_cb(NULL);
}

int lwm2m_init_image(void)
{
	int ret = 0;
	bool image_ok;
	uint8_t state = get_state(application_obj_id);

	image_ok = boot_is_img_confirmed();
	LOG_INF("Image is%s confirmed OK", image_ok ? "" : " not");
	if (!image_ok) {
		ret = boot_write_img_confirmed();
		if (ret) {
			LOG_ERR("Couldn't confirm this image: %d", ret);
			return ret;
		}

		LOG_INF("Marked image as OK");
		if (state == STATE_UPDATING) {
			LOG_INF("Firmware updated successfully");
			set_result(application_obj_id, RESULT_SUCCESS);
		}

	} else {
		if (state == STATE_UPDATING) {
			LOG_INF("Firmware failed to be updated");
			set_result(application_obj_id, RESULT_UPDATE_FAILED);
		}
	}

	return ret;
}
