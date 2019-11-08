/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <logging/log.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include "dfu_target_mcuboot.h"
#include "dfu_target_modem.h"

const struct dfu_target dfu_target_modem = {
	.init = dfu_target_modem_init,
	.offset_get = dfu_target_modem_offset_get,
	.write = dfu_target_modem_write,
	.done = dfu_target_modem_done,
};

const struct dfu_target dfu_target_mcuboot = {
	.init  = dfu_target_mcuboot_init,
	.offset_get = dfu_target_mcuboot_offset_get,
	.write = dfu_target_mcuboot_write,
	.done  = dfu_target_mcuboot_done,
};

#define MIN_SIZE_IDENTIFY_BUF 32

LOG_MODULE_REGISTER(dfu_target, CONFIG_DFU_TARGET_LOG_LEVEL);

static const struct dfu_target *current_target;

int dfu_target_img_type(const void *const buf, size_t len)
{
	if (IS_ENABLED(CONFIG_DFU_TARGET_MCUBOOT) &&
	    dfu_target_mcuboot_identify(buf)) {
		return DFU_TARGET_IMAGE_TYPE_MCUBOOT;
	}

	if (IS_ENABLED(CONFIG_DFU_TARGET_MODEM) &&
	    dfu_target_modem_identify(buf)) {
		return DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
	}

	if (len < MIN_SIZE_IDENTIFY_BUF) {
		return -EAGAIN;
	}

	LOG_ERR("No supported image type found");
	return -ENOTSUP;
}

int dfu_target_init(int img_type, size_t file_size)
{
	const struct dfu_target *new_target = NULL;

	if (IS_ENABLED(CONFIG_DFU_TARGET_MCUBOOT) &&
	    img_type == DFU_TARGET_IMAGE_TYPE_MCUBOOT) {
		new_target = &dfu_target_mcuboot;
	} else if (IS_ENABLED(CONFIG_DFU_TARGET_MODEM) &&
		   img_type == DFU_TARGET_IMAGE_TYPE_MODEM_DELTA) {
		new_target = &dfu_target_modem;
	}

	if (new_target == NULL) {
		LOG_ERR("Unknown image type");
		return -ENOTSUP;
	}

	/* The user is re-initializing with an previously aborted target.
	 * Avoid re-initializing generally to ensure that the download can
	 * continue where it left off. Re-initializing is required for modem
	 * upgrades to re-open the DFU socket that is closed on abort.
	 */
	if (new_target == current_target
	   && img_type != DFU_TARGET_IMAGE_TYPE_MODEM_DELTA) {
		return 0;
	}

	current_target = new_target;

	return current_target->init(file_size);
}

int dfu_target_offset_get(size_t *offset)
{
	if (current_target == NULL) {
		return -EACCES;
	}

	return current_target->offset_get(offset);
}

int dfu_target_write(const void *const buf, size_t len)
{
	if (current_target == NULL || buf == NULL) {
		return -EACCES;
	}

	return current_target->write(buf, len);
}

int dfu_target_done(bool successful)
{
	int err;

	if (current_target == NULL) {
		return -EACCES;
	}

	err = current_target->done(successful);

	if (successful) {
		current_target = NULL;
	}

	if (err != 0) {
		LOG_ERR("Unable to clean up dfu_target");
		return err;
	}

	return 0;
}

