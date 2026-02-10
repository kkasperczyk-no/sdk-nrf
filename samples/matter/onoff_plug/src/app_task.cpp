/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "lib/core/CHIPError.h"
#include "lib/support/CodeUtils.h"

#include <setup_payload/OnboardingCodesUtil.h>

#include <app-common/zap-generated/callback.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::OnOff;
using namespace ::chip::app::Clusters::RandomNumberGenerator;
using namespace ::chip::DeviceLayer;

constexpr EndpointId kOnOffPlugEndpointId = 1;

bool emberAfRandomNumberGeneratorClusterGenerateCallback(chip::app::CommandHandler *commandObj, const chip::app::ConcreteCommandPath &commandPath,
	const RandomNumberGenerator::Commands::Generate::DecodableType &commandData)
{
	int16_t minValue = commandData.minValue;
	int16_t maxValue = commandData.maxValue;


	LOG_INF("Generating random number between %d and %d", minValue, maxValue);

	int16_t randomNumber = sys_rand16_get() % (maxValue - minValue + 1) + minValue;

	LOG_INF("Random number generated: %d", randomNumber);

	Protocols::InteractionModel::Status status = RandomNumberGenerator::Attributes::GeneratedNumber::Set(commandPath.mEndpointId, randomNumber);
	
	commandObj->AddStatus(commandPath, status);

	if (status == Protocols::InteractionModel::Status::Success) {
		return true;
	}

	return false;
}

bool emberAfOnOffClusterCustomOnOffCommandCallback(CommandHandler *commandObj, const ConcreteCommandPath &commandPath, const OnOff::Commands::CustomOnOffCommand::DecodableType &commandData)
{
	LOG_INF("CustomOnOffCommand received");

	commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);

	return true;
}

void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if ((DK_BTN2_MSK & hasChanged) & state) {
		Nrf::PostTask([] {
			Nrf::GetBoard()
				.GetLED(Nrf::DeviceLeds::LED2)
				.Set(!Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());

			SystemLayer().ScheduleLambda([] {
				Protocols::InteractionModel::Status status = Clusters::OnOff::Attributes::OnOff::Set(
					kOnOffPlugEndpointId, Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());

				if (status != Protocols::InteractionModel::Status::Success) {
					LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
				}
			});
		});
	}
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value)
{
	ClusterId clusterId = attributePath.mClusterId;
	AttributeId attributeId = attributePath.mAttributeId;

	if (clusterId == OnOff::Id){
		if (attributeId == OnOff::Attributes::OnOff::Id) {
			LOG_INF("Cluster OnOff: attribute OnOff set to %" PRIu8 "", *value);
	
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(*value);
		} else if (attributeId == OnOff::Attributes::CustomOnOffAttribute::Id) {
			LOG_INF("Cluster OnOff: attribute CustomOnOffAttribute set to %" PRIu8 "", *value);
		}
	} else if (clusterId == RandomNumberGenerator::Id && attributeId == RandomNumberGenerator::Attributes::GeneratedNumber::Id) {
		int16_t generatedNumber = 0;
		memcpy(&generatedNumber, value, sizeof(int16_t));
		LOG_INF("Cluster RandomNumberGenerator: attribute GeneratedNumber set to %d", generatedNumber);
	}
}

void MatterRandomNumberGeneratorPluginServerInitCallback() {

}