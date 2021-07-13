.. _matter_weather_station_sample:

Matter: Weather station
#######################

.. contents::
   :local:
   :depth: 2

This weather station sample demonstrates the usage of the :ref:`Matter <ug_matter>` (formerly Project Connected Home over IP, Project CHIP) application layer to build a weather station device that lets you remotely gather different kind of data, such as temperature, air pressure, and relative humidity, using the device sensors.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy53_nrf5340_cpuapp

To program and gather logs from the Thingy:53 device, you need the external J-Link programmer.
If you own an nRF5340 DK that has an on-board J-Link programmer, you can also use it for this purpose.

To commission the weather station device and :ref:`control it remotely <matter_weather_station_sample_remote_control>` through a Thread network, you also need the following:

* A Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.

.. note::
    |matter_gn_required_note|

Overview
********

The sample uses single button for controlling the device state.
The weather station device is periodically performing temperature, air pressure, and relative humidity measurements.
You can read the results using the Matter controller.

The Matter controller communicates with the weather station device over the Matter protocol using Zigbee Cluster Library (ZCL).
The library describes data measurements within the proper clusters.

The sample can be tested remotely over the Thread protocol, which requires additional Matter controller device that can be configured either on PC or mobile (for remote testing in a network).
You can start testing after :ref:`building and running the sample <matter_weather_station_sample_remote_control>`.

.. _matter_weather_station_network_mode:

Remote testing in a network
===========================

By default, the Matter accessory device has Thread disabled, and it must be paired with the Matter controller over Bluetooth LE to get configuration from it if you want to use the device within a Thread network.
To do this, the device must be made discoverable over the Bluetooth LE (that is enabled automatically) and the controller must get the commissioning information from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

Configuration
*************

|config|

User interface
**************

Button 1:
    This button is used for performing device factory reset.
    To do so it must be pressed for 6 seconds.
    Releasing the button within the 6-second window cancels the factory reset procedure.

Serial Wire Debug port:
    Used for getting logs from the device or communicating with it through the command-line interface.
    It is enabled only for the debug configuration of a sample.
    See the :ref:`build configuration <matter_weather_station_sample_selecting_build_configuration>` to learn how to select debug configuration.

NFC port with antenna attached:
    Used for obtaining the commissioning information from the Matter accessory device to start the :ref:`commissioning procedure <matter_weather_station_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/weather_station`

.. include:: /includes/build_and_run.txt

.. _matter_weather_station_sample_selecting_build_configuration:

Selecting build configuration
=============================

This sample supports two types of build configurations:

* Debug - enables additional features allowing to verify application behavior like logs or command-line shell.
* Release - enables only necessary application functionalities in order to optimize its performance.

You can find configuration options for both build types in the corresponding `prj_debug.conf` and `prj_release.conf` files.
By default the release configuration is selected.
If you want to build the target with debug configuration, run the following command:

.. parsed-literal::
   :class: highlight

   west build -b thingy53_nrf5340_cpuapp -- -DCMAKE_BUILD_TYPE=debug

Testing
=======

.. note::
    This sample is still in development and offers no testing scenarios at the moment.

.. _matter_weather_station_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter weather station device from a Thread network.
The weather station sample supports `Commissioning the device`_, which allows you to set up testing environment and remotely control the sample over a Matter-enabled Thread network.

.. _matter_weather_station_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_commissioning_start
    :end-before: matter_door_lock_sample_commissioning_end

To start the commissioning procedure, the controller must get the commissioning information from the Matter accessory device.
The data payload, which includes the device discriminator and setup PIN code, is encoded within a QR code and shared using a NFC tag.
When using debug configuration information can be also obtained through the RTT interface logs.

Dependencies
************

This sample uses the Matter library, which includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
