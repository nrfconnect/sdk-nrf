/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_lbs_data_provider.h"

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
#include "binding/binding_handler.h"
#endif

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace Nrf;

static const bt_uuid *sServiceUuid = BT_UUID_LBS;
static const bt_uuid *sUuidLED = BT_UUID_LBS_LED;
static const bt_uuid *sUuidButton = BT_UUID_LBS_BUTTON;
static const bt_uuid *sUuidCcc = BT_UUID_GATT_CCC;

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
void ProcessCommand(const EmberBindingTableEntry &aBinding, OperationalDeviceProxy *aDevice,
		    Nrf::Matter::BindingHandler::BindingData &aData)
{
	CHIP_ERROR ret = CHIP_NO_ERROR;

	auto onSuccess = [dataRef = Platform::New<Nrf::Matter::BindingHandler::BindingData>(aData)](
				 const ConcreteCommandPath &commandPath, const StatusIB &status,
				 const auto &dataResponse) { Matter::BindingHandler::OnInvokeCommandSucces(dataRef); };

	auto onFailure =
		[dataRef = Platform::New<Nrf::Matter::BindingHandler::BindingData>(aData)](CHIP_ERROR aError) mutable {
			Matter::BindingHandler::OnInvokeCommandFailure(dataRef, aError);
		};

	if (aDevice) {
		/* We are validating connection is ready once here instead of multiple times in each case statement
		 * below. */
		VerifyOrDie(aDevice->ConnectionReady());
	}

	Clusters::OnOff::Commands::Toggle::Type toggleCommand;
	if (aDevice) {
		ret = Controller::InvokeCommandRequest(aDevice->GetExchangeManager(),
						       aDevice->GetSecureSession().Value(), aBinding.remote,
						       toggleCommand, onSuccess, onFailure);
	} else {
		Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();
		ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, aBinding.fabricIndex, aBinding.groupId,
							    toggleCommand);
	}

	if (CHIP_NO_ERROR != ret) {
		LOG_ERR("Invoke command request ERROR: %s", ErrorStr(ret));
	}
}
#endif

uint8_t BleLBSDataProvider::GattNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data,
					       uint16_t length)
{
	BleLBSDataProvider *provider = static_cast<BleLBSDataProvider *>(
		BLEConnectivityManager::Instance().FindBLEProvider(*bt_conn_get_dst(conn)));

	VerifyOrExit(data, );
	VerifyOrExit(provider, );

#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
	VerifyOrExit(length == sizeof(mCurrentSwitchPosition), );

	/* Save data received in the notification. */
	memcpy(&provider->mCurrentSwitchPosition, data, length);
	DeviceLayer::PlatformMgr().ScheduleWork(NotifySwitchCurrentPositionAttributeChange,
						reinterpret_cast<intptr_t>(provider));
#endif

#ifdef CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE
	if (provider->mInvokeCommandCallback) {
		provider->mInvokeCommandCallback(*provider, Clusters::OnOff::Id, Clusters::OnOff::Commands::Toggle::Id,
						 ProcessCommand);
	}
#endif

exit:
	return BT_GATT_ITER_CONTINUE;
}

void BleLBSDataProvider::Init()
{
	/* Do nothing in this case */
}

void BleLBSDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
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

void BleLBSDataProvider::GattWriteCallback(bt_conn *conn, uint8_t err, bt_gatt_write_params *params)
{
	if (!params) {
		return;
	}

	if (params->length != sizeof(mOnOff)) {
		return;
	}

	BleLBSDataProvider *provider = static_cast<BleLBSDataProvider *>(
		BLEConnectivityManager::Instance().FindBLEProvider(*bt_conn_get_dst(conn)));

	if (!provider) {
		return;
	}

	/* Save data received in GATT write response. */
	memcpy(&provider->mOnOff, params->data, params->length);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyOnOffAttributeChange, reinterpret_cast<intptr_t>(provider));
}

CHIP_ERROR BleLBSDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer)
{
	if (clusterId != Clusters::OnOff::Id && clusterId != Clusters::BridgedDeviceBasicInformation::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (!mDevice.mConn) {
		return CHIP_ERROR_INCORRECT_STATE;
	}

	LOG_INF("Updating state of the BleLBSDataProvider, cluster ID: %u, attribute ID: %u.", clusterId, attributeId);

	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		memcpy(mGattWriteDataBuffer, buffer, sizeof(mOnOff));

		mGattWriteParams.data = mGattWriteDataBuffer;
		mGattWriteParams.offset = 0;
		mGattWriteParams.length = sizeof(mOnOff);
		mGattWriteParams.handle = mLedCharacteristicHandle;
		mGattWriteParams.func = BleLBSDataProvider::GattWriteCallback;

		int err = bt_gatt_write(mDevice.mConn, &mGattWriteParams);
		if (err) {
			LOG_ERR("GATT write operation failed");
			return CHIP_ERROR_INTERNAL;
		}

		return CHIP_NO_ERROR;
	}
	case Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id:
		/* Node label is just updated locally and there is no need to propagate the information to the end
		 * device. */
		break;
	default:
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}

	return CHIP_NO_ERROR;
}

const bt_uuid *BleLBSDataProvider::GetServiceUuid()
{
	return sServiceUuid;
}

void BleLBSDataProvider::Subscribe()
{
	VerifyOrReturn(mDevice.mConn, LOG_ERR("Invalid connection object"));

	/* Configure subscription for the button characteristic */
	mGattSubscribeParams.ccc_handle = mCccHandle;
	mGattSubscribeParams.value_handle = mButtonCharacteristicHandle;
	mGattSubscribeParams.value = BT_GATT_CCC_NOTIFY;
	mGattSubscribeParams.notify = BleLBSDataProvider::GattNotifyCallback;
	mGattSubscribeParams.subscribe = nullptr;

	if (CheckSubscriptionParameters(&mGattSubscribeParams)) {
		int err = bt_gatt_subscribe(mDevice.mConn, &mGattSubscribeParams);
		if (err) {
			LOG_ERR("Subscribe to button characteristic failed with error %d", err);
		}
	} else {
		LOG_ERR("Invalid button subscription parameters provided");
	}
}

int BleLBSDataProvider::ParseDiscoveredData(bt_gatt_dm *discoveredData)
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

void BleLBSDataProvider::NotifyOnOffAttributeChange(intptr_t context)
{
	BleLBSDataProvider *provider = reinterpret_cast<BleLBSDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, &provider->mOnOff,
				    sizeof(provider->mOnOff));
}

#ifdef CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE
void BleLBSDataProvider::NotifySwitchCurrentPositionAttributeChange(intptr_t context)
{
	BleLBSDataProvider *provider = reinterpret_cast<BleLBSDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::Switch::Id, Clusters::Switch::Attributes::CurrentPosition::Id,
				    &provider->mCurrentSwitchPosition, sizeof(provider->mCurrentSwitchPosition));
}
#endif

bool BleLBSDataProvider::CheckSubscriptionParameters(bt_gatt_subscribe_params *params)
{
	/* If any of these is not met, the bt_gatt_subscribe() generates an assert at runtime */
	VerifyOrReturnValue(params && params->notify, false);
	VerifyOrReturnValue(params->value, false);
	VerifyOrReturnValue(params->ccc_handle, false);

	return true;
}
