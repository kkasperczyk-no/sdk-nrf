/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <logging/log.h>

#include "nrf_802154.h"
#include "mpsl_radio_notification.h"

#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>

LOG_MODULE_REGISTER(app);

using namespace ::chip::DeviceLayer;

static bool radioActive;

static void power_management_irq(void)
{
	// FIXME: it is likely that states should be opposite, as in practice "goes active" log is printed very close to "sleep" request
	// or just toggling the variable is not enough
	if (radioActive) {
		LOG_INF("Radio goes active");
	} else {
		LOG_INF("Radio goes inactive");
	}
	radioActive = !radioActive;

}

static int power_management_hook_init(const struct device *dev)
{
	int res = mpsl_radio_notification_cfg_set(MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH, MPSL_RADIO_NOTIFICATION_DISTANCE_420US, SWI2_IRQn);
	
	if (res) {
		LOG_ERR("mpsl_radio_notification_cfg_set failed with %d", res);
		return -1;
	}

	NVIC_ClearPendingIRQ(SWI2_IRQn);
	NVIC_SetPriority(SWI2_IRQn, 4);
	NVIC_EnableIRQ(SWI2_IRQn);

	IRQ_CONNECT(SWI2_IRQn, 4, power_management_irq, NULL, 0);

	return 0;
}

int main()
{
	int ret = 0;
	CHIP_ERROR err = CHIP_NO_ERROR;

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

#ifdef CONFIG_OPENTHREAD_MTD_SED
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
		goto exit;
	}

	ConnectivityManager::ThreadPollingConfig pollingConfig;
	pollingConfig.Clear();
	pollingConfig.ActivePollingIntervalMS = CONFIG_OPENTHREAD_POLL_PERIOD;
	pollingConfig.InactivePollingIntervalMS = CONFIG_OPENTHREAD_POLL_PERIOD;

	err = ConnectivityMgr().SetThreadPollingConfig(pollingConfig);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadPollingConfig() failed");
		goto exit;
	}
#else
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
		goto exit;
	}
#endif

	ret = GetAppTask().StartApp();
	if (ret != 0) {
		err = chip::System::MapErrorZephyr(ret);
	}

exit:
	return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}

SYS_INIT(power_management_hook_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);