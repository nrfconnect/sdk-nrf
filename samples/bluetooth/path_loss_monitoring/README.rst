.. _bluetooth_path_loss_monitoring:

Bluetooth: Path loss monitoring
###############################

.. contents::
	 :local:
	 :depth: 2

The Path loss monitoring sample demonstrates how to evaluate Bluetooth® LE signal quality using the path loss monitoring feature.
It consists of a Central and a Peripheral device.
The Central continuously monitors the signal strength and classifies the connection quality using onboard LEDs.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

To run this sample, you must use two compatible development kits, one programmed with the Central application, and the other with the Peripheral application.

Overview
********

This sample illustrates Bluetooth LE path loss monitoring by calculating the signal degradation between two connected devices.
The Central application scans for a Peripheral named ``path_loss_peripheral`` and connects to it.
After a successful connection, the Central measures path loss using the following formula:

.. code-block:: none

	 path loss = TX Power - RSSI

The application defines the following three signal quality zones based on the path loss value:

* Low path loss (strong signal) - **LED0** is lit.
* Medium path loss - **LED1** is lit.
* High path loss (weak signal) - **LED2** is lit.

The Peripheral advertises its presence and accepts connections.

User interface
**************

Central
=======

* **LED0:** Indicates **Low** path loss (good signal).
* **LED1:** Indicates **Medium** path loss.
* **LED2:** Indicates **High** path loss (weak signal).

No buttons are used.

Peripheral
==========

No direct user interface.
The device advertises under the name ``path_loss_peripheral``.

Configuration
*************

|config|

Additional configuration
========================

Check and configure the following Kconfig options:

* :kconfig:option:`CONFIG_BT_CENTRAL` - Enables the Bluetooth LE Central role (Central application).
* :kconfig:option:`CONFIG_BT_PERIPHERAL` - Enables the Bluetooth LE Peripheral role (Peripheral application).
* :kconfig:option:`CONFIG_BT_DEVICE_NAME` - Sets the advertising name of the Peripheral.
* :kconfig:option:`CONFIG_BT_SCAN` - Enables scanning for Bluetooth LE advertisements (Central only).
* :kconfig:option:`CONFIG_BT_SCAN_FILTER_ENABLE` - Enables name-based filtering for scan results (Central only).
* :kconfig:option:`CONFIG_BT_PATH_LOSS_MONITORING` - Enables the Bluetooth LE path loss monitoring feature on both devices.
* :kconfig:option:`CONFIG_DK_LIBRARY` - Enables the LEDs on the development kits.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/path_loss_monitoring`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample

.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. Program one DK with the :file:`samples/bluetooth/path_loss_monitoring/central` code.
#. Program another DK with the :file:`samples/bluetooth/path_loss_monitoring/peripheral` code.
#. Power on both devices.
#. The Central will start scanning and connect automatically to the Peripheral.
#. Once connected, observe the LEDs on the Central to evaluate signal strength:

   * Bring the devices closer or move them farther apart to change the signal quality.
   * Optionally, place an obstruction between the devices to see how this affects signal quality.
#. Monitor the virtual serial ports for logs showing connection status and real-time path loss.

Sample output
=============

The following output is logged from the Central device:

.. code-block:: console

	 [00:00:01.000,000] <inf> central_unit: Starting path loss monitoring sample
	 [00:00:01.500,000] <inf> central_unit: Scanning for peripherals...
	 [00:00:03.020,000] <inf> central_unit: Found peripheral: FF:EE:DD:CC:BB:AA
	 [00:00:03.050,000] <inf> central_unit: Connection to FF:EE:DD:CC:BB:AA
	 [00:00:03.800,000] <inf> central_unit: Connected to FF:EE:DD:CC:BB:AA
	 [00:00:03.810,000] <inf> central_unit: Path loss monitoring enabled
	 [00:00:05.010,000] <inf> central_unit: Zone 0. Path loss: 30 dBm
	 [00:00:06.010,000] <inf> central_unit: Zone 1. Path loss: 45 dBm
	 [00:00:07.010,000] <inf> central_unit: Zone 2. Path loss: 63 dBm

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nrf_bt_scan_readme`
* :ref:`dk_buttons_and_leds_readme`

It uses the following Zephyr libraries:

* :file:`include/zephyr/bluetooth/bluetooth.h`
* :file:`include/zephyr/bluetooth/conn.h`
* :file:`include/zephyr/bluetooth/uuid.h`
* :file:`include/zephyr/bluetooth/addr.h`
* :file:`include/zephyr/bluetooth/gap.h`
* :file:`include/zephyr/logging/log.h`
* :file:`include/zephyr/kernel.h`
* :file:`include/bluetooth/scan.h`
* :file:`include/dk_buttons_and_leds.h`
