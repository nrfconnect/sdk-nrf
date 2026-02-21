.. _samples_test_app:

Bluetooth: Bluetooth LE master test app
#######################################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE master test app is a Bluetooth® Low Energy (LE) central application designed to test basic Bluetooth functionality in peripheral samples.
This application acts as a tester in the central role to validate Bluetooth LE peripheral applications in the |NCS|.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires a Bluetooth LE peripheral device for testing.
You can use any of the following peripheral samples:

* :ref:`peripheral_uart`
* :ref:`peripheral_hids_keyboard`
* :ref:`peripheral_hids_mouse`
* :ref:`peripheral_lbs`

Overview
********

The Bluetooth LE master test app implements a Bluetooth LE central device that can discover and connect to peripheral devices.
It performs automatic device discovery, service discovery, connection management, and supports security features.

The application follows a specific test sequence:

1. Initialization - Initializes the Bluetooth stack and sets up required modules.
#. Scanning - Actively scans for supported peripheral devices.
#. Connection - Automatically connects to discovered devices.
#. Service discovery - Discovers and logs GATT services.
#. Connection maintenance - Maintains the connection for five seconds.
#. Disconnection - Disconnects gracefully.
#. Reconnection - Waits three seconds, then reconnects.
#. Extended test - Maintains the connection for 10 seconds.
#. Final disconnection - Completes the test cycle.
#. Success indication - Prints ``SUCCESS`` when all steps are completed.

Supported device types
======================

The application is configured to discover and test the following Bluetooth LE peripheral devices:

* Nordic UART Service
* Nordic Throughput
* NCS HIDS Keyboard
* NCS HIDS Mouse
* Nordic Lightweight Battery Service (LBS)
* Test Beacon
* HID devices (through UUID filtering)

Debugging
*********

In this sample, the UART console is used to display test progress and results.
Debug messages are not displayed in the UART console, instead, they are printed by the RTT logger.

To view debug messages, follow the procedure described in :ref:`testing_rtt_connect`.

FEM support
***********

.. include:: /includes/sample_fem_support.txt

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

* :kconfig:option:`CONFIG_BT_CENTRAL`
* :kconfig:option:`CONFIG_BT_SCAN`
* :kconfig:option:`CONFIG_BT_GATT_DM`
* :kconfig:option:`CONFIG_BT_SMP`

Building and running
********************

.. |sample path| replace:: :file:`tests/samples/bluetooth/samples_test_app`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test the basic functionality:

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      1. Connect the device to the computer to access UART 0.
         If you use a development kit, UART 0 is forwarded as a serial port.
         |serial_port_number_list|
      #. |connect_terminal|
      #. Reset the kit.
      #. Observe that the text ``Starting Bluetooth LE test application`` is printed on the COM listener.
      #. Power on a Bluetooth LE peripheral device that matches one of the supported device types.
      #. Observe that the application discovers and connects to the peripheral device.
      #. Monitor the console output for successful connection and service discovery.
      #. Verify that the application completes the full test cycle and prints ``SUCCESS``.

   .. group-tab:: nRF54 DKs

      .. note::
          |nrf54_buttons_leds_numbering|

      1. Connect the device to the computer to access UART 0.
         If you use a development kit, UART 0 is forwarded as a serial port.
         |serial_port_number_list|
      #. |connect_terminal|
      #. Reset the kit.
      #. Observe that the text ``Starting Bluetooth LE test application`` is printed on the COM listener.
      #. Power on a Bluetooth LE peripheral device that matches one of the supported device types.
      #. Observe that the application discovers and connects to the peripheral device.
      #. Monitor the console output for successful connection and service discovery.
      #. Verify that the application completes the full test cycle and prints ``SUCCESS``.

Sample output
=============

The application provides detailed console output, including:

* Bluetooth initialization status
* Device discovery information
* Connection establishment details
* Service discovery results
* Security pairing information
* Test progress and results

The following is an example of the output:

.. code-block:: console

   Starting BLE test application
   Bluetooth initialized
   UART initialized
   Scan module initialized
   Scanning successfully started
   Filters matched. Address: AA:BB:CC:DD:EE:FF connectable: 1
   Connected
   Service discovery completed
   Disconnect successful
   SUCCESS

Testing workflow
================

The application supports both manual and automated testing.

Manual testing
--------------

To test the sample manually, complete the following steps:

1. Build and flash the application to a development kit.
#. Power on a Bluetooth LE peripheral device (for example, Nordic UART Service sample).
#. Monitor the console output for successful connection and service discovery.
#. Verify that the application completes the full test cycle.

Automated testing
-----------------

The application is designed for integration with automated test frameworks, providing:

* Seamless CI/CD pipeline support.
* Structured test results.
* Comprehensive documentation of test procedures.
* Full compatibility with Nordic Semiconductor's test infrastructure.

Troubleshooting
===============

If you have issues while using the sample, consider the following potential causes and recommended checks:

1. No devices discovered - Ensure the peripheral device is advertising with the correct name.
#. Connection failures - Check device compatibility and verify the Bluetooth stack configuration.
#. Service discovery errors - Verify that the peripheral device implements the expected services.
#. Build errors - Ensure all required dependencies are properly installed.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`boards/arm/nrf*/board.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:api_peripherals`:

   * :file:`include/uart.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
