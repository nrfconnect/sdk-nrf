/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>
#include "emds_flash.h"
#include <nrf_erratas.h>

#if defined CONFIG_SOC_FLASH_NRF_RRAM
#include <hal/nrf_rramc.h>
#include <zephyr/sys/barrier.h>
#define RRAM                  DT_INST(0, soc_nv_flash)
#define RRAM_START            DT_REG_ADDR(RRAM)
#define RRAM_SIZE             DT_REG_SIZE(RRAM)
#define EMDS_FLASH_BLOCK_SIZE DT_PROP(RRAM, write_block_size)
#define WRITE_BUFFER_SIZE     NRF_RRAMC_CONFIG_WRITE_BUFF_SIZE_MAX
#else
#include <nrfx_nvmc.h>
#define FLASH                 DT_INST(0, soc_nv_flash)
#define EMDS_FLASH_BLOCK_SIZE DT_PROP(FLASH, write_block_size)
#endif

LOG_MODULE_REGISTER(emds_flash, CONFIG_EMDS_LOG_LEVEL);

#define ADDR_OFFS_MASK 0x0000FFFF

/* Allocation Table Entry */
struct emds_ate {
	uint16_t id; /* data id */
	uint16_t offset; /* data offset within sector */
	uint16_t len; /* data len within sector */
	uint8_t crc8_data; /* crc8 check of the data entry */
	uint8_t crc8; /* crc8 check of the ate entry */
} __packed;

enum ate_type {
	ATE_TYPE_VALID = BIT(0),
	ATE_TYPE_INVALIDATED = BIT(1),
	ATE_TYPE_ERASED = BIT(2),
	ATE_TYPE_UNKNOWN = BIT(3)
};

BUILD_ASSERT(offsetof(struct emds_ate, crc8) == sizeof(struct emds_ate) - sizeof(uint8_t),
	     "crc8 must be the last member");

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#if NRF52_ERRATA_242_PRESENT
#include <hal/nrf_power.h>
/* Disable POFWARN by writing POFCON before a write or erase operation.
 * Do not attempt to write or erase if EVENTS_POFWARN is already asserted.
 */
static bool pofcon_enabled;

static int suspend_pofwarn(void)
{
	if (!nrf52_errata_242()) {
		return 0;
	}

	bool enabled;
	nrf_power_pof_thr_t pof_thr;

	pof_thr = nrf_power_pofcon_get(NRF_POWER, &enabled);

	if (enabled) {
		nrf_power_pofcon_set(NRF_POWER, false, pof_thr);

		/* This check need to be reworked once POFWARN event will be
		 * served by zephyr.
		 */
		if (nrf_power_event_check(NRF_POWER, NRF_POWER_EVENT_POFWARN)) {
			nrf_power_pofcon_set(NRF_POWER, true, pof_thr);
			return -ECANCELED;
		}
	}

	return 0;
}

static void restore_pofwarn(void)
{
	nrf_power_pof_thr_t pof_thr;

	if (pofcon_enabled) {
		pof_thr = nrf_power_pofcon_get(NRF_POWER, NULL);

		nrf_power_pofcon_set(NRF_POWER, true, pof_thr);
		pofcon_enabled = false;
	}
}

#define SUSPEND_POFWARN() suspend_pofwarn()
#define RESUME_POFWARN()  restore_pofwarn()
#else
#define SUSPEND_POFWARN() 0
#define RESUME_POFWARN()
#endif /* NRF52_ERRATA_242_PRESENT */

static inline bool is_aligned_32(uint32_t data)
{
	return (data & 0x3) ? false : true;
}

static inline bool is_within_bounds(off_t addr, size_t len, off_t boundary_start,
				    size_t boundary_size)
{
	return (addr >= boundary_start &&
			(addr < (boundary_start + boundary_size)) &&
			(len <= (boundary_start + boundary_size - addr)));
}

static inline bool is_regular_addr_valid(off_t addr, size_t len)
{
#if defined CONFIG_SOC_FLASH_NRF_RRAM
	return is_within_bounds(addr, len, RRAM_START, RRAM_START + RRAM_SIZE);
#else
	return is_within_bounds(addr, len, 0, nrfx_nvmc_flash_size_get());
#endif
}

static void nvmc_wait_ready(void)
{
#if defined CONFIG_SOC_FLASH_NRF_RRAM
	while (!nrf_rramc_ready_check(NRF_RRAMC)) {
	}
#else
	while (!nrfx_nvmc_write_done_check()) {
	}
#endif
}

#if defined CONFIG_SOC_FLASH_NRF_RRAM
static void commit_changes(size_t len)
{
	if (nrf_rramc_empty_buffer_check(NRF_RRAMC)) {
		/* The internal write-buffer has been committed to RRAM and is now empty. */
		return;
	}

	if ((len % WRITE_BUFFER_SIZE) == 0) {
		/* Our last operation was buffer size-aligned, so we're done. */
		return;
	}

	nrf_rramc_task_trigger(NRF_RRAMC, NRF_RRAMC_TASK_COMMIT_WRITEBUF);

	barrier_dmem_fence_full();
}
#endif

static int flash_direct_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	uint32_t flash_addr = offset;

	ARG_UNUSED(dev);

	if (!is_regular_addr_valid(flash_addr, len)) {
		return -EINVAL;
	}

	if (!is_aligned_32(flash_addr)) {
		return -EINVAL;
	}

	if (len % sizeof(uint32_t)) {
		return -EINVAL;
	}

	flash_addr += DT_REG_ADDR(SOC_NV_FLASH_NODE);

	nvmc_wait_ready();

	if (SUSPEND_POFWARN()) {
		return -ECANCELED;
	}

#if defined CONFIG_SOC_FLASH_NRF_RRAM
	nrf_rramc_config_t config = {.mode_write = true, .write_buff_size = WRITE_BUFFER_SIZE};

	nrf_rramc_config_set(NRF_RRAMC, &config);
	memcpy((void *)flash_addr, data, len);

	barrier_dmem_fence_full(); /* Barrier following our last write. */
	commit_changes(len);

	config.mode_write = false;
	nrf_rramc_config_set(NRF_RRAMC, &config);
#else
	uint32_t data_addr = (uint32_t)data;

	while (len >= sizeof(uint32_t)) {
		nrfx_nvmc_word_write(flash_addr, UNALIGNED_GET((uint32_t *)data_addr));

		flash_addr += sizeof(uint32_t);
		data_addr += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}
#endif

	RESUME_POFWARN();

	nvmc_wait_ready();

	return 0;
}

static size_t align_size(struct emds_fs *fs, size_t len)
{
	uint8_t write_block_size = fs->flash_params->write_block_size;

	if (write_block_size <= 1U) {
		return len;
	}
	return (len + (write_block_size - 1U)) & ~(write_block_size - 1U);
}

static int ate_wrt(struct emds_fs *fs, const struct emds_ate *entry)
{
	size_t ate_size = align_size(fs, sizeof(struct emds_ate));

	if (ate_size % fs->flash_params->write_block_size) {
		return -EINVAL;
	}

	int rc = flash_direct_write(fs->flash_dev, fs->ate_wra, entry, sizeof(struct emds_ate));

	if (rc) {
		return rc;
	}

	fs->ate_wra -= ate_size;
	return 0;
}

static int data_wrt(struct emds_fs *fs, const void *data, size_t len)
{
	const uint8_t *data8 = (const uint8_t *)data;
	int rc;
	off_t offset;
	size_t blen;
	size_t temp_len = len;
	uint8_t buf[EMDS_FLASH_BLOCK_SIZE];

	if (!temp_len) {
		/* Nothing to write, avoid changing the flash protection */
		return 0;
	}

	offset = fs->offset;
	offset += fs->data_wra_offset & ADDR_OFFS_MASK;

	blen = temp_len & ~(fs->flash_params->write_block_size - 1U);
	/* Writes multiples of 4 bytes to flash */
	if (blen > 0) {
		rc = flash_direct_write(fs->flash_dev, offset, data8, blen);
		if (rc) {
			return rc;
		}

		temp_len -= blen;
		offset += blen;
		data8 += blen;
	}

	if (temp_len) {
		(void)memcpy(buf, data8, temp_len);
		(void)memset(buf + temp_len, fs->flash_params->erase_value,
			     fs->flash_params->write_block_size - temp_len);
		rc = flash_direct_write(fs->flash_dev, offset, buf,
					fs->flash_params->write_block_size);
		if (rc) {
			return rc;
		}
	}

	fs->data_wra_offset += align_size(fs, len);
	return 0;
}

static int check_erased(struct emds_fs *fs, uint32_t addr, size_t len)
{
	size_t bytes_to_cmp;
	uint8_t cmp[EMDS_FLASH_BLOCK_SIZE];
	uint8_t buf[EMDS_FLASH_BLOCK_SIZE];

	(void)memset(cmp, 0xff, EMDS_FLASH_BLOCK_SIZE);
	while (len) {
		bytes_to_cmp = MIN(EMDS_FLASH_BLOCK_SIZE, len);
		if (flash_read(fs->flash_dev, addr, buf, bytes_to_cmp)) {
			return -EIO;
		}

		if (memcmp(cmp, buf, bytes_to_cmp)) {
			return -EINVAL;
		}

		len -= bytes_to_cmp;
		addr += bytes_to_cmp;
	}
	return 0;
}

static int is_ate_valid(const struct emds_ate *entry)
{
	return entry->crc8 == crc8_ccitt(0xff, entry, offsetof(struct emds_ate, crc8));
}

static int entry_wrt(struct emds_fs *fs, uint16_t id, const void *data, size_t len)
{
	int rc;
	struct emds_ate entry;

	entry.id = id;
	entry.offset = fs->data_wra_offset;
	entry.len = (uint16_t)len;
	entry.crc8_data = crc8_ccitt(0xff, data, len);
	entry.crc8 = crc8_ccitt(0xff, &entry, offsetof(struct emds_ate, crc8));
	rc = data_wrt(fs, data, len);
	if (rc) {
		return rc;
	}

	rc = ate_wrt(fs, &entry);
	if (rc) {
		return rc;
	}

	return 0;
}

static enum ate_type ate_check(struct emds_fs *fs, uint32_t addr, struct emds_ate *entry)
{
	uint8_t read_buf[fs->ate_size];
	int rc = flash_read(fs->flash_dev, addr, read_buf, fs->ate_size);

	if (rc) {
		return ATE_TYPE_UNKNOWN;
	}

	memset(entry, 0, sizeof(struct emds_ate));
	if (!memcmp(entry, read_buf, sizeof(struct emds_ate))) {
		return ATE_TYPE_INVALIDATED;
	}

	memset(entry, fs->flash_params->erase_value, sizeof(struct emds_ate));
	if (!memcmp(entry, read_buf, sizeof(struct emds_ate))) {
		return ATE_TYPE_ERASED;
	}

	memcpy(entry, read_buf, sizeof(struct emds_ate));
	if (is_ate_valid(entry)) {
		return ATE_TYPE_VALID;
	}

	return ATE_TYPE_UNKNOWN;
}

static int ate_last_recover(struct emds_fs *fs)
{
	struct emds_ate end_ate;
	enum ate_type type = 0;
	uint8_t expect_field = 0xFF;

	fs->ate_wra = fs->offset + fs->sector_cnt * fs->sector_size - fs->ate_size;
	fs->data_wra_offset = 0;
	while (type != ATE_TYPE_ERASED) {
		/* Ate wra has reached the start of the data area */
		if (fs->ate_wra < fs->offset) {
			fs->force_erase = true;
			fs->ate_wra = fs->offset;
			fs->data_wra_offset = 0;
			return 0;
		}

		type = ate_check(fs, fs->ate_wra, &end_ate);

		/* If an unexpected entry type occurs we force erase on next prepare */
		if (!(type & expect_field)) {
			fs->force_erase = true;
		}

		switch (type) {
		case ATE_TYPE_VALID:
			fs->data_wra_offset = align_size(fs, end_ate.offset + end_ate.len);
			fs->ate_wra -= fs->ate_size;
			expect_field = ATE_TYPE_VALID | ATE_TYPE_ERASED;
			break;

		case ATE_TYPE_INVALIDATED:
			expect_field = ATE_TYPE_VALID | ATE_TYPE_INVALIDATED | ATE_TYPE_ERASED;
			fs->ate_wra -= fs->ate_size;
			break;

		case ATE_TYPE_UNKNOWN:
			fs->force_erase = true;
			fs->ate_wra -= fs->ate_size;
			break;

		case ATE_TYPE_ERASED:
		/* fall through*/
		default:
			break;
		}
	}

	/* Verify that flash area between ate and data pointer is writeable */
	if (check_erased(fs, fs->offset + fs->data_wra_offset,
			 fs->ate_wra - (fs->offset + fs->data_wra_offset) + fs->ate_size)) {
		fs->force_erase = true;
	}

	return 0;
}

static int old_entries_invalidate(struct emds_fs *fs)
{
	int rc = 0;
	uint8_t inval_buf[fs->ate_size];
	uint32_t addr = fs->ate_wra + fs->ate_size;

	memset(inval_buf, 0, sizeof(inval_buf));
	while (addr <= (fs->offset + fs->sector_cnt * fs->sector_size) - fs->ate_size) {
		rc = flash_write(fs->flash_dev, addr, inval_buf, sizeof(inval_buf));
		if (rc) {
			return rc;
		}

		addr += fs->ate_size;
	}

	return 0;
}

int emds_flash_init(struct emds_fs *fs)
{
	if (fs->is_initialized) {
		return -EACCES;
	}

	k_mutex_init(&fs->emds_lock);
	if (!fs->flash_dev) {
		LOG_ERR("No valid flash device found");
		return -ENXIO;
	}

	fs->flash_params = flash_get_parameters(fs->flash_dev);
	if (fs->flash_params == NULL) {
		LOG_ERR("Could not obtain flash parameters");
		return -EINVAL;
	}

	if (fs->flash_params->write_block_size > EMDS_FLASH_BLOCK_SIZE ||
	    fs->flash_params->write_block_size == 0) {
		LOG_ERR("Unsupported write block size: %i", fs->flash_params->write_block_size);
		return -EINVAL;
	}

	struct flash_pages_info info;
	int rc = flash_get_page_info_by_offs(fs->flash_dev, fs->offset, &info);

	if (rc) {
		LOG_ERR("Unable to get page info");
		return -EINVAL;
	}

	if (!fs->sector_size || fs->sector_size % info.size) {
		LOG_ERR("Invalid sector size");
		return -EINVAL;
	}

	if (fs->sector_cnt < 1) {
		LOG_ERR("Configuration error - sector count");
		return -EINVAL;
	}

	k_mutex_lock(&fs->emds_lock, K_FOREVER);
	fs->ate_size = align_size(fs, sizeof(struct emds_ate));

	rc = ate_last_recover(fs);
	k_mutex_unlock(&fs->emds_lock);
	if (rc) {
		return rc;
	}

	fs->is_initialized = true;
	return 0;
}

int emds_flash_clear(struct emds_fs *fs)
{
	int rc = flash_erase(fs->flash_dev, fs->offset, fs->sector_size * fs->sector_cnt);

	if (rc) {
		return rc;
	}

	rc = check_erased(fs, fs->offset, fs->sector_size * fs->sector_cnt);
	if (rc) {
		return -ENXIO;
	}
	ate_last_recover(fs);
	return rc;
}

ssize_t emds_flash_write(struct emds_fs *fs, uint16_t id, const void *data, size_t len)
{
	if (!fs->is_initialized || !fs->is_prepeared) {
		LOG_ERR("EMDS flash not initialized or not ready for write");
		return -EACCES;
	}

	if (fs->ate_size + align_size(fs, len) > emds_flash_free_space_get(fs)) {
		return -ENOMEM;
	}

	if (len == 0) {
		return 0;
	}

	int rc = entry_wrt(fs, id, data, len);

	if (rc) {
		return rc;
	}

	return len;
}

ssize_t emds_flash_read(struct emds_fs *fs, uint16_t id, void *data, size_t len)
{
	if (!fs->is_initialized) {
		LOG_ERR("EMDS flash not initialized");
		return -EACCES;
	}

	int rc;
	uint32_t wlk_addr = fs->ate_wra;
	struct emds_ate wlk_ate;

	while (true) {
		rc = flash_read(fs->flash_dev, wlk_addr, &wlk_ate, sizeof(struct emds_ate));
		if (rc) {
			return rc;
		}

		if ((wlk_ate.id == id) && (is_ate_valid(&wlk_ate))) {
			break;
		}

		wlk_addr += fs->ate_size;
		if (wlk_addr >= fs->offset + fs->sector_cnt * fs->sector_size) {
			return -ENXIO;
		}
	}

	if (len < wlk_ate.len) {
		return -ENOMEM;
	}

	rc = flash_read(fs->flash_dev, fs->offset + wlk_ate.offset, data, wlk_ate.len);
	if (rc) {
		return rc;
	}

	if (wlk_ate.crc8_data != crc8_ccitt(0xff, data, wlk_ate.len)) {
		return -EFAULT;
	}

	return wlk_ate.len;
}

int emds_flash_prepare(struct emds_fs *fs, int byte_size)
{
	if (!fs->is_initialized) {
		LOG_ERR("EMDS flash not initialized");
		return -EACCES;
	}

	if (byte_size > (fs->sector_cnt * fs->sector_size) - fs->ate_size) {
		return -ENOMEM;
	}

	int rc = old_entries_invalidate(fs);

	if (rc) {
		return rc;
	}

	if (fs->force_erase || (byte_size > emds_flash_free_space_get(fs))) {
		emds_flash_clear(fs);
		fs->force_erase = false;
	}

	fs->is_prepeared = true;
	return 0;
}

ssize_t emds_flash_free_space_get(struct emds_fs *fs)
{
	ssize_t space = fs->ate_wra - (fs->data_wra_offset + fs->offset);

	return (space > fs->ate_size) ? space : 0;
}
