/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"

#include <app-common/zap-generated/ids/Clusters.h>
#include <app/reporting/reporting.h>
#include <app/util/generic-callbacks.h>
#include <lib/support/Span.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

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

CHIP_ERROR BridgeManager::AddBridgedDevices(BridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
					    uint8_t &devicesPairIndex)
{
	chip::Optional<uint8_t> index;

	return AddBridgedDevices(device, dataProvider, devicesPairIndex, mCurrentDynamicEndpointId, index);
}

CHIP_ERROR BridgeManager::AddBridgedDevices(BridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
					    uint8_t &devicesPairIndex, uint16_t endpointId)
{
	chip::Optional<uint8_t> index;
	index.SetValue(devicesPairIndex);

	return AddBridgedDevices(device, dataProvider, devicesPairIndex, endpointId, index);
}

CHIP_ERROR BridgeManager::AddBridgedDevices(BridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
					    uint8_t &devicesPairIndex, uint16_t endpointId,
					    chip::Optional<uint8_t> &index)
{
	CHIP_ERROR err = AddDevices(device, dataProvider, index, endpointId);

	if (err != CHIP_NO_ERROR) {
		return err;
	}

	if (!index.HasValue()) {
		return CHIP_ERROR_INTERNAL;
	}

	devicesPairIndex = index.Value();

	return CHIP_NO_ERROR;
}

CHIP_ERROR BridgeManager::RemoveBridgedDevice(uint16_t endpoint, uint8_t &devicesPairIndex)
{
	uint8_t index = 0;

	while (index < kMaxBridgedDevices) {
		if (mDevicesMap.Contains(index)) {
			const DevicePair *devicesPtr = mDevicesMap[index];
			if (devicesPtr->mDevice->GetEndpointId() == endpoint) {
				LOG_INF("Removed dynamic endpoint %d (index=%d)", endpoint, index);
				/* Free dynamically allocated memory */
				emberAfClearDynamicEndpoint(index);
				if (mDevicesMap.Erase(index)) {
					mNumberOfProviders--;
					devicesPairIndex = index;
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

CHIP_ERROR BridgeManager::CreateEndpoint(uint8_t index, uint16_t endpointId)
{
	if (!mDevicesMap[index]) {
		LOG_ERR("Cannot retrieve bridged device from index %d", index);
		return CHIP_ERROR_INTERNAL;
	}

	auto &storedDevice = mDevicesMap[index]->mDevice;
	EmberAfStatus ret = emberAfSetDynamicEndpoint(
		index, endpointId, storedDevice->mEp,
		Span<DataVersion>(storedDevice->mDataVersion, storedDevice->mDataVersionSize),
		Span<const EmberAfDeviceType>(storedDevice->mDeviceTypeList, storedDevice->mDeviceTypeListSize));

	if (ret == EMBER_ZCL_STATUS_SUCCESS) {
		LOG_INF("Added device to dynamic endpoint %d (index=%d)", endpointId, index);
		storedDevice->Init(endpointId);
		return CHIP_NO_ERROR;
	} else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS) {
		LOG_ERR("Failed to add dynamic endpoint: Internal error!");
		RemoveBridgedDevice(endpointId, index); // TODO: check if this
							// is ok, we need to
							// cleanup the unused
							// devices
		return CHIP_ERROR_INTERNAL;
	} else {
		return CHIP_ERROR_SENTINEL;
	}
}

CHIP_ERROR BridgeManager::AddDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider,
				     chip::Optional<uint8_t> &devicesPairIndex, uint16_t endpointId)
{
	uint8_t index = 0;
	CHIP_ERROR err;

	Platform::UniquePtr<BridgedDevice> device(aDevice);
	Platform::UniquePtr<BridgedDeviceDataProvider> provider(aDataProvider);
	VerifyOrReturnError(device && provider, CHIP_ERROR_INTERNAL);

	provider->Init();

	/* Maximum number of Matter bridged devices is controlled inside mDevicesMap,
	   but the data providers may be created independently, so let's ensure we do not
	   violate the maximum number of supported instances. */
	VerifyOrReturnError(!mDevicesMap.IsFull(), CHIP_ERROR_INTERNAL);
	VerifyOrReturnError(mNumberOfProviders + 1 <= kMaxDataProviders, CHIP_ERROR_INTERNAL);
	mNumberOfProviders++;

	/* The adding algorithm differs depending on the devicesPairIndex value:
	 * - If devicesPairIndex has value it means that index and endpoint id are specified and should be assigned
	 * based on the input arguments (e.g. the device was loaded from storage and has to use specific data).
	 * - If devicesPairIndex has no value it means the default monotonically increasing numbering should be used.
	 */
	if (devicesPairIndex.HasValue()) {
		index = devicesPairIndex.Value();

		/* The requested index is already used. */
		if (mDevicesMap.Contains(index)) {
			return CHIP_ERROR_INTERNAL;
		}

		mDevicesMap.Insert(index, DevicePair(std::move(device), std::move(provider)));
		err = CreateEndpoint(index, endpointId);

		if (err == CHIP_NO_ERROR) {
			devicesPairIndex.SetValue(index);
			/* Make sure that the following endpoint id assignments will be monotonically continued from the
			 * biggest assigned number. */
			mCurrentDynamicEndpointId =
				mCurrentDynamicEndpointId > endpointId ? mCurrentDynamicEndpointId : endpointId;
		}

		return err;
	} else {
		while (index < kMaxBridgedDevices) {
			/* Find the first empty index in the bridged devices list */
			if (!mDevicesMap.Contains(index)) {
				mDevicesMap.Insert(index, DevicePair(std::move(device), std::move(provider)));

				/* Assign the free endpoint ID. */
				do {
					err = CreateEndpoint(index, endpointId);

					/* Handle wrap condition */
					if (++mCurrentDynamicEndpointId < mFirstDynamicEndpointId) {
						mCurrentDynamicEndpointId = mFirstDynamicEndpointId;
					}
				} while (err == CHIP_ERROR_SENTINEL);

				if (err == CHIP_NO_ERROR) {
					devicesPairIndex.SetValue(index);
				}

				return err;
			}
			index++;
		}
	}

	LOG_ERR("Failed to add dynamic endpoint: No endpoints available!");

	return CHIP_ERROR_NO_MEMORY;
}

CHIP_ERROR BridgeManager::HandleRead(uint16_t index, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(Instance().mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto &device = Instance().mDevicesMap[index]->mDevice;
	return device->HandleRead(clusterId, attributeMetadata->attributeId, buffer, maxReadLength);
}

CHIP_ERROR BridgeManager::HandleWrite(uint16_t index, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	VerifyOrReturnError(attributeMetadata && buffer, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnValue(Instance().mDevicesMap.Contains(index), CHIP_ERROR_INTERNAL);

	auto &device = Instance().mDevicesMap[index]->mDevice;
	CHIP_ERROR err = device->HandleWrite(clusterId, attributeMetadata->attributeId, buffer);

	/* After updating Matter BridgedDevice state, forward request to the non-Matter device. */
	if (err == CHIP_NO_ERROR) {
		return Instance().mDevicesMap[index]->mProvider->UpdateState(clusterId, attributeMetadata->attributeId,
									     buffer);
	}
	return err;
}

void BridgeManager::HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize)
{
	VerifyOrReturn(data);

	/* The state update was triggered by non-Matter device, find related Matter Bridged Device to update it as
	well.
	 */
	for (auto &item : Instance().mDevicesMap.mMap) {
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
	    BridgeManager::Instance().HandleRead(endpointIndex, clusterId, attributeMetadata, buffer, maxReadLength)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}

EmberAfStatus emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
						    const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	if (CHIP_NO_ERROR ==
	    BridgeManager::Instance().HandleWrite(endpointIndex, clusterId, attributeMetadata, buffer)) {
		return EMBER_ZCL_STATUS_SUCCESS;
	} else {
		return EMBER_ZCL_STATUS_FAILURE;
	}
}
