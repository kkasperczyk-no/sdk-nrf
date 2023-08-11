/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridged_devices_creator.h"
#include "bridge_manager.h"
#include "bridge_storage_manager.h"
#include "bridged_device_factory.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
void AddDevice(int deviceType, const char *nodeLabel, BridgedDeviceDataProvider *provider)
{
	VerifyOrReturn(provider != nullptr, LOG_ERR("Cannot allocate data provider of given type"));

	auto *newBridgedDevice =
		BridgeFactory::GetBridgedDeviceFactory().Create(static_cast<BridgedDevice::DeviceType>(deviceType),
								nodeLabel,
								static_cast<BridgedDevice::DeviceType>(deviceType));

	if (newBridgedDevice == nullptr) {
		delete provider;
		LOG_ERR("Cannot allocate Matter device of given type");
	}

	uint8_t index;
	uint8_t count = 0;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	size_t indexesCount = 0;
	uint16_t endpointId;
	bool deviceRefresh = false;

	CHIP_ERROR err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevice, provider, index);

	if (err == CHIP_NO_ERROR) {
		/* Check if a device is already present in the storage. */
		if (BridgeStorageManager::Instance().LoadBridgedDeviceEndpointId(endpointId, index)) {
			deviceRefresh = true;
		}

		if (!BridgeStorageManager::Instance().StoreBridgedDevice(newBridgedDevice, index)) {
			LOG_ERR("Error: failed to store bridged device");
			return;
		}

#ifdef CONFIG_BRIDGED_DEVICE_BT
		BLEBridgedDeviceProvider *bleProvider = static_cast<BLEBridgedDeviceProvider *>(provider);

		bt_addr_le_t addr;
		if (!bleProvider->GetBtAddress(&addr)) {
			return;
		}

		if (!BridgeStorageManager::Instance().StoreBtAddress(addr, index)) {
			LOG_ERR("Error: failed to store bridged device Bluetooth address");
		}
#endif

		/* If a device was not present in the storage before, put new index on the list and increment the count
		 * number of stored devices. */
		if (!deviceRefresh) {
			BridgeStorageManager::Instance().LoadBridgedDevicesIndexes(
				indexes, BridgeManager::kMaxBridgedDevices, indexesCount);

			if (indexesCount >= BridgeManager::kMaxBridgedDevices) {
				return;
			}

			indexes[indexesCount] = index;
			indexesCount++;

			if (!BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, indexesCount)) {
				LOG_ERR("Error: failed to store bridged devices indexes");
				return;
			}

			BridgeStorageManager::Instance().LoadBridgedDevicesCount(count);

			if (!BridgeStorageManager::Instance().StoreBridgedDevicesCount(count + 1)) {
				LOG_ERR("Error: failed to store bridged devices count");
				return;
			}
		}

	} else if (err == CHIP_ERROR_INVALID_STRING_LENGTH) {
		LOG_ERR("Error: too long node label (max %d)", BridgedDevice::kNodeLabelSize);
	} else if (err == CHIP_ERROR_NO_MEMORY) {
		LOG_ERR("Error: no memory");
	} else if (err == CHIP_ERROR_INVALID_ARGUMENT) {
		LOG_ERR("Error: invalid device type");
	} else {
		LOG_ERR("Error: internal");
	}
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

void BridgedDeviceCreator::CreateDevice(int deviceType, const char *nodeLabel
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
#elif defined(CONFIG_BRIDGED_DEVICE_SIMULATED)
	provider = CreateSimulatedProvider(deviceType);
	/* The device is simulated, so it can be added immediately. */
	AddDevice(deviceType, nodeLabel, provider);
#else
	return;
#endif
}

bool BridgedDeviceCreator::RemoveDevice(int endpointId)
{
	uint8_t index;
	uint8_t count = 0;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	size_t indexesCount = 0;

	if (CHIP_NO_ERROR != BridgeManager::Instance().RemoveBridgedDevice(endpointId, index)) {
		return false;
	}

	if (!BridgeStorageManager::Instance().LoadBridgedDevicesIndexes(indexes, BridgeManager::kMaxBridgedDevices,
									indexesCount)) {
		LOG_ERR("Error: failed to load remove stored bridged device");
		return false;
	}

	bool indexFound = false;
	for (auto i = 0; i < static_cast<int>(indexesCount); i++) {
		if (indexes[i] == index) {
			indexFound = true;
		}

		if (indexFound && ((i + 1) < BridgeManager::kMaxBridgedDevices)) {
			indexes[i] = indexes[i + 1];
		}
	}

	if (indexFound) {
		indexesCount--;
	} else {
		return false;
	}

	if (!BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, indexesCount)) {
		LOG_ERR("Error: failed to store bridged devices indexes");
		return false;
	}

	if (!BridgeStorageManager::Instance().LoadBridgedDevicesCount(count)) {
		LOG_ERR("Error: failed to load bridged devices count");
		return false;
	}

	if (!BridgeStorageManager::Instance().StoreBridgedDevicesCount(count - 1)) {
		LOG_ERR("Error: failed to store bridged devices count");
		return false;
	}

	if (!BridgeStorageManager::Instance().RemoveBridgedDeviceEndpointId(index)) {
		LOG_ERR("Error: failed to remove bridged device endpoint id");
		return false;
	}

	BridgeStorageManager::Instance().RemoveBridgedDeviceNodeLabel(index);

	if (!BridgeStorageManager::Instance().RemoveBridgedDeviceType(index)) {
		LOG_ERR("Error: failed to remove bridged device type");
		return false;
	}

#ifdef CONFIG_BRIDGED_DEVICE_BT
	if (!BridgeStorageManager::Instance().RemoveBtAddress(index)) {
		LOG_ERR("Error: failed to remove bridged device Bluetooth address");
		return false;
	}
#endif

	return true;
}
