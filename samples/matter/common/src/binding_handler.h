/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>
#include <app/CommandSender.h>
#include <app/clusters/bindings/BindingManager.h>
#include <controller/InvokeInteraction.h>
#include <platform/CHIPDeviceLayer.h>

using namespace chip;

namespace{
typedef void (*DeviceChangedHandler)(const EmberBindingTableEntry &binding, OperationalDeviceProxy *deviceProxy,
				    void *context);
}

class BindingHandler {
public:
	struct BindingData {
		chip::EndpointId EndpointId;
		chip::CommandId CommandId;
		chip::ClusterId ClusterId;
		uint8_t Value;
		bool IsGroup;
	};

	CHIP_ERROR Init(DeviceChangedHandler handler);
	void PrintBindingTable();
	bool IsGroupBound();

	static void DeviceWorkerHandler(intptr_t aContext);
	static void OnInvokeCommandSucces();
	static void OnInvokeCommandFailure(BindingData &bindingData, CHIP_ERROR aError);

	static BindingHandler &Instance()
	{
		static BindingHandler sBindingHandler;
		return sBindingHandler;
	}
	DeviceChangedHandler mDeviceChangedHandler;

private:
	static void DeviceChangedCallback(const EmberBindingTableEntry &binding, OperationalDeviceProxy *deviceProxy, void *context);
	static void DeviceContextReleaseHandler(void *context);
	static void InitInternal();

	bool mCaseSessionRecovered = false;
};
