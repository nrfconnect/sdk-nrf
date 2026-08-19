/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing MAC address provisioning definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_ENTROPY_GENERATOR)
#include <zephyr/random/random.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

#include <common/util.h>
#include <common/mac_addr.h>

#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ficr))
#error "The nRF71 Wi-Fi driver requires an enabled FICR node in the devicetree"
#endif

#define XICR_FICR_BASE DT_REG_ADDR(DT_NODELABEL(ficr))

/* UICR is hardware fixed to the secure domain, so the node is not present in
 * the non-secure address map. Non-secure builds fall back to FICR.
 */
#define XICR_UICR_PRESENT DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uicr))

#if XICR_UICR_PRESENT
#define XICR_UICR_BASE DT_REG_ADDR(DT_NODELABEL(uicr))
#endif

/* MACADDR[n] offsets within the xICR register blocks, as documented in the
 * nRF7120 Product Specification. The MDK for the nRF7120 does not expose these
 * registers yet.
 * TODO: use NRF_FICR->MACADDR[n] / NRF_UICR->MACADDR[n] once available.
 */
#define XICR_FICR_MACADDR_OFFSET 0xC00U
#define XICR_UICR_MACADDR_OFFSET 0x420U
#define XICR_MACADDR_SLOT_SIZE   0x8U
#define XICR_MACADDR_LOW_OFFSET  0x0U
#define XICR_MACADDR_HIGH_OFFSET 0x4U

/* Both MACADDR[n].LOW and MACADDR[n].HIGH carry the address in the ADDR field,
 * bits [23:0]. The upper byte is unused.
 */
#define XICR_MACADDR_ADDR_MASK 0xFFFFFFU

/* FICR.INFO.DEVICEID[n], used to seed a locally administered address. */
#define XICR_FICR_DEVICEID_OFFSET 0x304U

/* Bits of the first octet of a MAC address, see IEEE 802. */
#define MAC_ADDR_MULTICAST_BIT BIT(0)
#define MAC_ADDR_LOCAL_BIT     BIT(1)

/* Device-unique part of the address, that is, bits [23:0]. */
#define MAC_ADDR_DEV_UNIQUE_OFFSET 3

/**
 * @brief Reconstruct a MAC address from the MACADDR[n] register pair.
 *
 * HIGH holds bits [47:24] of the address, that is, the OUI, and LOW holds bits
 * [23:0], that is, the device-unique part. Both are MSB first, so the address
 * F4:CE:36:11:22:33 is programmed as HIGH = 0x00F4CE36, LOW = 0x00112233.
 *
 * Note that this is the opposite byte order to the nRF70 OTP, where the first
 * transmitted octet sits in the least significant byte of the word.
 */
static void mac_addr_from_regs(uint32_t low,
			       uint32_t high,
			       uint8_t mac_addr[WIFI_MAC_ADDR_LEN])
{
	mac_addr[0] = (uint8_t)(high >> 16);
	mac_addr[1] = (uint8_t)(high >> 8);
	mac_addr[2] = (uint8_t)(high);
	mac_addr[3] = (uint8_t)(low >> 16);
	mac_addr[4] = (uint8_t)(low >> 8);
	mac_addr[5] = (uint8_t)(low);
}

/**
 * @brief Derive the address of a secondary interface from a base address.
 *
 * The device-unique part is incremented by the interface index and the locally
 * administered bit is set, so that the derived addresses do not collide with
 * globally unique addresses assigned to other devices.
 */
void nrf_wifi_mac_addr_derive(uint8_t mac_addr[WIFI_MAC_ADDR_LEN],
			      unsigned char vif_idx)
{
	uint32_t dev_unique = sys_get_be24(&mac_addr[MAC_ADDR_DEV_UNIQUE_OFFSET]);

	dev_unique = (dev_unique + vif_idx) & XICR_MACADDR_ADDR_MASK;
	sys_put_be24(dev_unique, &mac_addr[MAC_ADDR_DEV_UNIQUE_OFFSET]);

	mac_addr[0] &= ~MAC_ADDR_MULTICAST_BIT;
	mac_addr[0] |= MAC_ADDR_LOCAL_BIT;
}

bool nrf_wifi_mac_addr_regs_empty(uint32_t low, uint32_t high)
{
	low &= XICR_MACADDR_ADDR_MASK;
	high &= XICR_MACADDR_ADDR_MASK;

	/* Slots that have never been programmed read as the reset value,
	 * 0xFFFFFFFF. Mass-market dies ship with an empty FICR MAC address, and
	 * UICR is empty unless it has been programmed with a MAC address.
	 *
	 * An all-zero pair is treated as empty as well. Such a pair is never a
	 * usable address, and it is what is read back where the registers are
	 * not provided at all. Only the pair is checked, so that a
	 * device-unique part of zero, that is, the first unit of an OUI, stays
	 * usable.
	 */
	return ((low == XICR_MACADDR_ADDR_MASK) ||
		(high == XICR_MACADDR_ADDR_MASK) ||
		((low == 0) && (high == 0)));
}

int nrf_wifi_xicr_mac_addr_slot_read(bool uicr,
				     unsigned char slot,
				     uint32_t *low,
				     uint32_t *high)
{
	uintptr_t addr;

	if (!low || !high || (slot >= NRF_WIFI_XICR_MAC_ADDR_SLOTS)) {
		return -EINVAL;
	}

	if (uicr) {
#if XICR_UICR_PRESENT
		addr = XICR_UICR_BASE + XICR_UICR_MACADDR_OFFSET;
#else
		return -ENOTSUP;
#endif
	} else {
		addr = XICR_FICR_BASE + XICR_FICR_MACADDR_OFFSET;
	}

	addr += (uintptr_t)slot * XICR_MACADDR_SLOT_SIZE;

	*low = sys_read32(addr + XICR_MACADDR_LOW_OFFSET);
	*high = sys_read32(addr + XICR_MACADDR_HIGH_OFFSET);

	return 0;
}

/**
 * @brief Get the MAC address of an interface from one xICR register block.
 *
 * Slots are never mixed between UICR and FICR, so a block is either used in
 * its entirety or skipped.
 */
static int xicr_block_mac_addr_get(bool uicr,
				   unsigned char vif_idx,
				   uint8_t mac_addr[WIFI_MAC_ADDR_LEN])
{
	uint8_t slot_addr[NRF_WIFI_XICR_MAC_ADDR_SLOTS][WIFI_MAC_ADDR_LEN];
	bool slot_valid[NRF_WIFI_XICR_MAC_ADDR_SLOTS] = { false };
	int base_slot = -1;
	int ret;

	for (unsigned char slot = 0; slot < NRF_WIFI_XICR_MAC_ADDR_SLOTS; slot++) {
		uint32_t low, high;

		ret = nrf_wifi_xicr_mac_addr_slot_read(uicr, slot, &low, &high);
		if (ret) {
			return ret;
		}

		if (nrf_wifi_mac_addr_regs_empty(low, high)) {
			LOG_DBG("%s: %s MACADDR[%u] not programmed",
				__func__, uicr ? "UICR" : "FICR", slot);
			continue;
		}

		mac_addr_from_regs(low, high, slot_addr[slot]);

		if (!nrf_wifi_utils_is_mac_addr_valid((const char *)slot_addr[slot])) {
			LOG_WRN("%s: Invalid %s MACADDR[%u]: %02X:%02X:%02X:%02X:%02X:%02X",
				__func__, uicr ? "UICR" : "FICR", slot,
				slot_addr[slot][0], slot_addr[slot][1], slot_addr[slot][2],
				slot_addr[slot][3], slot_addr[slot][4], slot_addr[slot][5]);
			continue;
		}

		slot_valid[slot] = true;

		if (base_slot < 0) {
			base_slot = slot;
		}
	}

	if (base_slot < 0) {
		return -ENOENT;
	}

	/* All slots programmed: slot n serves VIF n. */
	if ((vif_idx < NRF_WIFI_XICR_MAC_ADDR_SLOTS) && slot_valid[vif_idx]) {
		memcpy(mac_addr, slot_addr[vif_idx], WIFI_MAC_ADDR_LEN);
		return 0;
	}

	/* Only one slot programmed: it serves VIF0 and the remaining
	 * interfaces derive their address from it.
	 */
	memcpy(mac_addr, slot_addr[base_slot], WIFI_MAC_ADDR_LEN);

	if (vif_idx > 0) {
		nrf_wifi_mac_addr_derive(mac_addr, vif_idx);
	}

	return 0;
}

#if defined(CONFIG_WIFI_NRF71_XICR_MAC_ADDRESS_FALLBACK_RANDOM)
/**
 * @brief Generate a locally administered MAC address.
 *
 * The address is seeded from the factory programmed device identifier so that
 * it stays the same across reboots, and falls back to the entropy source if
 * the device identifier is not programmed either.
 */
static int random_mac_addr_get(unsigned char vif_idx,
			       uint8_t mac_addr[WIFI_MAC_ADDR_LEN])
{
	uint32_t seed[2];

	seed[0] = sys_read32(XICR_FICR_BASE + XICR_FICR_DEVICEID_OFFSET);
	seed[1] = sys_read32(XICR_FICR_BASE + XICR_FICR_DEVICEID_OFFSET + sizeof(uint32_t));

	if ((seed[0] == UINT32_MAX) && (seed[1] == UINT32_MAX)) {
#if defined(CONFIG_ENTROPY_GENERATOR)
		LOG_WRN("%s: Device identifier not programmed, using entropy source",
			__func__);
		sys_rand_get(seed, sizeof(seed));
#else
		LOG_ERR("%s: Device identifier not programmed and no entropy source",
			__func__);
		return -ENOENT;
#endif
	}

	mac_addr[0] = (uint8_t)(seed[1] >> 8);
	mac_addr[1] = (uint8_t)(seed[1]);
	mac_addr[2] = (uint8_t)(seed[0] >> 24);
	mac_addr[3] = (uint8_t)(seed[0] >> 16);
	mac_addr[4] = (uint8_t)(seed[0] >> 8);
	mac_addr[5] = (uint8_t)(seed[0]);

	nrf_wifi_mac_addr_derive(mac_addr, vif_idx);

	return 0;
}
#endif /* CONFIG_WIFI_NRF71_XICR_MAC_ADDRESS_FALLBACK_RANDOM */

int nrf_wifi_xicr_mac_addr_get(unsigned char vif_idx,
			       uint8_t mac_addr[WIFI_MAC_ADDR_LEN],
			       enum nrf_wifi_mac_addr_src *src)
{
	enum nrf_wifi_mac_addr_src mac_addr_src = NRF_WIFI_MAC_ADDR_SRC_FICR;
	int ret = -ENOENT;

	if (!mac_addr || (vif_idx >= MAX_NUM_VIFS)) {
		return -EINVAL;
	}

#if XICR_UICR_PRESENT
	mac_addr_src = NRF_WIFI_MAC_ADDR_SRC_UICR;
	ret = xicr_block_mac_addr_get(true, vif_idx, mac_addr);
#endif

	if (ret) {
		mac_addr_src = NRF_WIFI_MAC_ADDR_SRC_FICR;
		ret = xicr_block_mac_addr_get(false, vif_idx, mac_addr);
	}

#if defined(CONFIG_WIFI_NRF71_XICR_MAC_ADDRESS_FALLBACK_RANDOM)
	if (ret) {
		mac_addr_src = NRF_WIFI_MAC_ADDR_SRC_RANDOM;
		ret = random_mac_addr_get(vif_idx, mac_addr);
	}
#endif

	if (ret) {
		LOG_ERR("%s: No MAC address available for VIF%u", __func__, vif_idx);
		return ret;
	}

	/* The source is the part that is not observable otherwise, the address
	 * itself is reported by the network stack, so keep it out of the
	 * default log level.
	 */
	LOG_INF("VIF%u MAC address from %s",
		vif_idx,
		(mac_addr_src == NRF_WIFI_MAC_ADDR_SRC_UICR) ? "UICR" :
		(mac_addr_src == NRF_WIFI_MAC_ADDR_SRC_FICR) ? "FICR" : "random");
	LOG_DBG("VIF%u MAC address %02X:%02X:%02X:%02X:%02X:%02X",
		vif_idx,
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);

	if (src) {
		*src = mac_addr_src;
	}

	return 0;
}
