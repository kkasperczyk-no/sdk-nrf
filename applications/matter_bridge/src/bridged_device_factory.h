/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridge_util.h"
#include "bridged_device.h"
#include "bridged_device_data_provider.h"
#include <lib/support/CHIPMem.h>

#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
#include "humidity_sensor.h"
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
#include "simulated_humidity_sensor_data_provider.h"
#endif
#endif

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
#include "onoff_light.h"
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
#include "simulated_onoff_light_data_provider.h"
#endif
#ifdef CONFIG_BRIDGED_DEVICE_BLE
#include "ble_onoff_light_data_provider.h"
#endif
#endif

#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
#include "temperature_sensor.h"
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
#include "simulated_temperature_sensor_data_provider.h"
#endif
#endif

namespace BridgeFactory
{
using UpdateAttributeCallback = BridgedDeviceDataProvider::UpdateAttributeCallback;
using DeviceType = BridgedDevice::DeviceType;
using BridgedDeviceFactory = DeviceFactory<BridgedDevice, const char *>;

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
using SimulatedDataProviderFactory = DeviceFactory<BridgedDeviceDataProvider, UpdateAttributeCallback>;
#endif

#ifdef CONFIG_BRIDGED_DEVICE_BLE
using BleDataProviderFactory = DeviceFactory<BridgedDeviceDataProvider, UpdateAttributeCallback>;
#endif

inline BridgedDeviceFactory &GetBridgedDeviceFactory()
{
	static BridgedDeviceFactory sBridgedDeviceFactory{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ DeviceType::HumiditySensor,
		  [](const char *nodeLabel) { return chip::Platform::New<HumiditySensorDevice>(nodeLabel); } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](const char *nodeLabel) { return chip::Platform::New<OnOffLightDevice>(nodeLabel); } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ BridgedDevice::DeviceType::TemperatureSensor,
		  [](const char *nodeLabel) { return chip::Platform::New<TemperatureSensorDevice>(nodeLabel); } },
#endif
	};
	return sBridgedDeviceFactory;
}

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
inline SimulatedDataProviderFactory &GetSimulatedDataProviderFactory()
{
	static SimulatedDataProviderFactory sDeviceDataProvider{
#ifdef CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
		{ DeviceType::HumiditySensor,
		  [](UpdateAttributeCallback clb) {
			  return chip::Platform::New<SimulatedHumiditySensorDataProvider>(clb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](UpdateAttributeCallback clb) {
			  return chip::Platform::New<SimulatedOnOffLightDataProvider>(clb);
		  } },
#endif
#ifdef CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
		{ DeviceType::TemperatureSensor,
		  [](UpdateAttributeCallback clb) {
			  return chip::Platform::New<SimulatedTemperatureSensorDataProvider>(clb);
		  } },
#endif
	};
	return sDeviceDataProvider;
}
#endif

#ifdef CONFIG_BRIDGED_DEVICE_BLE
inline BleDataProviderFactory &GetBleDataProviderFactory()
{
	static BleDataProviderFactory sDeviceDataProvider{
#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
		{ DeviceType::OnOffLight,
		  [](UpdateAttributeCallback clb) { return chip::Platform::New<BleOnOffLightDataProvider>(clb); } },
#endif
	};
	return sDeviceDataProvider;
}
#endif

} // namespace BridgeFactory
