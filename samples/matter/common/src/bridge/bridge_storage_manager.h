/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device.h"
#include "persistent_storage_util.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include <zephyr/bluetooth/addr.h>
#endif

/*
 * The class implements the following key-values storage structure:
 *
 * /br/
 *		/brd_cnt/ /<uint8_t>/
 *		/brd_ids/ /<uint8_t[brd_cnt]>/
 * 		/brd/
 *			/eid/
 *				/0/	/<uint16_t>/
 *				/1/	/<uint16_t>/
 *				.
 *				.
 *				/n/ /<uint16_t>/
 *			/label/
 *				/0/	/<char[]>/
 *				/1/	/<char[]>/
 *				.
 *				.
 *				/n/ /<char[]>/
 *			/type/
 *				/0/	/<uint16_t>/
 *				/1/	/<uint16_t>/
 *				.
 *				.
 *				/n/ /<uint16_t>/
 *			/bt/
 *				/addr/
 *					/0/	/<bt_addr_le_t>/
 *					/1/	/<bt_addr_le_t>/
 *					.
 *					.
 *					/n/	/<bt_addr_le_t>/
 */
class BridgeStorageManager {
public:
	static constexpr auto kMaxIndexLength = 3;

	BridgeStorageManager()
		: mBridge("br", strlen("br")), mBridgedDevicesCount("brd_cnt", strlen("brd_cnt"), &mBridge),
		  mBridgedDevicesIndexes("brd_ids", strlen("brd_ids"), &mBridge),
		  mBridgedDevice("brd", strlen("brd"), &mBridge),
		  mBridgedDeviceEndpointId("eid", strlen("eid"), &mBridgedDevice),
		  mBridgedDeviceNodeLabel("label", strlen("label"), &mBridgedDevice),
		  mBridgedDeviceType("type", strlen("type"), &mBridgedDevice)
#ifdef CONFIG_BRIDGED_DEVICE_BT
		  ,
		  mBt("bt", strlen("bt"), &mBridgedDevice), mBtAddress("addr", strlen("addr"), &mBt)
#endif
	{
	}

	static BridgeStorageManager &Instance()
	{
		static BridgeStorageManager sInstance;
		return sInstance;
	}

	bool Init() { return PersistentStorage::Instance().Init(); }
	bool StoreBridgedDevicesCount(uint8_t count);
	bool LoadBridgedDevicesCount(uint8_t &count);
	bool StoreBridgedDevicesIndexes(uint8_t *indexes, uint8_t count);
	bool LoadBridgedDevicesIndexes(uint8_t *indexes, uint8_t maxCount, size_t &count);
	bool StoreBridgedDeviceEndpointId(uint16_t endpointId, uint8_t bridgedDeviceIndex);
	bool LoadBridgedDeviceEndpointId(uint16_t &endpointId, uint8_t bridgedDeviceIndex);
	bool RemoveBridgedDeviceEndpointId(uint8_t bridgedDeviceIndex);
	bool StoreBridgedDeviceNodeLabel(const char *label, size_t labelLength, uint8_t bridgedDeviceIndex);
	bool LoadBridgedDeviceNodeLabel(char *label, size_t labelMaxLength, size_t &labelLength,
					uint8_t bridgedDeviceIndex);
	bool RemoveBridgedDeviceNodeLabel(uint8_t bridgedDeviceIndex);
	bool StoreBridgedDeviceType(uint16_t deviceType, uint8_t bridgedDeviceIndex);
	bool LoadBridgedDeviceType(uint16_t &deviceType, uint8_t bridgedDeviceIndex);
	bool RemoveBridgedDeviceType(uint8_t bridgedDeviceIndex);
	bool StoreBridgedDevice(BridgedDevice *device, uint8_t index);

#ifdef CONFIG_BRIDGED_DEVICE_BT
	bool StoreBtAddress(bt_addr_le_t addr, uint8_t bridgedDeviceIndex);
	bool LoadBtAddress(bt_addr_le_t &addr, uint8_t bridgedDeviceIndex);
	bool RemoveBtAddress();
#endif

private:
	PersistentStorageNode mBridge;
	PersistentStorageNode mBridgedDevicesCount;
	PersistentStorageNode mBridgedDevicesIndexes;
	PersistentStorageNode mBridgedDevice;
	PersistentStorageNode mBridgedDeviceEndpointId;
	PersistentStorageNode mBridgedDeviceNodeLabel;
	PersistentStorageNode mBridgedDeviceType;
#ifdef CONFIG_BRIDGED_DEVICE_BT
	PersistentStorageNode mBt;
	PersistentStorageNode mBtAddress;
#endif
};
