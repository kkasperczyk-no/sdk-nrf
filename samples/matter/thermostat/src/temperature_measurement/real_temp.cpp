/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor.h"

#include <stdint.h>

#include <temp_sensor_manager.h>

#include <DeviceInfoProviderImpl.h>

#include <controller/ReadInteraction.h>
#include <zephyr/logging/log.h>
#include <app-common/zap-generated/ids/Attributes.h>

using namespace ::chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app;

LOG_MODULE_DECLARE(app);

namespace
{

constexpr chip::EndpointId mTemperatureMeasurementEndpointId = 1;

} // namespace

RealSensor::RealSensor()
{
	BindingHandler::Instance().Init(TemperatureMeasurementReadHandler);
}

void RealSensor::TemperatureMeasurement()
{
	BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
	data->ClusterId = Clusters::TemperatureMeasurement::Id;
	data->EndpointId = mTemperatureMeasurementEndpointId;
	data->IsGroup = BindingHandler::Instance().IsGroupBound();
	DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler, reinterpret_cast<intptr_t>(data));
}

void RealSensor::TemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
						   OperationalDeviceProxy *aDevice, void *aContext)
{
	auto onSuccess = [](const ConcreteDataAttributePath &attributePath, const auto &dataResponse) {
		ChipLogProgress(NotSpecified, "Read Thermostat attribute succeeded");
		int16_t responseValue = dataResponse.Value();
		if (!(dataResponse.IsNull())) {
			TempSensorManager::Instance().SetLocalTemperature(responseValue);
		}
	};

	auto onFailure = [](const ConcreteDataAttributePath *attributePath, CHIP_ERROR error) {
		ChipLogError(NotSpecified, "Read Thermostat attribute failed: %" CHIP_ERROR_FORMAT, error.Format());
	};

	VerifyOrReturn(aDevice != nullptr && aDevice->ConnectionReady(), LOG_ERR("Device invalid"););

	Controller::ReadAttribute<Clusters::TemperatureMeasurement::Attributes::MeasuredValue::TypeInfo>(
		aDevice->GetExchangeManager(), aDevice->GetSecureSession().Value(), binding.remote, onSuccess,
		onFailure);

}
