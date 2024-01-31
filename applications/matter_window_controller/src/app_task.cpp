/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "board/led_widget.h"

#include "input_data.h"
#include <ei_wrapper.h>

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu/smp/dfu_over_smp.h"
#endif

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app);

using namespace ::chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app;

namespace
{
enum class FunctionTimerMode { kDisabled, kFactoryResetTrigger, kFactoryResetComplete };
enum class LedState { kAlive, kAdvertisingBle, kConnectedBle, kProvisioned };

#if CONFIG_AVERAGE_CURRENT_CONSUMPTION <= 0
#error Invalid CONFIG_AVERAGE_CURRENT_CONSUMPTION value set
#endif

const device *sBme688SensorDev = DEVICE_DT_GET_ONE(bosch_bme680);
const device *adxl362Dev = DEVICE_DT_GET(DT_NODELABEL(adxl362));

} /* namespace */

k_timer sMeasurementsTimer;
constexpr size_t kMeasurementsIntervalMs = 100;
bool isFirstPrediction = true;
bool isPredictionActive = false;

const float kPredictedValueThreshold = 0.8;
const float kAnomalyThreshold = 6;
static const char *anomalyLabel = "anomaly";
static const char *curent_label;
static uint8_t prediction_streak;
const uint8_t kPredictionStreakThreshold = 2;

const char * parseLabel(const char * label) {
	if (strcmp(label, "sine") == 0) {
		return "rotate";
	} else if (strcmp(label, "triangle") == 0) {
		return "up-down";
	} else if (strcmp(label, "idle") == 0) {
		return "idle";
	} else {
		return label;
	}
}

void AppTask::MeasurementsTimerHandler()
{
	int result = sensor_sample_fetch(adxl362Dev);
	if (result != 0) {
		LOG_ERR("Fetching sensor data from ADX failed with: %d", result);
		return;
	}

	struct sensor_value x;
	struct sensor_value y;
	struct sensor_value z;

	float measurements[3];

	result = sensor_channel_get(adxl362Dev, SENSOR_CHAN_ACCEL_X, &x);
	if (result) {
		LOG_ERR("Get x channel failed (%d)", result);
		return;
	}
	result = sensor_channel_get(adxl362Dev, SENSOR_CHAN_ACCEL_Y, &y);
	if (result) {
		LOG_ERR("Get y channel failed (%d)", result);
		return;
	}
	result = sensor_channel_get(adxl362Dev, SENSOR_CHAN_ACCEL_Z, &z);
	if (result) {
		LOG_ERR("Get z channel failed (%d)", result);
		return;
	}

	measurements[0] = sensor_value_to_double(&x);
	measurements[1] = sensor_value_to_double(&y);
	measurements[2] = sensor_value_to_double(&z);

	// LOG_ERR("Measured values: x %f, y %f, z %f", measurements[0], measurements[1], measurements[2]);

	result = ei_wrapper_add_data(measurements, 3);
	if (result) {
		LOG_ERR("Cannot provide input data (err: %d)", result);
		return;
	}
	// int result = sensor_sample_fetch(adxl362Dev);
	// if (result == 0) {
	// 	LOG_ERR("Fetching sensor data ok");
	// } else {
	// 	LOG_ERR("Fetching sensor data from ADX failed with: %d", result);
	// 	return;
	// }

	// struct sensor_value sensorData;
	// result = sensor_channel_get(adxl362Dev, SENSOR_CHAN_ACCEL_XYZ, &sensorData);
	// if (result == 0) {
	// 	LOG_ERR("Getting sensor data ok");
	// } else {
	// 	LOG_ERR("Getting sensor data from ADX failed with: %d", result);
	// }

	if (isPredictionActive) {
		// LOG_ERR("Prediction in progress, cannot start new one");
		return;
	}

	size_t window_shift;
	size_t frame_shift;

	if (isFirstPrediction) {
		window_shift = 0;
		frame_shift = 0;
	} else {
		window_shift = 0;
		frame_shift = 3;
	}

	result = ei_wrapper_start_prediction(window_shift, frame_shift);
	if (result) {
		LOG_ERR("Cannot start prediction (result: %d)", result);
	} else {
		// LOG_ERR("Prediction started...");
		isFirstPrediction = false;
		isPredictionActive = true;
	}
}

void AppTask::UpdateClustersState()
{
	// const int result = sensor_sample_fetch(sBme688SensorDev);

	// if (result == 0) {

	// } else {
	// 	LOG_ERR("Fetching data from BME688 sensor failed with: %d", result);
	// }
}

void AppTask::UpdateLedState()
{
	if (!Instance().mGreenLED || !Instance().mBlueLED || !Instance().mRedLED) {
		return;
	}

	Instance().mGreenLED->Set(false);
	Instance().mBlueLED->Set(false);
	Instance().mRedLED->Set(false);

	switch (Nrf::GetBoard().GetDeviceState()) {
	case Nrf::DeviceState::DeviceAdvertisingBLE:
		Instance().mBlueLED->Blink(Nrf::LedConsts::StatusLed::Disconnected::kOn_ms,
					   Nrf::LedConsts::StatusLed::Disconnected::kOff_ms);
		break;
	case Nrf::DeviceState::DeviceDisconnected:
		Instance().mGreenLED->Blink(Nrf::LedConsts::StatusLed::Disconnected::kOn_ms,
					    Nrf::LedConsts::StatusLed::Disconnected::kOff_ms);
		break;
	case Nrf::DeviceState::DeviceConnectedBLE:
		Instance().mBlueLED->Blink(Nrf::LedConsts::StatusLed::BleConnected::kOn_ms,
					   Nrf::LedConsts::StatusLed::BleConnected::kOff_ms);
		break;
	case Nrf::DeviceState::DeviceProvisioned:
		Instance().mRedLED->Blink(Nrf::LedConsts::StatusLed::Disconnected::kOn_ms,
					  Nrf::LedConsts::StatusLed::Disconnected::kOff_ms);
		Instance().mBlueLED->Blink(Nrf::LedConsts::StatusLed::Disconnected::kOn_ms,
					   Nrf::LedConsts::StatusLed::Disconnected::kOff_ms);
		break;
	default:
		break;
	}
}

static void EIResultReady(int err)
{
	isPredictionActive = false;
	if (err) {
		LOG_ERR("EIResultReady callback returned error (err: %d)", err);
		return;
	}

	// LOG_ERR("EI classification results:");
	// LOG_ERR("======================");

	const char *label;
	const char *newLabel = curent_label;
	float value;
	float anomaly;
	bool increment = true;

	if (ei_wrapper_classifier_has_anomaly()) {
		err = ei_wrapper_get_anomaly(&anomaly);
		if (err) {
			LOG_ERR("Cannot get anomaly (err: %d)", err);
			return;
		} else {
			// LOG_ERR("Anomaly: %.2f", anomaly);
		}
	}

	err = ei_wrapper_get_next_classification_result(&label, &value, NULL);

	if (err) {
		LOG_ERR("Cannot get classification results (err: %d)", err);
	} else {
		// if (anomaly > kAnomalyThreshold) {
		// 	curent_label = anomalyLabel;
		if (value >= kPredictedValueThreshold) {
			newLabel = label;
		} else {
			increment = false;
		}

		if (newLabel != curent_label) {
			if ((!newLabel || !curent_label) || strcmp(newLabel, curent_label)) {
				curent_label = newLabel;
				prediction_streak = 0;
			}
		}

		if (increment) {
			prediction_streak++;
		}

		if (prediction_streak >= kPredictionStreakThreshold) {
			LOG_ERR("Predicted label: %s \t value:  %.2f", parseLabel(curent_label), value);
			prediction_streak = 0;
			curent_label = "idle";
		}
	}
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	/* Set references for RGB LED */
	mRedLED = &Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED1);
	mGreenLED = &Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2);
	mBlueLED = &Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3);

	if (!Nrf::GetBoard().Init(nullptr, UpdateLedState)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	if (!device_is_ready(sBme688SensorDev)) {
		LOG_ERR("BME688 sensor device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	Nrf::GetDFUOverSMP().StartServer();
#endif

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	if (!device_is_ready(adxl362Dev)) {
		LOG_ERR("ADX sensor device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	} else {
		LOG_ERR("ADX sensor device ready====================");
	}

	int err = ei_wrapper_init(EIResultReady);

	if (err) {
		LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)\n", err);
		return chip::System::MapErrorZephyr(-ENODEV);
	} else {
		LOG_ERR("Edge Impulse wrapper initialized successfully");
	}

	// if (ARRAY_SIZE(input_data) < ei_wrapper_get_window_size()) {
	// 	LOG_ERR("Not enough input data");
	// 	return chip::System::MapErrorZephyr(-ENODEV);
	// }

	// if (ARRAY_SIZE(input_data) % ei_wrapper_get_frame_size() != 0) {
	// 	LOG_ERR("Improper number of input samples");
	// 	return chip::System::MapErrorZephyr(-ENODEV);
	// }

	// LOG_ERR("Machine learning model sampling frequency: %zu", ei_wrapper_get_classifier_frequency());
	// LOG_ERR("Labels assigned by the model:");
	// for (size_t i = 0; i < ei_wrapper_get_classifier_label_count(); i++) {
	// 	LOG_ERR("- %s", ei_wrapper_get_classifier_label(i));
	// }

	// size_t cnt = 0;

	// /* input_data is defined in input_data.h file. */
	// err = ei_wrapper_add_data(&input_data[cnt], ei_wrapper_get_window_size());
	// if (err) {
	// 	LOG_ERR("Cannot provide input data (err: %d)", err);
	// 	LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE");
	// 	return chip::System::MapErrorZephyr(-ENODEV);
	// }
	// cnt += ei_wrapper_get_window_size();

	/* Initialize timers */
	k_timer_init(
		&sMeasurementsTimer, [](k_timer *) { Nrf::PostTask([] { MeasurementsTimerHandler(); }); }, nullptr);
	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}

// const struct {} sensor_manager_def_include_once;

// static struct sm_trigger sensor_trigger = {
// 	.cfg = {
// 		.type = SENSOR_TRIG_MOTION,
// 		.chan = SENSOR_CHAN_ACCEL_XYZ,
// 	},
// 	.activation = {
// 		.type = ACT_TYPE_ABS,
// 		.thresh = FLOAT_TO_SENSOR_VALUE (CONFIG_ADXL362_ACTIVITY_THRESHOLD *
// 			(IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_8G) ? 8.0 :
// 			 IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_4G) ? 4.0 :
// 			 IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_2G) ? 2.0 : 1/0)
// 			 / 2048.0),
// 		.timeout_ms = (CONFIG_APP_SENSOR_SLEEP_TO) * 1000
// 	}
// };

// static const struct caf_sampled_channel accel_chan[] = {
// 	{
// 		.chan = SENSOR_CHAN_ACCEL_XYZ,
// 		.data_cnt = 3,
// 	},
// };

// static const struct sm_sensor_config sensor_configs[] = {
// 	{
// 		.dev = DEVICE_DT_GET(DT_NODELABEL(adxl362)),
// 		.event_descr = CONFIG_ML_APP_SENSOR_EVENT_DESCR,
// 		.chans = accel_chan,
// 		.chan_cnt = ARRAY_SIZE(accel_chan),
// 		.sampling_period_ms = 20,
// 		.active_events_limit = 3,
// 		.trigger = &sensor_trigger,
// 	},
// };
