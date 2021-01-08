.. _chip_light_bulb_sample:

Connected Home over IP: Light bulb
##################################

.. contents::
   :local:
   :depth: 2

This light bulb sample demonstrates the usage of the `Connected Home over IP`_ application layer to build a white dimmable light bulb device.
This device works as a CHIP accessory, meaning it can be paired and controlled remotely over a CHIP network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

The sample uses buttons to test changing the light bulb and device states, and LEDs to show the state of these changes.
It can be tested either standalone by using a single DK that runs the light bulb application or remotely over the Thread protocol, which requires more devices.
The remote testing requires either commissioning by the CHIP controller device or using the test mode.

Commissioning into a network
****************************

By default, the CHIP device has Thread disabled, and it must be paired with the CHIP controller over Bluetooth LE to get configuration from it if you want to use the device within a Thread network.
To do this, the device must be made discoverable manually (for security reasons), the controller must get the commissioning information from the CHIP device and provision the device into the network.
For details, see the :ref:`chip_light_bulb_sample_remote_control_commissioning` section.

.. _chip_light_bulb_sample_test_mode:

Test mode
*********

Alternatively to the commissioning procedure, you can use the test mode, which allows to join a Thread network with default static parameters and static cryptographic keys.
|button3_note|

.. note::
    The test mode is not CHIP-compliant and it only works together with CHIP controller and other devices which use the same default configuration.

TODO: Add reference to the light switch sample.
This sample allows light bulb device to cooperate with the light switch device programmed with the `_chip_light_switch_sample`_ to create simplied network and control light bulb remotely without using Android compatible smartphone.
Pressing **Button 3**, besides starting Thread with default configuration, is starting publishing light bulb service messages for predefined period of time in order to advertise light bulb device IP address to the light switch device.
If for some reason light switch device was not able to receive messages in the given time, you can start publishing service again by pressing **Button 3**.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840

TODO: Rephrase it referencing to the light switch.
If you want to commission and control the light bulb device remotely through a Thread network, use the `Android CHIPTool`_ application as the CHIP controller.
You will need a smartphone compatible with Android for this purpose.

User Interface
**************

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short Flash On (50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is waiting for a commissioning application to connect.
    * Rapid Even Flashing (100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected through Bluetooth LE.
    * Short Flash Off (950 ms on/50 ms off) - The device is fully provisioned, but does not yet have full Thread network or service connectivity.
    * Solid On - The device is fully provisioned and has full Thread network and service connectivity.

LED 2:
    Shows the state of the light bulb.
    The following states are possible:

    * Solid On - The light bulb is on.
    * Off - The light bulb is off.

Button 1:
    Initiates the factory reset of the device.

Button 2:
    Changes the light bulb state to the opposite one.

Button 3:
    Starts the Thread networking in the :ref:`test mode <chip_light_bulb_sample_test_mode>` using the default configuration and starts light bulb service publishing for predefined period of time.

Button 4:
    Starts the the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time and makes the device discoverable over Bluetooth LE.
    This button is used during the :ref:`commissioning procedure <chip_light_bulb_sample_remote_control_commissioning>`.

SEGGER J-Link USB port:
    Used for getting logs from the device or communicating with it through the command-line interface.

NFC port with antenna attached:
    Optionally used for obtaining the commissioning information from the CHIP device to start the :ref:`commissioning procedure <chip_light_bulb_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/connectedhomeip/light_bulb`

.. include:: /includes/build_and_run.txt

Testing
=======

After building the sample and programming it to your development kit, test its basic features by performing the following steps:

#. |connect_kit|
#. |connect_terminal|
#. Observe that initially **LED 2** is off.
#. Press **Button 2** to lit the light bulb.
   The **LED 2** should turn on and the following messages appear on the console:

   .. code-block:: console

      I: Turn On Action has been initiated
      I: Turn On Action has been completed

#. Press **Button 2** one more time to turn off the light again.
   The **LED 2** should turn off and the following messages appear on the console:

   .. code-block:: console

      I: Turn Off Action has been initiated
      I: Turn Off Action has been completed

#. Press **Button 1** to initiate factory reset of the device.

The device is rebooted after all its settings are erased.

TODO: Add testing section with the light switch

.. _chip_light_bulb_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the CHIP light bulb device from a Thread network.
You can use one of the following options to enable this option:

* :ref:`chip_light_bulb_sample_remote_control_commissioning` that allows you to set up testing environment and remotely control the sample over a CHIP-enabled Thread network.
* :ref:`chip_light_bulb_sample_test_mode` that allows you to test the sample functionalities in a Thread network with default parameters, without commissioning.
  |button3_note|

.. _chip_light_bulb_sample_remote_control_commissioning:

Commissioning the device
------------------------

To commission the light bulb device, go to the `Commissioning nRF Connect Accessory using Android CHIPTool`_ tutorial and complete the procedures described there.
As part of this tutorial, you will build and flash OpenThread RCP firmware, configure Thread Border Router, build and install `Android CHIPTool`_, commission the device, and send CHIP commands that cover `Testing`_ scenarios.

In CHIP, the commissioning procedure (called rendezvous) is done over Bluetooth LE between a CHIP device and the CHIP controller, where the controller has the commissioner role.
When the procedure is finished, the device should be equipped with all information needed to securely operate in the CHIP network.

During the last part of the commissioning procedure, that is the provisioning operation, Thread network credentials are sent from the CHIP controller to the CHIP device.
As a result, the device is able to join the Thread network and communicate with other Thread devices in the network.

To start the commissioning procedure, the controller must get the commissioning information from the CHIP device.
The data payload, which includes the device discriminator and setup PIN code, is encoded within a QR code, printed to the UART console, and can be shared using an NFC tag.

Dependencies
************

This sample uses Connected Home over IP library which includes the |NCS| platform integration layer:

* `Connected Home over IP`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`

.. |button3_note| replace:: Use **Button 3** to enable this mode after building and running the sample.
