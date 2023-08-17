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
CHIP_ERROR StoreDevice(BridgedDevice *device, BridgedDeviceDataProvider *provider, uint8_t index)
{
	uint16_t endpointId;
	uint8_t count = 0;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	size_t indexesCount = 0;
	bool deviceRefresh = false;

	/* Check if a device is already present in the storage. */
	if (BridgeStorageManager::Instance().LoadBridgedDeviceEndpointId(endpointId, index)) {
		deviceRefresh = true;
	}

	if (!BridgeStorageManager::Instance().StoreBridgedDevice(device, index)) {
		LOG_ERR("Failed to store bridged device");
		return CHIP_ERROR_INTERNAL;
	}

/* Store additional information that are not present for every generic BridgedDevice. */
#ifdef CONFIG_BRIDGED_DEVICE_BT
	BLEBridgedDeviceProvider *bleProvider = static_cast<BLEBridgedDeviceProvider *>(provider);

	bt_addr_le_t addr;
	if (!bleProvider->GetBtAddress(&addr)) {
		return CHIP_ERROR_INTERNAL;
	}

	if (!BridgeStorageManager::Instance().StoreBtAddress(addr, index)) {
		LOG_ERR("Failed to store bridged device's Bluetooth address");
		return CHIP_ERROR_INTERNAL;
	}
#endif

	/* If a device was not present in the storage before, put new index on the end of list and increment the count
	 * number of stored devices. */
	if (!deviceRefresh) {
		BridgeStorageManager::Instance().LoadBridgedDevicesIndexes(indexes, BridgeManager::kMaxBridgedDevices,
									   indexesCount);

		if (indexesCount >= BridgeManager::kMaxBridgedDevices) {
			return CHIP_ERROR_BUFFER_TOO_SMALL;
		}

		indexes[indexesCount] = index;
		indexesCount++;

		if (!BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, indexesCount)) {
			LOG_ERR("Failed to store bridged devices indexes.");
			return CHIP_ERROR_INTERNAL;
		}

		BridgeStorageManager::Instance().LoadBridgedDevicesCount(count);

		if (!BridgeStorageManager::Instance().StoreBridgedDevicesCount(count + 1)) {
			LOG_ERR("Failed to store bridged devices count.");
			return CHIP_ERROR_INTERNAL;
		}
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AddDevice(int deviceType, const char *nodeLabel, BridgedDeviceDataProvider *provider,
		     chip::Optional<uint8_t> index, chip::Optional<uint16_t> endpointId)
{
	VerifyOrReturnError(provider != nullptr, CHIP_ERROR_INVALID_ARGUMENT,
			    LOG_ERR("Cannot allocate data provider of given type"));

	auto *newBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
		static_cast<BridgedDevice::DeviceType>(deviceType), nodeLabel);

	if (newBridgedDevice == nullptr) {
		delete provider;
		LOG_ERR("Cannot allocate Matter device of given type");
		return CHIP_ERROR_INTERNAL;
	}

	CHIP_ERROR err;
	uint8_t deviceIndex = 0;

	if (index.HasValue() && endpointId.HasValue()) {
		deviceIndex = index.Value();
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevice, provider, deviceIndex,
								  endpointId.Value());
	} else {
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevice, provider, deviceIndex);
	}

	if (err == CHIP_NO_ERROR) {
		err = StoreDevice(newBridgedDevice, provider, deviceIndex);
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
	chip::Optional<uint8_t> index;
	chip::Optional<uint16_t> endpointId;
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

		AddDevice(ctx->deviceType, ctx->nodeLabel, ctx->provider, ctx->index, ctx->endpointId);
	}
}

BridgedDeviceDataProvider *CreateBleProvider(int deviceType, const char *nodeLabel, int bleDeviceIndex,
					     chip::Optional<uint8_t> index, chip::Optional<uint16_t> endpointId)
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
	contextPtr->index = index;
	contextPtr->endpointId = endpointId;

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
					      ,
					      chip::Optional<uint8_t> index, chip::Optional<uint16_t> endpointId)
{
	BridgedDeviceDataProvider *provider = nullptr;

#if defined(CONFIG_BRIDGED_DEVICE_BT)
	/* The device cannot be created in line, it has to wait for connected callback. */
	provider = CreateBleProvider(deviceType, nodeLabel, bleDeviceIndex, index, endpointId);

	if (!provider) {
		return CHIP_ERROR_INTERNAL;
	}

#elif defined(CONFIG_BRIDGED_DEVICE_SIMULATED)
	provider = CreateSimulatedProvider(deviceType);
	/* The device is simulated, so it can be added immediately. */
	return AddDevice(deviceType, nodeLabel, provider, index, endpointId);
#else
	return CHIP_ERROR_NOT_IMPLEMENTED;
#endif

	return CHIP_NO_ERROR;
}

CHIP_ERROR BridgedDeviceCreator::RemoveDevice(int endpointId)
{
	uint8_t index;
	uint8_t count = 0;
	uint8_t indexes[BridgeManager::kMaxBridgedDevices] = { 0 };
	size_t indexesCount = 0;
	CHIP_ERROR err = BridgeManager::Instance().RemoveBridgedDevice(endpointId, index);

	if (CHIP_NO_ERROR != err) {
		LOG_ERR("Failed to remove bridged device");
		return err;
	}

	if (!BridgeStorageManager::Instance().LoadBridgedDevicesIndexes(indexes, BridgeManager::kMaxBridgedDevices,
									indexesCount)) {
		LOG_ERR("Failed to load stored bridged device indexes");
		return CHIP_ERROR_INTERNAL;
	}

	/* Find the required index on the list, remove it and move all following indexes one position earlier. */
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
		return CHIP_ERROR_NOT_FOUND;
	}

	/* Update the current indexes list. */
	if (!BridgeStorageManager::Instance().StoreBridgedDevicesIndexes(indexes, indexesCount)) {
		LOG_ERR("Failed to store bridged devices indexes.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!BridgeStorageManager::Instance().LoadBridgedDevicesCount(count)) {
		LOG_ERR("Failed to load bridged devices count.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!BridgeStorageManager::Instance().StoreBridgedDevicesCount(count - 1)) {
		LOG_ERR("Failed to store bridged devices count.");
		return CHIP_ERROR_INTERNAL;
	}

	if (!BridgeStorageManager::Instance().RemoveBridgedDeviceEndpointId(index)) {
		LOG_ERR("Failed to remove bridged device endpoint id.");
		return CHIP_ERROR_INTERNAL;
	}

	/* Ignore error, as node label may not be present in the storage. */
	BridgeStorageManager::Instance().RemoveBridgedDeviceNodeLabel(index);

	if (!BridgeStorageManager::Instance().RemoveBridgedDeviceType(index)) {
		LOG_ERR("Failed to remove bridged device type.");
		return CHIP_ERROR_INTERNAL;
	}

#ifdef CONFIG_BRIDGED_DEVICE_BT
	if (!BridgeStorageManager::Instance().RemoveBtAddress(index)) {
		LOG_ERR("Failed to remove bridged device Bluetooth address.");
		return CHIP_ERROR_INTERNAL;
	}
#endif

	return CHIP_NO_ERROR;
}
