.. _peripheral_mds:

Bluetooth: Peripheral Memfault Diagnostic Service (MDS)
#######################################################

.. contents::
   :local:
   :depth: 2

The Peripheral Memfault Diagnostic Service sample demonstrates how to use the :ref:`mds_readme` with the `Memfault SDK`_ in an |NCS| Bluetooth application to collect core dumps and metrics.
The Memfault diagnostic data is sent through a Bluetooth gateway.

To get started with Memfault integration in |NCS|, see :ref:`ug_memfault`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

In this sample, the Memfault SDK is used as a module in the |NCS| to collect core dumps, reboot reasons, metrics, and trace events from devices and send them through a Bluetooth gateway to the Memfault cloud.
See `Memfault terminology`_ for more details on the various Memfault concepts.
The sample also includes the BAS functionalities.

Metrics
=======

      The sample shows how to capture user-specific metrics.
      It defines the following metrics:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      * ``button_press_count`` - The number of **Button 3** presses.
      * ``battery_soc_pct`` - The simulated battery level.
      * ``button_elapsed_time_ms`` - The time measured between two **Button 1** presses.

      These metrics are defined in the :file:`samples/bluetooth/peripheral_mds/memfault_config/memfault_metrics_heartbeat_config.def` file.
      For more details about the metrics, see `Memfault: Collecting Device Metrics`_.

      There are also metrics that are specific to |NCS|.
      :ref:`mod_memfault` adds these system proprietary metrics.
      The following metrics are enabled by default in this sample:

      * Bluetooth metrics, enabled and disabled using the :kconfig:option:`CONFIG_MEMFAULT_NCS_BT_METRICS` Kconfig option.

      * ``ncs_bt_connection_count`` - Number of Bluetooth connections.
      * ``ncs_bt_connection_time_ms`` - Bluetooth connection time.

         Time with at least one live Bluetooth connection.
      * ``ncs_bt_bond_count`` - Number of Bluetooth bonds.
      * Stack usage metrics shows the free stack space in bytes.
        Configurable by the :kconfig:option:`CONFIG_MEMFAULT_NCS_STACK_METRICS` Kconfig option.

      * ``ncs_bt_rx_unused_stack`` - HCI RX thread stack.
      * ``ncs_bt_tx_unused_stack`` - HCI TX thread stack.

   .. group-tab:: nRF54 DKs

      The sample shows how to capture user-specific metrics.
      It defines the following metrics:

      * ``button_press_count`` - The number of **Button 2** presses.
      * ``battery_soc_pct`` - The simulated battery level.
      * ``button_elapsed_time_ms`` - The time measured between two **Button 0** presses.

      These metrics are defined in the :file:`samples/bluetooth/peripheral_mds/memfault_config/memfault_metrics_heartbeat_config.def` file.
      For more details about the metrics, see `Memfault: Collecting Device Metrics`_.

      There are also metrics that are specific to |NCS|.
      :ref:`mod_memfault` adds these system proprietary metrics.
      The following metrics are enabled by default in this sample:

      * Bluetooth metrics, enabled and disabled using the :kconfig:option:`CONFIG_MEMFAULT_NCS_BT_METRICS` Kconfig option.

      * ``ncs_bt_connection_count`` - Number of Bluetooth connections.
      * ``ncs_bt_connection_time_ms`` - Bluetooth connection time.

         Time with at least one live Bluetooth connection.
      * ``ncs_bt_bond_count`` - Number of Bluetooth bonds.

      * Stack usage metrics shows the free stack space in bytes.
        Configurable by the :kconfig:option:`CONFIG_MEMFAULT_NCS_STACK_METRICS` Kconfig option.

      * ``ncs_bt_rx_unused_stack`` - HCI RX thread stack.
      * ``ncs_bt_tx_unused_stack`` - HCI TX thread stack.

Error tracking with trace events
================================

      The sample implements the following user-defined trace reason for demonstration purposes:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      ``button_state_changed`` - Collected every time when **Button 2** changes its state.

      The trace events are defined in the :file:`samples/bluetooth/peripheral_mds/memfault_config/memfault_trace_reason_user_config.def` file.
      See `Memfault: Error Tracking with Trace Events`_ for more details about trace events.

   .. group-tab:: nRF54 DKs

      The sample implements the following user-defined trace reason for demonstration purposes:

      ``button_state_changed`` - Collected every time when **Button 1** changes its state.

      The trace events are defined in the :file:`samples/bluetooth/peripheral_mds/memfault_config/memfault_trace_reason_user_config.def` file.
      See `Memfault: Error Tracking with Trace Events`_ for more details about trace events.

Core dumps
==========

      You can trigger core dumps in this sample using the following methods:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

         * **Button 4** - Triggers a hardfault exception by division by zero.
         * ``mfl crash`` shell command - Triggers an assertion fail.

      When a fault occurs, it results in crashes that are captured by Memfault.
      After your development kit reboots and reconnects with the Bluetooth gateway, it sends core dump data to the Memfault cloud for further inspection and analysis.

   .. group-tab:: nRF54 DKs

      You can trigger core dumps in this sample using the following methods:

         * **Button 3** - Triggers a hardfault exception by division by zero.
         * ``mfl crash`` shell command - Triggers an assertion fail.

      When a fault occurs, it results in crashes that are captured by Memfault.
      After your development kit reboots and reconnects with the Bluetooth gateway, it sends core dump data to the Memfault cloud for further inspection and analysis.

Memfault shell
==============

The Bluetooth MDS sample enables a shell interface by default.
You can use it instead of the development kit buttons to trigger a separate event.

For a list of available commands, see `Memfault Demo CLI`_.

.. _peripheral_mds_user_interface:

User interface
**************

The sample supports a simple user interface.
You can control the sample using predefined buttons, while LEDs are used to display information.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when the development kit is connected.

      Button 1:
         Press this button to start time measuring.
         The second press stops time measuring.

      Button 2:
         Triggers the ``button_state_changed`` trace event.

      Button 3:
         Every press of this button is counted under the ``button_press_count`` metric.

      Button 4:
         Simulate a development kit crash by triggering a hardfault exception by division by zero.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when the development kit is connected.

      Button 0:
         Press this button to start time measuring.
         The second press stops time measuring.

      Button 1:
         Triggers the ``button_state_changed`` trace event.

      Button 2:
         Every press of this button is counted under the ``button_press_count`` metric.

      Button 3:
         Simulate a development kit crash by triggering a hardfault exception by division by zero.

Configuration
*************

|config|

The Memfault SDK allows configuring some of its options using Kconfig.
For the options not configurable using Kconfig, use the :file:`samples/bluetooth/peripheral_mds/memfault_config/memfault_platform_config.h` file.
See `Memfault SDK`_ for more information.

To send data to the Memfault cloud through a Bluetooth gateway, you must configure a project key using the :kconfig:option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY` Kconfig option.
You can find your project key in the project settings at `Memfault Dashboards`_.
Use the :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID` Kconfig option to set a static device ID.
For this sample, the device ID is ``ncs-ble-testdevice`` by default.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_mds`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

Before testing, ensure that your device is configured with the project key of your Memfault project.

|test_sample|

Testing with nRF Memfault mobile applications
---------------------------------------------

You can use the `nRF Memfault for Android`_ or the `nRF Memfault for iOS`_ applications for testing the :ref:`mds_readme`.
You can also use them for your custom applications using the Memfault Diagnostic Service.

1. |connect_terminal_ANSI|
#. Reset your development kit.
#. Observe that the sample starts.
#. On your mobile phone with access to the Internet, open the nRF Memfault application.
#. In the mobile application, tap the :guilabel:`Start` button.
#. Look for your device running the Memfault Diagnostic Service in the :guilabel:`Discovered devices` list.
#. Select your device from the list.
#. Tap :guilabel:`Connect` button to connect with your development kit.
#. Wait for the application to establish connection with your development kit.

   In the mobile application, observe that Memfault chunks are forwarded from your device to the Memfault Cloud.
#. Return to the terminal and press Tab to confirm that the Memfault shell is working.
   The shell commands available are displayed.

   To learn about the Memfault shell commands, issue command ``mflt help``.
#. Use the buttons to trigger Memfault crashes, traces and metrics collection.

   See :ref:`peripheral_mds_user_interface` for details about button functions.
#. Explore the Memfault user interface to see the errors and metrics sent from your device.
#. When you have finished collecting diagnostic data, tap :guilabel:`Disconnect` to the close connection with your development kit.

   As the bond information is preserved, you can tap :guilabel:`Connect` again to immediately reconnect to the device.

Testing with MDS BLE gateway script
-----------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Run the following command in the |NCS| root directory to install the MDS BLE gateway script dependencies:

         .. code-block:: console

            pip install --user -r scripts/memfault/requirements.txt

      #. Connect the nRF52 development kit to your PC that uses the :ref:`mds_ble_gateway_script`.
      #. Start the :file:`mds_ble_gateway.py` script with the correct parameters, for example:

         .. code-block:: console

            python3 mds_ble_gateway.py --snr 682900407 --com COM0

      #. Wait for the script to establish a connection with your development kit.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further analysis.
         When connected, the new data is periodically transferred to the cloud with the interval configured in the :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL` Kconfig option.
      #. On the terminal running the script, you can observe the Memfault chunk counter:

         .. code-block:: console

            Sending..
            Forwarded 2 Memfault Chunks

      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The symbol file is located in the build folder: :file:`peripheral_memfault/build/zephyr/zephyr.elf`:

         a. Open `Memfault`_ in a web browser.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.

         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Run the following command in the |NCS| root directory to install the MDS BLE gateway script dependencies:

         .. code-block:: console

            pip install --user -r scripts/memfault/requirements.txt

      #. Connect the nRF52 development kit to your PC that uses the :ref:`mds_ble_gateway_script`.
      #. Start the :file:`mds_ble_gateway.py` script with the correct parameters, for example:

         .. code-block:: console

            python3 mds_ble_gateway.py --snr 682900407 --com COM0

      #. Wait for the script to establish a connection with your development kit.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further analysis.
         When connected, the new data is periodically transferred to the cloud with the interval configured in the :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL` Kconfig option.
      #. On the terminal running the script, you can observe the Memfault chunk counter:

         .. code-block:: console

            Sending..
            Forwarded 2 Memfault Chunks

      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The symbol file is located in the build folder: :file:`peripheral_memfault/build/zephyr/zephyr.elf`:

         a. Open `Memfault`_ in a web browser.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.

         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

Testing with Memfault WebBluetooth Client
-----------------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      .. note::
         The Web Bluetooth API used by the `Memfault WebBluetooth Client`_ is an experimental feature.
         The functionality depends on your browser and computer OS compatibility.

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Open a recent version of the `Google Chrome browser`_.
      #. Run the `Memfault WebBluetooth Client`_ script to forward Memfault diagnostic data to the cloud.
         For more details, see the `Memfault WebBluetooth Client source code`_.
      #. Make sure that your development kit is advertising.
      #. In the browser, click the :guilabel:`Connect` button and select your device from the list.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further the analysis.
         When connected, the new data is periodically flushed to the cloud with the interval configured by the Kconfig option :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL`.
      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The :file:`zephyr.elf` symbol file is located in the build folder :file:`peripheral_memfault/build/zephyr`.

         a. In a web browser, navigate to `Memfault`_.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``.
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.
         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

   .. group-tab:: nRF54 DKs

      .. note::
         The Web Bluetooth API used by the `Memfault WebBluetooth Client`_ is an experimental feature.
         The functionality depends on your browser and computer OS compatibility.

      1. |connect_terminal_ANSI|
      #. Reset your development kit.
      #. Observe that the sample starts.
      #. Open a recent version of the `Google Chrome browser`_.
      #. Run the `Memfault WebBluetooth Client`_ script to forward Memfault diagnostic data to the cloud.
         For more details, see the `Memfault WebBluetooth Client source code`_.
      #. Make sure that your development kit is advertising.
      #. In the browser, click the :guilabel:`Connect` button and select your device from the list.
      #. Upon connection, data already collected by the `Memfault SDK`_ is forwarded to the cloud for further the analysis.
         When connected, the new data is periodically flushed to the cloud with the interval configured by the Kconfig option :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL`.
      #. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed.
         The :file:`zephyr.elf` symbol file is located in the build folder :file:`peripheral_memfault/build/zephyr`.

         a. In a web browser, navigate to `Memfault`_.
         #. Log in to your account and select the project you created earlier.
         #. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left side menu.
            You can see your newly connected device and the software version in the list.
         #. Select the software version number for your device and click :guilabel:`Upload` to upload the symbol file.

      #. Return to the terminal and press the Tab button on your keyboard to confirm that the Memfault shell is working.
         The shell commands available are displayed.

         To learn about the Memfault shell commands, issue command ``mflt help``.
      #. Use the buttons to trigger Memfault crashes, traces and metrics collection.
         See :ref:`peripheral_mds_user_interface` for details about button functions.
      #. Explore the Memfault user interface to see the errors and metrics sent from your device.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`mds_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`mod_memfault`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`samples/bluetooth/gatt/bas.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
