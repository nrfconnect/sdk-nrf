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

LOG_MODULE_REGISTER(flash_ipuc, CONFIG_FLASH_IPUC_LOG_LEVEL);

#define FLASH_WRITE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), write_block_size)
#define WRITE_BLOCK_SIZE       FLASH_WRITE_BLOCK_SIZE
#define ERASE_VALUE	       0xff

BUILD_ASSERT(WRITE_BLOCK_SIZE > 0, "zephyr,flash: write_block_size expected to be non-zero");

#define IPUC_MEM_COMPONENT_MAX_SIZE 32

struct ipuc_context {
	struct zcbor_string component_id;
	uint8_t component_id_buf[IPUC_MEM_COMPONENT_MAX_SIZE];
	uintptr_t address;
	size_t size;
	bool read_access;
};

#define DEFINE_NRF_IPUC_DATA(x, _)                                                                 \
	static struct ipuc_context ipuc_context_data_##x = {                                       \
		.component_id = {NULL, 0},                                                         \
		.address = 0,                                                                      \
		.size = 0,                                                                         \
	}

#define DEFINE_NRF_IPUC(x, _)                                                                      \
	DEVICE_DEFINE(flash_nrf_ipuc_##x, "nrf_ipuc_" #x, NULL, /* init */                         \
		      NULL,					/* pm */                           \
		      &ipuc_context_data_##x, NULL,		/* config */                       \
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &nrf_ipuc_api)

#define DEFINE_NRF_IPUC_REF(x, _) &__device_flash_nrf_ipuc_##x

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
	struct ipuc_context *ctx = NULL;
	bool last_chunk = false;

	if (dev == NULL) {
		return -EINVAL;
	}

	ctx = (struct ipuc_context *)dev->data;

	if (ctx->component_id.value == NULL) {
		return -EBADF;
	}

	LOG_DBG("write: %p:%zu", (void *)offset, len);

	if (len == 0) {
		last_chunk = true;
	}

	if (offset + len > ctx->size) {
		return -ENOMEM;
	}

	suit_plat_err_t plat_ret =
		suit_ipuc_write(&ctx->component_id, offset, (uintptr_t)data, len, last_chunk);

	if (plat_ret != SUIT_PLAT_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int nrf_ipuc_erase(const struct device *dev, off_t offset, size_t size)
{
	static uint8_t erase_block[WRITE_BLOCK_SIZE] = {ERASE_VALUE};
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct ipuc_context *ctx = NULL;

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

	LOG_DBG("erase: %p:%zu", (void *)offset, size);
	while (size > 0) {
		plat_ret = suit_ipuc_write(&ctx->component_id, offset, (uintptr_t)erase_block,
					   sizeof(erase_block), false);
		offset += sizeof(erase_block);
		size -= sizeof(erase_block);
	}

	if (plat_ret != SUIT_PLAT_SUCCESS) {
		return -EIO;
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

static const struct flash_driver_api nrf_ipuc_api = {
	.read = nrf_ipuc_read,
	.write = nrf_ipuc_write,
	.erase = nrf_ipuc_erase,
	.get_parameters = nrf_ipuc_get_parameters,
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
		LOG_ERR("Maximum number of IPUCs reached");
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

			plat_err = suit_ipuc_write_setup(&ctx->component_id, encryption_info,
							 compression_info);
			if (plat_err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to setup IPUC %d write: %d", i, plat_err);
				break;
			}

			LOG_INF("IPUC for address range (0x%lx, 0x%x) created", ctx->address,
				ctx->size);
		}

		if (encryption_info == NULL) {
			ctx->read_access = read_access_check(role);
		} else {
			ctx->read_access = false;
		}

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

	dev = get_free_dev();
	if ((dev == NULL) || (ipuc_address == NULL) || (ipuc_size == NULL)) {
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
		plat_err = suit_ipuc_write_setup(&ctx->component_id, NULL, NULL);
		if (plat_err != SUIT_PLAT_SUCCESS) {
			flash_ipuc_release(dev);
			return NULL;
		}

		LOG_INF("Cache IPUC at idx %d for address range (0x%lx, 0x%x) created", i_max,
			ctx->address, ctx->size);
	}

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
		dev = (struct device *)ipuc_devs[i];
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
