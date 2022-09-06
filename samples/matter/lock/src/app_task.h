/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "bolt_lock_manager.h"
#include "led_widget.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#endif

#ifdef CONFIG_MCUMGR_SMP_BT
#include "dfu_over_smp.h"
#endif

struct k_timer;

class AppTask {
public:
	CHIP_ERROR StartApp();

	void PostEvent(const AppEvent &event);
	void UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source);

private:
	friend AppTask &GetAppTask();
	CHIP_ERROR Init();

	void CancelTimer();
	void StartTimer(uint32_t timeoutInMs);
	void DispatchEvent(const AppEvent &event);

	static void LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source);
	static void UpdateStatusLED();
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void UpdateLedStateEventHandler(const AppEvent &event);
	static void FunctionTimerEventHandler(const AppEvent &event);
	static void FunctionHandler(const AppEvent &event);
	static void StartBLEAdvertisementAndLockActionEventHandler(const AppEvent &event);
	static void LockActionEventHandler(const AppEvent &event);
	static void StartBLEAdvertisementHandler(const AppEvent &event);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void TimerEventHandler(k_timer *timer);
#ifdef CONFIG_MCUMGR_SMP_BT
	static void RequestSMPAdvertisingStart(void);
#endif

	enum class Function : uint8_t {
		NoneSelected = 0,
		SoftwareUpdate = 0,
		FactoryReset,
#if CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP
		AdvertisingStart,
#endif
		Invalid
	};

	Function mFunction = Function::NoneSelected;
	bool mFunctionTimerActive = false;

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif

	static AppTask sAppTask;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
