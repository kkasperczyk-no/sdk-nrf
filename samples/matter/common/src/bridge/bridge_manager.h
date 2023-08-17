/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridge_util.h"
#include "bridged_device.h"
#include "bridged_device_data_provider.h"

class BridgeManager {
public:
	static constexpr uint8_t kMaxBridgedDevices = CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;

	using LoadStoredBridgedDevices = CHIP_ERROR (*)();

	/**
	 * @brief Initialize BridgeManager instance.
	 *
	 * @param loadStoredBridgedDevicesCb callback to method capable of loading and adding bridged devices stored in
	 * persistent storage
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR Init(LoadStoredBridgedDevices loadStoredBridgedDevicesCb);

	/**
	 * @brief Add pair of bridged device and its data provider using default index and endpoint id assignment.
	 *
	 * @param device address of valid bridged device object
	 * @param dataProvider address of valid data provider object
	 * @param devicesPairIndex reference to the index object that will be filled with pair's index assigned by the
	 * bridge
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddBridgedDevices(BridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
				     uint8_t &devicesPairIndex);

	/**
	 * @brief Add pair of bridged device and its data provider using specific index and endpoint id.
	 *
	 * @param device address of valid bridged device object
	 * @param dataProvider address of valid data provider object
	 * @param devicesPairIndex reference to the index object that contains index value required to be assigned and
	 * that will be filled with pair's index finally assigned by the bridge
	 * @param endpointId value of endpoint id required to be assigned
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddBridgedDevices(BridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
				     uint8_t &devicesPairIndex, uint16_t endpointId);

	/**
	 * @brief Remove bridged device.
	 *
	 * @param endpoint value of endpoint id specifying the bridged device to be removed
	 * @param devicesPairIndex reference to the index object that will be filled with pair's index obtained by the
	 * bridge
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR RemoveBridgedDevice(uint16_t endpoint, uint8_t &devicesPairIndex);

	static CHIP_ERROR HandleRead(uint16_t index, chip::ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength);
	static CHIP_ERROR HandleWrite(uint16_t index, chip::ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer);
	static void HandleUpdate(BridgedDeviceDataProvider &dataProvider, chip::ClusterId clusterId,
				 chip::AttributeId attributeId, void *data, size_t dataSize);

	static BridgeManager &Instance()
	{
		static BridgeManager sInstance;
		return sInstance;
	}

private:
	static constexpr uint8_t kMaxDataProviders = CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER;

	using DevicePtr = chip::Platform::UniquePtr<BridgedDevice>;
	using ProviderPtr = chip::Platform::UniquePtr<BridgedDeviceDataProvider>;
	struct DevicePair {
		DevicePair() : mDevice(nullptr), mProvider(nullptr) {}
		DevicePair(DevicePtr device, ProviderPtr provider)
			: mDevice(std::move(device)), mProvider(std::move(provider))
		{
		}
		DevicePair &operator=(DevicePair &&other)
		{
			mDevice = std::move(other.mDevice);
			mProvider = std::move(other.mProvider);
			return *this;
		}
		operator bool() const { return mDevice && mProvider; }
		DevicePair &operator=(const DevicePair &other) = delete;
		DevicePtr mDevice;
		ProviderPtr mProvider;
	};
	using DeviceMap = FiniteMap<DevicePair, kMaxBridgedDevices>;

	/**
	 * @brief Add pair of bridged device and its data provider using optional index and endpoint id. This is a
	 * wrapper method invoked by public AddBridgedDevices methods that maps integer indexes to optionals are assigns
	 * output index values.
	 *
	 * @param device address of valid bridged device object
	 * @param dataProvider address of valid data provider object
	 * @param devicesPairIndex reference to the index object that will be filled with pair's index assigned by the
	 * bridge
	 * @param endpointId value of endpoint id required to be assigned
	 * @param index reference to the optional index object that shall have a valid value set if the value is meant
	 * to be used to index assignment, or shall not have a value set if the default index assignment should be used.
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddBridgedDevices(BridgedDevice *device, BridgedDeviceDataProvider *dataProvider,
				     uint8_t &devicesPairIndex, uint16_t endpointId, chip::Optional<uint8_t> &index);

	/**
	 * @brief Add pair of bridged device and its data provider using optional index and endpoint id. The method
	 * creates a map entry, matches the bridged device object with the data provider object and creates Matter
	 * dynamic endpoint.
	 *
	 * @param device address of valid bridged device object
	 * @param dataProvider address of valid data provider object
	 * @param devicesPairIndex reference to the optional index object that shall have a valid value set if the value
	 * is meant to be used to index assignment, or shall not have a value set if the default index assignment should
	 * be used.
	 * @param endpointId value of endpoint id required to be assigned
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR AddDevices(BridgedDevice *aDevice, BridgedDeviceDataProvider *aDataProvider,
			      chip::Optional<uint8_t> &devicesPairIndex, uint16_t endpointId);

	/**
	 * @brief Create Matter dynamic endpoint.
	 *
	 * @param index index in Matter Data Model's (ember) array to store the endpoint
	 * @param endpointId value of endpoint id to be created
	 * @return CHIP_NO_ERROR on success
	 * @return other error code on failure
	 */
	CHIP_ERROR CreateEndpoint(uint8_t index, uint16_t endpointId);

	DeviceMap mDevicesMap;
	uint16_t mNumberOfProviders{ 0 };

	chip::EndpointId mFirstDynamicEndpointId;
	chip::EndpointId mCurrentDynamicEndpointId;
	bool mIsInitialized = false;
};
