/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <logging/log.h>

#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>

#include "drivers/gpio.h"
#include "mpsl_radio_notification.h"

LOG_MODULE_REGISTER(app);

using namespace ::chip::DeviceLayer;

// TODO: Pin number should be changed to the one connected to the FEM
const char *kGPIOControllerName = "GPIO_0";
gpio_pin_t kFemControlPin = 13;
const struct device *kGPIOController = device_get_binding(kGPIOControllerName);
static bool sFemPinConfigured;
static bool sRadioActive;

static void power_management_irq(void)
{
	if (sRadioActive) {
		LOG_INF("Radio goes active");
	} else {
		LOG_INF("Radio goes inactive");
	}
	sRadioActive = !sRadioActive;

	// Change FEM enabling pin state
	if (sFemPinConfigured) {
		gpio_pin_toggle(kGPIOController, kFemControlPin);
	}
}

static int power_management_hook_init(const struct device *dev)
{
	int res = mpsl_radio_notification_cfg_set(MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH,
						  MPSL_RADIO_NOTIFICATION_DISTANCE_200US, SWI2_IRQn);

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

	if (kGPIOController) {
		if (gpio_pin_configure(kGPIOController, kFemControlPin, GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Cannot configure kFemControlPin");
		} else {
			if (gpio_pin_set(kGPIOController, kFemControlPin, 0)) {
				LOG_ERR("Cannot set kFemControlPin");
			} else {
				sFemPinConfigured = true;
			}
		}
	} else {
		LOG_ERR("Cannot find kGPIOController");
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

SYS_INIT(power_management_hook_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
