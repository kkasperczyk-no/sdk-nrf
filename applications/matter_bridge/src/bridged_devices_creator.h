/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

namespace BridgedDeviceCreator
{
CHIP_ERROR CreateDevice(int deviceType, const char *nodeLabel
#ifdef CONFIG_BRIDGED_DEVICE_BT
			,
			int bleDeviceIndex
#endif
);
CHIP_ERROR RemoveDevice(int endpointId);
} /* namespace BridgedDeviceCreator */
