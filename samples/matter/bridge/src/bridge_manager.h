/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device.h"

#if CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
#include "onoff_light.h"
#endif

#if CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
#include "temperature_sensor.h"
#endif

#if CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
#include "humidity_sensor.h"
#endif

class BridgeManager {
public:
	void Init();
	CHIP_ERROR AddBridgedDevice(BridgedDevice::DeviceType bridgedDeviceType, const char *nodeLabel);
	CHIP_ERROR RemoveBridgedDevice(uint16_t endpoint);
	static BridgedDevice * GetBridgedDevice(uint16_t endpoint, const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer);
	static CHIP_ERROR HandleRead(uint16_t endpoint, chip::ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength);
	static CHIP_ERROR HandleWrite(uint16_t endpoint, chip::ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer);

private:
	CHIP_ERROR AddDevice(BridgedDevice *device);

	static constexpr uint8_t kMaxBridgedDevices = CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;

	BridgedDevice *mBridgedDevices[kMaxBridgedDevices];

	chip::EndpointId mFirstDynamicEndpointId;
	chip::EndpointId mCurrentDynamicEndpointId;

	friend BridgeManager &GetBridgeManager();
	static BridgeManager sBridgeManager;
};

inline BridgeManager &GetBridgeManager()
{
	return BridgeManager::sBridgeManager;
}
