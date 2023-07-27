/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

struct BLEBridgedDevice;

class BLEBridgedDeviceProvider : public BridgedDeviceDataProvider {
public:
	explicit BLEBridgedDeviceProvider(UpdateAttributeCallback callback) : BridgedDeviceDataProvider(callback) {}
	virtual ~BLEBridgedDeviceProvider() = default;

	virtual bt_uuid *GetServiceUuid() = 0;
	virtual void MatchBleDevice(BLEBridgedDevice *device) = 0;
	virtual int ParseDiscoveredData(struct bt_gatt_dm *discoveredData) = 0;

protected:
	BLEBridgedDevice *mDevice;
};

struct BLEBridgedDevice {
	using DeviceConnectedCallback = void (*)(BLEBridgedDevice *device, struct bt_gatt_dm *discoveredData,
						 bool discoverySucceeded, void *context);

	bt_addr_le_t mAddr;
	DeviceConnectedCallback mConnectedCallback;
	void *mConnectedCallbackContext;
	bt_uuid *mServiceUuid;
	struct bt_conn *mConn;
	BLEBridgedDeviceProvider *provider;
};
