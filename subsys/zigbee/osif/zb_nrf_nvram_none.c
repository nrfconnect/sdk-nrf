/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "zboss_api.h"


#ifdef ZB_USE_NVRAM
static uint8_t zb_nvram_buf[CONFIG_ZIGBEE_NVRAM_PAGE_COUNT][CONFIG_ZIGBEE_NVRAM_PAGE_SIZE];


/* ZBOSS callout that should be called once flash erase page operation is finished. */
void zb_nvram_erase_finished(zb_uint8_t page);


void zb_osif_nvram_init(const zb_char_t *name)
{
	ZVUNUSED(name);
	memset(zb_nvram_buf, 0xFF, sizeof(zb_nvram_buf));
}

zb_uint32_t zb_get_nvram_page_length(void)
{
	return CONFIG_ZIGBEE_NVRAM_PAGE_SIZE;
}

zb_uint8_t zb_get_nvram_page_count(void)
{
	return CONFIG_ZIGBEE_NVRAM_PAGE_COUNT;
}

zb_ret_t zb_osif_addr_read(zb_uint32_t address, zb_uint16_t len, zb_uint8_t *buf)
{
	uint8_t *p_address = (uint8_t *)address;

	if (p_address < &zb_nvram_buf[0][0]) {
		return RET_INVALID_PARAMETER;
	}

	if ((p_address + len) > &zb_nvram_buf[CONFIG_ZIGBEE_NVRAM_PAGE_COUNT - 1]
					       [CONFIG_ZIGBEE_NVRAM_PAGE_SIZE - 1]) {
		return RET_INVALID_PARAMETER_2;
	}

	memcpy(buf, p_address, len);

	return RET_OK;
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

	if (len == 0) {
		return RET_OK;
	}

	memcpy(buf, &zb_nvram_buf[page][pos], len);

	return RET_OK;
}

zb_ret_t zb_osif_nvram_write(zb_uint8_t page, zb_uint32_t pos, void *buf,
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

	if (len == 0) {
		return RET_OK;
	}

	memcpy(&zb_nvram_buf[page][pos], buf, len);

	return RET_OK;
}

zb_ret_t zb_osif_nvram_erase_async(zb_uint8_t page)
{
	if (page < zb_get_nvram_page_count()) {
		memset(&zb_nvram_buf[page][0], 0xFF, zb_get_nvram_page_length());
	}

	zb_nvram_erase_finished(page);
	return RET_OK;
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
zb_bool_t zb_osif_prod_cfg_check_presence(void)
{
	return ZB_FALSE;
}

zb_ret_t zb_osif_prod_cfg_read_header(zb_uint8_t *prod_cfg_hdr,
						      zb_uint16_t hdr_len)
{
	memset(prod_cfg_hdr, 0xFF, hdr_len);
	return RET_OK;
}


zb_ret_t zb_osif_prod_cfg_read(zb_uint8_t *buffer, zb_uint16_t len, zb_uint16_t offset)
{
	ZVUNUSED(offset);
	memset(buffer, 0xFF, len);
	return RET_OK;
}

#endif /* ZB_PRODUCTION_CONFIG */

#endif /* ZB_USE_NVRAM */
