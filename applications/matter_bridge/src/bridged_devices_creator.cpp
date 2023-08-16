/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"
#include "bridge_storage_manager.h"
#include "bridged_device_factory.h"
#include "bridged_devices_creator.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
CHIP_ERROR AddDevice(int deviceType, const char *nodeLabel, BridgedDeviceDataProvider *provider)
{
	VerifyOrReturnError(provider != nullptr, CHIP_ERROR_INVALID_ARGUMENT,
			    LOG_ERR("Cannot allocate data provider of given type"));

	auto *newBridgedDevice =
		BridgeFactory::GetBridgedDeviceFactory().Create(static_cast<BridgedDevice::DeviceType>(deviceType),
								nodeLabel,
								static_cast<BridgedDevice::DeviceType>(deviceType));

	if (newBridgedDevice == nullptr) {
		delete provider;
		LOG_ERR("Cannot allocate Matter device of given type");
		return CHIP_ERROR_INTERNAL;
	}

	CHIP_ERROR err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevice, provider);

	if (err == CHIP_NO_ERROR) {
	} else if (err == CHIP_ERROR_INVALID_STRING_LENGTH) {
		LOG_ERR("Error: too long node label (max %d)", BridgedDevice::kNodeLabelSize);
	} else if (err == CHIP_ERROR_NO_MEMORY) {
		LOG_ERR("Error: no memory");
	} else if (err == CHIP_ERROR_INVALID_ARGUMENT) {
		LOG_ERR("Error: invalid device type");
	} else {
		LOG_ERR("Error: internal");
	}
	return err;
}

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
BridgedDeviceDataProvider *CreateSimulatedProvider(int deviceType)
{
	return BridgeFactory::GetSimulatedDataProviderFactory().Create(
		static_cast<BridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);
}
#endif /* CONFIG_BRIDGED_DEVICE_SIMULATED */

#ifdef CONFIG_BRIDGED_DEVICE_BT

struct BluetoothConnectionContext {
	int deviceType;
	char nodeLabel[BridgedDevice::kNodeLabelSize];
	BLEBridgedDeviceProvider *provider;
};

void BluetoothDeviceConnected(BLEBridgedDevice *device, bt_gatt_dm *discoveredData, bool discoverySucceeded,
			      void *context)
{
	if (context && device && discoveredData && discoverySucceeded) {
		BluetoothConnectionContext *ctx = reinterpret_cast<BluetoothConnectionContext *>(context);
		chip::Platform::UniquePtr<BluetoothConnectionContext> ctxPtr(ctx);

		if (ctxPtr->provider->MatchBleDevice(device)) {
			return;
		}

		if (ctxPtr->provider->ParseDiscoveredData(discoveredData)) {
			return;
		}

		AddDevice(ctx->deviceType, ctx->nodeLabel, ctx->provider);
	}
}

BridgedDeviceDataProvider *CreateBleProvider(int deviceType, const char *nodeLabel, int bleDeviceIndex)
{
	/* The device object can be created once the Bluetooth LE connection will be established. */
	BluetoothConnectionContext *context = chip::Platform::New<BluetoothConnectionContext>();

	if (!context) {
		return nullptr;
	}

	chip::Platform::UniquePtr<BluetoothConnectionContext> contextPtr(context);
	contextPtr->deviceType = deviceType;

	if (nodeLabel) {
		strcpy(contextPtr->nodeLabel, nodeLabel);
	}

	BridgedDeviceDataProvider *provider = BridgeFactory::GetBleDataProviderFactory().Create(
		static_cast<BridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);

	if (!provider) {
		return nullptr;
	}

	contextPtr->provider = static_cast<BLEBridgedDeviceProvider *>(provider);

	CHIP_ERROR err = BLEConnectivityManager::Instance().Connect(
		bleDeviceIndex, BluetoothDeviceConnected, contextPtr.get(), contextPtr->provider->GetServiceUuid());

	if (err == CHIP_NO_ERROR) {
		contextPtr.release();
	}

	return provider;
}
#endif /* CONFIG_BRIDGED_DEVICE_BT */
} /* namespace */

CHIP_ERROR BridgedDeviceCreator::CreateDevice(int deviceType, const char *nodeLabel
#ifdef CONFIG_BRIDGED_DEVICE_BT
					,
					int bleDeviceIndex
#endif
)
{
	BridgedDeviceDataProvider *provider = nullptr;

#if defined(CONFIG_BRIDGED_DEVICE_BT)
	/* The device cannot be created in line, it has to wait for connected callback. */
	provider = CreateBleProvider(deviceType, nodeLabel, bleDeviceIndex);

	if (!provider) {
		return CHIP_ERROR_INTERNAL;
	}

#elif defined(CONFIG_BRIDGED_DEVICE_SIMULATED)
	provider = CreateSimulatedProvider(deviceType);
	/* The device is simulated, so it can be added immediately. */
	return AddDevice(deviceType, nodeLabel, provider);
#else
	return CHIP_ERROR_NOT_IMPLEMENTED;
#endif
}

CHIP_ERROR BridgedDeviceCreator::RemoveDevice(int endpointId)
{
	return BridgeManager::Instance().RemoveBridgedDevice(endpointId);
}
