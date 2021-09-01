/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <usb/usb_device.h>

#include <logging/log.h>

#include <platform/CHIPDeviceLayer.h>
#include <support/CHIPMem.h>

LOG_MODULE_REGISTER(app);

using namespace ::chip::DeviceLayer;

int main()
{
	int ret = 0;
	CHIP_ERROR err = CHIP_NO_ERROR;

	err = chip::System::MapErrorZephyr(usb_enable(NULL));
	if (err != CHIP_NO_ERROR) {
		goto exit;
	}

	err = chip::Platform::MemoryInit();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Platform::MemoryInit() failed");
		goto exit;
	}

	LOG_INF("Init CHIP stack");
	err = PlatformMgr().InitChipStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().InitChipStack() failed");
		goto exit;
	}

	LOG_INF("Starting CHIP task");
	err = PlatformMgr().StartEventLoopTask();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
		goto exit;
	}

	LOG_INF("Init Thread stack");
	err = ThreadStackMgr().InitThreadStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed");
		goto exit;
	}

	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
		goto exit;
	}

	ret = GetAppTask().StartApp();
	if (ret != 0) {
		err = chip::System::MapErrorZephyr(ret);
	}

exit:
	return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
