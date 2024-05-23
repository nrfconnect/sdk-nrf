/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <suit_memory_layout.h>
#include <zephyr/drivers/flash/flash_simulator.h>

/* Definitions for SOC internal nonvolatile memory */
#if (DT_NODE_EXISTS(DT_NODELABEL(mram1x))) /* nrf54H20 */
#define INTERNAL_NVM_START (DT_REG_ADDR(DT_NODELABEL(mram1x)))
#define INTERNAL_NVM_SIZE  DT_REG_SIZE(DT_NODELABEL(mram1x))
#elif (DT_NODE_EXISTS(DT_NODELABEL(mram10))) /* nrf54H20 */
#define INTERNAL_NVM_START (DT_REG_ADDR(DT_NODELABEL(mram10)))
#define INTERNAL_NVM_SIZE  DT_REG_SIZE(DT_NODELABEL(mram10)) + DT_REG_SIZE(DT_NODELABEL(mram11))
#elif (DT_NODE_EXISTS(DT_NODELABEL(flash0))) /* nrf52 or flash simulator */
#define INTERNAL_NVM_START DT_REG_ADDR(DT_NODELABEL(flash0))
#define INTERNAL_NVM_SIZE  DT_REG_SIZE(DT_NODELABEL(flash0))
#elif (DT_NODE_EXISTS(DT_NODELABEL(rram0))) /* nrf54l15 */
#define INTERNAL_NVM_START DT_REG_ADDR(DT_NODELABEL(rram0))
#define INTERNAL_NVM_SIZE  DT_REG_SIZE(DT_NODELABEL(rram0))
#else
#error "No recognizable internal nvm nodes found."
#endif

#if IS_ENABLED(CONFIG_FLASH)
#if (DT_NODE_EXISTS(DT_CHOSEN(zephyr_flash_controller)))
#define SUIT_PLAT_INTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller))
#else
#define SUIT_PLAT_INTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_flash))
#endif
#else
#define SUIT_PLAT_INTERNAL_NVM_DEV NULL
#endif

#if (DT_NODE_EXISTS(DT_CHOSEN(extmem_device)))
#define EXTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(extmem_device))
#else
#define EXTERNAL_NVM_DEV NULL
#endif

struct nvm_area {
	uintptr_t na_start;
	size_t na_size;
	const struct device *na_fdev;
};

/* List of nonvolatile memory regions. */
static const struct nvm_area nvm_area_map[] = {
	{
		.na_start = INTERNAL_NVM_START,
		.na_size = INTERNAL_NVM_SIZE,
		.na_fdev = SUIT_PLAT_INTERNAL_NVM_DEV,
	},
	{
		.na_start = CONFIG_SUIT_MEMORY_LAYOUT_EXTMEM_ADDRESS_RANGE_START,
		.na_size = CONFIG_SUIT_MEMORY_LAYOUT_EXTMEM_ADDRESS_RANGE_SIZE,
		.na_fdev = EXTERNAL_NVM_DEV,
	},
};

struct ram_area {
	uintptr_t ra_start;
	size_t ra_size;
};

/* List of RAM memories accessible for ram_sink */
static struct ram_area ram_area_map[] = {
#if (DT_NODE_EXISTS(DT_NODELABEL(ram0x))) /* nrf54H20 */
	{
		.ra_start = DT_REG_ADDR(DT_NODELABEL(ram0x)),
		.ra_size = DT_REG_SIZE(DT_NODELABEL(ram0x)),
	},
#endif					  /* ram0x */
#if (DT_NODE_EXISTS(DT_NODELABEL(ram20))) /* nrf54H20 */
	{
		.ra_start = DT_REG_ADDR(DT_NODELABEL(ram20)),
		.ra_size = DT_REG_SIZE(DT_NODELABEL(ram20)),
	},
#endif							/* ram20 */
#if (DT_NODE_EXISTS(DT_NODELABEL(cpuapp_ram0x_region))) /* nrf54H20 */
	{
		.ra_start = DT_REG_ADDR(DT_NODELABEL(cpuapp_ram0x_region)),
		.ra_size = DT_REG_SIZE(DT_NODELABEL(cpuapp_ram0x_region)),
	},
#endif							/* ram0x */
#if (DT_NODE_EXISTS(DT_NODELABEL(shared_ram20_region))) /* nrf54H20 */
	{
		.ra_start = DT_REG_ADDR(DT_NODELABEL(shared_ram20_region)),
		.ra_size = DT_REG_SIZE(DT_NODELABEL(shared_ram20_region)),
	},
#endif					  /* ram20 */
#if (DT_NODE_EXISTS(DT_NODELABEL(sram0))) /* nrf52 or simulator */
	{
		.ra_start = DT_REG_ADDR(DT_NODELABEL(sram0)),
		.ra_size = DT_REG_SIZE(DT_NODELABEL(sram0)),
	},
#endif /* sram0 */
};

/* In case of tests on native_posix RAM emulation is used and visible below
 * mem_for_sim_ram is used as the buffer for emulation. It is here to allow not only
 * write but also read operations with emulated RAM. Address is translated to point to
 * the buffer. Size is taken from dts so it is required that the sram0 node is defined.
 */
#if (DT_NODE_EXISTS(DT_NODELABEL(sram0))) && defined(CONFIG_BOARD_NATIVE_POSIX)
#define SIM_RAM_SIZE DT_REG_SIZE(DT_NODELABEL(sram0))
#else
#define SIM_RAM_SIZE 0
#endif

static uint8_t mem_for_sim_ram[SIM_RAM_SIZE];

static uintptr_t area_address_get(const struct nvm_area *area)
{
	if (IS_ENABLED(CONFIG_FLASH_SIMULATOR)) {
		size_t size;

		return (uintptr_t)flash_simulator_get_memory(area->na_fdev, &size);
	}

	return area->na_start;
}

static bool address_in_area(uintptr_t address, uintptr_t area_start, size_t area_size)
{
	uintptr_t area_end = area_start + area_size;

	return area_start <= address && address < area_end;
}

static const struct nvm_area *find_area(uintptr_t address)
{
	for (int i = 0; i < ARRAY_SIZE(nvm_area_map); i++) {
		if (address_in_area(address, area_address_get(&nvm_area_map[i]),
				    nvm_area_map[i].na_size)) {
			return &nvm_area_map[i];
		}
	}

	return NULL;
}

static const struct ram_area *find_ram_area(uintptr_t address)
{
	for (int i = 0; i < ARRAY_SIZE(ram_area_map); i++) {
		if (address_in_area(address, (uintptr_t)ram_area_map[i].ra_start,
				    ram_area_map[i].ra_size)) {
			return &ram_area_map[i];
		}
	}

	return NULL;
}

static const struct nvm_area *find_area_by_device(const struct device *fdev)
{
	for (int i = 0; i < ARRAY_SIZE(nvm_area_map); i++) {
		if (nvm_area_map[i].na_fdev == fdev) {
			return &nvm_area_map[i];
		}
	}

	return NULL;
}

bool suit_memory_global_address_to_nvm_address(uintptr_t address, struct nvm_address *result)
{
	const struct nvm_area *area = find_area(address);

	if (result == NULL) {
		return false;
	}

	if (area == NULL) {
		return false;
	}

	result->fdev = area->na_fdev;
	result->offset = address - area_address_get(area);
	return true;
}

bool suit_memory_nvm_address_to_global_address(struct nvm_address *address, uintptr_t *result)
{
	if (address == NULL || result == NULL) {
		return false;
	}

	const struct nvm_area *area = find_area_by_device(address->fdev);

	if (area == NULL) {
		return false;
	}

	*result = area_address_get(area) + address->offset;

	return true;
}

bool suit_memory_global_address_is_in_nvm(uintptr_t address)
{
	return find_area(address) != NULL;
}

bool suit_memory_global_address_range_is_in_nvm(uintptr_t address, size_t size)
{
	/* Zero-sized ranges are treated as if they were one byte in size. */
	size = MAX(1, size);

	const struct nvm_area *start = find_area(address);
	const struct nvm_area *end = find_area(address + size - 1);

	return start != NULL && start == end;
}

bool suit_memory_global_address_is_in_ram(uintptr_t address)
{
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX)) {
		return !suit_memory_global_address_is_in_nvm(address);
	}

	return find_ram_area(address) != NULL;
}

uintptr_t suit_memory_global_address_to_ram_address(uintptr_t address)
{
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX) && DT_NODE_EXISTS(DT_NODELABEL(sram0))) {
		const struct ram_area *area = find_ram_area(address);

		if (area) {
			uintptr_t offset = address - area->ra_start;

			return (uintptr_t)mem_for_sim_ram + offset;
		}
	}

	return address;
}

bool suit_memory_global_address_range_is_in_ram(uintptr_t address, size_t size)
{
	/* Zero-sized ranges are treated as if they were one byte in size. */
	size = MAX(1, size);

	if (IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX)) {
		return !suit_memory_global_address_range_is_in_nvm(address, size);
	}

	const struct ram_area *start = find_ram_area(address);
	const struct ram_area *end = find_ram_area(address + size - 1);

	return start != NULL && start == end;
}

bool suit_memory_global_address_is_directly_readable(uintptr_t address)
{
	return !suit_memory_global_address_is_in_external_memory(address);
}

bool suit_memory_global_address_is_in_external_memory(uintptr_t address)
{
	return address_in_area(address, CONFIG_SUIT_MEMORY_LAYOUT_EXTMEM_ADDRESS_RANGE_START,
			       CONFIG_SUIT_MEMORY_LAYOUT_EXTMEM_ADDRESS_RANGE_SIZE);
}

bool suit_memory_global_address_range_is_in_external_memory(uintptr_t address, size_t size)
{
	/* Zero-sized ranges are treated as if they were one byte in size. */
	size = MAX(1, size);

	uintptr_t end_addr = address + size - 1;

	return suit_memory_global_address_is_in_external_memory(address) &&
	       suit_memory_global_address_is_in_external_memory(end_addr) &&
	       end_addr >= address; /* Overflow check. */
}

bool suit_memory_global_address_to_external_memory_offset(uintptr_t address, uintptr_t *offset)
{
	if (!suit_memory_global_address_is_in_external_memory(address)) {
		return false;
	}

	if (offset == NULL) {
		return false;
	}

	*offset = address - CONFIG_SUIT_MEMORY_LAYOUT_EXTMEM_ADDRESS_RANGE_START;
	return true;
}

const struct device *suit_memory_external_memory_device_get(void)
{
	return EXTERNAL_NVM_DEV;
}
