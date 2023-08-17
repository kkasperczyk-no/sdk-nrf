/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/core/Optional.h>

namespace BridgedDeviceCreator
{
/**
 * @brief Create a bridged device.
 *
 * @param deviceType the Matter device type of a bridged device to be created
 * @param nodeLabel node label of a Matter device to be created
 * @param bleDeviceIndex index of Bluetooth LE device on the scanned devices list to be bridged with Matter bridged
 * device
 * @param index optional index object that shall have a valid value set if the value is meant
 * to be used to index assignment, or shall not have a value set if the default index assignment should be used.
 * @param endpointId optional endpoint id object that shall have a valid value set if the value is meant
 * to be used to endpoint id assignment, or shall not have a value set if the default endpoint id assignment should be
 * used.
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR CreateDevice(int deviceType, const char *nodeLabel
#ifdef CONFIG_BRIDGED_DEVICE_BT
			,
			int bleDeviceIndex
#endif
			,
			chip::Optional<uint8_t> index = chip::Optional<uint8_t>(),
			chip::Optional<uint16_t> endpointId = chip::Optional<uint16_t>());

/**
 * @brief Remove bridged device.
 *
 * @param endpoint value of endpoint id specifying the bridged device to be removed
 * @return CHIP_NO_ERROR on success
 * @return other error code on failure
 */
CHIP_ERROR RemoveDevice(int endpointId);
} /* namespace BridgedDeviceCreator */
