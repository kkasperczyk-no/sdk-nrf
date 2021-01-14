/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#pragma once

#include <controller/CHIPDeviceController.h>
#include <transport/raw/UDP.h>

#include <atomic>

/** @class LightSwitch
 *  @brief Class for controlling a CHIP light bulb over a Thread network
 *
 *  Features:
 *  - discovering a CHIP light bulb which advertises itself by sending Thread multicast packets
 *  - toggling and dimming the connected CHIP light bulb by sending appropriate CHIP messages
 */
class LightSwitch {
public:
	CHIP_ERROR Init();

	void StartDiscovery();
	void StopDiscovery();
	bool IsDiscoveryEnabled() const;

	CHIP_ERROR Pair(const chip::Inet::IPAddress &lightBulbAddress);
	CHIP_ERROR ToggleLight();
	CHIP_ERROR SetLightLevel(uint8_t level);

private:
	chip::Transport::UDP mDiscoveryServiceEndpoint;
	chip::Controller::DeviceCommissioner mCommissioner;
	std::atomic_bool mDiscoveryEnabled;
};
