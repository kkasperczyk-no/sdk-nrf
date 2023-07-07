/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"

#include <app-common/zap-generated/ids/Clusters.h>
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

CHIP_ERROR BridgeManager::AddBridgedDevice(BridgedDevice::DeviceType bridgedDeviceType, const char *nodeLabel)
{
	Platform::UniquePtr<BridgedDevice> device(nullptr);

	if (nodeLabel && (strlen(nodeLabel) >= BridgedDevice::kNodeLabelSize)) {
		return CHIP_ERROR_INVALID_STRING_LENGTH;
	}

	switch (bridgedDeviceType) {
#if CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE
	case BridgedDevice::DeviceType::OnOffLight: {
		LOG_INF("Adding OnOff Light bridged device");
		device.reset(Platform::New<OnOffLightDevice>(nodeLabel));
		break;
	}
#endif /* CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE */
#if CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE
	case BridgedDevice::DeviceType::TemperatureSensor: {
		LOG_INF("Adding TemperatureSensor bridged device");
		device.reset(Platform::New<TemperatureSensorDevice>(nodeLabel));
		break;
	}
#endif /* CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE */
#if CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE
	case BridgedDevice::DeviceType::HumiditySensor: {
		LOG_INF("Adding HumiditySensor bridged device");
		device.reset(Platform::New<HumiditySensorDevice>(nodeLabel));
		break;
	}
#endif /* CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE */
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (device == nullptr || device->mDataVersion == nullptr) {
		return CHIP_ERROR_NO_MEMORY;
	}

	CHIP_ERROR err = AddDevice(device.get());

	/* In case adding succedeed, release the ownership. */
	if (err == CHIP_NO_ERROR) {
		device.release();
	}

	return err;
}

CHIP_ERROR BridgeManager::RemoveBridgedDevice(uint16_t endpoint)
{
	uint8_t index = 0;

	while (index < kMaxBridgedDevices) {
		if (nullptr != mBridgedDevices[index]) {
			if (mBridgedDevices[index]->GetEndpointId() == endpoint) {
				LOG_INF("Removed dynamic endpoint %d (index=%d)", endpoint, index);
				/* Free dynamically allocated memory */
				emberAfClearDynamicEndpoint(index);
				Platform::Delete(mBridgedDevices[index]);
				mBridgedDevices[index] = nullptr;
				return CHIP_NO_ERROR;
			}
		}
		index++;
	}

	return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR BridgeManager::AddDevice(BridgedDevice *device)
{
	uint8_t index = 0;

	while (index < kMaxBridgedDevices) {
		/* Find the first empty index in the brdiged devices list */
		if (nullptr == mBridgedDevices[index]) {
			EmberAfStatus ret;
			while (true) {
				ret = emberAfSetDynamicEndpoint(
					index, mCurrentDynamicEndpointId, device->mEp,
					Span<DataVersion>(device->mDataVersion, device->mDataVersionSize),
					Span<const EmberAfDeviceType>(device->mDeviceTypeList,
								      device->mDeviceTypeListSize));

				if (ret == EMBER_ZCL_STATUS_SUCCESS) {
					LOG_INF("Added device to dynamic endpoint %d (index=%d)",
						mCurrentDynamicEndpointId, index);
					mBridgedDevices[index] = device;
					device->Init(mCurrentDynamicEndpointId);
					return CHIP_NO_ERROR;
				} else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS) {
					LOG_ERR("Failed to add dynamic endpoint: Internal error!");
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

BridgedDevice *BridgeManager::GetBridgedDevice(uint16_t endpoint, const EmberAfAttributeMetadata *attributeMetadata,
					       uint8_t *buffer)
{
	if (endpoint >= kMaxBridgedDevices) {
		return nullptr;
	}

	BridgedDevice *device = sBridgeManager.mBridgedDevices[endpoint];
	if ((device == nullptr) || !buffer || !attributeMetadata) {
		return nullptr;
	}
	return device;
}

CHIP_ERROR BridgeManager::HandleRead(uint16_t endpoint, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	BridgedDevice *device = GetBridgedDevice(endpoint, attributeMetadata, buffer);

	if (!device) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return device->HandleRead(clusterId, attributeMetadata->attributeId, buffer, maxReadLength);
}

CHIP_ERROR BridgeManager::HandleWrite(uint16_t endpoint, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	BridgedDevice *device = GetBridgedDevice(endpoint, attributeMetadata, buffer);

	if (!device) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return device->HandleWrite(clusterId, attributeMetadata->attributeId, buffer);
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
