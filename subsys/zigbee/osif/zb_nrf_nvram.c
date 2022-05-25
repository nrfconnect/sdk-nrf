/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <pm_config.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>

#include <zboss_api.h>

#ifdef ZB_USE_NVRAM

/* Size of logical ZBOSS NVRAM page in bytes. */
#define ZBOSS_NVRAM_PAGE_SIZE (PM_ZBOSS_NVRAM_SIZE / CONFIG_ZIGBEE_NVRAM_PAGE_COUNT)
#define PHYSICAL_PAGE_SIZE 0x1000
BUILD_ASSERT((ZBOSS_NVRAM_PAGE_SIZE % PHYSICAL_PAGE_SIZE) == 0,
	     "The size must be a multiply of physical page size.");

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

/* ZBOSS callout that should be called once flash erase page operation
 * is finished.
 */
void zb_nvram_erase_finished(zb_uint8_t page);

static const struct flash_area *fa; /* ZBOSS nvram */

#ifdef ZB_PRODUCTION_CONFIG
static const struct flash_area *fa_pc; /* production config */
#endif

void zb_osif_nvram_init(const zb_char_t *name)
{
	ARG_UNUSED(name);
	int ret;

	ret = flash_area_open(PM_ZBOSS_NVRAM_ID, &fa);
	if (ret) {
		LOG_ERR("Can't open ZBOSS NVRAM flash area");
	}

#ifdef ZB_PRODUCTION_CONFIG
	ret = flash_area_open(PM_ZBOSS_PRODUCT_CONFIG_ID, &fa_pc);
	if (ret) {
		LOG_ERR("Can't open product config flash area");
	}
#endif
}

zb_uint32_t zb_get_nvram_page_length(void)
{
	return ZBOSS_NVRAM_PAGE_SIZE;
}

zb_uint8_t zb_get_nvram_page_count(void)
{
	return CONFIG_ZIGBEE_NVRAM_PAGE_COUNT;
}

static zb_uint32_t get_page_base_offset(int page_num)
{
	return (page_num * zb_get_nvram_page_length());
}

zb_ret_t zb_osif_nvram_read(zb_uint8_t page, zb_uint32_t pos, zb_uint8_t *buf,
			    zb_uint16_t len)
{
	if (page >= zb_get_nvram_page_count()) {
		return RET_PAGE_NOT_FOUND;
	}

	if (pos + len > zb_get_nvram_page_length()) {
		return RET_INVALID_PARAMETER;
	}

	if (!buf) {
		return RET_INVALID_PARAMETER_3;
	}

	if (!len) {
		return RET_INVALID_PARAMETER_4;
	}
	LOG_DBG("Function: %s, page: %d, pos: %d, len: %d",
		__func__, page, pos, len);

	uint32_t flash_addr = get_page_base_offset(page) + pos;

	int err = flash_area_read(fa, flash_addr, buf, len);

	if (err) {
		LOG_ERR("Read error: %d", err);
		return RET_ERROR;
	}
	return RET_OK;
}

zb_ret_t zb_osif_nvram_write(zb_uint8_t page, zb_uint32_t pos, void *buf,
			     zb_uint16_t len)
{
	uint32_t flash_addr = get_page_base_offset(page) + pos;

	if (page >= zb_get_nvram_page_count()) {
		return RET_PAGE_NOT_FOUND;
	}

	if (pos + len > zb_get_nvram_page_length()) {
		return RET_INVALID_PARAMETER;
	}

	if (!buf) {
		return RET_INVALID_PARAMETER_3;
	}

	if (len == 0) {
		return RET_OK;
	}

	if (!(len >> 2)) {
		return RET_INVALID_PARAMETER_4;
	}

	LOG_DBG("Function: %s, page: %d, pos: %d, len: %d",
		__func__, page, pos, len);

	int err = flash_area_write(fa, flash_addr, buf, len);

	if (err) {
		LOG_ERR("Write error: %d", err);
		return RET_ERROR;
	}

	return RET_OK;
}

zb_ret_t zb_osif_nvram_erase_async(zb_uint8_t page)
{
	zb_ret_t ret = RET_OK;

	if (page < zb_get_nvram_page_count()) {
		int err = flash_area_erase(fa, get_page_base_offset(page),
					   zb_get_nvram_page_length());
		if (err) {
			LOG_ERR("Erase error: %d", err);
			ret = RET_ERROR;
		}
	}
	zb_nvram_erase_finished(page);
	return ret;
}

void zb_osif_nvram_wait_for_last_op(void)
{
	/* empty for synchronous erase and write */
}

void zb_osif_nvram_flush(void)
{
	/* empty for synchronous erase and write */
}


#ifdef ZB_PRODUCTION_CONFIG

#define ZB_OSIF_PRODUCTION_CONFIG_MAGIC             { 0xE7, 0x37, 0xDD, 0xF6 }
#define ZB_OSIF_PRODUCTION_CONFIG_MAGIC_SIZE        4

zb_bool_t zb_osif_prod_cfg_check_presence(void)
{
	zb_uint8_t hdr[ZB_OSIF_PRODUCTION_CONFIG_MAGIC_SIZE] =
		ZB_OSIF_PRODUCTION_CONFIG_MAGIC;
	zb_uint8_t buffer[ZB_OSIF_PRODUCTION_CONFIG_MAGIC_SIZE] = {0};

	int err = flash_area_read(fa_pc, 0, buffer,
				  ZB_OSIF_PRODUCTION_CONFIG_MAGIC_SIZE);

	if (!err) {
		return ((zb_bool_t) !memcmp(buffer, hdr, sizeof(buffer)));

	} else {
		return ZB_FALSE;
	}
}

zb_ret_t zb_osif_prod_cfg_read_header(zb_uint8_t *prod_cfg_hdr,
				      zb_uint16_t hdr_len)
{
	int err = flash_area_read(fa_pc, ZB_OSIF_PRODUCTION_CONFIG_MAGIC_SIZE,
				  prod_cfg_hdr, hdr_len);

	if (err) {
		LOG_ERR("Prod conf header read error: %d", err);
		return RET_ERROR;
	}
	return RET_OK;
}


zb_ret_t zb_osif_prod_cfg_read(zb_uint8_t *buffer,
			       zb_uint16_t len,
			       zb_uint16_t offset)
{
	uint32_t pc_offset = ZB_OSIF_PRODUCTION_CONFIG_MAGIC_SIZE + offset;
	int err = flash_area_read(fa_pc, pc_offset, buffer, len);

	if (err) {
		LOG_ERR("Prod conf read error: %d", err);
		return RET_ERROR;
	}
	return RET_OK;
}

#endif  /* ZB_PRODUCTION_CONFIG */

#endif  /* ZB_USE_NVRAM */
