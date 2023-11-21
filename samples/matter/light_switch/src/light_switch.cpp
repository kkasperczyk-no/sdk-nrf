/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "light_switch.h"
#include "app_event.h"
#include "binding_handler.h"

#ifdef CONFIG_CHIP_LIB_SHELL
#include "shell_commands.h"
#endif

#include <app/server/Server.h>
#include <controller/InvokeInteraction.h>
#include <platform/CHIPDeviceLayer.h>
#include <app/util/binding-table.h>
#include <zephyr/logging/log.h>

using namespace chip;
using namespace chip::app;

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

void LightSwitch::Init(chip::EndpointId aLightSwitchEndpoint)
{
	BindingHandler::Instance().Init(&SwitchChangedHandler);
#ifdef CONFIG_CHIP_LIB_SHELL
	SwitchCommands::RegisterSwitchCommands();
#endif
	mLightSwitchEndpoint = aLightSwitchEndpoint;
}

void LightSwitch::InitiateActionSwitch(Action mAction)
{
	BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
	if (data) {
		data->EndpointId = mLightSwitchEndpoint;
		data->ClusterId = Clusters::OnOff::Id;
		switch (mAction) {
		case Action::Toggle:
			data->CommandId = Clusters::OnOff::Commands::Toggle::Id;
			break;
		case Action::On:
			data->CommandId = Clusters::OnOff::Commands::On::Id;
			break;
		case Action::Off:
			data->CommandId = Clusters::OnOff::Commands::Off::Id;
			break;
		default:
			Platform::Delete(data);
			return;
		}
		data->IsGroup = BindingHandler::Instance().IsGroupBound();
		DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler,
							reinterpret_cast<intptr_t>(data));
	}
}

void LightSwitch::DimmerChangeBrightness()
{
	static uint16_t sBrightness;
	BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
	if (data) {
		data->EndpointId = mLightSwitchEndpoint;
		data->CommandId = Clusters::LevelControl::Commands::MoveToLevel::Id;
		data->ClusterId = Clusters::LevelControl::Id;
		/* add to brightness 3 to approximate 1% step of brightness after each call dimmer change. */
		sBrightness += kOnePercentBrightnessApproximation;
		if (sBrightness > kMaximumBrightness) {
			sBrightness = 0;
		}
		data->Value = (uint8_t)sBrightness;
		data->IsGroup = BindingHandler::Instance().IsGroupBound();
		DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler,
							reinterpret_cast<intptr_t>(data));
	}
}

void LightSwitch::SwitchChangedHandler(const EmberBindingTableEntry &binding, OperationalDeviceProxy *deviceProxy,
				       void *context)
{
	VerifyOrReturn(context != nullptr, LOG_ERR("Invalid context for the switch handler"););
	BindingHandler::BindingData *data = static_cast<BindingHandler::BindingData *>(context);

	if (binding.type == EMBER_MULTICAST_BINDING && data->IsGroup) {
		switch (data->ClusterId) {
		case Clusters::OnOff::Id:
			OnOffProcessCommand(data->CommandId, binding, nullptr, context);
			break;
		case Clusters::LevelControl::Id:
			LevelControlProcessCommand(data->CommandId, binding, nullptr, context);
			break;
		default:
			ChipLogError(NotSpecified, "Invalid binding group command data");
			break;
		}
	} else if (binding.type == EMBER_UNICAST_BINDING && !data->IsGroup) {
		switch (data->ClusterId) {
		case Clusters::OnOff::Id:
			OnOffProcessCommand(data->CommandId, binding, deviceProxy, context);
			break;
		case Clusters::LevelControl::Id:
			LevelControlProcessCommand(data->CommandId, binding, deviceProxy, context);
			break;
		default:
			ChipLogError(NotSpecified, "Invalid binding unicast command data");
			break;
		}
	}
}

void LightSwitch::OnOffProcessCommand(CommandId aCommandId, const EmberBindingTableEntry &aBinding,
				      OperationalDeviceProxy *aDevice, void *aContext)
{
	CHIP_ERROR ret = CHIP_NO_ERROR;
	BindingHandler::BindingData *data = reinterpret_cast<BindingHandler::BindingData *>(aContext);

	auto onSuccess = [](const ConcreteCommandPath &commandPath, const StatusIB &status, const auto &dataResponse) {
		BindingHandler::OnInvokeCommandSucces();
	};

	auto onFailure = [dataRef = *data](CHIP_ERROR aError) mutable {
		BindingHandler::OnInvokeCommandFailure(dataRef, aError);
	};

	if (aDevice) {
		/* We are validating connection is ready once here instead of multiple times in each case statement
		 * below. */
		VerifyOrDie(aDevice->ConnectionReady());
	}

	switch (aCommandId) {
	case Clusters::OnOff::Commands::Toggle::Id:
		Clusters::OnOff::Commands::Toggle::Type toggleCommand;
		if (aDevice) {
			ret = Controller::InvokeCommandRequest(aDevice->GetExchangeManager(),
							       aDevice->GetSecureSession().Value(), aBinding.remote,
							       toggleCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, aBinding.fabricIndex,
								    aBinding.groupId, toggleCommand);
		}
		break;

	case Clusters::OnOff::Commands::On::Id:
		Clusters::OnOff::Commands::On::Type onCommand;
		if (aDevice) {
			ret = Controller::InvokeCommandRequest(aDevice->GetExchangeManager(),
							       aDevice->GetSecureSession().Value(), aBinding.remote,
							       onCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, aBinding.fabricIndex,
								    aBinding.groupId, onCommand);
		}
		break;

	case Clusters::OnOff::Commands::Off::Id:
		Clusters::OnOff::Commands::Off::Type offCommand;
		if (aDevice) {
			ret = Controller::InvokeCommandRequest(aDevice->GetExchangeManager(),
							       aDevice->GetSecureSession().Value(), aBinding.remote,
							       offCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, aBinding.fabricIndex,
								    aBinding.groupId, offCommand);
		}
		break;
	default:
		LOG_DBG("Invalid binding command data - commandId is not supported");
		break;
	}
	if (CHIP_NO_ERROR != ret) {
		LOG_ERR("Invoke OnOff Command Request ERROR: %s", ErrorStr(ret));
	}
}

void LightSwitch::LevelControlProcessCommand(CommandId aCommandId, const EmberBindingTableEntry &aBinding,
					     OperationalDeviceProxy *aDevice, void *aContext)
{
	BindingHandler::BindingData *data = reinterpret_cast<BindingHandler::BindingData *>(aContext);

	auto onSuccess = [](const ConcreteCommandPath &commandPath, const StatusIB &status, const auto &dataResponse) {
		BindingHandler::OnInvokeCommandSucces();
	};

	auto onFailure = [dataRef = *data](CHIP_ERROR aError) mutable {
		BindingHandler::OnInvokeCommandFailure(dataRef, aError);
	};

	CHIP_ERROR ret = CHIP_NO_ERROR;

	if (aDevice) {
		/* We are validating connection is ready once here instead of multiple times in each case statement
		 * below. */
		VerifyOrDie(aDevice->ConnectionReady());
	}

	switch (aCommandId) {
	case Clusters::LevelControl::Commands::MoveToLevel::Id: {
		Clusters::LevelControl::Commands::MoveToLevel::Type moveToLevelCommand;
		moveToLevelCommand.level = data->Value;
		if (aDevice) {
			ret = Controller::InvokeCommandRequest(aDevice->GetExchangeManager(),
							       aDevice->GetSecureSession().Value(), aBinding.remote,
							       moveToLevelCommand, onSuccess, onFailure);
		} else {
			Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
			ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, aBinding.fabricIndex,
								    aBinding.groupId, moveToLevelCommand);
		}
	} break;
	default:
		LOG_DBG("Invalid binding command data - commandId is not supported");
		break;
	}
	if (CHIP_NO_ERROR != ret) {
		LOG_ERR("Invoke Group Command Request ERROR: %s", ErrorStr(ret));
	}
}
