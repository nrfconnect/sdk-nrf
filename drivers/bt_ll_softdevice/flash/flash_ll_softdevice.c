/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <soc.h>
#include <drivers/flash.h>
#include <sys/util.h>

#include <nrfx_nvmc.h>

#include "sdc_soc.h"
#include "multithreading_lock.h"

#define SOC_NV_FLASH_NODE DT_NODELABEL(flash0)
#define SOC_NV_FLASH_CONTROLLER_NODE DT_NODELABEL(flash_controller)

/* NOTE: The driver supports unaligned writes, but some file systems (like FCB)
 * may use the driver sub-optimally as a result. Word aligned writes are faster
 * and require less overhead. This value can be changed to 4 to minimize this
 * overhead.
 */
#define FLASH_DRIVER_WRITE_BLOCK_SIZE 1

static struct {
	/** Used to ensure a single ongoing operation at any time.  */
	struct k_mutex lock;
	/** Used to wait for the asynchronous operation to complete */
	struct k_sem sync;
	const void *data;
	off_t addr;
	uint32_t len;
	uint32_t prev_len;
	uint32_t tmp_word;      /**< Used for unalinged writes. */
	/* NOTE: Read is not async, so not a part of this enum. */
	enum {
		FLASH_OP_NONE,
		FLASH_OP_WRITE,
		FLASH_OP_ERASE
	} op;
} flash_state;

/* Forward declarations */
static int sdc_flash_read(const struct device *dev,
			off_t offset,
			void *data,
			size_t len);
static int sdc_flash_write(const struct device *dev,
			off_t offset,
			const void *data,
			size_t len);
static int sdc_flash_erase(const struct device *dev,
			off_t offset,
			size_t size);
static int sdc_flash_write_protection_set(const struct device *dev,
					bool enable);

static int flash_op_execute(void);

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void sdc_flash_page_layout_get(
	const struct device *dev,
	const struct flash_pages_layout **layout,
	size_t *layout_size);
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */

static const struct flash_parameters *
sdc_flash_parameters_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters flash_parameters = {
		.write_block_size = FLASH_DRIVER_WRITE_BLOCK_SIZE,
		.erase_value = 0xff,
	};

	return &flash_parameters;
}

static const struct flash_driver_api flash_api = {
	.read = sdc_flash_read,
	.write = sdc_flash_write,
	.erase = sdc_flash_erase,
	.write_protection = sdc_flash_write_protection_set,
	.get_parameters = sdc_flash_parameters_get,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = sdc_flash_page_layout_get,
#endif  /* CONFIG_FLASH_PAGE_LAYOUT */
};


/* Utility functions */
static inline bool is_addr_valid(off_t addr, size_t len)
{
	if ((addr < 0) ||
	    (len > nrfx_nvmc_flash_size_get()) ||
	    (addr >  nrfx_nvmc_flash_size_get() - len)) {
		return false;
	}

	return true;
}

static inline bool is_aligned_32(off_t addr)
{
	return ((addr & 0x3) == 0);
}

static inline off_t align_32(off_t addr)
{
	return (addr & ~0x3);
}

static inline size_t bytes_to_words(size_t bytes)
{
	return bytes / sizeof(uint32_t);
}

static inline bool is_page_aligned(off_t addr)
{
	return (addr & (nrfx_nvmc_flash_page_size_get() - 1)) == 0;
}

static void flash_operation_complete_callback(uint32_t status)
{
	__ASSERT_NO_MSG(flash_state.op == FLASH_OP_WRITE ||
			flash_state.op == FLASH_OP_ERASE);

	int err;

	if (status == SDC_SOC_FLASH_CMD_STATUS_SUCCESS) {
		flash_state.addr += flash_state.prev_len;
		flash_state.data = (const void *) ((intptr_t) flash_state.data +
						   flash_state.prev_len);
		flash_state.len -= flash_state.prev_len;
	} else {
		/* Timeout occurred - retry the operation. */
		__ASSERT_NO_MSG(status == SDC_SOC_FLASH_CMD_STATUS_TIMEOUT);
	}

	if (flash_state.len > 0) {
		err = flash_op_execute();
		/* All inputs should have been validated on the first call. */
		__ASSERT(err == 0, "Continued flash operation failed");
	} else {
		flash_state.op = FLASH_OP_NONE;
		k_sem_give(&flash_state.sync);
	}
}

static size_t offset_32(const void *addr)
{
	return (size_t) addr & 0x3;
}

/**
 * Copies unaligned data into a word (uint32_t).
 *
 * This function is used when either @p src or @p dst is an non-word aligned
 * pointer.
 *
 * If the length exceeds the number of remaining in the word, the data is
 * truncated. Example: Attempting to copy 4 bytes into `0x0000 0001` will only
 * copy the first 3 bytes of the input data and return 3.
 *
 * @param[in,out] word_dst Pointer to 32-bit word that will be appropriately
 *                         filled with as much of data from @p src as possible.
 * @param[in]     dst      Destination address pointer (flash). Used to calcuate
 *                         offset into @p word_dst. This is where the data
 *                         should eventually be copied into.
 * @param[in]     src      Source data pointer where data is copied from.
 * @param[in]     len      Length of the data pointed to by @p src.
 *
 *
 * @returns the number of bytes copied into @p word_dst.
 */
static size_t unaligned_word_copy(uint32_t *word_dst,
				  const void *dst,
				  const void *src,
				  size_t len)
{
	/* Example: Writing one byte from src to dst via word_dst.
	 *
	 * |    |<----- src (len = 1)
	 * |  |<------- dst
	 * | 0 1 2 3 |
	 * | address |
	 *
	 * max_offset             : max(1, 2) => 2
	 * remaining_bytes_in_word: 4 - 2     => 2
	 * bytes_to_copy          : MIN(1, 2) => 1
	 */
	size_t max_offset = MAX(offset_32(dst), offset_32(src));
	size_t remaining_bytes_in_word = sizeof(uint32_t) - max_offset;
	size_t bytes_to_copy = MIN(len, remaining_bytes_in_word);

	/* nRF52832 Product specification:
	 *  Only full 32-bit words can be written to Flash using the
	 *  NVMC interface. To write less than 32 bits to Flash, write
	 *  the data as a word, and set all the bits that should remain
	 *  unchanged in the word to '1'.
	 */
	*word_dst = ~0;
	memcpy(&((uint8_t *)word_dst)[offset_32(dst)], src, bytes_to_copy);
	return bytes_to_copy;
}

static int flash_op_write(void)
{
	if (is_aligned_32(flash_state.addr) &&
	    is_aligned_32((off_t) flash_state.data) &&
	    flash_state.len >= sizeof(uint32_t)) {
		flash_state.prev_len = MIN(align_32(flash_state.len),
					   nrfx_nvmc_flash_page_size_get());
		return sdc_soc_flash_write_async(
			(uint32_t) flash_state.addr,
			flash_state.data,
			bytes_to_words(flash_state.prev_len),
			flash_operation_complete_callback);
	} else {
		flash_state.prev_len = unaligned_word_copy(
			&flash_state.tmp_word,
			(void *)flash_state.addr,
			flash_state.data,
			flash_state.len);
		return sdc_soc_flash_write_async(
			(uint32_t)align_32(flash_state.addr),
			&flash_state.tmp_word,
			1,
			flash_operation_complete_callback);
	}
}

static int flash_op_execute(void)
{
	int err;

	if (flash_state.op == FLASH_OP_WRITE) {
		err = flash_op_write();
	} else if (flash_state.op == FLASH_OP_ERASE) {
		flash_state.prev_len = nrfx_nvmc_flash_page_size_get();
		err = sdc_soc_flash_page_erase_async(
			(uint32_t)flash_state.addr,
			flash_operation_complete_callback);
	} else {
		__ASSERT(0, "Unsupported operation");
		err = -EINVAL;
	}

	return err;
}

static int flash_op_execute_with_lock(void)
{
	int err;

	err = MULTITHREADING_LOCK_ACQUIRE();
	if (!err) {
		err = flash_op_execute();
		MULTITHREADING_LOCK_RELEASE();
	}
	return err;
}

/* Driver API. */

static int sdc_flash_read(const struct device *dev,
			     off_t offset,
			     void *data,
			     size_t len)
{
	int err;

	if (!is_addr_valid(offset, len)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	offset += DT_REG_ADDR(SOC_NV_FLASH_NODE);

	/* Don't read flash while another flash operation is ongoing. */
	err = k_mutex_lock(&flash_state.lock, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);
	memcpy(data, (void *)offset, len);
	k_mutex_unlock(&flash_state.lock);

	return 0;
}

static int sdc_flash_write(const struct device *dev,
			      off_t offset,
			      const void *data,
			      size_t len)
{
	int err;

	if (!is_addr_valid(offset, len)) {
		return -EINVAL;
	}

	offset += DT_REG_ADDR(SOC_NV_FLASH_NODE);

	err = k_mutex_lock(&flash_state.lock, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);
	__ASSERT_NO_MSG(flash_state.op == FLASH_OP_NONE);
	flash_state.op = FLASH_OP_WRITE;
	flash_state.data = data;
	flash_state.addr = offset;
	flash_state.len = len;

	err = flash_op_execute_with_lock();
	if (!err) {
		err = k_sem_take(&flash_state.sync, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);
	}

	k_mutex_unlock(&flash_state.lock);
	return err;
}

static int sdc_flash_erase(const struct device *dev, off_t offset, size_t len)
{
	int err;

	/* Follows the behavior of soc_flash_nrf.c */
	if (!(is_page_aligned(offset) && is_page_aligned(len)) ||
	    !is_addr_valid(offset, len)) {
		return -EINVAL;
	}

	size_t page_count = len / nrfx_nvmc_flash_page_size_get();

	if (page_count == 0) {
		return 0;
	}

	offset += DT_REG_ADDR(SOC_NV_FLASH_NODE);

	err = k_mutex_lock(&flash_state.lock, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);
	__ASSERT_NO_MSG(flash_state.op == FLASH_OP_NONE);
	flash_state.op = FLASH_OP_ERASE;
	flash_state.addr = offset;
	flash_state.len = len;

	err = flash_op_execute_with_lock();
	if (!err) {
		err = k_sem_take(&flash_state.sync, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);
	}

	k_mutex_unlock(&flash_state.lock);
	return err;
}

static int sdc_flash_write_protection_set(const struct device *dev, bool enable)
{
	/* The BLE controller handles the write protection automatically. */
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;

static void sdc_flash_page_layout_get(
	const struct device *dev,
	const struct flash_pages_layout **layout,
	size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* defined(CONFIG_FLASH_PAGE_LAYOUT) */

static int flash_init(const struct device *dev)
{
	k_sem_init(&flash_state.sync, 0, 1);
	k_mutex_init(&flash_state.lock);

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_layout.pages_count = nrfx_nvmc_flash_page_count_get();
	dev_layout.pages_size = nrfx_nvmc_flash_page_size_get();
#endif

	return 0;
}

// TODO TORA: upmerge confirmation from Robert needed.
DEVICE_DT_DEFINE(SOC_NV_FLASH_CONTROLLER_NODE, flash_init,
		 device_pm_control_nop, NULL, NULL, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_api);
