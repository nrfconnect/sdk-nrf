.. |matter_name| replace:: Matter bridge
.. |matter_type| replace:: application
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_wifi| replace:: ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |sample path| replace:: :file:`applications/matter_bridge`
.. |matter_qr_code_payload| replace:: MT:Y.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_bridge.png

.. include:: /includes/matter/shortcuts.txt

.. _matter_bridge_app_description:

Application guide
#################

.. contents::
   :local:
   :depth: 2

The Matter bridge application demonstrates how to use the :ref:`Matter <ug_matter>` application layer to build a bridge device.
You can use this application as a reference for creating your own application.

See the :ref:`ug_matter_overview_bridge` page for more information about the Matter bridge specification and architecture.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

To test the Matter bridge application with the :ref:`Bluetooth® LE bridged device <matter_bridge_app_bridged_support>`, you also need the following:

* An additional development kit compatible with one of the following Bluetooth LE samples:

  * :ref:`peripheral_lbs`
  * :zephyr:code-sample:`ble_peripheral_esp`

* A micro-USB cable for every development kit to connect it to the PC.

To commission the Matter bridge device and control it remotely through a Wi-Fi® network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
This requires additional hardware depending on your setup.

.. note::
    |matter_gn_required_note|

.. _matter_bridge_app_overview:

Overview
********

The application uses buttons to control the device state and a single LED to show the state of a device.
The Matter bridge has capability of representing non-Matter bridged devices as dynamic endpoints.
The application supports bridging the following Matter device types:

* On/Off Light
* On/Off Light Switch
* Generic Switch
* Temperature Sensor
* Humidity Sensor

If the Matter device type required by your use case is not supported, you can extend the application.
For information about how to add a new bridged Matter device type to the application, see the :ref:`matter_bridge_app_extending` section.

Except for the On/Off Light Switch, all of the listed device types are enabled by default.
To disable one of them, set any of the following configuration options:

* :option:`CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE` to ``n`` to disable On/Off Light.
* :option:`CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE` to ``n`` to disable Generic Switch
* :option:`CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE` to ``n`` to disable Temperature Sensor.
* :option:`CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE` to ``n`` to disable Humidity Sensor.

Additionally, you can choose to use the On/Off Light Switch implementation instead of the Generic Switch implementation for a switch device.
To enable the On/Off Light Switch implementation, set the following configuration options:

* :option:`CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE` to ``n`` to disable Generic Switch.
* :option:`CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE` to ``y`` to enable On/Off Light Switch.

See :ref:`cmake_options` for instructions on how to add these options to your build.

The Matter bridge device has an additional functionality, enabling it to work as a smart plug.
This feature provides an additional endpoint with an ID equal to 2, which represents Matter on/off smart plug device type functionality.
This means that you can integrate the Matter bridge functionality into your end product, such as a smart plug, and avoid having to use a standalone bridge device.
This is an optional feature and can be enabled by :ref:`Configuring the smart plug functionality <matter_bridge_smart_plug_functionality>`.

.. _matter_bridge_app_bridged_support:

Bridged device support
======================

The application supports two bridged device configurations that are mutually exclusive:

* Simulated bridged device - This configuration simulates the functionality of bridged devices by generating pseudorandom measurement results and changing the light state in fixed-time intervals.
* Bluetooth LE bridged device - This configuration allows to connect a real peripheral Bluetooth LE device to the Matter bridge and represent its functionalities using :ref:`Matter Data Model <ug_matter_overview_data_model>`.
  The application supports the following Bluetooth LE services:

  * Nordic Semiconductor's :ref:`LED Button Service <lbs_readme>` - represented by the Matter On/Off Light and Generic Switch device types.
    The service can be configured to use the On/Off Light Switch instead of the Generic Switch device type.
  * Zephyr's :zephyr:code-sample:`ble_peripheral_esp` - represented by the Matter Temperature Sensor and Humidity Sensor device types.

If the Bluetooth LE service required by your use case is not supported, you can extend the application.
For information about how to add a new Bluetooth LE service support to the application, see the :ref:`matter_bridge_app_extending_ble_service` section.

Depending on the bridged device you want to support in your application, :ref:`enable it using the appropriate Kconfig option <matter_bridge_app_bridged_support_configs>`.
The Matter bridge supports adding and removing bridged devices dynamically at application runtime using `Matter CLI commands`_ from a dedicated Matter bridge shell module.

.. _matter_bridge_app_ui:

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   If pressed while the Matter smart plug functionality is enabled, the button changes the state of the smart plug device.

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   If the :option:`CONFIG_BRIDGED_DEVICE_BT` Kconfig option is set to ``y``, shows the current state of Bridge's Bluetooth LE connectivity.
   The following states are possible:

   * Turned Off - The Bridge device is in the idle state and has no Bluetooth LE devices paired.
   * Solid On - The Bridge device is in the idle state and all connections to the Bluetooth LE bridged devices are stable.
   * Slow Even Flashing (1000 ms on / 1000 ms off) - The Bridge device lost connection to at least one Bluetooth LE bridged device.
   * Even Flashing (300 ms on / 300 ms off) - The scan for Bluetooth LE devices is in progress.
   * Fast Even Flashing (100 ms on / 100 ms off) - The Bridge device is connecting to the Bluetooth LE device and waiting for the Bluetooth LE authentication PIN code.

   If used with the Matter smart plug functionality enabled, it shows the state of the smart plug device.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

.. _matter_bridge_cli:

Matter CLI commands
===================

Use the following commands to control the Matter bridge device:

* :ref:`matter_bridge_cli_add`
* :ref:`matter_bridge_cli_remove`
* :ref:`matter_bridge_cli_list`
* :ref:`matter_bridge_cli_onoff`
* :ref:`matter_bridge_cli_onoff_switch`
* :ref:`matter_bridge_cli_scan`
* :ref:`matter_bridge_cli_add_bluetooth`
* :ref:`matter_bridge_cli_pincode`

To see all available subcommands via the CLI, use the following command:

.. code-block:: console

   matter_bridge help

Use the ``click to show`` on each command below to see the details.

.. _matter_bridge_cli_add:

matter_bridge add
   Adding a simulated bridged device to the Matter bridge

   .. toggle::

      Use the following command:

      .. parsed-literal::
         :class: highlight

         matter_bridge add *<device_type>* *["node_label"]*

      In this command:

      * *<device_type>* is the Matter device type to use for the bridged device.
        The argument is mandatory and accepts the following values:

         * ``15`` - Generic Switch.
         * ``256`` - On/Off Light.
         * ``259`` - On/Off Light Switch.
         * ``770`` - Temperature Sensor.
         * ``775`` - Humidity Sensor.

      * *["node label"]* is the node label for the bridged device.
        The argument is optional and you can use it to better personalize the device in your application.

      Example command:

      .. code-block:: console

         uart:~$ matter_bridge add 256 "Kitchen Light"

.. _matter_bridge_cli_remove:

matter_bridge remove
   Removing a bridged device from the Matter bridge

   .. toggle::

      Use the following command:

      .. parsed-literal::
         :class: highlight

         matter_bridge remove *<bridged_device_endpoint_id>*

      In this command, *<bridged_device_endpoint_id>* is the endpoint ID of the bridged device to be removed.

      Example command:

      .. code-block:: console

         uart:~$ matter_bridge remove 3


.. _matter_bridge_cli_list:

matter_bridge list
   Showing a list of all bridged devices and their endpoints

   .. toggle::

      Use the following command:

      .. parsed-literal::
         :class: highlight

         matter_bridge list

      The terminal output is similar to the following one:

      .. code-block:: console

         Bridged devices list:
         ---------------------------------------------------------------------
         | Endpoint ID |        Name        |           Type
         ---------------------------------------------------------------------
         | 3           | My Light           | OnOffLight (0x0100)
         | 4           | My Temperature     | TemperatureSensor (0x0302)
         | 5           | My Humidity        | HumiditySensor (0x0307)
         | 6           | My Light Switch    | OnOffLightSwitch (0x0103)
         ---------------------------------------------------------------------
         Total: 4 device(s)


.. _matter_bridge_cli_onoff:

matter_bridge onoff
   Controlling a simulated On/Off Light bridged device

   .. toggle::

      Use the following command:

      .. parsed-literal::
         :class: highlight

         matter_bridge onoff *<new_state>* *<endpoint>*

      In this command:

      * *<new_state>*  is the new state (``0`` - off and ``1`` - on) that will be set on the simulated On/Off Light device.
      * *<endpoint>*  is the endpoint on which the bridged On/Off Light device is implemented

      Example command:

      .. code-block:: console

         uart:~$ matter_bridge onoff 1 3

      Note that the above command will only work if the :option:`CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL` option is selected in the build configuration.
      If the Kconfig option is not selected, the simulated device changes its state periodically in autonomous manner and cannot be controlled by using shell commands.

.. _matter_bridge_cli_onoff_switch:

matter_bridge onoff_switch
   Controlling a simulated On/Off Light Switch bridged device

   .. toggle::

      Use the following command:

      .. parsed-literal::
         :class: highlight

         matter_bridge onoff_switch *<new_state>* *<endpoint>*

      In this command:

      * *<new_state>*  is the new state (``0`` - off and ``1`` - on) that will be set on the simulated On/Off Light Switch device.
      * *<endpoint>*  is the endpoint on which the bridged On/Off Light Switch device is implemented

      Example command:

      .. code-block:: console

         uart:~$ matter_bridge onoff_switch 1 3

.. _matter_bridge_cli_scan:

matter_bridge scan
   Getting a list of Bluetooth LE devices available to be added

   .. toggle::

      Use the following command:

      .. code-block:: console
         :class: highlight

         matter_bridge scan

      The terminal output is similar to the following one:

      .. code-block:: console

         Scan result:
         ---------------------------------------------------------------------
         | Index |      Address      |                   UUID
         ---------------------------------------------------------------------
         | 0     | e6:11:40:96:a0:18 | 0x181a (Environmental Sensing Service)
         | 1     | c7:44:0f:3e:bb:f0 | 0xbcd1 (Led Button Service)

.. _matter_bridge_cli_add_bluetooth:

matter_bridge add <bluetooth>
   Adding a Bluetooth LE bridged device to the Matter bridge

   .. toggle::

      Use the following command:

      .. parsed-literal::
         :class: highlight

         matter_bridge add *<ble_device_index>* *["node label"]*

      In this command:

      * *<ble_device_index>* is the Bluetooth LE device index on the list returned by the ``scan`` command.
        The argument is mandatory and accepts only the values returned by the ``scan`` command.

      * *["node label"]* is the node label for the bridged device.
        The argument is optional and you can use it to better personalize the device in your application.

      Example command:

      .. code-block:: console

         uart:~$ matter_bridge add 0 "Kitchen Light"

      The terminal output is similar to the following one:

      .. code-block:: console

         ----------------------------------------------------------------------------------------
         | Bridged Bluetooth LE device authentication                                           |
         |                                                                                      |
         | Insert pin code displayed by the Bluetooth LE peripheral device                      |
         | to authenticate the pairing operation.                                               |
         |                                                                                      |
         | To do that, use matter_bridge pincode <ble_device_index> <pincode> shell command.    |
         ----------------------------------------------------------------------------------------

      To complete the adding process, you must use the ``pincode`` command to insert the authentication pincode displayed by the bridged device.

.. _matter_bridge_cli_pincode:

matter_bridge pincode
   Inserting a Bluetooth LE authentication pincode

   .. toggle::

      Use the following command:

      .. parsed-literal::
         :class: highlight

         matter_bridge pincode *<ble_device_index>* *<ble_pincode>*

      In this command:

      * *<ble_device_index>* is the Bluetooth LE device index on the list returned by the ``scan`` command.
      * *<ble_pincode>* is the Bluetooth LE authentication pincode of the bridged device to be paired.

      Example command:

      .. code-block:: console

         uart:~$ matter_bridge pincode 0 305051

      The terminal output is similar to the following one:

      .. code-block:: console

         I: Pairing completed: E3:9D:5E:51:AD:14 (random), bonded: 1

         I: Security changed: level 4
         I: The GATT discovery completed
         I: Added device to dynamic endpoint 3 (index=0)
         I: Added device to dynamic endpoint 4 (index=1)
         I: Created 0x100 device type on the endpoint 3
         I: Created 0xf device type on the endpoint 4

Configuration
*************

.. _matter_bridge_app_custom_configs:

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal_53ek.txt

.. _matter_bridge_app_bridged_support_configs:

Bridged device configuration
============================

You can enable the :ref:`matter_bridge_app_bridged_support` by using the following Kconfig options:

* :option:`CONFIG_BRIDGED_DEVICE_SIMULATED` - For the simulated bridged device.
* :option:`CONFIG_BRIDGED_DEVICE_BT` - For the Bluetooth LE bridged device.

The simulated On/Off Light bridged device can operate in the following modes:

* Autonomous - The simulated device periodically changes its state.
  To build the simulated On/Off Light data provider in this mode, select the :option:`CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC` Kconfig option.
* Controllable - The user can explicitly control the On/Off state by using shell commands.
  To build the simulated On/Off Light data provider in this mode, select the :option:`CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL` Kconfig option.
  This is enabled by default.

Additionally, you can decide how many bridged devices the bridge application will support.
The decision will make an impact on the flash and RAM memory usage, and is verified in the compilation stage.
The application uses dynamic memory allocation and stores bridged device objects on the heap, so it may be necessary to increase the heap size using the :kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP_SIZE` Kconfig option.
Use the following configuration options to customize the number of supported bridged devices:

* :option:`CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER` - For changing the maximum number of non-Matter bridged devices supported by the bridge application
* :option:`CONFIG_BRIDGE_MAX_DYNAMIC_ENDPOINTS_NUMBER` - For changing the maximum number of Matter endpoints used for bridging devices by the bridge application.
  This option does not have to be equal to :option:`CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER`, as it is possible to use non-Matter devices that are represented using more than one Matter endpoint.

The following configuration options are available, click on the toggle to see the details:

Configuring the number of Bluetooth LE bridged devices
------------------------------------------------------

.. toggle::

   You have to consider several factors to decide how many Bluetooth LE bridged devices to support.
   Every Bluetooth LE bridged device uses a separate Bluetooth LE connection, so you need to consider the number of supported Bluetooth LE connections (selected using the :kconfig:option:`CONFIG_BT_MAX_CONN` Kconfig option) when you configure the number of devices.
   Since the Matter stack uses one Bluetooth LE connection for commissioning, the maximum number of connections you can use for bridged devices is one less than is configured using the :kconfig:option:`CONFIG_BT_MAX_CONN` Kconfig option.

   Increasing the number of Bluetooth LE connections affects the RAM usage on both the application and network cores.

   .. tabs::

      You can configure the number of Bluetooth LE bridged devices depending on your device and configuration:

      .. group-tab:: nRF53 DKs

         The nRF53 Series supports the Matter bridge over Wi-Fi and Matter bridge over Thread configurations.

         .. tabs::

            .. group-tab:: Matter bridge over Wi-Fi

               You can increase the number of Bluetooth LE connections if you decrease the size of the Bluetooth LE TX/RX buffers used by the Bluetooth controller, but this will decrease the communication throughput.
               The default number of Bluetooth LE connections that you can select using the default configuration is ``10`` for Matter, which effectively means 9 bridged devices.

               Build the target using the following command in the project directory to enable a configuration that increases the number of Bluetooth LE connections to ``20`` (which effectively means 19 bridged devices) by decreasing the size of Bluetooth LE TX/RX buffers:

               .. parsed-literal::
                  :class: highlight

                  west build -b nrf5340dk/nrf5340/cpuapp -p -- -Dmatter_bridge_SHIELD=nrf7002ek -DSB_CONFIG_WIFI_NRF70=y -DCONFIG_BRIDGED_DEVICE_BT=y -DEXTRA_CONF_FILE="bt_max_connections_app.conf" -Dipc_radio_EXTRA_CONF_FILE="bt_max_connections_net.conf" -DFILE_SUFFIX=nrf70ek

            .. group-tab:: Matter bridge over Thread

               You cannot increase the default number of Bluetooth LE connections in this configuration using overlays.
               This is because the configuration uses both Thread and Bluetooth LE protocols, and limited RAM memory.
               You can still increase the number of connections by modifying the board files and decreasing the buffer sizes.
               The default number of connections is ``8``, which effectively means 7 bridged devices.

      .. group-tab:: nRF70 DKs

         The nRF70 Series supports the Matter bridge over Wi-Fi configuration.

         .. tabs::

            .. group-tab:: Matter bridge over Wi-Fi

               You can increase the number of Bluetooth LE connections if you decrease the size of the Bluetooth LE TX/RX buffers used by the Bluetooth controller, but this will decrease the communication throughput.

               Build the target using the following command in the project directory to enable a configuration that increases the number of Bluetooth LE connections to ``20`` (which effectively means 19 bridged devices) by decreasing the sizes of Bluetooth LE TX/RX buffers:

               .. parsed-literal::
                  :class: highlight

                  west build -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_BRIDGED_DEVICE_BT=y -DEXTRA_CONF_FILE="bt_max_connections_app.conf" -Dipc_radio_EXTRA_CONF_FILE="bt_max_connections_net.conf"

      .. group-tab:: nRF54LM20 DKs

         The nRF54LM20 supports the Matter bridge over Wi-Fi and Matter bridge over Thread configurations.

         .. tabs::

            .. group-tab:: Matter bridge over Wi-Fi

               You can increase the number of Bluetooth LE connections, but you may need to decrease the size of the Bluetooth LE TX/RX buffers used by the Bluetooth controller to balance the memory usage, and this will decrease the communication throughput.
               Build the target using the following command in the project directory to enable a configuration that increases the number of Bluetooth LE connections to ``20`` (which effectively means 19 bridged devices) by decreasing the sizes of Bluetooth LE TX/RX buffers:

               .. parsed-literal::
                  :class: highlight

                  west build -b nrf54lm20dk/nrf54lm20a/cpuapp -- -DSB_CONFIG_WIFI_NRF70=y -Dmatter_bridge_SHIELD=nrf7002eb2 -DCONFIG_BRIDGED_DEVICE_BT=y -DEXTRA_CONF_FILE="bt_max_connections_app.conf"

            .. group-tab:: Matter bridge over Thread

               You can increase the number of Bluetooth LE connections, but you may need to decrease the size of the Bluetooth LE TX/RX buffers used by the Bluetooth controller to balance the memory usage, and this will decrease the communication throughput.
               Build the target using the following command in the project directory to enable a configuration that increases the number of Bluetooth LE connections to ``20`` (which effectively means 19 bridged devices) by decreasing the sizes of Bluetooth LE TX/RX buffers:

               .. parsed-literal::
                  :class: highlight

                  west build -b nrf54lm20dk/nrf54lm20a/cpuapp -- -DCONFIG_BRIDGED_DEVICE_BT=y -DEXTRA_CONF_FILE="bt_max_connections_app.conf"

Configuring Bluetooth LE connection and scan parameters
-------------------------------------------------------

.. toggle::

   You can set your own Bluetooth LE connection parameters instead of accepting the default ones requested by the peripheral device.
   You can disable configuring the parameters by setting the :option:`CONFIG_BRIDGE_FORCE_BT_CONNECTION_PARAMS` Kconfig option to ``n``.

   Use the following Kconfig options to set the desired parameters:

   - :option:`CONFIG_BRIDGE_BT_SCAN_WINDOW` - The duration a central actively scans for devices within the scan interval.
   - :option:`CONFIG_BRIDGE_BT_SCAN_INTERVAL` - Time between consecutive Bluetooth LE scan windows.
   - :option:`CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MIN` - The minimum time requested by the central (the bridge) after which the peripheral device should wake up to communicate.
   - :option:`CONFIG_BRIDGE_BT_CONNECTION_INTERVAL_MAX` - The maximum time requested by the central (the bridge) after which the peripheral device should wake up to communicate.
   - :option:`CONFIG_BRIDGE_BT_CONNECTION_TIMEOUT` - The time since the last packet was successfully received until the devices consider the connection lost.
   - :option:`CONFIG_BRIDGE_BT_CONNECTION_LATENCY` - Allows the peripheral to skip waking up for a certain number of connection events if it does not have any data to send.

   The parameters in this application have been selected based on the :ref:`multiprotocol_bt_thread` information in the :ref:`ug_multiprotocol_support` section.

.. _matter_bridge_app_bt_security:

Configuring the Bluetooth LE security
-------------------------------------

.. toggle::

   The application uses Bluetooth LE Security Manager Protocol (SMP) to provide secure connection between the Matter bridge and Bluetooth LE bridged devices.

   .. note::
      Do not confuse the Bluetooth LE Security Manager Protocol (SMP) abbreviation with the Bluetooth LE Simple Management Protocol that has the same abbreviation and is used for the Device Firmware Upgrade process.

   The following security levels are defined by the Bluetooth LE specification:

   * Security Level 1 - supports communication without security.
   * Security Level 2 - supports AES-CMAC communication encryption, but does not require device authentication.
   * Security Level 3 - supports AES-CMAC communication encryption, requires device authentication and pairing.
   * Security Level 4 - supports ECDHE communication encryption, requires authentication and pairing.

   To read more about the Bluetooth LE security implementation in Zephyr, see the Security section of the :ref:`bluetooth-arch` page.
   By default, the Matter bridge application has SMP enabled and supports security levels 2, 3 and 4.

   You can disable the Bluetooth LE security mechanisms by setting the :kconfig:option:`CONFIG_BT_SMP` Kconfig option to ``n``.
   This is strongly not recommended, as it leads to unencrypted communication with bridged devices, which makes them vulnerable to the security attacks.

   You can select the minimum security level required by the application.
   When selected, the Matter bridge will require setting the selected minimum level from the connected Bluetooth LE bridged device.
   If the bridged device supports also levels higher than the selected minimum, the devices may negotiate using the highest shared security level.
   In case the bridged device does not support the minimum required level, the connection will be terminated.
   To select the minimum security level, set the :option:`CONFIG_BRIDGE_BT_MINIMUM_SECURITY_LEVEL` Kconfig option to ``2``, ``3`` or ``4``.

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt

|matter_ble_advertising_auto|

.. _matter_bridge_smart_plug_functionality:

Advanced building options
=========================

.. include:: /includes/matter/building_and_running/advanced/intro.txt
.. include:: /includes/matter/building_and_running/advanced/building_nrf54lm20dk_7002eb2.txt
.. include:: /includes/matter/building_and_running/advanced/wifi_flash.txt

Matter smart plug functionality
-------------------------------

.. toggle::

   To enable the Matter smart plug functionality, run the following command with *board_target* replaced with the board target name:

   .. parsed-literal::
      :class: highlight

      west build -b *board_target* -p -- -DEXTRA_CONF_FILE=onoff_plug.conf

.. _matter_bridge_testing:

Testing
*******

.. include:: /includes/matter/testing/intro.txt

Testing with CHIP Tool
======================

.. |node_id| replace:: 1

Complete the following steps to test the |matter_name| device using CHIP Tool.

.. _prepare_for_testing:

.. include:: /includes/matter/testing/1_prepare_matter_network_thread_wifi.txt
.. include:: /includes/matter/testing/2_prepare_dk.txt
.. include:: /includes/matter/testing/3_commission_thread_wifi.txt

.. rst-class:: numbered-step

Communicate with bridged devices
--------------------------------

Depending on the chosen bridged devices configuration, complete the steps in one of the following tabs:

.. tabs::

   .. group-tab:: Testing with simulated bridged devices

      1. Using the terminal emulator connected to the bridge, run the following :ref:`Matter CLI command <matter_bridge_cli>` to add a new bridged device:

         .. parsed-literal::
            :class: highlight

            uart:~$ matter_bridge add *<device_type>*

         The *<device_type>* is the value of the Matter device type that will be used to represent a new bridged device in the Matter Data Model.
         See the description in :ref:`matter_bridge_cli` for the list of supported values.
         For example, if you want to add a new bridged device that will be exposed as an On/Off Light endpoint, use the following command:

         .. code-block:: console

            uart:~$ matter_bridge add 256

         The terminal output is similar to the following one:

         .. code-block:: console

            I: Adding OnOff Light bridged device
            I: Added device to dynamic endpoint 3 (index=0)

      #. Write down the value for the bridged device dynamic endpoint ID.
         This is going to be used in the next steps (*<bridged_device_endpoint_ID>*).
      #. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` to read the value of an attribute from the bridged device endpoint.
         For example, read the value of the *on-off* attribute from the *onoff* cluster using the following command:

         .. parsed-literal::
            :class: highlight

            ./chip-tool onoff read on-off |node_id| *<bridged_device_endpoint_ID>*

   .. group-tab:: Testing with Bluetooth LE bridged devices

      1. Build and program the one of the following Bluetooth LE samples to an additional development kit compatible with the sample:

         * :ref:`peripheral_lbs`
         * :zephyr:code-sample:`ble_peripheral_esp`

      #. Connect the development kit that is running the Bluetooth LE sample to the PC.
      #. |connect_terminal_ANSI|
      #. Using the terminal emulator connected to the bridge, run the following :ref:`Matter CLI command <matter_bridge_cli>` to scan for available Bluetooth LE devices:

         .. code-block:: console

            uart:~$ matter_bridge scan

         The terminal output is similar to the following one, with an entry for each connected Bluetooth LE device:

         .. code-block:: console

            Scan result:
            ---------------------------------------------------------------------
            | Index |      Address      |                   UUID
            ---------------------------------------------------------------------
            | 0     | e6:11:40:96:a0:18 | 0x181a (Environmental Sensing Service)
            | 1     | c7:44:0f:3e:bb:f0 | 0xbcd1 (Led Button Service)

      #. Write down the value for the desired Bluetooth LE device index.
         This is going to be used in the next steps (*<bluetooth_device_index>*).

      #. Using the terminal emulator connected to the bridge, run the following :ref:`Matter CLI command <matter_bridge_cli>` to add a new bridged device:

         .. parsed-literal::
            :class: highlight

            uart:~$ matter_bridge add *<bluetooth_device_index>*

         The *<bluetooth_device_index>* is a Bluetooth LE device index that was scanned in the previous step.
         For example, if you want to add a new Bluetooth LE bridged device with index ``1``, use the following command:

         .. code-block:: console

            uart:~$ matter_bridge add 1

         The terminal output is similar to the following one:

         .. parsed-literal::
            :class: highlight

            I: Connected: C7:44:0F:3E:BB:F0 (random)
            ----------------------------------------------------------------------------------------
            | Bridged Bluetooth LE device authentication                                           |
            |                                                                                      |
            | Insert pin code displayed by the Bluetooth LE peripheral device                      |
            | to authenticate the pairing operation.                                               |
            |                                                                                      |
            | To do that, use matter_bridge pincode <ble_device_index> <pincode> shell command.    |
            ----------------------------------------------------------------------------------------

      #. Write down the authentication pincode value from the Bluetooth LE bridged device terminal.
         The terminal output is similar to the following one:

         .. parsed-literal::
            :class: highlight

            Passkey for FD:D6:53:EB:92:3A (random): 350501

         In the above example output, the displayed pincode value is  ``350501``.
         It will be used in the next steps as *<bluetooth_authentication_pincode>*.

      #. Insert the authentication pincode of the bridged device in the Matter bridge terminal.
         To insert the pincode, run the following :ref:`Matter CLI command <matter_bridge_cli>` with *<bluetooth_authentication_pincode>* replaced by the value read in the previous step:

         .. parsed-literal::
            :class: highlight

            uart:~$ matter_bridge pincode *<bluetooth_device_index>* *<bluetooth_authentication_pincode>*

         For example, if you want to add a new Bluetooth LE bridged device with index ``1`` and pincode ``350501``, use the following command:

         .. code-block:: console

            uart:~$ matter_bridge pincode 1 350501

         The terminal output is similar to the following one:

         .. parsed-literal::
            :class: highlight

            I: Pairing completed: E3:9D:5E:51:AD:14 (random), bonded: 1

            I: Security changed: level 4
            I: The GATT discovery completed
            I: Added device to dynamic endpoint 3 (index=0)
            I: Added device to dynamic endpoint 4 (index=1)
            I: Created 0x100 device type on the endpoint 3
            I: Created 0xf device type on the endpoint 4

         For the LED Button Service and the Environmental Sensor, two endpoints are created:

         * For the LED Button Service, one implements the On/Off Light device and the other implements the Generic Switch or On/Off Light Switch device.
         * For the Environmental Sensor, one implements the Temperature Sensor and the other implements the Humidity Sensor.

      #. Write down the value for the bridged device dynamic endpoint ID.
         This is going to be used in the next steps (*<bridged_device_endpoint_ID>*).

      #. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` to read the value of an attribute from the bridged device endpoint.
         For example, read the value of the *on-off* attribute from the *onoff* cluster using the following command:

         .. parsed-literal::
            :class: highlight

            ./chip-tool onoff read on-off |node_id| *<bridged_device_endpoint_ID>*


         If you are using the Generic Switch implementation, read the value of the *current-position* attribute from the *switch* cluster using the following command:

         .. parsed-literal::
            :class: highlight

            ./chip-tool switch read current-position |node_id| *<bridged_device_endpoint_ID>*

         Note that the Generic Switch is implemented as a momentary switch.
         This means that, in contrast to the latching switch, it remains switched on only as long as the physical button is pressed.

         In case of the Environmental Sensor, the current temperature and humidity measurements forwarded by the Bluetooth LE Environmental Sensor can be read as follows:

            * temperature:

               .. parsed-literal::
                  :class: highlight

                  ./chip-tool temperaturemeasurement read measured-value |node_id| *<bridged_device_endpoint_ID>*

            * humidity:

               .. parsed-literal::
                  :class: highlight

                  ./chip-tool relativehumiditymeasurement read measured-value |node_id| *<bridged_device_endpoint_ID>*


Testing with bridged device working as a client
===============================================

To test the bridged device working as a client, you need to enable On/Off Light Switch device support (see `matter_bridge_app_overview`_).
The On/Off Light Switch device is capable of controlling the On/Off Light state of another device, such as the :ref:`Matter Light Bulb <matter_light_bulb_sample>` sample working on an additional development kit.
After building this application and the :ref:`Matter Light Bulb <matter_light_bulb_sample>` sample, and programming them to the development kits, complete the following steps to test the bridged device working as a client:

.. rst-class:: numbered-step

Commission the devices to the Matter network
--------------------------------------------

Complete the first three steps from the :ref:`prepare_for_testing` section.

Use two different node ID values for the bridge and the light bulb device.
Write down the values for the bridge and the light bulb device node IDs.
These values are going to be used in the next steps (*<bridge_node_ID>* and *<light_bulb_node_ID>*).

.. rst-class:: numbered-step

Add a bridged light switch device
---------------------------------

Complete the following steps to add a bridged light switch device using either a simulated or a Bluetooth LE device.

.. tabs::

      .. group-tab:: Testing with simulated bridged light switch device

         a. Using the terminal emulator connected to the bridge, run the following :ref:`Matter CLI command <matter_bridge_cli>` to add a new bridged device:

            .. parsed-literal::
               :class: highlight

               uart:~$ matter_bridge add 259

            The terminal output is similar to the following one:

            .. code-block:: console

               I: Adding OnOff Light Switch bridged device
               I: Added device to dynamic endpoint 3 (index=0)

         #. Write down the value for the bridged device dynamic endpoint ID.
            This is going to be used in the next steps (*<bridged_light_switch_endpoint_ID>*).

      .. group-tab:: Testing with Bluetooth LE bridged light switch device

         a. Build and program the :ref:`Peripheral LBS Bluetooth LE <peripheral_lbs>` sample to an additional development kit.
         #. Connect the development kit that is running the Bluetooth LE sample to the PC.
         #. Using the terminal emulator connected to the bridge, run the following :ref:`Matter CLI command <matter_bridge_cli>` to scan for available Bluetooth LE devices:

            .. code-block:: console

               uart:~$ matter_bridge scan

            The terminal output is similar to the following one, with an entry for each connected Bluetooth LE device:

            .. code-block:: console

               Scan result:
               ---------------------------------------------------------------------
               | Index |      Address      |                   UUID
               ---------------------------------------------------------------------
               | 0     | c7:44:0f:3e:bb:f0 | 0xbcd1 (Led Button Service)

         #. Write down the value for the desired Bluetooth LE device index.
            This is going to be used in the next steps (*<bluetooth_device_index>*).
         #. Using the terminal emulator connected to the bridge, run the following :ref:`Matter CLI command <matter_bridge_cli>` to add a new bridged device:

            .. parsed-literal::
               :class: highlight

               uart:~$ matter_bridge add *<bluetooth_device_index>*

            The *<bluetooth_device_index>* is the Bluetooth LE device index that was scanned in the previous step.
            For example, if you want to add a new Bluetooth LE bridged device with index ``0``, use the following command:

            .. code-block:: console

               uart:~$ matter_bridge add 0

            The terminal output is similar to the following one:

            .. parsed-literal::
               :class: highlight

               I: Added device to dynamic endpoint 3 (index=0)
               I: Added device to dynamic endpoint 4 (index=1)
               I: Created 0x100 device type on the endpoint 3
               I: Created 0x103 device type on the endpoint 4

         #. Write down the value for the ``0x103`` bridged device dynamic endpoint ID.
            This is going to be used in the next steps (*<bridged_light_switch_endpoint_ID>*).

.. rst-class:: numbered-step

Add proper ACL for the light bulb device
----------------------------------------

Use the following command, with the *<bridge_node_ID>* and *<light_bulb_node_ID>* values from the first step:

.. parsed-literal::
   :class: highlight

   chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": [*<bridge_node_ID>*], "targets": [{"cluster": 6, "endpoint": 1, "deviceType": null}, {"cluster": 8, "endpoint": 1, "deviceType": null}]}]' *<light_bulb_node_ID>* 0

.. rst-class:: numbered-step

Write a binding table to the bridge to inform the device about all endpoints
----------------------------------------------------------------------------

Use the following command, with the *<bridge_node_ID>* and *<light_bulb_node_ID>* values from the first step:

.. parsed-literal::
   :class: highlight

   chip-tool binding write binding '[{"fabricIndex": 1, "node": *<light_bulb_node_ID>*, "endpoint": 1, "cluster": 6}]' *<bridge_node_ID>* *<bridged_light_switch_endpoint_ID>*

.. rst-class:: numbered-step

Control the light bulb device
-----------------------------

Complete the following steps depending on your configuration:

.. tabs::

   .. group-tab:: Testing with simulated bridged light switch device

      a. Using the terminal emulator connected to the bridge, turn on the |Second LED| located on the bound light bulb device, by running the following :ref:`Matter CLI command <matter_bridge_cli>`:

         .. parsed-literal::
            :class: highlight

            uart:~$ matter_bridge onoff_switch 1 *<bridged_light_switch_endpoint_ID>*

      #. Using the terminal emulator connected to the bridge, turn off the |Second LED| located on the bound light bulb device, by running the following :ref:`Matter CLI command <matter_bridge_cli>`:

         .. parsed-literal::
            :class: highlight

            uart:~$ matter_bridge onoff_switch 0 *<bridged_light_switch_endpoint_ID>*

   .. group-tab:: Testing with Bluetooth LE bridged light switch device

      a. On the Peripheral LBS device, press the |First Button| to turn on the |Second LED| located on the bound light bulb device.
      #. On the Peripheral LBS device, release the |First Button| to turn off the light again.


Testing with commercial ecosystem
=================================

.. include:: /includes/matter/testing/ecosystem.txt

Dependencies
************

.. include:: /includes/matter/dependencies.txt

.. toctree::
   :maxdepth: 1
   :caption: Subpages
   :glob:

   bridge_configs
