/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "ble_bridged_device.h"
#include "bridged_device_data_provider.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>

class BLEConnectivityManager {
public:
	struct ScannedDevice {
		bt_addr_le_t mAddr;
		bt_le_conn_param mConnParam;
	};

	static constexpr uint16_t kScanTimeoutMs = 10000;
	static constexpr uint16_t kMaxScannedDevices = 16;
	static constexpr uint16_t kMaxCreatedDevices = 16;
	static constexpr uint8_t kMaxServiceUuids = CONFIG_BT_SCAN_UUID_CNT;

	CHIP_ERROR Init(bt_uuid **serviceUuids, uint8_t serviceUuidsCount);
	CHIP_ERROR Scan();
	CHIP_ERROR StopScan();
	CHIP_ERROR Connect(uint8_t index, BLEBridgedDevice::DeviceConnectedCallback callback, void *context,
			   bt_uuid *serviceUuid);
	BLEBridgedDevice *FindBLEBridgedDevice(struct bt_conn *conn);

	static void FilterMatch(struct bt_scan_device_info *device_info, struct bt_scan_filter_match *filter_match,
				bool connectable);
	static void ScanTimeoutCallback(k_timer *timer);
	static void ScanTimeoutHandle(intptr_t context);
	static void ConnectionHandler(struct bt_conn *conn, uint8_t conn_err);
	static void DisconnectionHandler(struct bt_conn *conn, uint8_t reason);
	static void DiscoveryCompletedHandler(struct bt_gatt_dm *dm, void *context);
	static void DiscoveryNotFound(struct bt_conn *conn, void *context);
	static void DiscoveryError(struct bt_conn *conn, int err, void *context);

private:
	bool mScanActive;
	k_timer mScanTimer;
	uint8_t mScannedDevicesCounter;
	uint8_t mCreatedDevicesCounter;
	ScannedDevice mScannedDevices[kMaxScannedDevices];
	BLEBridgedDevice mCreatedDevices[kMaxCreatedDevices];
	bt_uuid *mServicesUuid[kMaxServiceUuids];
	uint8_t mServicesUuidCount;

	friend BLEConnectivityManager &GetBLEConnectivityManager();
	static BLEConnectivityManager sBLEConnectivityManager;
};

inline BLEConnectivityManager &GetBLEConnectivityManager()
{
	return BLEConnectivityManager::sBLEConnectivityManager;
}
