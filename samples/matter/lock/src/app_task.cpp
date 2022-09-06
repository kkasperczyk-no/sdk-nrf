/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app_config.h"
#include "bolt_lock_manager.h"
#include "led_widget.h"

#ifdef CONFIG_NET_L2_OPENTHREAD
#include "thread_util.h"
#endif

#include <platform/CHIPDeviceLayer.h>

#include "board_util.h"
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

#ifdef CONFIG_CHIP_WIFI
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <platform/nrfconnect/wifi/NrfWiFiDriver.h>
#endif

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr uint32_t kFactoryResetTriggerTimeout = 3000;
constexpr uint32_t kFactoryResetCancelWindowTimeout = 3000;
constexpr size_t kAppEventQueueSize = 10;
constexpr uint8_t kButtonPushEvent = 1;
constexpr uint8_t kButtonReleaseEvent = 0;
#if NUMBER_OF_BUTTONS == 2
constexpr uint32_t kAdvertisingTriggerTimeout = 3000;
#endif
constexpr EndpointId kLockEndpointId = 1;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
k_timer sFunctionTimer;

LEDWidget sStatusLED;
LEDWidget sLockLED;
#if NUMBER_OF_LEDS == 4
LEDWidget sFactoryResetSignalLED;
LEDWidget sFactoryResetSignalLED_1;
#endif

bool sIsNetworkProvisioned;
bool sIsNetworkEnabled;
bool sHaveBLEConnections;
} /* namespace */

AppTask AppTask::sAppTask;

#ifdef CONFIG_CHIP_WIFI
app::Clusters::NetworkCommissioning::Instance
	sWiFiCommissioningInstance(0, &(NetworkCommissioning::NrfWiFiDriver::Instance()));
#endif

CHIP_ERROR AppTask::Init()
{
	/* Initialize CHIP stack */
	LOG_INF("Init CHIP stack");

	CHIP_ERROR err = chip::Platform::MemoryInit();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Platform::MemoryInit() failed");
		return err;
	}

	err = PlatformMgr().InitChipStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().InitChipStack() failed");
		return err;
	}

#if defined(CONFIG_NET_L2_OPENTHREAD)
	err = ThreadStackMgr().InitThreadStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed");
		return err;
	}

#ifdef CONFIG_OPENTHREAD_MTD_SED
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
#endif /* CONFIG_OPENTHREAD_MTD_SED */
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
		return err;
	}

#ifdef CONFIG_OPENTHREAD_DEFAULT_TX_POWER
	err = SetDefaultThreadOutputPower();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Cannot set default Thread output power");
		return err;
	}
#endif /* CONFIG_OPENTHREAD_DEFAULT_TX_POWER */
#elif defined(CONFIG_CHIP_WIFI)
	sWiFiCommissioningInstance.Init();
#else
	return CHIP_ERROR_INTERNAL;
#endif /* CONFIG_NET_L2_OPENTHREAD */

	/* Initialize LEDs */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

	sStatusLED.Init(SYSTEM_STATE_LED);
	sLockLED.Init(LOCK_STATE_LED);
	sLockLED.Set(BoltLockMgr().IsLocked());
#if NUMBER_OF_LEDS == 4
	sFactoryResetSignalLED.Init(FACTORY_RESET_SIGNAL_LED);
	sFactoryResetSignalLED_1.Init(FACTORY_RESET_SIGNAL_LED1);
#endif

	UpdateStatusLED();

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return chip::System::MapErrorZephyr(ret);
	}

	/* Initialize function timer */
	k_timer_init(&sFunctionTimer, &AppTask::TimerEventHandler, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

#ifdef CONFIG_MCUMGR_SMP_BT
	/* Initialize DFU over SMP */
	GetDFUOverSMP().Init(RequestSMPAdvertisingStart);
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize lock manager */
	BoltLockMgr().Init(LockStateChanged);

	/* Initialize CHIP server */
#if CONFIG_CHIP_FACTORY_DATA
	ReturnErrorOnFailure(mFactoryDataProvider.Init());
	SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
	SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);
	SetCommissionableDataProvider(&mFactoryDataProvider);
#else
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif
	static CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();

	ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

	/*
	 * Add CHIP event handler and start CHIP thread.
	 * Note that all the initialization code should happen prior to this point to avoid data races
	 * between the main and the CHIP threads.
	 */
	PlatformMgr().AddEventHandler(ChipEventHandler, 0);

	err = PlatformMgr().StartEventLoopTask();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
		return err;
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	AppEvent event = {};

	while (true) {
		k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
		DispatchEvent(event);
	}

	return CHIP_NO_ERROR;
}

#if NUMBER_OF_BUTTONS == 2
void AppTask::StartBLEAdvertisementAndLockActionEventHandler(const AppEvent &event)
{
	if (event.ButtonEvent.Action == kButtonPushEvent) {
		sAppTask.StartTimer(kAdvertisingTriggerTimeout);
		sAppTask.mFunction = Function::AdvertisingStart;
	} else {
		if (sAppTask.mFunction == Function::AdvertisingStart) {
			sAppTask.CancelTimer();
			sAppTask.mFunction = Function::NoneSelected;

			AppEvent button_event;
			button_event.Type = AppEvent::Button;
			button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON;
			button_event.ButtonEvent.Action = kButtonReleaseEvent;
			button_event.Handler = LockActionEventHandler;
			sAppTask.PostEvent(&button_event);
		}
	}
}
#endif

void AppTask::LockActionEventHandler(const AppEvent &event)
{
	if (BoltLockMgr().IsLocked()) {
		BoltLockMgr().Unlock(BoltLockManager::OperationSource::kButton);
	} else {
		BoltLockMgr().Lock(BoltLockManager::OperationSource::kButton);
	}
}

void AppTask::ButtonEventHandler(uint32_t button_state, uint32_t has_changed)
{
	AppEvent button_event;
	button_event.Type = AppEvent::Button;

#if NUMBER_OF_BUTTONS == 2
	if (BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON_MASK & has_changed) {
		button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON;
		button_event.ButtonEvent.Action = (BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON_MASK & button_state) ?
							  kButtonPushEvent :
							  kButtonReleaseEvent;
		button_event.Handler = StartBLEAdvertisementAndLockActionEventHandler;
		sAppTask.PostEvent(button_event);
	}
#else
	if (LOCK_BUTTON_MASK & button_state & has_changed) {
		button_event.ButtonEvent.PinNo = LOCK_BUTTON;
		button_event.ButtonEvent.Action = kButtonPushEvent;
		button_event.Handler = LockActionEventHandler;
		sAppTask.PostEvent(button_event);
	}

	if (BLE_ADVERTISEMENT_START_BUTTON_MASK & button_state & has_changed) {
		button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_BUTTON;
		button_event.ButtonEvent.Action = kButtonPushEvent;
		button_event.Handler = StartBLEAdvertisementHandler;
		sAppTask.PostEvent(button_event);
	}
#endif

	if (FUNCTION_BUTTON_MASK & has_changed) {
		button_event.ButtonEvent.PinNo = FUNCTION_BUTTON;
		button_event.ButtonEvent.Action =
			(FUNCTION_BUTTON_MASK & button_state) ? kButtonPushEvent : kButtonReleaseEvent;
		button_event.Handler = FunctionHandler;
		sAppTask.PostEvent(button_event);
	}
}

void AppTask::TimerEventHandler(k_timer *timer)
{
	AppEvent event;
	event.Type = AppEvent::Timer;
	event.TimerEvent.Context = k_timer_user_data_get(timer);
	event.Handler = FunctionTimerEventHandler;
	sAppTask.PostEvent(event);
}

void AppTask::FunctionTimerEventHandler(const AppEvent &event)
{
	if (event.Type != AppEvent::Timer)
		return;

	/* If we reached here, the button was held past kFactoryResetTriggerTimeout, initiate factory reset */
	if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == Function::SoftwareUpdate) {
		LOG_INF("Factory Reset Triggered. Release button within %ums to cancel.", kFactoryResetTriggerTimeout);

		/* Start timer for kFactoryResetCancelWindowTimeout to allow user to cancel, if required. */
		sAppTask.StartTimer(kFactoryResetCancelWindowTimeout);
		sAppTask.mFunction = Function::FactoryReset;

		/* Turn off all LEDs before starting blink to make sure blink is co-ordinated. */
		sStatusLED.Set(false);
#if NUMBER_OF_LEDS == 4
		sFactoryResetSignalLED.Set(false);
		sFactoryResetSignalLED_1.Set(false);
#endif

		sStatusLED.Blink(500);
#if NUMBER_OF_LEDS == 4
		sFactoryResetSignalLED.Blink(500);
		sFactoryResetSignalLED_1.Blink(500);
#endif
	} else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == Function::FactoryReset) {
		/* Actually trigger Factory Reset */
		sAppTask.mFunction = Function::NoneSelected;

		chip::Server::GetInstance().ScheduleFactoryReset();
	}
}

#ifdef CONFIG_MCUMGR_SMP_BT
void AppTask::RequestSMPAdvertisingStart(void)
{
	AppEvent event;
	event.Type = AppEvent::StartSMPAdvertising;
	event.Handler = [](AppEvent *) { GetDFUOverSMP().StartBLEAdvertising(); };
	sAppTask.PostEvent(event);
}
#endif

void AppTask::FunctionHandler(const AppEvent &event)
{
	if (event.ButtonEvent.PinNo != FUNCTION_BUTTON)
		return;

	/* To trigger software update: press the FUNCTION_BUTTON button briefly (< kFactoryResetTriggerTimeout)
	 * To initiate factory reset: press the FUNCTION_BUTTON for kFactoryResetTriggerTimeout +
	 * kFactoryResetCancelWindowTimeout All LEDs start blinking after kFactoryResetTriggerTimeout to signal factory
	 * reset has been initiated. To cancel factory reset: release the FUNCTION_BUTTON once all LEDs start blinking
	 * within the kFactoryResetCancelWindowTimeout.
	 */
	if (event.ButtonEvent.Action == kButtonPushEvent) {
		if (!sAppTask.mFunctionTimerActive && sAppTask.mFunction == Function::NoneSelected) {
			sAppTask.StartTimer(kFactoryResetTriggerTimeout);

			sAppTask.mFunction = Function::SoftwareUpdate;
		}
	} else {
		/* If the button was released before factory reset got initiated, trigger a software update. */
		if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == Function::SoftwareUpdate) {
			sAppTask.CancelTimer();
			sAppTask.mFunction = Function::NoneSelected;

#ifdef CONFIG_MCUMGR_SMP_BT
			GetDFUOverSMP().StartServer();
#else
			LOG_INF("Software update is disabled");
#endif
		} else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == Function::FactoryReset) {
#if NUMBER_OF_LEDS == 4
			sFactoryResetSignalLED.Set(false);
			sFactoryResetSignalLED_1.Set(false);
#endif
			UpdateStatusLED();
			sAppTask.CancelTimer();
			sAppTask.mFunction = Function::NoneSelected;
			LOG_INF("Factory Reset has been Canceled");
		}
	}
}

void AppTask::StartBLEAdvertisementHandler(const AppEvent &)
{
	if (Server::GetInstance().GetFabricTable().FabricCount() != 0) {
		LOG_INF("Matter service BLE advertising not started - device is already commissioned");
		return;
	}

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE advertising is already enabled");
		return;
	}

	if (Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() != CHIP_NO_ERROR) {
		LOG_ERR("OpenBasicCommissioningWindow() failed");
	}
}

void AppTask::UpdateLedStateEventHandler(const AppEvent &event)
{
	if (event.Type == AppEvent::UpdateLedState) {
		event.UpdateLedStateEvent.LedWidget->UpdateState();
	}
}

void AppTask::LEDStateUpdateHandler(LEDWidget &ledWidget)
{
	AppEvent event;
	event.Type = AppEvent::UpdateLedState;
	event.Handler = UpdateLedStateEventHandler;
	event.UpdateLedStateEvent.LedWidget = &ledWidget;
	sAppTask.PostEvent(event);
}

void AppTask::UpdateStatusLED()
{
	/* Update the status LED.
	 *
	 * If thread and service provisioned, keep the LED On constantly.
	 *
	 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
	 * rate of 100ms.
	 *
	 * Otherwise, blink the LED On for a very short time. */
	if (sIsNetworkProvisioned && sIsNetworkEnabled) {
		sStatusLED.Set(true);
	} else if (sHaveBLEConnections) {
		sStatusLED.Blink(100, 100);
	} else {
		sStatusLED.Blink(50, 950);
	}
}

void AppTask::ChipEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
{
	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
#ifdef CONFIG_CHIP_NFC_COMMISSIONING
		if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started) {
			if (NFCMgr().IsTagEmulationStarted()) {
				LOG_INF("NFC Tag emulation is already started");
			} else {
				ShareQRCodeOverNFC(
					chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
			}
		} else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
			NFCMgr().StopTagEmulation();
		}
#endif
		sHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
		UpdateStatusLED();
		break;
#if defined(CONFIG_NET_L2_OPENTHREAD)
	case DeviceEventType::kDnssdPlatformInitialized:
#if CONFIG_CHIP_OTA_REQUESTOR
		InitBasicOTARequestor();
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
		break;
	case DeviceEventType::kThreadStateChange:
		sIsNetworkProvisioned = ConnectivityMgr().IsThreadProvisioned();
		sIsNetworkEnabled = ConnectivityMgr().IsThreadEnabled();
#elif defined(CONFIG_CHIP_WIFI)
	case DeviceEventType::kWiFiConnectivityChange:
		sIsNetworkProvisioned = ConnectivityMgr().IsWiFiStationProvisioned();
		sIsNetworkEnabled = ConnectivityMgr().IsWiFiStationEnabled();
#if CONFIG_CHIP_OTA_REQUESTOR
		if (event->WiFiConnectivityChange.Result == kConnectivity_Established) {
			InitBasicOTARequestor();
		}
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
#endif
		UpdateStatusLED();
		break;
	default:
		break;
	}
}

void AppTask::CancelTimer()
{
	k_timer_stop(&sFunctionTimer);
	mFunctionTimerActive = false;
}

void AppTask::StartTimer(uint32_t timeoutInMs)
{
	k_timer_start(&sFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
	mFunctionTimerActive = true;
}

void AppTask::LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	switch (state) {
	case BoltLockManager::State::kLockingInitiated:
		LOG_INF("Lock action initiated");
		sLockLED.Blink(50, 50);
		break;
	case BoltLockManager::State::kLockingCompleted:
		LOG_INF("Lock action completed");
		sLockLED.Set(true);
		break;
	case BoltLockManager::State::kUnlockingInitiated:
		LOG_INF("Unlock action initiated");
		sLockLED.Blink(50, 50);
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		LOG_INF("Unlock action completed");
		sLockLED.Set(false);
		break;
	}

	/* Handle changing attribute state in the application */
	sAppTask.UpdateClusterState(state, source);
}

void AppTask::PostEvent(const AppEvent &event)
{
	if (k_msgq_put(&sAppEventQueue, &event, K_NO_WAIT) != 0) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::DispatchEvent(const AppEvent &event)
{
	if (event.Handler) {
		event.Handler(event);
	} else {
		LOG_INF("Event received with no handler. Dropping event.");
	}
}

void AppTask::UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	DlLockState newLockState;

	switch (state) {
	case BoltLockManager::State::kLockingCompleted:
		newLockState = DlLockState::kLocked;
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		newLockState = DlLockState::kUnlocked;
		break;
	default:
		newLockState = DlLockState::kNotFullyLocked;
		break;
	}

	SystemLayer().ScheduleLambda([newLockState, source] {
		chip::app::DataModel::Nullable<chip::app::Clusters::DoorLock::DlLockState> currentLockState;
		chip::app::Clusters::DoorLock::Attributes::LockState::Get(kLockEndpointId, currentLockState);

		if (currentLockState.IsNull()) {
			/* Initialize lock state with start value, but not invoke lock/unlock. */
			chip::app::Clusters::DoorLock::Attributes::LockState::Set(kLockEndpointId, newLockState);
		} else {
			LOG_INF("Updating LockState attribute");

			if (!DoorLockServer::Instance().SetLockState(kLockEndpointId, newLockState, source)) {
				LOG_ERR("Failed to update LockState attribute");
			}
		}
	});
}
