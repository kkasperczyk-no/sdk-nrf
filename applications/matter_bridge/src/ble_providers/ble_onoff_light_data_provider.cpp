/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_onoff_light_data_provider.h"

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include "binding_handler.h"
#include <app/util/binding-table.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

static bt_uuid *sServiceUuid = BT_UUID_LBS;
static bt_uuid *sUuidLED = BT_UUID_LBS_LED;
static bt_uuid *sUuidButton = BT_UUID_LBS_BUTTON;
static bt_uuid *sUuidCcc = BT_UUID_GATT_CCC;

uint8_t BleOnOffLightDataProvider::GattNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data,
						      uint16_t length)
{
	LOG_ERR("XDDDDDDDDDDDDDd");
	BleOnOffLightDataProvider *provider = static_cast<BleOnOffLightDataProvider *>(
		BLEConnectivityManager::Instance().FindBLEProvider(*bt_conn_get_dst(conn)));
	BindingHandler::BindingData *data2;

	VerifyOrExit(data, );
	VerifyOrExit(length == sizeof(mOnOff), );
	VerifyOrExit(provider, );

	/* TODO: Implement invoking command through the binding for OnOff light switch or update state of generic
	 * switch. */
	data2 = Platform::New<BindingHandler::BindingData>();
	data2->EndpointId = 4;
	data2->CommandId = Clusters::OnOff::Commands::Toggle::Id;
	data2->ClusterId = Clusters::OnOff::Id;

	DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler, reinterpret_cast<intptr_t>(data2));

exit:

	return BT_GATT_ITER_CONTINUE;
}

void BleOnOffLightDataProvider::Init()
{
	/* Do nothing in this case */
	BindingHandler::Instance().Init(&SwitchChangedHandler);
}

void BleOnOffLightDataProvider::SwitchChangedHandler(const EmberBindingTableEntry &binding,
						     OperationalDeviceProxy *deviceProxy, void *context)
{
	VerifyOrReturn(context != nullptr, LOG_ERR("Invalid context for the switch handler"););
	BindingHandler::BindingData *data = static_cast<BindingHandler::BindingData *>(context);

	if (binding.type == EMBER_MULTICAST_BINDING && data->IsGroup) {
		switch (data->ClusterId) {
		case Clusters::OnOff::Id:
			OnOffProcessCommand(data->CommandId, binding, nullptr, context);
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
		default:
			ChipLogError(NotSpecified, "Invalid binding unicast command data");
			break;
		}
	}
}

void BleOnOffLightDataProvider::OnOffProcessCommand(CommandId aCommandId, const EmberBindingTableEntry &aBinding,
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

void BleOnOffLightDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
						  size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, clusterId, attributeId, data, dataSize);
	}

	/* Set the previous LED state on the ble device after retrieving the connection. */
	if (Clusters::BridgedDeviceBasicInformation::Id == clusterId &&
	    Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id == attributeId &&
	    sizeof(bool) == dataSize) {
		/* Set the LED state only if the reachable status is true */
		if (*reinterpret_cast<bool *>(data)) {
			UpdateState(Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id,
				    reinterpret_cast<uint8_t *>(&mOnOff));
		}
	}
}

void BleOnOffLightDataProvider::GattWriteCallback(bt_conn *conn, uint8_t err, bt_gatt_write_params *params)
{
	if (!params) {
		return;
	}

	if (params->length != sizeof(mOnOff)) {
		return;
	}

	BleOnOffLightDataProvider *provider = static_cast<BleOnOffLightDataProvider *>(
		BLEConnectivityManager::Instance().FindBLEProvider(*bt_conn_get_dst(conn)));

	if (!provider) {
		return;
	}

	/* Save data received in GATT write response. */
	memcpy(&provider->mOnOff, params->data, params->length);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

CHIP_ERROR BleOnOffLightDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						  uint8_t *buffer)
{
	if (clusterId != Clusters::OnOff::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (!mDevice.mConn) {
		return CHIP_ERROR_INCORRECT_STATE;
	}

	LOG_INF("Updating state of the BleOnOffLightDataProvider, cluster ID: %u, attribute ID: %u.", clusterId,
		attributeId);

	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		memcpy(mGattWriteDataBuffer, buffer, sizeof(mOnOff));

		mGattWriteParams.data = mGattWriteDataBuffer;
		mGattWriteParams.offset = 0;
		mGattWriteParams.length = sizeof(mOnOff);
		mGattWriteParams.handle = mLedCharacteristicHandle;
		mGattWriteParams.func = BleOnOffLightDataProvider::GattWriteCallback;

		int err = bt_gatt_write(mDevice.mConn, &mGattWriteParams);
		if (err) {
			LOG_ERR("GATT write operation failed");
			return CHIP_ERROR_INTERNAL;
		}

		return CHIP_NO_ERROR;
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return CHIP_NO_ERROR;
}

bt_uuid *BleOnOffLightDataProvider::GetServiceUuid()
{
	return sServiceUuid;
}

void BleOnOffLightDataProvider::Subscribe()
{
	VerifyOrReturn(mDevice.mConn, LOG_ERR("Invalid connection object"));

	/* Configure subscription for the button characteristic */
	mGattSubscribeParams.ccc_handle = mCccHandle;
	mGattSubscribeParams.value_handle = mButtonCharacteristicHandle;
	mGattSubscribeParams.value = BT_GATT_CCC_NOTIFY;
	mGattSubscribeParams.notify = BleOnOffLightDataProvider::GattNotifyCallback;

	if (CheckSubscriptionParameters(&mGattSubscribeParams)) {
		int err = bt_gatt_subscribe(mDevice.mConn, &mGattSubscribeParams);
		if (err) {
			LOG_ERR("Subscribe to button characteristic failed with error %d", err);
		}
	} else {
		LOG_ERR("Invalid button subscription parameters provided");
	}
}

int BleOnOffLightDataProvider::ParseDiscoveredData(bt_gatt_dm *discoveredData)
{
	const bt_gatt_dm_attr *gatt_chrc;
	const bt_gatt_dm_attr *gatt_desc;

	gatt_chrc = bt_gatt_dm_char_by_uuid(discoveredData, sUuidLED);
	if (!gatt_chrc) {
		LOG_ERR("No LED characteristic found.");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidLED);
	if (!gatt_desc) {
		LOG_ERR("No LED characteristic value found.");
		return -EINVAL;
	}
	mLedCharacteristicHandle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(discoveredData, sUuidButton);
	if (!gatt_chrc) {
		LOG_ERR("No Button characteristic found.");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidButton);
	if (!gatt_desc) {
		LOG_ERR("No Button characteristic value found.");
		return -EINVAL;
	}
	mButtonCharacteristicHandle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidCcc);
	if (!gatt_desc) {
		LOG_ERR("No CCC descriptor found.");
		return -EINVAL;
	}
	mCccHandle = gatt_desc->handle;

	/* All characteristics are correct so start the new subscription */
	Subscribe();

	return 0;
}

void BleOnOffLightDataProvider::NotifyAttributeChange(intptr_t context)
{
	BleOnOffLightDataProvider *provider = reinterpret_cast<BleOnOffLightDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, &provider->mOnOff,
				    sizeof(provider->mOnOff));
}

bool BleOnOffLightDataProvider::CheckSubscriptionParameters(bt_gatt_subscribe_params *params)
{
	/* If any of these is not met, the bt_gatt_subscribe() generates an assert at runtime */
	VerifyOrReturnValue(params && params->notify, false);
	VerifyOrReturnValue(params->value, false);
	VerifyOrReturnValue(params->ccc_handle, false);

	return true;
}
