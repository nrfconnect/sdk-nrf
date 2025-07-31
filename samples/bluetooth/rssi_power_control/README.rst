.. _bluetooth_automated_power_control:

Bluetooth: Automated power control
##################################

.. contents::
	 :local:
	 :depth: 2

The Automated power control sample demonstrates how to monitor the Bluetooth® Low Energy signal strength (RSSI) and dynamically adjust the transmit (TX) power of a connected Peripheral device.
There are two versions of this sample: One for the central device (in the :file:`rssi_power_control/central` folder) and one for the peripheral device (in the :file:`rssi_power_control/peripheral` folder)

The sample for the central device collects RSSI data and sends power control commands to optimize the connection quality, which in turn optimizes the current consumption.

The sample for the peripheral device is simply advertising its existence, allowing the central device to establish a connection.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You must use two compatible development kits to run this sample, one running the version for the central device (:file:`rssi_power_control/central`) and one running the version for the peripheral device (:file:`rssi_power_control/peripheral`).

Overview
********

This sample illustrates Bluetooth LE power control by having the central device read the RSSI of an active connection and use it to adjust the transmit power of the peripheral version.
The central version directly invokes SoftDevice Controller (SDC) APIs to configure power control behavior based on RSSI measurements.
This approach only works on single-core platforms where the Bluetooth Controller and application run on the same core.

There are two ways to monitor the connection parameters:

* Terminal output - Connects to the Central device's virtual serial port to view RSSI and TX power logs in real time.
* Graphical plotting - Uses the provided script to visualize signal strength over time:

	.. code-block:: console

		python3 tools/plot_path_loss.py --comport COMPORT

.. note::
	Make sure no terminal is open on the same serial port when using the Python plotting script, as it requires exclusive access.
	Example of serial port - ``/dev/ttyACM1``.

This sample works only on single-core SoCs because it directly calls ``sdc_*`` functions (such as :c:func:`sdc_hci_cmd_vs_set_power_control_request_params`) that are only accessible on the core running the Bluetooth Controller.
On dual-core platforms such as nRF5340 or nRF54H20 SoCs, the Bluetooth Controller runs on the network core, making these calls inaccessible from the application core and resulting in linker errors.

Workaround for dual-core SoCs
=============================

To adapt this sample for dual-core SoCs:

1. Remove all ``sdc_*`` function calls from the application.
#. Use Zephyr HCI commands to interface with the controller:

  .. code-block:: c

     struct net_buf *buf = bt_hci_cmd_create(...);
     bt_hci_cmd_send_sync(...);

Refer to the :zephyr:code-sample:`bluetooth_hci_pwr_ctrl` sample or Nordic DevZone for reference implementations.

User interface
**************

The following is the user interface used by the two versions of this sample:

Central device:

* Virtual serial output displays live RSSI and TX power data.
* Optional plotting provides a visual graph of signal variation over time.

Peripheral device:

* No direct user interface.
* Automatically accepts connections and responds to TX power control commands.

Configuration
*************

|config|

Additional configuration
========================

Check and configure the following Kconfig options:

* :kconfig:option:`CONFIG_BT_CENTRAL` - Enables the Bluetooth LE Central role for the Central application.
* :kconfig:option:`CONFIG_BT_PERIPHERAL` - Enables the Bluetooth LE Peripheral role for the Peripheral application.
* :kconfig:option:`CONFIG_BT_DEVICE_NAME` - Sets the advertised name of the Peripheral (default: ``power_control_peripheral``).
* :kconfig:option:`CONFIG_BT_SCAN` - Enables scanning for Bluetooth LE advertisements (Central only).
* :kconfig:option:`CONFIG_BT_SCAN_FILTER_ENABLE` - Enables name-based filtering for Bluetooth scan results (Central only).
* :kconfig:option:`CONFIG_BT_CTLR_CONN_RSSI` - Enables controller-side RSSI reporting for active connections.
* :kconfig:option:`CONFIG_BT_TRANSMIT_POWER_CONTROL` - Enables support for Bluetooth LE transmit power control.
* :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL` - Allows dynamic adjustment of transmit power based on RSSI feedback.
* :kconfig:option:`CONFIG_BT_HCI_RAW` - Enables raw HCI interface for communication between the application and controller cores (used in IPC radio configurations).
* :kconfig:option:`CONFIG_LOG` - Enables logging through Zephyr's logging subsystem (optional but useful for debugging).

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/power_control_demo`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample

.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. Program one development kit with the power control firmware for the peripheral device.
#. Program another development kit with the power control firmware for the central device.
#. Open a terminal on the Central's virtual serial port or run the plotting script.
#. Power on both devices.
#. The Central will scan and connect to the Peripheral automatically.
#. Observe RSSI and TX power data printed or plotted.
#. Move the devices or add obstructions to see dynamic power adjustment.

Sample output
=============

The following output is logged from the central device:

.. code-block:: console

	 [00:00:01.000,000] I: Starting BLE Central
	 [00:00:01.500,000] I: Scanning for peripherals...
	 [00:00:03.020,000] I: Found peripheral: FF:EE:DD:CC:BB:AA (random)
	 [00:00:03.050,000] I: Connection to FF:EE:DD:CC:BB:AA (random)
	 [00:00:03.800,000] I: Connected to FF:EE:DD:CC:BB:AA (random)
	 [00:00:05.010,000] I: Successful initialization of power control
	 [00:00:06.010,000] I: Tx: 0 dBm, RSSI: -33 dBm
	 [00:00:07.010,000] I: Tx: -9 dBm, RSSI: -45 dBm

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nrf_bt_scan_readme`

It also relies on the following Zephyr's Bluetooth and HCI layers:

* :ref:`zephyr:kernel_api`:

	* :file:`include/zephyr/kernel.h`

* :ref:`zephyr:bluetooth_api`:

	* :file:`include/zephyr/bluetooth/addr.h`
	* :file:`include/zephyr/bluetooth/bluetooth.h`
	* :file:`include/zephyr/bluetooth/conn.h`

* :ref:`zephyr:logging_api`:

	* :file:`include/zephyr/logging/log.h`

If using the optional Python plotting script, the following dependencies are required:

* Python 3.6 or newer
* The following Python modules:

	.. code-block:: console

		pip install pyserial matplotlib
