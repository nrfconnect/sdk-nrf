/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/drivers/flash.h>
#include <drivers/flash/flash_ipuc.h>
#include <suit_ipuc.h>
#include <suit_plat_decode_util.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/storage/flash_map.h>
#ifdef CONFIG_FLASH_SIMULATOR
#include <zephyr/drivers/flash/flash_simulator.h>
#endif /* CONFIG_FLASH_SIMULATOR */
#include "soc_flash_nrf.h"

/* __ALIGNED macro is not defined on NATIVE POSIX. This platform uses __aligned macro. */
#ifndef __ALIGNED
#ifdef __aligned
#define __ALIGNED __aligned
#endif
#endif

#ifdef CONFIG_DCACHE_LINE_SIZE
#define CACHE_ALIGNMENT CONFIG_DCACHE_LINE_SIZE
#else
#define CACHE_ALIGNMENT 4
#endif

#define FLASH_WRITE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), write_block_size)
#define WRITE_BLOCK_SIZE       FLASH_WRITE_BLOCK_SIZE
#define ERASE_VALUE	       0xff

BUILD_ASSERT(WRITE_BLOCK_SIZE > 0, "zephyr,flash: write_block_size expected to be non-zero");

#define IPUC_MEM_COMPONENT_MAX_SIZE 32
#define FLASH_IPUC_IMG_MAX	    512
#define FLASH_IPUC_IMG_PREFIX	    dfu_target_img_

/* The main structure that bind image IDs to the address and size, that identifies one of the
 * available IPUCs.
 */
struct flash_ipuc_img {
	uintptr_t address;
	size_t size;
	uint8_t id;
	struct device *fdev;
};

/* The partition offset is relative to the parent nodes.
 * In most cases, the direct parent is a group of partitions and the grandparent hold the offset
 * in the absolute (dereferenceable) address space.
 */
#define GPARENT_OFFSET(label) (DT_REG_ADDR(DT_GPARENT(DT_NODELABEL(label))))

/* Get the absolute address of a fixed partition, identified by a label. */
#define FIXED_PARTITION_ADDRESS(label) (GPARENT_OFFSET(label) + FIXED_PARTITION_OFFSET(label))

/* Initialize flash_ipuc_img structure, based on the partition label and image ID number. */
#define PARTITION_INIT(index, label)                                                               \
	{                                                                                          \
		.address = FIXED_PARTITION_ADDRESS(label),                                         \
		.size = FIXED_PARTITION_SIZE(label),                                               \
		.id = index,                                                                       \
		.fdev = NULL,                                                                      \
	},

/* Verify the dfu_target_img_<n> index: it should not be less than one to avoid
 * conflicts with DFU partition (envelope) identification schemes.
 */
#define INDEX_IN_RAGE(index) IN_RANGE(index, 1, (FLASH_IPUC_IMG_MAX - 1))
#define PARTITION_INIT_IF_INDEX_OK(label, index)                                                   \
	IF_ENABLED(UTIL_BOOL(INDEX_IN_RANGE(index)), (PARTITION_INIT(index, label)))

/* Only partitions with a parent node compatible with "fixed-paritions" should
 * be taken into account.
 */
#define PARTITION_DEFINE_(index, label)                                                            \
	IF_ENABLED(FIXED_PARTITION_EXISTS(label), (PARTITION_INIT_IF_INDEX_OK(label, index)))

/* Concatenate prefix with the image ID and return the structure initialization code snippet. */
#define PARTITION_DEFINE(index, prefix) PARTITION_DEFINE_(index, prefix##index)

/* Define the global list of bindings between partition address, size and image ID number. */
static struct flash_ipuc_img ipuc_imgs[] = {
	LISTIFY(FLASH_IPUC_IMG_MAX, PARTITION_DEFINE, (), FLASH_IPUC_IMG_PREFIX)};

/* The main IPUC context structure. */
struct ipuc_context {
	struct zcbor_string component_id;
	uint8_t component_id_buf[IPUC_MEM_COMPONENT_MAX_SIZE];
	uintptr_t address;
	size_t size;
	bool read_access;
	bool setup_pending;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout pages_layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

#define DEFINE_NRF_IPUC_DATA(x, _)                                                                 \
	static struct ipuc_context ipuc_context_data_##x = {                                       \
		.component_id = {NULL, 0},                                                         \
		.address = 0,                                                                      \
		.size = 0,                                                                         \
		.setup_pending = false,                                                            \
		.pages_layout = {0},                                                               \
	}

#define DEFINE_NRF_IPUC(x, _)                                                                      \
	DEVICE_DEFINE(flash_nrf_ipuc_##x, "nrf_ipuc_" #x, NULL, /* init */                         \
		      NULL,					/* pm */                           \
		      &ipuc_context_data_##x, NULL,		/* config */                       \
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &nrf_ipuc_api)

#define DEFINE_NRF_IPUC_REF(x, _) &__device_flash_nrf_ipuc_##x

#define MAX_SINGLE_BYTE_WRITE_TIME_US 4
#define MAX_SSF_IPUC_WRITE_COMMUNICATION_TIME_US 1200

LOG_MODULE_REGISTER(flash_ipuc, CONFIG_FLASH_IPUC_LOG_LEVEL);

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_RPC_HOST)

struct ipuc_write_chunk_data {
	struct zcbor_string *component_id;
	size_t offset;
	uintptr_t buffer;
	size_t chunk_size;
	bool last_chunk;
};

static int ipuc_write_op(void *context)
{
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct ipuc_write_chunk_data *data = (struct ipuc_write_chunk_data *)context;

	plat_ret = suit_ipuc_write(data->component_id, data->offset, data->buffer,
				   data->chunk_size, data->last_chunk);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Write error: %d", plat_ret);
		return -EIO;
	}

	return FLASH_OP_DONE;
}

static int ipuc_synchronous_write(struct zcbor_string *component_id, size_t offset,
				  uintptr_t buffer, size_t chunk_size, bool last_chunk)
{
	struct ipuc_write_chunk_data data = {
		.component_id = component_id,
		.offset = offset,
		.buffer = buffer,
		.chunk_size = chunk_size,
		.last_chunk = last_chunk,
	};

	struct flash_op_desc flash_op_desc = {
		.handler = ipuc_write_op,
		/* Currently data is passed as context despite of the type mismatch,
		 * as nrf_flash_sync_exe simply passes the context without any operations.

		 * However, a better approach might be creating a global context variable for
		 * the flash_ipuc module which would store all of the data.
		 * This would not only remove the issue with the type mismatch, it would
		 * also allow the driver to (for example) handle different addresses
		 * in a different way, for example not synchronyze if the NVM memory is
		 * not in the same bank as the memory used by the radio protocols.
		 */
		.context = (void *) &data
	};

	/* We are only accounting for SSF communication time when sending the request,
	 * the response time does not matter, as the flash write has already finished
	 * by then.
	 */
	uint32_t requested_timeslot_duration = MAX_SINGLE_BYTE_WRITE_TIME_US * chunk_size +
					       MAX_SSF_IPUC_WRITE_COMMUNICATION_TIME_US;


	nrf_flash_sync_set_context(requested_timeslot_duration);
	return nrf_flash_sync_exe(&flash_op_desc);
}
#endif /* defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_RPC_HOST) */

static int ipuc_write(struct zcbor_string *component_id, size_t offset,
		      uintptr_t buffer, size_t chunk_size, bool last_chunk)
{
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_RPC_HOST)
	if (nrf_flash_sync_is_required()) {
		/* The driver internally always ensures there is no reinitialization */
		nrf_flash_sync_init();
		return ipuc_synchronous_write(component_id, offset, buffer, chunk_size, last_chunk);
	}
#endif
	plat_ret = suit_ipuc_write(component_id, offset, buffer, chunk_size, last_chunk);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Data write (%p, 0x%x) error: %d", (void *) buffer, chunk_size, plat_ret);
		return -EIO;
	}

	return 0;
}


static int nrf_ipuc_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct ipuc_context *ctx = NULL;

	if ((dev == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	ctx = (struct ipuc_context *)dev->data;
	if (!ctx->read_access) {
		return -EACCES;
	}

	if (ctx->component_id.value == NULL) {
		return -EBADF;
	}

	LOG_DBG("read: %p:%zu", (void *)offset, len);

	if (offset + len > ctx->size) {
		return -ENOMEM;
	}

	memcpy(data, (void *)(ctx->address + offset), len);

	return 0;
}

static int nrf_ipuc_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	uint8_t unaligned_data_buf[CACHE_ALIGNMENT] __ALIGNED(CACHE_ALIGNMENT) = {0};
	size_t unaligned_len = MIN(CACHE_ALIGNMENT - (((uintptr_t)data) % CACHE_ALIGNMENT), len);
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct ipuc_context *ctx = NULL;
	int ret = 0;

	if (dev == NULL) {
		return -EINVAL;
	}

	ctx = (struct ipuc_context *)dev->data;

	if (ctx->component_id.value == NULL) {
		return -EBADF;
	}

	if (offset + len > ctx->size) {
		return -ENOMEM;
	}

	if (ctx->setup_pending) {
		LOG_DBG("setup: %p:%zu", (void *)ctx->address, ctx->size);

		plat_ret = suit_ipuc_write_setup(&ctx->component_id, NULL, NULL);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to setup IPUC for writing: %d", plat_ret);
			return -EIO;
		}

		ctx->setup_pending = false;
	}

	LOG_DBG("write: %p:%zu", (void *)offset, len);

	if (len == 0) {
		plat_ret = suit_ipuc_write(&ctx->component_id, offset, 0, 0, true);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Last write (flush) error: %d", plat_ret);
			return -EIO;
		}

		return 0;
	}

	/* If the data buffer is not aligned to the cache lines:
	 *  - copy the unaligned part into stack-based aligned buffer
	 *  - write the internal buffer
	 *  - skip the unaligned bytes of the input buffer
	 */
	if (unaligned_len != CACHE_ALIGNMENT) {
		/* Optimize: Use a single write call if all bytes can be transferred using
		 * stack-based aligned buffer.
		 */
		if (len <= ARRAY_SIZE(unaligned_data_buf)) {
			unaligned_len = len;
		}

		memcpy(unaligned_data_buf, data, unaligned_len);

		LOG_DBG("align: %p:%zu", (void *)data, unaligned_len);

		ret = sys_cache_data_flush_range((void *)unaligned_data_buf,
						 sizeof(unaligned_data_buf));
		if (ret != 0 && ret != -EAGAIN && ret != -ENOTSUP) {
			LOG_ERR("Failed to flush cache buffer range (%p, 0x%x): %d",
				(void *)unaligned_data_buf, sizeof(unaligned_data_buf), ret);
			return -EIO;
		}

		ret = ipuc_write(&ctx->component_id, offset,
				 (uintptr_t)unaligned_data_buf, unaligned_len, false);
		if (ret != 0) {
			LOG_ERR("Unaligned data write (%p, 0x%x) error: %d",
				(void *)unaligned_data_buf, unaligned_len, ret);
			return ret;
		}

		offset += unaligned_len;
		len -= unaligned_len;
		data = (void *)((uintptr_t)data + unaligned_len);
	}

	/* If no more (aligned) bytes left - return. */
	if (len == 0) {
		return 0;
	}

	/* Write (now aligned) data buffer. */
	ret = sys_cache_data_flush_range((void *)data, len);
	if (ret != 0 && ret != -EAGAIN && ret != -ENOTSUP) {
		LOG_ERR("Failed to flush cache memory range (%p, 0x%x): %d", data, len, ret);
		return -EIO;
	}

	ret = ipuc_write(&ctx->component_id, offset, (uintptr_t)data, len, false);
	if (ret != 0) {
		LOG_ERR("Aligned data write (%p, 0x%x) error: %d", data, len, ret);
		return ret;
	}

	return 0;
}

static int nrf_ipuc_erase(const struct device *dev, off_t offset, size_t size)
{
	const uint8_t erase_block[WRITE_BLOCK_SIZE] __ALIGNED(CACHE_ALIGNMENT) = {
		[0 ... WRITE_BLOCK_SIZE - 1] = ERASE_VALUE};
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct ipuc_context *ctx = NULL;
	size_t bytes_erased = 0;

	if (dev == NULL) {
		return -EINVAL;
	}

	ctx = (struct ipuc_context *)dev->data;

	if (ctx->component_id.value == NULL) {
		return -EBADF;
	}

	if (offset + size > ctx->size) {
		return -ENOMEM;
	}

	int ret = sys_cache_data_flush_range((void *)erase_block, sizeof(erase_block));

	if (ret != 0 && ret != -EAGAIN && ret != -ENOTSUP) {
		LOG_ERR("Failed to flush cache range (%p, 0x%x): %d", erase_block,
			sizeof(erase_block), ret);
		return -EIO;
	}

	if (ctx->setup_pending) {
		/* Any write sets up the IPUC, which erases the whole area, so we may return here
		 * immediately.
		 */
		return 0;
	}

	LOG_DBG("erase: %p:%zu", (void *)offset, size);

	if (size == ctx->size) {
		/* Optimize: Use write_setup() to erase the whole area with a single SSF IPC call.
		 */
		LOG_DBG("setup: %p:%zu", (void *)ctx->address, ctx->size);

		plat_ret = suit_ipuc_write_setup(&ctx->component_id, NULL, NULL);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to erase IPUC through write setup: %d", plat_ret);
			return -EIO;
		}

		return 0;
	}

	while (size > 0) {
		bytes_erased = MIN(sizeof(erase_block), size);
		ret = ipuc_write(&ctx->component_id, offset, (uintptr_t)erase_block,
				 bytes_erased, false);
		if (ret != 0) {
			LOG_ERR("Failed to erase IPUC (%p:%zu): %d",
				(void *)(ctx->address + offset), bytes_erased, ret);
			return -EIO;
		}

		offset += bytes_erased;
		size -= bytes_erased;

		/* The erase operation generates a lot of SSF requests.
		 * It is necessary to give some time SDFW, so it is able to feed the watchdog.
		 */
		if ((offset % 0x10000 == 0) && (size > 0)) {
			k_sleep(K_MSEC(2));
		}
	}

	return 0;
}

static const struct flash_parameters *nrf_ipuc_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		/* Pretend to support single-byte access to avoid read/update/write cycles
		 * in case of unaligned write requests.
		 */
		.write_block_size = 1,
		.erase_value = ERASE_VALUE,
		.caps = {.no_explicit_erase = true},
	};

	return &parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void nrf_ipuc_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	struct ipuc_context *ctx = NULL;

	if (dev == NULL) {
		return;
	}

	ctx = (struct ipuc_context *)dev->data;

	*layout = &ctx->pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api nrf_ipuc_api = {
	.read = nrf_ipuc_read,
	.write = nrf_ipuc_write,
	.erase = nrf_ipuc_erase,
	.get_parameters = nrf_ipuc_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = nrf_ipuc_page_layout,
#endif
};

static bool read_access_check(suit_manifest_role_t role)
{
	switch (role) {
#ifdef CONFIG_SOC_NRF54H20_CPUAPP
	case SUIT_MANIFEST_APP_ROOT:
	case SUIT_MANIFEST_APP_RECOVERY:
	case SUIT_MANIFEST_APP_LOCAL_1:
	case SUIT_MANIFEST_APP_LOCAL_2:
	case SUIT_MANIFEST_APP_LOCAL_3:
		return true;
#endif /* CONFIG_SOC_NRF54H20_CPUAPP */
#ifdef CONFIG_SOC_NRF54H20_CPURAD
	case SUIT_MANIFEST_RAD_RECOVERY:
	case SUIT_MANIFEST_RAD_LOCAL_1:
	case SUIT_MANIFEST_RAD_LOCAL_2:
		return true;
#endif /* CONFIG_SOC_NRF54H20_CPURAD */
	default:
		break;
	}

	return false;
}

LISTIFY(CONFIG_FLASH_IPUC_COUNT, DEFINE_NRF_IPUC_DATA, (;), _);
LISTIFY(CONFIG_FLASH_IPUC_COUNT, DEFINE_NRF_IPUC, (;), _);

/* clang-format off */
static const struct device *ipuc_devs[CONFIG_FLASH_IPUC_COUNT] = {
	LISTIFY(CONFIG_FLASH_IPUC_COUNT, DEFINE_NRF_IPUC_REF, (,), _)};
/* clang-format on */

static struct device *get_free_dev(void)
{
	struct ipuc_context *ctx = NULL;
	struct device *dev = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(ipuc_devs); i++) {
		dev = (struct device *)ipuc_devs[i];
		ctx = (struct ipuc_context *)dev->data;
		if (ctx->component_id.value != NULL) {
			continue;
		} else {
			ctx->component_id.value = ctx->component_id_buf;
			ctx->component_id.len = 0;

			return dev;
		}
	}

	return NULL;
}

void flash_ipuc_release(struct device *dev)
{
	struct ipuc_context *ctx;

	if (dev == NULL) {
		return;
	}

	ctx = (struct ipuc_context *)dev->data;
	if (ctx->component_id.value != NULL) {
		ctx->component_id.value = NULL;
	}
}

static struct device *flash_component_ipuc(struct zcbor_string *component_id,
					   struct zcbor_string *encryption_info,
					   struct zcbor_string *compression_info, bool dry_run)
{
	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;
	suit_plat_err_t plat_err = SUIT_PLAT_SUCCESS;
	struct ipuc_context *ctx = NULL;
	struct device *dev = NULL;
	size_t count = 0;

	if ((component_id == NULL) || (component_id->value == NULL)) {
		LOG_ERR("Invalid component ID arg value");
		return NULL;
	}

	plat_err = suit_ipuc_get_count(&count);
	if (plat_err != SUIT_PLAT_SUCCESS) {
		LOG_WRN("Unable to read IPUC count");
		return NULL;
	}

	dev = get_free_dev();
	if (dev == NULL) {
		LOG_WRN("Unable to allocate new component IPUC device - no free instances");
		return NULL;
	}

	ctx = (struct ipuc_context *)dev->data;

	for (size_t i = 0; i < count; i++) {
		ctx->component_id.len = sizeof(ctx->component_id_buf);
		plat_err = suit_ipuc_get_info(i, &ctx->component_id, &role);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			LOG_WRN("Failed to read IPUC %d info: %d", i, plat_err);
			break;
		}

		/* Requested component ID must be equal to the IPUC component. */
		if (ctx->component_id.len != component_id->len) {
			continue;
		}
		if (memcmp(ctx->component_id.value, component_id->value, ctx->component_id.len) !=
		    0) {
			continue;
		}

		if (!dry_run) {
			uint8_t cpu_id = 0;

			plat_err = suit_plat_decode_component_id(
				&ctx->component_id, &cpu_id, (intptr_t *)&ctx->address, &ctx->size);
			if (plat_err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to decode IPUC %d component ID: %d", i, plat_err);
				continue;
			}

			LOG_INF("IPUC for address range (0x%lx, 0x%x) created", ctx->address,
				ctx->size);
		}

		if (encryption_info == NULL) {
			ctx->read_access = read_access_check(role);
		} else {
			ctx->read_access = false;
		}

		ctx->setup_pending = true;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
		ctx->pages_layout.pages_count = 1;
		ctx->pages_layout.pages_size = ctx->size;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

		return dev;
	}

	flash_ipuc_release(dev);

	return NULL;
}

struct device *flash_component_ipuc_create(struct zcbor_string *component_id,
					   struct zcbor_string *encryption_info,
					   struct zcbor_string *compression_info)
{
	return flash_component_ipuc(component_id, encryption_info, compression_info, false);
}

bool flash_component_ipuc_check(struct zcbor_string *component_id)
{
	struct device *dev = flash_component_ipuc(component_id, NULL, NULL, true);

	if (dev != NULL) {
		flash_ipuc_release(dev);

		return true;
	}

	return false;
}

static struct device *flash_cache_ipuc(uintptr_t min_address, uintptr_t *ipuc_address,
				       size_t *ipuc_size, bool dry_run)
{
	suit_plat_err_t plat_err = SUIT_PLAT_SUCCESS;
	size_t count = 0;
	struct ipuc_context *ctx = NULL;
	struct device *dev = NULL;
	suit_manifest_role_t role;
	intptr_t address = 0;
	size_t size = 0;
	uint8_t cpu_id = 0;
	size_t size_max = 0;
	size_t i_max = 0;
	uintptr_t range_addr;

	if ((ipuc_address == NULL) || (ipuc_size == NULL)) {
		LOG_WRN("Invalid input arguments: (0x%lx, 0x%lx)", (uintptr_t)ipuc_address,
			(uintptr_t)ipuc_size);
		return NULL;
	}

	dev = get_free_dev();
	if (dev == NULL) {
		LOG_WRN("Unable to allocate new cache IPUC device - no free instances");
		return NULL;
	}

	plat_err = suit_ipuc_get_count(&count);
	if (plat_err != SUIT_PLAT_SUCCESS) {
		LOG_WRN("Unable to read IPUC count");
		flash_ipuc_release(dev);
		return NULL;
	}

	ctx = (struct ipuc_context *)dev->data;

	for (size_t i = 0; i < count; i++) {
		ctx->component_id.len = sizeof(ctx->component_id_buf);
		plat_err = suit_ipuc_get_info(i, &ctx->component_id, &role);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			break;
		}

		ctx->read_access = read_access_check(role);
		if (!ctx->read_access) {
			continue;
		}

		plat_err =
			suit_plat_decode_component_id(&ctx->component_id, &cpu_id, &address, &size);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			LOG_INF("Failed to decode IPUC %d component ID: %d", i, plat_err);
			continue;
		}

		if (min_address < address) {
			range_addr = address;
		} else {
			range_addr = min_address;
		}

		if (range_addr < address + size) {
			if ((address + size - range_addr) > size_max) {
				size_max = address + size - range_addr;
				i_max = i;
			}
		}
	}

	if (size_max > 0) {
		plat_err = suit_ipuc_get_info(i_max, &ctx->component_id, &role);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			flash_ipuc_release(dev);
			return NULL;
		}

		plat_err = suit_plat_decode_component_id(&ctx->component_id, &cpu_id,
							 (intptr_t *)&ctx->address, &ctx->size);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			flash_ipuc_release(dev);
			return NULL;
		}
	} else {
		flash_ipuc_release(dev);
		return NULL;
	}

	if (!dry_run) {
		LOG_INF("Cache IPUC at idx %d for address range (0x%lx, 0x%x) created", i_max,
			ctx->address, ctx->size);
	}

	ctx->setup_pending = true;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	ctx->pages_layout.pages_count = 1;
	ctx->pages_layout.pages_size = ctx->size;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
	*ipuc_address = ctx->address;
	*ipuc_size = ctx->size;

	return dev;
}

struct device *flash_cache_ipuc_create(uintptr_t min_address, uintptr_t *ipuc_address,
				       size_t *ipuc_size)
{
	return flash_cache_ipuc(min_address, ipuc_address, ipuc_size, false);
}

bool flash_cache_ipuc_check(uintptr_t min_address, uintptr_t *ipuc_address, size_t *ipuc_size)
{
	struct device *dev = flash_cache_ipuc(min_address, ipuc_address, ipuc_size, true);

	if (dev != NULL) {
		flash_ipuc_release(dev);

		return true;
	}

	return false;
}

struct device *flash_ipuc_find(uintptr_t address, size_t size, uintptr_t *ipuc_address,
			       size_t *ipuc_size)
{
	struct ipuc_context *ctx = NULL;
	struct device *dev = NULL;

	if ((ipuc_address == NULL) || (ipuc_size == NULL)) {
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ipuc_devs); i++) {
		/* Due to the existence of permanent IPUCs (i.e. in the DFU cache RW module)
		 * IPUC list must be searched in the FILO order, so any temporary, newly
		 * created IPUC driver is returned by an immediate call to the find API.
		 */
		dev = (struct device *)ipuc_devs[ARRAY_SIZE(ipuc_devs) - 1 - i];
		ctx = (struct ipuc_context *)dev->data;
		if (ctx->component_id.value == NULL) {
			continue;
		}

		if ((ctx->address <= address) && (ctx->address + ctx->size >= address + size)) {
			*ipuc_address = ctx->address;
			*ipuc_size = ctx->size;

			return dev;
		}
	}

	return NULL;
}

bool flash_ipuc_setup_pending(const struct device *dev)
{
	struct ipuc_context *ctx;

	if (dev == NULL) {
		return false;
	}

	ctx = (struct ipuc_context *)dev->data;
	if (ctx->component_id.value != NULL) {
		return ctx->setup_pending;
	}

	return false;
}

struct device *flash_image_ipuc_create(size_t id, struct zcbor_string *encryption_info,
				       struct zcbor_string *compression_info,
				       uintptr_t *ipuc_address, size_t *ipuc_size)
{
	suit_manifest_role_t role = SUIT_MANIFEST_UNKNOWN;
	suit_plat_err_t plat_err = SUIT_PLAT_SUCCESS;
	struct ipuc_context *ctx = NULL;
	struct device *dev = NULL;
	size_t count = 0;
	uint8_t cpu_id = 0;
	size_t idx = 0;
#ifdef CONFIG_FLASH_SIMULATOR
	size_t f_size = 0;
	uintptr_t base_address = (uintptr_t)flash_simulator_get_memory(NULL, &f_size);
#else
	uintptr_t base_address = 0;
#endif

	if ((ipuc_address == NULL) || (ipuc_size == NULL)) {
		LOG_WRN("Invalid image input arguments: (0x%lx, 0x%lx)", (uintptr_t)ipuc_address,
			(uintptr_t)ipuc_size);
		return NULL;
	}

	for (idx = 0; idx < ARRAY_SIZE(ipuc_imgs); idx++) {
		if (ipuc_imgs[idx].id == id) {
			LOG_INF("Found IPUC image %d: (0x%lx, 0x%x)", ipuc_imgs[idx].id,
				ipuc_imgs[idx].address, ipuc_imgs[idx].size);
			break;
		}
	}

	if (idx >= ARRAY_SIZE(ipuc_imgs)) {
		LOG_INF("IPUC image with ID %d not found", id);
		return NULL;
	}

	if (ipuc_imgs[idx].fdev != NULL) {
		LOG_WRN("IPUC for image %d already created", id);
		return NULL;
	}

	plat_err = suit_ipuc_get_count(&count);
	if (plat_err != SUIT_PLAT_SUCCESS) {
		LOG_WRN("Unable to read IPUC count");
		return NULL;
	}

	dev = get_free_dev();
	if (dev == NULL) {
		LOG_WRN("Unable to allocate new image IPUC device - no free instances");
		return NULL;
	}

	ctx = (struct ipuc_context *)dev->data;

	for (size_t i = 0; i < count; i++) {
		ctx->component_id.len = sizeof(ctx->component_id_buf);
		plat_err = suit_ipuc_get_info(i, &ctx->component_id, &role);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			LOG_WRN("Failed to read IPUC %d info: %d", i, plat_err);
			break;
		}

		plat_err = suit_plat_decode_component_id(&ctx->component_id, &cpu_id,
							 (intptr_t *)&ctx->address, &ctx->size);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to decode IPUC %d component ID: %d", i, plat_err);
			continue;
		}

		if ((ctx->address != base_address + ipuc_imgs[idx].address) ||
		    (ctx->size != ipuc_imgs[idx].size)) {
			continue;
		}

		LOG_INF("IPUC for address range (0x%lx, 0x%x) created", ctx->address, ctx->size);

		if (encryption_info == NULL) {
			ctx->read_access = read_access_check(role);
		} else {
			ctx->read_access = false;
		}

		ipuc_imgs[idx].fdev = dev;
		ctx->setup_pending = true;
		*ipuc_address = ctx->address;
		*ipuc_size = ctx->size;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
		ctx->pages_layout.pages_count = 1;
		ctx->pages_layout.pages_size = ctx->size;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

		return dev;
	}

	flash_ipuc_release(dev);

	return NULL;
}

void flash_image_ipuc_release(size_t id)
{
	for (size_t i = 0; i < ARRAY_SIZE(ipuc_imgs); i++) {
		if (ipuc_imgs[i].id == id) {
			/* Flush any remaining data. */
			(void)nrf_ipuc_write(ipuc_imgs[i].fdev, 0, NULL, 0);
			flash_ipuc_release(ipuc_imgs[i].fdev);
			ipuc_imgs[i].fdev = NULL;
			break;
		}
	}
}
