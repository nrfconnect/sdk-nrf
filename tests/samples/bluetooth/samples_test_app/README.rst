.. _samples_test_app:

Bluetooth: BLE Master Test App
##############################

.. contents::
   :local:
   :depth: 2

The BLE Master Test App is a Bluetooth Low Energy (BLE) central application designed for testing basic Bluetooth functionality in peripheral samples. This application acts as a tester with central role to validate BLE peripheral applications in the Nordic Connect SDK (NCS).

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a BLE peripheral device for testing. You can use any of the following peripheral samples:

* :ref:`peripheral_uart`
* :ref:`peripheral_hids_keyboard`
* :ref:`peripheral_hids_mouse`
* :ref:`peripheral_lbs`

Overview
********

The BLE Master Test App implements a BLE central device that can discover and connect to peripheral devices. It performs automatic device discovery, service discovery, connection management, and security support.

The application follows a specific test sequence:

1. **Initialization**: Bluetooth stack initialization and module setup
2. **Scanning**: Active scanning for supported peripheral devices
3. **Connection**: Automatic connection to discovered devices
4. **Service Discovery**: GATT service discovery and logging
5. **Connection Maintenance**: Maintains connection for 5 seconds
6. **Disconnection**: Graceful disconnection
7. **Reconnection**: Waits 3 seconds, then reconnects
8. **Extended Test**: Maintains connection for 10 seconds
9. **Final Disconnection**: Completes the test cycle
10. **Success Indication**: Prints "SUCCESS" when all steps complete

Supported Device Types
======================

The application is configured to discover and test the following BLE peripheral devices:

* Nordic UART Service
* Nordic Throughput
* NCS HIDS Keyboard
* NCS HIDS Mouse
* Nordic LBS (Lightweight Battery Service)
* Test Beacon
* HID devices (via UUID filtering)

Debugging
*********

In this sample, the UART console is used to display test progress and results.
Debug messages are not displayed in the UART console, but are printed by the RTT logger instead.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

FEM support
***********

.. include:: /includes/sample_fem_support.txt

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

CONFIG_BT_CENTRAL - Enable Bluetooth Central role
   Enables the Bluetooth central role for device discovery and connection.

CONFIG_BT_SCAN - Enable Bluetooth scanning
   Enables Bluetooth scanning functionality for device discovery.

CONFIG_BT_GATT_DM - Enable GATT Discovery Manager
   Enables GATT service discovery functionality.

CONFIG_BT_SMP - Enable Security Manager Protocol
   Enables security pairing and bonding support.

Building and running
********************

.. |sample path| replace:: :file:`tests/samples/bluetooth/samples_test_app`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

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
      #. Observe that the text "Starting BLE test application" is printed on the COM listener.
      #. Power on a BLE peripheral device that matches one of the supported device types.
      #. Observe that the application discovers and connects to the peripheral device.
      #. Monitor the console output for successful connection and service discovery.
      #. Verify that the application completes the full test cycle and prints "SUCCESS".

   .. group-tab:: nRF54 DKs

      .. note::
          |nrf54_buttons_leds_numbering|

      1. Connect the device to the computer to access UART 0.
         If you use a development kit, UART 0 is forwarded as a serial port.
         |serial_port_number_list|
      #. |connect_terminal|
      #. Reset the kit.
      #. Observe that the text "Starting BLE test application" is printed on the COM listener.
      #. Power on a BLE peripheral device that matches one of the supported device types.
      #. Observe that the application discovers and connects to the peripheral device.
      #. Monitor the console output for successful connection and service discovery.
      #. Verify that the application completes the full test cycle and prints "SUCCESS".

Expected Behavior
=================

The application provides detailed console output including:

* Bluetooth initialization status
* Device discovery information
* Connection establishment details
* Service discovery results
* Security pairing information
* Test progress and results

Example output:
```
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
```

Testing Workflow
================

Manual Testing
--------------

1. Build and flash the application to a development kit
2. Power on a BLE peripheral device (for example, Nordic UART Service sample)
3. Monitor the console output for successful connection and service discovery
4. Verify that the application completes the full test cycle

Automated Testing
-----------------

The application is designed for integration with automated test frameworks:

* Supports CI/CD pipeline integration
* Provides structured test results
* Includes test procedure documentation
* Compatible with Nordic's test infrastructure

Troubleshooting
===============

Common Issues
-------------

1. **No devices discovered**: Ensure peripheral device is advertising with correct name
2. **Connection failures**: Check device compatibility and Bluetooth stack configuration
3. **Service discovery errors**: Verify peripheral device implements expected services
4. **Build errors**: Ensure all dependencies are properly installed

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
