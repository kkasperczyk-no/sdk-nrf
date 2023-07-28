/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"

#if CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
#include "onoff_light.h"
#include "onoff_light_data_provider.h"
#endif

#if CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
#include "temperature_sensor.h"
#include "temperature_sensor_data_provider.h"
#endif

#if CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
#include "humidity_sensor.h"
#include "humidity_sensor_data_provider.h"
#endif

#include <app-common/zap-generated/ids/Clusters.h>
#include <app/reporting/reporting.h>
#include <app/util/generic-callbacks.h>
#include <lib/support/Span.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

BridgeManager BridgeManager::sBridgeManager;

void BridgeManager::Init()
{
	/* The first dynamic endpoint is the last fixed endpoint + 1. */
	mFirstDynamicEndpointId = static_cast<chip::EndpointId>(
		static_cast<int>(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1))) + 1);

	mCurrentDynamicEndpointId = mFirstDynamicEndpointId;

	/* Disable the placeholder endpoint */
	emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)),
				     false);
}

CHIP_ERROR BridgeManager::AddBridgedDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider)
{
	const char *nodeLabel = aDevice->GetNodeLabel();
	if (nodeLabel && (strlen(nodeLabel) >= BridgedDevice::kNodeLabelSize)) {
		return CHIP_ERROR_INVALID_STRING_LENGTH;
	}

	aDataProvider->Init();
	CHIP_ERROR err = AddDevices(aDevice, aDataProvider);

	return err;
}

CHIP_ERROR BridgeManager::RemoveBridgedDevice(uint16_t endpoint)
{
	uint8_t index = 0;

	while (index < kMaxBridgedDevices) {
		if (mDevicesMap.Contains(index)) {
			const DevicePair *devicesPtr = mDevicesMap[index];
			if ((*devicesPtr).mDevice->GetEndpointId() == endpoint) {
				LOG_INF("Removed dynamic endpoint %d (index=%d)", endpoint, index);
				/* Free dynamically allocated memory */
				emberAfClearDynamicEndpoint(index);
				if (mDevicesMap.Erase(index)) {
					return CHIP_NO_ERROR;
				} else {
					LOG_ERR("Cannot remove bridged devices under index=%d", index);
					return CHIP_ERROR_NOT_FOUND;
				}
			}
		}
		index++;
	}
	return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR BridgeManager::AddDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider)
{
	uint8_t index = 0;
	Platform::UniquePtr<BridgedDevice> device(aDevice);
	Platform::UniquePtr<BridgedDeviceDataProvider> provider(aDataProvider);

	while (index < kMaxBridgedDevices) {
		/* Find the first empty index in the bridged devices list */
		if (!mDevicesMap.Contains(index)) {
			mDevicesMap.Insert(index, DevicePair(std::move(device), std::move(provider)));
			EmberAfStatus ret;
			while (true) {
				if (!mDevicesMap[index]) {
					LOG_ERR("Cannot retrieve bridged device from index %d", index);
					return CHIP_ERROR_INTERNAL;
				}
				auto &storedDevice = mDevicesMap[index]->mDevice;
				if (!storedDevice) {
					LOG_ERR("Bridged device stored under index %d is empty", index);
					return CHIP_ERROR_INTERNAL;
				}
				ret = emberAfSetDynamicEndpoint(
					index, mCurrentDynamicEndpointId, storedDevice->mEp,
					Span<DataVersion>(storedDevice->mDataVersion, storedDevice->mDataVersionSize),
					Span<const EmberAfDeviceType>(storedDevice->mDeviceTypeList,
								      storedDevice->mDeviceTypeListSize));

				if (ret == EMBER_ZCL_STATUS_SUCCESS) {
					LOG_INF("Added device to dynamic endpoint %d (index=%d)",
						mCurrentDynamicEndpointId, index);
					storedDevice->Init(mCurrentDynamicEndpointId);
					return CHIP_NO_ERROR;
				} else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS) {
					LOG_ERR("Failed to add dynamic endpoint: Internal error!");
					RemoveBridgedDevice(mCurrentDynamicEndpointId); // TODO: check if this is ok, we
											// need to cleanup the unused
											// devices
					return CHIP_ERROR_INTERNAL;
				}

				/* Handle wrap condition */
				if (++mCurrentDynamicEndpointId < mFirstDynamicEndpointId) {
					mCurrentDynamicEndpointId = mFirstDynamicEndpointId;
				}
			}
		}
		index++;
	}

	LOG_ERR("Failed to add dynamic endpoint: No endpoints available!");

	return CHIP_ERROR_NO_MEMORY;
}

BridgeManager::DevicePtr *
BridgeManager::GetBridgedDevice(uint16_t index, const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	if (index >= kMaxBridgedDevices) {
		return nullptr;
	}

	DevicePtr *device = const_cast<DevicePtr *>(&(sBridgeManager.mDevicesMap[index]->mDevice));
	if (!device || !buffer || !attributeMetadata) {
		return nullptr;
	}
	return device;
}

CHIP_ERROR BridgeManager::HandleRead(uint16_t index, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	DevicePtr *device = GetBridgedDevice(index, attributeMetadata, buffer);

	if (!device) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return (*device)->HandleRead(clusterId, attributeMetadata->attributeId, buffer, maxReadLength);
}

CHIP_ERROR BridgeManager::HandleWrite(uint16_t index, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	DevicePtr *device = GetBridgedDevice(index, attributeMetadata, buffer);

	if (!device) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	CHIP_ERROR err = (*device)->HandleWrite(clusterId, attributeMetadata->attributeId, buffer);

	/* After updating Matter BridgedDevice state, forward request to the non-Matter device. */
	if (err == CHIP_NO_ERROR) {
		return sBridgeManager.mDevicesMap[index]->mProvider->UpdateState(
			clusterId, attributeMetadata->attributeId, buffer);
	}
	return err;
}

void BridgeManager::HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize)
{
	if (!data) {
		return;
	}

	/* The state update was triggered by non-Matter device, find related Matter Bridged Device to update it as
	well.
	 */
	for (auto &item : sBridgeManager.mDevicesMap.mMap) {
		if (item.value.mProvider.get() == &dataProvider) {
			/* If the Bridged Device state was updated successfully, schedule sending Matter data report. */
			if (CHIP_NO_ERROR ==
			    item.value.mDevice->HandleAttributeChange(clusterId, attributeId, data, dataSize)) {
				MatterReportingAttributeChangeCallback(item.value.mDevice->GetEndpointId(), clusterId,
								       attributeId);
			}
		}
	}
}

EmberAfStatus emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
						   const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
						   uint16_t maxReadLength)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR ==
	    GetBridgeManager().HandleRead(endpointIndex, clusterId, attributeMetadata, buffer, maxReadLength)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}

EmberAfStatus emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
						    const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR == GetBridgeManager().HandleWrite(endpointIndex, clusterId, attributeMetadata, buffer)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}
