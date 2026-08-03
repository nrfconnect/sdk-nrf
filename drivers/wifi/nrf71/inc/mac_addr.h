/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing MAC address provisioning declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_MAC_ADDR_H__
#define __ZEPHYR_MAC_ADDR_H__

#include <zephyr/types.h>
#include <zephyr/net/wifi.h>

/** Source the MAC address of a virtual interface was taken from. */
enum nrf_wifi_mac_addr_src {
	/** MAC address programmed in UICR. */
	NRF_WIFI_MAC_ADDR_SRC_UICR,
	/** MAC address programmed in FICR. */
	NRF_WIFI_MAC_ADDR_SRC_FICR,
	/** Locally administered MAC address, derived at runtime. */
	NRF_WIFI_MAC_ADDR_SRC_RANDOM,
};

/**
 * @brief Get the MAC address of a virtual interface from the xICR registers.
 *
 * The MAC address is looked up in the following order of priority and the
 * first source with at least one programmed slot wins. A programmed UICR
 * address taking precedence over the factory programmed FICR one is specified
 * by the device, see the nRF7120 Product Specification.
 *
 *   1. UICR MACADDR[n] (skipped if UICR is not accessible, that is, in
 *      non-secure builds where UICR is mapped to the secure domain only)
 *   2. FICR MACADDR[n]
 *   3. A locally administered address derived at runtime, if
 *      CONFIG_WIFI_NRF71_XICR_MAC_ADDRESS_FALLBACK_RANDOM is enabled
 *
 * A source is used in its entirety, that is, slots are never mixed between
 * UICR and FICR. Within the selected source:
 *
 *   - if both slots are programmed, slot n is used for VIF n;
 *   - if only one slot is programmed, it is used for VIF0, and the remaining
 *     VIFs derive their address from it by incrementing the device-unique
 *     part and setting the locally administered bit.
 *
 * @param vif_idx Index of the virtual interface to get the MAC address for.
 * @param mac_addr Buffer of WIFI_MAC_ADDR_LEN bytes to store the MAC address.
 * @param src Optional, filled in with the source the address was taken from.
 *
 * @retval 0 On success.
 * @retval -EINVAL If the parameters are invalid.
 * @retval -ENOENT If no MAC address could be determined.
 */
int nrf_wifi_xicr_mac_addr_get(unsigned char vif_idx,
			       uint8_t mac_addr[WIFI_MAC_ADDR_LEN],
			       enum nrf_wifi_mac_addr_src *src);

/**
 * @brief Read a raw MAC address slot.
 *
 * Intended for provisioning and bring-up diagnostics, the driver itself uses
 * nrf_wifi_xicr_mac_addr_get().
 *
 * @param uicr Read UICR if true, FICR otherwise.
 * @param slot Slot index, that is, the n in MACADDR[n].
 * @param low Filled in with the raw MACADDR[n].LOW register value.
 * @param high Filled in with the raw MACADDR[n].HIGH register value.
 *
 * @retval 0 On success.
 * @retval -EINVAL If the parameters are invalid.
 * @retval -ENOTSUP If the requested register block is not accessible.
 */
int nrf_wifi_xicr_mac_addr_slot_read(bool uicr,
				     unsigned char slot,
				     uint32_t *low,
				     uint32_t *high);

/**
 * @brief Check whether a MAC address slot is empty, that is, not programmed.
 *
 * @param low Raw MACADDR[n].LOW register value.
 * @param high Raw MACADDR[n].HIGH register value.
 *
 * @return true if the slot holds no MAC address.
 */
bool nrf_wifi_mac_addr_regs_empty(uint32_t low, uint32_t high);

/**
 * @brief Derive the MAC address of a secondary interface from a base address.
 *
 * The device-unique part of the address, that is, bits [23:0], is incremented
 * by the interface index and the locally administered bit is set, so that the
 * derived address does not collide with a globally unique address assigned to
 * another device.
 *
 * @param mac_addr Base address, updated in place.
 * @param vif_idx Index of the virtual interface to derive the address for.
 */
void nrf_wifi_mac_addr_derive(uint8_t mac_addr[WIFI_MAC_ADDR_LEN],
			      unsigned char vif_idx);

/**
 * @brief Number of MAC address slots per xICR register block.
 */
#define NRF_WIFI_XICR_MAC_ADDR_SLOTS 2

#endif /* __ZEPHYR_MAC_ADDR_H__ */
