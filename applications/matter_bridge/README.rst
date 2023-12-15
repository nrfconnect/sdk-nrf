.. _matter_bridge_app:

Matter bridge
#############

.. contents::
   :local:
   :depth: 2

This Matter bridge application demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a bridge device.
The bridge device allows the use of non-Matter devices in a :ref:`Matter fabric <ug_matter_network_topologies_structure>` by exposing them as Matter endpoints.
The devices on the non-Matter side of the Matter bridge are called *bridged devices*.

The Matter bridge device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power 802.11ax (Wi-Fi 6) network.
You can use this application as a reference for creating your own application.

See :ref:`ug_matter_overview_bridge` for more information about the Matter bridge specification and architecture.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

To test the Matter bridge application with the :ref:`Bluetooth LE bridged device <matter_bridge_app_bridged_support>`, you also need the following:

* An additional development kit compatible with one of the following Bluetooth LE samples:

  * :ref:`peripheral_lbs`
  * :ref:`peripheral_esp`

* A micro-USB cable to connect the development kit to the PC.

To commission the Matter bridge device and control it remotely through a Wi-Fi network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
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

Except for the On/Off Light Switch, all of the listed device types are enabled by default.
To disable one of them, set any of the following configuration options:

* :kconfig:option:`CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE` to ``n`` to disable On/Off Light.
* :kconfig:option:`CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE` to ``n`` to disable Generic Switch
* :kconfig:option:`CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE` to ``n`` to disable Temperature Sensor.
* :kconfig:option:`CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE` to ``n`` to disable Humidity Sensor.

Additionally, you can choose to use the On/Off Light Switch implementation instead of the Generic Switch implementation for a switch device.
To enable the On/Off Light Switch implementation, set the following configuration options:

* :kconfig:option:`CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE` to ``n`` to disable Generic Switch.
* :kconfig:option:`CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE` to ``y`` to enable On/Off Light Switch.

The application supports over-the-air (OTA) device firmware upgrade (DFU) using either of the two following protocols:

* Matter OTA update protocol.
  Querying and downloading a new firmware image is done using the Matter operational network.
* Simple Management Protocol (SMP) over Bluetooth LE.
  The DFU is done using either a smartphone application or a PC command line tool.
  Note that this protocol is not part of the Matter specification.

In both cases, MCUboot secure bootloader is used to apply the new firmware image.

Matter OTA update protocol is enabled by default.
To configure the application to support the DFU over both Matter and SMP, use the ``-DCONFIG_CHIP_DFU_OVER_BT_SMP=y`` build flag.

See :ref:`cmake_options` for instructions on how to add these options to your build.


When building on the command line, run the following command:

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- *dfu_build_flag*

Replace *build_target* with the build target name of the hardware platform you are using (see `Requirements`_), and *dfu_build_flag* with the desired DFU build flag.
For example:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_CHIP_DFU_OVER_BT_SMP=y

For information about how to upgrade the device firmware using a PC or a smartphone, see the :ref:`matter_bridge_app_dfu` section.

.. _matter_bridge_app_bridged_support:

Bridged device support
======================

The application supports two bridged device configurations that are mutually exclusive:

* Simulated bridged device - This configuration simulates the functionality of bridged devices by generating pseudorandom measurement results and changing the light state in fixed-time intervals.
* Bluetooth LE bridged device - This configuration allows to connect a real peripheral Bluetooth LE device to the Matter bridge and represent its functionalities using :ref:`Matter Data Model <ug_matter_overview_data_model>`.
  The application supports the following Bluetooth LE services:

  * Nordic Semiconductor's :ref:`LED Button Service <lbs_readme>` - represented by the Matter On/Off Light and Generic Switch device types.
    The service can be configured to use the On/Off Light Switch instead of the Generic Switch device type.
  * Zephyr's :ref:`Environmental Sensing Service <peripheral_esp>` - represented by the Matter Temperature Sensor and Humidity Sensor device types.

Depending on the bridged device you want to support in your application, :ref:`enable it using the appropriate Kconfig option <matter_bridge_app_bridged_support_configs>`.
The Matter bridge supports adding and removing bridged devices dynamically at application runtime using `Matter CLI commands`_ from a dedicated Matter bridge shell module.

Remote testing in a network
===========================

By default, the Matter accessory device has no IPv6 network configured.
To use the device within a Wi-Fi network, you must pair it with the Matter controller over Bluetooth® LE to get the configuration from the controller.

The Bluetooth LE advertising starts automatically upon device startup, but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, you can re-enable it manually by pressing **Button (SW2)**.

Additionally, the controller must get the `Onboarding information`_ from the Matter accessory device and provision the device into the network.
For details, see the `Testing`_ section.

.. _matter_bridge_app_build_types:

Matter bridge build types
=========================

The bridge application uses different configuration files depending on the supported features.
Configuration files are provided for different build types and they are located in the application root directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by the sample.
This sample supports the following build types:

* ``debug`` -- Debug version of the application.
  You can use this version to enable additional features for verifying the application behavior, such as logs.
* ``release`` -- Release version of the application.
  You can use this version to enable only the necessary application functionalities to optimize its performance.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

User interface
**************

.. include:: ../../samples/matter/lock/README.rst
    :start-after: matter_door_lock_sample_led1_start
    :end-before: matter_door_lock_sample_led1_end

Button 1:
    Depending on how long you press the button:

    * If pressed for less than three seconds:

      * If the device is not provisioned to the Matter network, it initiates the SMP server (Simple Management Protocol) and Bluetooth LE advertising for Matter commissioning.
        After that, the Direct Firmware Update (DFU) over Bluetooth Low Energy can be started.
        (See `Updating the device firmware`_.)
        Bluetooth LE advertising makes the device discoverable over Bluetooth LE for the predefined period of time (15 minutes by default).

      * If the device is already provisioned to the Matter network it re-enables the SMP server.
        After that, the DFU over Bluetooth Low Energy can be started.
        (See `Updating the device firmware`_.)

    * If pressed for more than three seconds, it initiates the factory reset of the device.
      Releasing the button within a 3-second window of the initiation cancels the factory reset procedure.

.. include:: ../../samples/matter/lock/README.rst
    :start-after: matter_door_lock_sample_jlink_start
    :end-before: matter_door_lock_sample_jlink_end

.. _matter_bridge_cli:

Matter CLI commands
===================

You can use the following commands to control the Matter bridge device:

Getting a list of Bluetooth LE devices available to be added
   Use the following command:

   .. code-block:: console

      matter_bridge scan

   The terminal output is similar to the following one:

   .. code-block:: console

      Scan result:
      ---------------------------------------------------------------------
      | Index |      Address      |                   UUID
      ---------------------------------------------------------------------
      | 0     | e6:11:40:96:a0:18 | 0x181a (Environmental Sensing Service)
      | 1     | c7:44:0f:3e:bb:f0 | 0xbcd1 (Led Button Service)

Adding a simulated bridged device to the Matter bridge
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

   * *["node label"]*  is the node label for the bridged device.
      The argument is optional and you can use it to better personalize the device in your application.

   Example command:

   .. code-block:: console

      uart:~$ matter_bridge add 256 "Kitchen Light"

Controlling a simulated On/Off Light bridged device
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

   Note that the above command will only work if the :kconfig:option:`CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL` option is selected in the build configuration.
   If the Kconfig option is not selected, the simulated device changes its state periodically in autonomous manner and can not be controlled by using shell commands.

Controlling a simulated On/Off Light Switch bridged device
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


Adding a Bluetooth LE bridged device to the Matter bridge
   Use the following command:

   .. parsed-literal::
      :class: highlight

      matter_bridge add *<ble_device_index>* *["node label"]*

   In this command:

   * *<ble_device_index>* is the Bluetooth LE device index on the list returned by the ``scan`` command.
     The argument is mandatory and accepts only the values returned by the ``scan`` command.

   * *["node label"]*  is the node label for the bridged device.
     The argument is optional and you can use it to better personalize the device in your application.

   Example command:

   .. code-block:: console

      uart:~$ matter_bridge add 0 "Kitchen Light"

   The terminal output is similar to the following one:

   .. code-block:: console

      I: Added device to dynamic endpoint 3 (index=0)
      I: Added device to dynamic endpoint 4 (index=1)
      I: Created 0x100 device type on the endpoint 3
      I: Created 0xf device type on the endpoint 4


Removing a bridged device from the Matter bridge
   Use the following command:

   .. parsed-literal::
      :class: highlight

      matter_bridge remove *<bridged_device_endpoint_id>*

   In this command, *<bridged_device_endpoint_id>* is the endpoint ID of the bridged device to be removed.

   Example command:

   .. code-block:: console

      uart:~$ matter_bridge remove 3

Configuration
*************

|config|

.. _matter_bridge_app_bridged_support_configs:

Bridged device configuration
============================

You can enable the :ref:`matter_bridge_app_bridged_support` by using the following Kconfig options:

* :kconfig:option:`CONFIG_BRIDGED_DEVICE_SIMULATED` - For the simulated bridged device.
* :kconfig:option:`CONFIG_BRIDGED_DEVICE_BT` - For the Bluetooth LE bridged device.

The simulated On/Off Light bridged device can operate in the following modes:

* Autonomous - The simulated device periodically changes its state.
  To build the simulated On/Off Light data provider in this mode, select the :kconfig:option:`CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC` Kconfig option.
* Controllable - The user can explicitly control the On/Off state by using shell commands.
  To build the simulated On/Off Light data provider in this mode, select the :kconfig:option:`CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL` Kconfig option.
  This is enabled by default.

Additionally, you can decide how many bridged devices the bridge application will support.
The decision will make an impact on the flash and RAM memory usage, and is verified in the compilation stage.
The application uses dynamic memory allocation and stores bridged device objects on the heap, so it may be necessary to increase the heap size using the :kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP_SIZE` Kconfig option.
Use the following configuration options to customize the number of supported bridged devices:

* :kconfig:option:`CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER` - For changing the maximum number of non-Matter bridged devices supported by the bridge application
* :kconfig:option:`CONFIG_BRIDGE_MAX_DYNAMIC_ENDPOINTS_NUMBER` - For changing the maximum number of Matter endpoints used for bridging devices by the bridge application.
  This option does not have to be equal to :kconfig:option:`CONFIG_BRIDGE_MAX_BRIDGED_DEVICES_NUMBER`, as it is possible to use non-Matter devices that are represented using more than one Matter endpoint.

Configuring the number of Bluetooth LE bridged devices
------------------------------------------------------

You have to consider several factors to decide how many Bluetooth LE bridged devices to support.
Every Bluetooth LE bridged device uses a separate Bluetooth LE connection, so you need to consider the number of supported Bluetooth LE connections (selected using the :kconfig:option:`CONFIG_BT_MAX_CONN` Kconfig option) when you configure the number of devices.
Since the Matter stack uses one Bluetooth LE connection for commissioning, the maximum number of connections you can use for bridged devices is one less than is configured using the :kconfig:option:`CONFIG_BT_MAX_CONN` Kconfig option.

Increasing the number of Bluetooth LE connections affects the RAM usage on both the application and network cores.
The current maximum number of Bluetooth LE connections that can be selected using the default configuration is ``10``.
You can increase the number of Bluetooth LE connections if you decrease the size of the Bluetooth LE TX/RX buffers used by the Bluetooth controller, but this will decrease the communication throughput.
Build the target using the following command in the project directory to enable a configuration that increases the number of Bluetooth LE connections to ``20`` by decreasing the sizes of Bluetooth LE TX/RX buffers:

.. parsed-literal::
   :class: highlight

   west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_BRIDGED_DEVICE_BT=y -DOVERLAY_CONFIG="overlay-bt_max_connections_app.conf" -Dhci_rpmsg_OVERLAY_CONFIG="*absoule_path*/overlay-bt_max_connections_net.conf"

Replace *absolute_path* with the absolute path to the Matter bridge application on your local disk.

Building and running
********************

.. |sample path| replace:: :file:`applications/matter_bridge`

.. include:: /includes/build_and_run.txt

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`matter_bridge_app_build_types`, depending on your building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: For example, you can replace the

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf7002dk_nrf5340_cpuapp`` by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b nrf7002dk_nrf5340_cpuapp -d build_nrf7002dk_nrf5340_cpuapp -- -DCONF_FILE=prj_release.conf

The ``build_nrf7002dk_nrf5340_cpuapp`` parameter specifies the output directory for the build files.

.. note::
   If the selected board does not support the selected build type, the build is interrupted.
   For example, if the ``shell`` build type is not supported by the selected board, the following notification appears:

   .. code-block:: console

      File not found: ./ncs/nrf/applications/matter_bridge/configuration/nrf7002dk_nrf5340_cpuapp/prj_shell.conf

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

#. |connect_kit|
#. |connect_terminal_ANSI|
#. Observe that the **LED1** starts flashing (short flash on).
   This means that the sample has automatically started the Bluetooth LE advertising.
#. Commission the device to the Matter network.
   See `Commissioning the device`_ for more information.
   During the commissioning process, write down the value for the bridge node ID.
   This is going to be used in the next steps (*<bridge_node_ID>*).
#. Depending on the chosen bridged devices configuration, complete the steps in one of the following tabs:

   .. tabs::

      .. group-tab:: Testing with simulated bridged devices

         a. Using the terminal emulator connected to the bridge, run the following :ref:`Matter CLI command <matter_bridge_cli>` to add a new bridged device:

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

               ./chip-tool onoff read on-off *<bridge_node_ID>* *<bridged_device_endpoint_ID>*

      .. group-tab:: Testing with Bluetooth LE bridged devices

         a. Build and program the one of the following Bluetooth LE samples to an additional development kit compatible with the sample:

            * :ref:`peripheral_lbs`
            * :ref:`peripheral_esp`

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

               ./chip-tool onoff read on-off *<bridge_node_ID>* *<bridged_device_endpoint_ID>*


            If you are using the Generic Switch implementation, read the value of the *current-position* attribute from the *switch* cluster using the following command:

            .. parsed-literal::
               :class: highlight

               ./chip-tool switch read current-position *<bridge_node_ID>* *<bridged_device_endpoint_ID>*

            Note that the Generic Switch is implemented as a momentary switch.
            This means that, in contrast to the latching switch, it remains switched on only as long as the physical button is pressed.

            In case of the Environmental Sensor, the current temperature and humidity measurements forwarded by the Bluetooth LE Environmental Sensor can be read as follows:

              * temperature:

                .. parsed-literal::
                   :class: highlight

                   ./chip-tool temperaturemeasurement read measured-value *<bridge_node_ID>* *<bridged_device_endpoint_ID>*

              * humidity:

                .. parsed-literal::
                   :class: highlight

                    ./chip-tool relativehumiditymeasurement read measured-value *<bridge_node_ID>* *<bridged_device_endpoint_ID>*

Testing with bridged device working as a client
-----------------------------------------------

To test the bridged device working as a client, you need to enable On/Off Light Switch device support (see `matter_bridge_app_overview_`).
The On/Off Light Switch device is capable of controlling the On/Off Light state of another device, such as the :ref:`Matter Light Bulb <matter_light_bulb_sample>` sample working on an additional development kit.
After building this application and the :ref:`Matter Light Bulb <matter_light_bulb_sample>` sample, and programming them to the development kits, complete the following steps to test the bridged device working as a client:

1. |connect_kit|
#. |connect_terminal_ANSI|
#. If the devices were not erased during the programming, press and hold **Button 1** on each device until the factory reset takes place.
#. Commission the devices to the Matter network.
   See `Commissioning the device`_ for more information.

   During the commissioning process, write down the values for the bridge node ID and the light bulb node ID.
   These IDs are going to be used in the next steps (*<bridge_node_ID>* and *<light_bulb_node_ID>*, respectively).
#. Add a bridged light switch device.
   Complete the following steps to use either a simulated or a Bluetooth LE device.

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

#. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` ("Writing ACL to the ``accesscontrol`` cluster" section) to add proper ACL for the light bulb device.
   Use the following command, with the *<bridge_node_ID>* and *<light_bulb_node_ID>* values from the commissioning step:

   .. parsed-literal::
      :class: highlight

      chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": [*<bridge_node_ID>*], "targets": [{"cluster": 6, "endpoint": 1, "deviceType": null}, {"cluster": 8, "endpoint": 1, "deviceType": null}]}]' *<light_bulb_node_ID>* 0

#. Write a binding table to the bridge to inform the device about all endpoints by running the following command:

   .. parsed-literal::
      :class: highlight

      chip-tool binding write binding '[{"fabricIndex": 1, "node": *<light_bulb_node_ID>*, "endpoint": 1, "cluster": 6}]' *<bridge_node_ID>* *<bridged_light_switch_endpoint_ID>*

#. Complete the following steps depending on your configuration:

   .. tabs::

      .. group-tab:: Testing with simulated bridged light switch device

         a. Using the terminal emulator connected to the bridge, turn on the **LED 2** located on the bound light bulb device, by running the following :ref:`Matter CLI command <matter_bridge_cli>`:

            .. parsed-literal::
               :class: highlight

               uart:~$ matter_bridge onoff_switch 1 *<bridged_light_switch_endpoint_ID>*

         #. Using the terminal emulator connected to the bridge, turn off the **LED 2** located on the bound light bulb device, by running the following :ref:`Matter CLI command <matter_bridge_cli>`:

            .. parsed-literal::
               :class: highlight

               uart:~$ matter_bridge onoff_switch 0 *<bridged_light_switch_endpoint_ID>*

      .. group-tab:: Testing with Bluetooth LE bridged light switch device

         a. On the Peripheral LBS device, press **Button 1** to turn on the **LED 2** located on the bound light bulb device.
         #. On the Peripheral LBS device, release **Button 1** to turn off the light again.


Enabling remote control
=======================

Remote control allows you to control the Matter bridge device from a Wi-Fi network.

`Commissioning the device`_ allows you to set up a testing environment and remotely control the sample over a Matter-enabled Wi-Fi network.

Commissioning the device
------------------------

.. note::
   Before starting the commissioning to Matter procedure, ensure that there is no other Bluetooth LE connection established with the device.

To commission the device, go to the :ref:`ug_matter_gs_testing` page and complete the steps for the Matter network environment and the Matter controller you want to use.
After choosing the configuration, the guide walks you through the following steps:

* Build and install the Matter controller.
* Commission the device.
* Send Matter commands that cover scenarios described in the `Testing`_ section.

If you are new to Matter, the recommended approach is to use :ref:`CHIP Tool for Linux or macOS <ug_matter_configuring_controller>`.

Onboarding information
----------------------

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this application, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Matter bridge application onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: /images/matter_qr_code_bridge.png
            :width: 200px
            :alt: QR code for commissioning the Matter bridge device

       - MT:06PS042C00KA0648G00
       - 34970112332

.. _matter_bridge_app_dfu:

Updating the device firmware
============================

To update the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial in the Matter documentation.

Dependencies
************

This application uses the Matter library, which includes the |NCS| platform integration layer:

* `Matter`_

In addition, the application uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The application depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
