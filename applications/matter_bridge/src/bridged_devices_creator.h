/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/core/Optional.h>

#include <zephyr/bluetooth/addr.h>

namespace BridgedDeviceCreator
{
#ifdef CONFIG_BRIDGED_DEVICE_BT
/**
 * @brief Create a bridged device.
 *
 * @param deviceType the Matter device type of a bridged device to be created
 * @param nodeLabel node label of a Matter device to be created
 * @param btAddress the Bluetooth LE address of a device to be bridged with created Matter device
 * @param index optional index object that shall have a valid value set if the value is meant
 * to be used to index assignment, or shall not have a value set if the default index assignment should be used.
 * @param endpointId optional endpoint id object that shall have a valid value set if the value is meant
 * to be used to endpoint id assignment, or shall not have a value set if the default endpoint id assignment should be
 * used.
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR CreateDevice(int deviceType, const char *nodeLabel, bt_addr_le_t btAddress,
			chip::Optional<uint8_t> index = chip::Optional<uint8_t>(),
			chip::Optional<uint16_t> endpointId = chip::Optional<uint16_t>());
#else
/**
 * @brief Create a bridged device.
 *
 * @param deviceType the Matter device type of a bridged device to be created
 * @param nodeLabel node label of a Matter device to be created
 * @param index optional index object that shall have a valid value set if the value is meant
 * to be used to index assignment, or shall not have a value set if the default index assignment should be used.
 * @param endpointId optional endpoint id object that shall have a valid value set if the value is meant
 * to be used to endpoint id assignment, or shall not have a value set if the default endpoint id assignment should be
 * used.
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR CreateDevice(int deviceType, const char *nodeLabel,
			chip::Optional<uint8_t> index = chip::Optional<uint8_t>(),
			chip::Optional<uint16_t> endpointId = chip::Optional<uint16_t>());
#endif

/**
 * @brief Remove bridged device.
 *
 * @param endpointId value of endpoint id specifying the bridged device to be removed
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR RemoveDevice(int endpointId);
} /* namespace BridgedDeviceCreator */
