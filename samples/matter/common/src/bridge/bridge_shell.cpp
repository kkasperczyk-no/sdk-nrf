/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"
#include "bridged_device_factory.h"

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#ifdef CONFIG_BRIDGED_DEVICE_BLE
#include "ble_bridged_device.h"
#include "ble_connectivity_manager.h"
#endif /* CONFIG_BRIDGED_DEVICE_BLE */

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
static BridgedDeviceDataProvider *add_simulated_provider(int deviceType)
{
	return BridgeFactory::GetSimulatedDataProviderFactory().Create(
		static_cast<BridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);
}
#endif /* CONFIG_BRIDGED_DEVICE_SIMULATED */

#ifdef CONFIG_BRIDGED_DEVICE_BLE

static BridgedDeviceDataProvider *add_ble_provider(int deviceType)
{
	return BridgeFactory::GetBleDataProviderFactory().Create(static_cast<BridgedDevice::DeviceType>(deviceType),
								 BridgeManager::HandleUpdate);
}

struct BluetoothConnectionContext {
	const struct shell *shell;
	int deviceType;
	char nodeLabel[BridgedDevice::kNodeLabelSize];
	BLEBridgedDeviceProvider *provider;
};
#endif /* CONFIG_BRIDGED_DEVICE_BLE */

static void add_device(const struct shell *shell, int deviceType, char *nodeLabel, BridgedDeviceDataProvider *provider)
{
	auto *newBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
		static_cast<BridgedDevice::DeviceType>(deviceType), nodeLabel);
	auto *newDataProvider = provider;

	if (!newBridgedDevice || !newDataProvider) {
		shell_fprintf(shell, SHELL_ERROR, "Cannot allocate device of given type\n");
		return;
	}

	CHIP_ERROR err = GetBridgeManager().AddBridgedDevices(newBridgedDevice, newDataProvider);
	if (err == CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else if (err == CHIP_ERROR_INVALID_STRING_LENGTH) {
		shell_fprintf(shell, SHELL_ERROR, "Error: too long node label (max %d)\n",
			      BridgedDevice::kNodeLabelSize);
	} else if (err == CHIP_ERROR_NO_MEMORY) {
		shell_fprintf(shell, SHELL_ERROR, "Error: no memory\n");
	} else if (err == CHIP_ERROR_INVALID_ARGUMENT) {
		shell_fprintf(shell, SHELL_ERROR, "Error: invalid device type\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: internal\n");
	}
}

#ifdef CONFIG_BRIDGED_DEVICE_BLE
static void bluetooth_device_connected(BLEBridgedDevice *device, struct bt_gatt_dm *discoveredData,
				       bool discoverySucceeded, void *context)
{
	if (context && device && discoveredData && discoverySucceeded) {
		BluetoothConnectionContext *ctx = reinterpret_cast<BluetoothConnectionContext *>(context);

		ctx->provider->MatchBleDevice(device);
		if (ctx->provider->ParseDiscoveredData(discoveredData)) {
			return;
		}

		add_device(ctx->shell, ctx->deviceType, ctx->nodeLabel, ctx->provider);
		chip::Platform::Delete(ctx);
	}
}
#endif /* CONFIG_BRIDGED_DEVICE_BLE */

static int add_bridged_device(const struct shell *shell, size_t argc, char **argv)
{
	int deviceType = strtoul(argv[1], NULL, 0);
	char *nodeLabel = nullptr;

#ifdef CONFIG_BRIDGED_DEVICE_BLE
	int bleDeviceIndex = strtoul(argv[2], NULL, 0);

	if (argv[3]) {
		nodeLabel = argv[3];
	}

	/* The device object can be created once the Bluetooth LE connection will be established. */
	BluetoothConnectionContext *context = chip::Platform::New<BluetoothConnectionContext>();
	context->shell = shell;
	context->deviceType = deviceType;

	if (nodeLabel) {
		strcpy(context->nodeLabel, nodeLabel);
	}

	BridgedDeviceDataProvider *provider = add_ble_provider(context->deviceType);

	if (!provider) {
		return -ENOMEM;
	}

	context->provider = reinterpret_cast<BLEBridgedDeviceProvider *>(provider);
	GetBLEConnectivityManager().Connect(bleDeviceIndex, bluetooth_device_connected, context,
					    context->provider->GetServiceUuid());

#else

	if (argv[2]) {
		nodeLabel = argv[2];
	}

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
	/* The device is simulated, so it can be added immediately. */
	BridgedDeviceDataProvider *provider = add_simulated_provider(deviceType);
	add_device(shell, deviceType, nodeLabel, provider);
#else
	return -ENOTSUP;
#endif /* CONFIG_BRIDGED_DEVICE_SIMULATED */

#endif /* CONFIG_BRIDGED_DEVICE_BLE */

	return 0;
}

static int remove_bridged_device(const struct shell *shell, size_t argc, char **argv)
{
	int endpointId = strtoul(argv[1], NULL, 0);

	if (CHIP_NO_ERROR == GetBridgeManager().RemoveBridgedDevice(endpointId)) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: device not found\n");
	}
	return 0;
}

#ifdef CONFIG_BRIDGED_DEVICE_BLE
static int scan_bridged_devices(const struct shell *shell, size_t argc, char **argv)
{
	shell_fprintf(shell, SHELL_INFO, "Scanning...\n");

	GetBLEConnectivityManager().Scan();

	return 0;
}
#endif /* CONFIG_BRIDGED_DEVICE_BLE */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_matter_bridge,
#ifdef CONFIG_BRIDGED_DEVICE_BLE
	SHELL_CMD_ARG(
		add, NULL,
		"Adds bridged device. \n"
		"Usage: add <bridged_device_type> <ble_device_index> [node_label]\n"
		"* bridged_device_type - the bridged device's type, e.g. 256 - OnOff Light, 770 - TemperatureSensor, 775 - HumiditySensor\n"
		"* ble_device_index - the Bluetooth LE device's index on the list returned by the scan command\n"
		"* node_label - the optional bridged device's node label\n",
		add_bridged_device, 3, 1),
#else
	SHELL_CMD_ARG(
		add, NULL,
		"Adds bridged device. \n"
		"Usage: add <bridged_device_type> [node_label]\n"
		"* bridged_device_type - the bridged device's type, e.g. 256 - OnOff Light, 770 - TemperatureSensor, 775 - HumiditySensor\n"
		"* node_label - the optional bridged device's node label\n",
		add_bridged_device, 2, 1),
#endif /* CONFIG_BRIDGED_DEVICE_BLE */
	SHELL_CMD_ARG(
		remove, NULL,
		"Removes bridged device. \n"
		"Usage: remove <bridged_device_endpoint_id>\n"
		"* bridged_device_endpoint_id - the bridged device's endpoint on which it was previously created\n",
		remove_bridged_device, 2, 0),
#ifdef CONFIG_BRIDGED_DEVICE_BLE
	SHELL_CMD_ARG(scan, NULL,
		      "Scan for Bluetooth LE devices to bridge. \n"
		      "Usage: scan\n",
		      scan_bridged_devices, 1, 0),
#endif /* CONFIG_BRIDGED_DEVICE_BLE */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(matter_bridge, &sub_matter_bridge, "Matter bridge commands", NULL);
