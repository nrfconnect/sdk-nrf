.. _zigbee_weather_station_app:

Thingy:53: Zigbee weather station
#################################

.. contents::
   :local:
   :depth: 2

The Zigbee weather station is an out-of-the-box application for the Nordic Thingy:53.
It allows you to build a weather station that uses the Thingy:53's sensors to remotely gather different kinds of data, such as temperature, air pressure, and relative humidity.
The application uses the Zigbee protocol, meaning the device can be paired and controlled remotely over a Zigbee network.
You can use this application as a reference for creating your own application.

.. note::
   :ref:`lib_zigbee_fota` is not yet supported for this application.
   See the :ref:`Limitations <lib_zigbee_fota_limitations>` section on the Zigbee FOTA library page.


Application overview
********************

The Zigbee weather station application acts as a :ref:`Zigbee Sleepy End Device <zigbee_ug_sed>`.
During startup, it initializes the Thingy:53's built-in sensors and starts periodic data readouts of temperature, pressure, and relative humidity.
Each readout is followed by an update of Zigbee Cluster Library (ZCL) attributes associated with measurements.
Attributes can be accessed over the Zigbee network according to the ZCL specification, meaning that you can read out a given attribute on demand or create a binding for periodic notifications.

.. WIP: PLACEHOLDER

   OTA and DFU
   ===========

   The application supports over-the-air (OTA) device firmware upgrade (DFU) using the :ref:`lib_zigbee_fota` library.
   The MCUboot secure bootloader is used to apply the new firmware image.
   For information about how to upgrade the device firmware, see the :ref:`zigbee_weather_station_app_dfu` section.

.. _zigbee_weather_station_app_requirements:

Requirements
************

The application supports the following development kit:

.. table-from-sample-yaml::

You also need to program a Zigbee network coordinator on a separate compatible development kit to commission the Zigbee weather station device and control it remotely through a Zigbee network.
The Zigbee network coordinator forms the network that the weather station node will join and share its measurement data with.
See the :ref:`zigbee_weather_station_setup` section for more information.

To program a Thingy:53 device whose preprogrammed MCUboot bootloader has been erased, you need an external J-Link programmer.
If you have an nRF5340 DK that has an onboard J-Link programmer, you can use it for this purpose.

If the Thingy:53 device is programmed with a Thingy:53-compatible sample or application, you can also update the firmware using MCUboot's serial recovery or DFU over Bluetooth® Low Energy (LE).
See :ref:`thingy53_app_guide` for details.

For capturing packets while testing, you can use `nRF Sniffer for 802.15.4`_ with `Wireshark configured for Zigbee <Configuring Wireshark for Zigbee_>`_.

User interface
**************

LED (LD1):
   Shows the overall state of the device and its connectivity.
   The following states are possible:

   * ``release`` build type

     * Even flashing (red color, 500 ms on/500 ms off) - The device is in the Identify mode (after network commissioning).

   * ``debug`` build type

     * Constant light (blue) - The device is connected to a Zigbee network.
     * Even flashing (red color, 500 ms on/500 ms off) - The device is in the Identify mode (after network commissioning).

   .. note::
      Thingy:53 allows you to control RGB components of its single LED independently.
      This means that the listed color components can overlap, creating additional color effects.
      For example, **LED (LD1)** can indicate that the device is connected to a Zigbee network in blue, but if the device goes into the Identify mode at the same time, the flashing to indicate this will be purple, not red.

Power switch (SW1):
   Used for switching Thingy:53 on and off.

Button (SW3):
   Depending on how long the button is pressed:

   * If pressed for less than five seconds, it starts or cancels the Identify mode.
   * If pressed for five seconds, it initiates the factory reset of the device.
     The length of the button press can be edited using the :kconfig:option:`CONFIG_FACTORY_RESET_PRESS_TIME_SECONDS` Kconfig option from :ref:`lib_zigbee_application_utilities`.
     Releasing the button within this time does not trigger the factory reset procedure.

   Additionally, pressing the button shortly or releasing it before five seconds has elapsed indicates user input, which allows a Sleepy End Device to start the network rejoin procedure if it was previously stopped (for example, after timeout).

USB port:
   Used for getting logs from the device.
   It is enabled only for the ``debug`` build type of the application.
   See the :ref:`zigbee_weather_station_app_select_build_type` section to learn how to select the debug configuration.

Configuration
*************

|config|

.. _zigbee_weather_station_setup:

Setup
=====

Use one of the following options to set up the Zigbee network coordinator:

* For :ref:`zigbee_weather_station_app_testing` purposes, program the :ref:`Zigbee shell <zigbee_shell_sample>` sample as the network coordinator to be able to use the interactive shell for readings and bindings.
  Read the sample documentation for more information.
* For generic usage of the application, you can use the standard :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>`.

.. _zigbee_weather_station_app_configuration_readout:

Configuration options
=====================

The following application-specific Kconfig option values can be set to custom values:

.. _CONFIG_FIRST_WEATHER_CHECK_DELAY_SECONDS:

CONFIG_FIRST_WEATHER_CHECK_DELAY_SECONDS - Delay after application initialization
   This configuration option defines the delay from the moment of the application initialization to the first sensor data readout.
   The value is provided in seconds, with the default value set to ``5``.

.. _CONFIG_WEATHER_CHECK_PERIOD_SECONDS:

CONFIG_WEATHER_CHECK_PERIOD_SECONDS - How often sensor data is read
   This configuration option defines the period of cyclic sensor readouts after the first readout.
   The value is provided in seconds, with the default value set to ``60``.

.. _zigbee_weather_station_app_build_types:

Zigbee weather station build types
==================================

The Zigbee weather station application does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types, and they are located in the :file:`configuration/thingy53_nrf5340_cpuapp` directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by the Zigbee weather station application, depending on the building method.
This application supports the following build types:

* ``debug`` - Debug version of the application.
  Use this version to enable additional features for verifying the application behavior, such as logs.
* ``release`` - Release version of the application.
  Use this version to enable only the necessary application functionalities to optimize its performance.

.. note::
   :ref:`zigbee_weather_station_app_select_build_type` is optional.
   The ``debug`` build type is used by default if no build type is explicitly selected.

Logging in the ``debug`` build type
===================================

In the ``debug`` build type, the application also uses serial console over USB for logging.
Besides initialization logs, the following sets of measurement-related data are logged after a measurements update:

* Values measured by sensor.
  For example:

  .. code-block:: console

     [00:20:06.458,770] <inf> app: Sensor     T: 26.760000 [*C] P: 96.723000 [kPa] H: 23.178000 [%]

* Values set as corresponding ZCL attributes (transformed according to specification).
  For example:

  .. code-block:: console

     [00:20:06.458,801] <inf> app: Attributes T:      2676      P:       967       H:      2317

Building and running
********************

.. |application path| replace:: :file:`applications/zigbee_weather_station`

.. include:: /includes/application_build_and_run.txt

.. _zigbee_weather_station_app_select_build_type:

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`zigbee_weather_station_app_build_types`, depending on your building method.

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

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``thingy53_nrf5340_cpuapp`` by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b thingy53_nrf5340_cpuapp -d build_thingy53_nrf5340_cpuapp -- -DCONF_FILE=prj_release.conf

The ``build_thingy53_nrf5340_cpuapp`` parameter specifies the output directory for the build files.

.. note::
   If the selected board does not support the selected build type, the build is interrupted.
   For example, if the ``shell`` build type is not supported by the selected board, the following notification appears:

   .. code-block:: console

      File not found: ./ncs/nrf/applications/zigbee_weather_station/configuration/thingy53_nrf5340_cpuapp/prj_shell.conf

.. _zigbee_weather_station_app_testing:

Testing
=======

.. Note::
   * Part of the testing procedure assumes you are using the ``debug`` :ref:`build type <zigbee_weather_station_app_build_types>`, as it provides more feedback with **LED (LD1)** and logs through the USB console.

     These steps related to the ``debug`` build type mention the ``debug`` build type and are not required if you are using the ``release`` build type.
   * The provided measurement data values depend on the Thingy:53 device's surrounding conditions.
   * If you want to capture packets with Wireshark, install `nRF Sniffer for 802.15.4`_ and `configure Wireshark for use with Zigbee <Configuring Wireshark for Zigbee_>`_.

     Wireshark response frame examples are provided below for reference.

After programming the application to your device, complete the following steps to test it:

1. ``debug`` build type: |connect_generic|
   The connection is needed for gathering logs from the Zigbee weather station application.
#. Turn on the :ref:`Zigbee shell <zigbee_shell_sample>` sample programmed as the network coordinator to one of the compatible development kits.
   See the :ref:`zigbee_shell_sample_testing` section of the sample to learn how to set it up as a coordinator.
   Once turned on, the network coordinator forms the network and opens it for other devices to join for 180 seconds.
#. Turn on the Thingy:53 device.
   The application tries to join the Zigbee network automatically.
#. |connect_terminal_generic|
   When the connection is established, initialization logs start to appear in the console.
   They can look like the following:

   .. code-block:: console

      [00:00:02.027,435] <inf> app: Starting...
      [00:00:02.027,526] <inf> app: Initializing sensor...
      [00:00:02.027,526] <inf> app: Initializing sensor...OK
      [00:00:02.027,862] <inf> app: Starting...OK

   Around 5 seconds after the initialization, periodic sensor readouts and attribute setting logs start to appear in the console.
   They will look similar to the following:

   .. code-block:: console

      [00:00:07.013,793] <inf> app: Sensor     T: 23.470000 [*C] P: 98.347000 [kPa] H: 33.344000 [%]
      [00:00:07.013,824] <inf> app: Attributes T:      2347      P:       983       H:      3334
      [00:01:07.002,777] <inf> app: Sensor     T: 23.520000 [*C] P: 98.344000 [kPa] H: 33.485000 [%]
      [00:01:07.002,807] <inf> app: Attributes T:      2352      P:       983       H:      3348
      [00:02:06.966,003] <inf> app: Sensor     T: 24.360000 [*C] P: 98.344000 [kPa] H: 32.410000 [%]
      [00:02:06.966,033] <inf> app: Attributes T:      2436      P:       983       H:      3241

   .. note::
      These logs appear regardless of whether the device has joined the network.

#. When the device joins the network, **LED (LD1)** lights up with a constant blue color.

   Additionally, with the ``debug`` build type and console used for logging, output similar to the following appears:

   .. code-block:: console

      [00:00:02.029,174] <inf> zigbee_app_utils: Zigbee stack initialized
      [00:00:02.035,003] <inf> zigbee_app_utils: Device started for the first time
      [00:00:02.035,003] <inf> zigbee_app_utils: Start network steering
      [00:00:02.035,064] <inf> zigbee_app_utils: Started network rejoin procedure.
      [00:00:05.637,512] <inf> zigbee_app_utils: Joined network successfully (Extended PAN ID: f4ce3655fc08bf6b, PAN ID: 0xbf9e)

   .. note::
      As timestamps in this snippet indicate, these logs are likely to appear before the periodic sensor or attribute logs.

You can now use the development kit with the :ref:`Zigbee shell <zigbee_shell_sample>` sample (the network coordinator) to read the weather attributes and create bindings for periodic notifications.

Reading the weather attributes
------------------------------

After the device has connected to the Zigbee network, you can read the weather attributes from it.
Complete the following steps using the development kit programmed with the :ref:`Zigbee shell <zigbee_shell_sample>` sample (the network coordinator):

1. Run the following command to read the temperature attribute:

   .. code-block:: console

      uart:~$ zcl attr read 0x1ca9 42 0x0402 0x0104 0x0000

   In this command:

   * ``0x1ca9`` is the short network address of Thingy:53.
   * ``42`` is the endpoint number for aggregating the clusters.
   * ``0x0402`` is the temperature cluster ID.
   * ``0x0104`` is the Zigbee Home Automation profile ID.
   * ``0x0000`` is the short network address of the coordinator device.

   The output will be similar to the following one:

   .. code-block:: console

      ID: 0 Type: 29 Value: 2394

   Here, the value ``2394`` means 23.94 degrees Celsius.

   In Wireshark, the corresponding response frame for this output looks like as follows:

   .. code-block::

      ZigBee HA	2298	4756.825525	0x1ca9	0x0000	0x1ca9	0x0000	f4:ce:36:23:d7:4e:77:1d	f4:ce:36:55:fc:08:bf:6b		0xbf9e	ZCL: Read Attributes Response, Seq: 0

      Frame 2298: 68 bytes on wire (544 bits), 68 bytes captured (544 bits)
      IEEE 802.15.4 Data, Dst: 0x0000, Src: 0x1ca9
      ZigBee Network Layer Data, Dst: 0x0000, Src: 0x1ca9
      ZigBee Application Support Layer Data, Dst Endpt: 64, Src Endpt: 42
      ZigBee Cluster Library Frame, Command: Read Attributes Response, Seq: 0
         Frame Control Field: Profile-wide (0x18)
         Sequence Number: 0
         Command: Read Attributes Response (0x01)
         Status Record
            Attribute: Measured Value (0x0000)
            Status: Success (0x00)
            Data Type: 16-Bit Signed Integer (0x29)
            Measured Value: 23.94 [°C]

#. Run the following command to read the pressure attribute:

   .. code-block:: console

      uart:~$ zcl attr read 0x1ca9 42 0x0403 0x0104 0x0000

   Compared with the previous command, ``0x0403`` in this command represents for the pressure cluster ID.
   The output will be similar to the following one:

   .. code-block:: console

      ID: 0 Type: 29 Value: 984

   Here, the value ``984`` means 98.4 kPa.
   In Wireshark, the corresponding response frame for this output looks like as follows:

   .. code-block::

      ZigBee HA	2608	5396.494835	0x1ca9	0x0000	0x1ca9	0x0000	f4:ce:36:23:d7:4e:77:1d	f4:ce:36:55:fc:08:bf:6b		0xbf9e	ZCL: Read Attributes Response, Seq: 1

      Frame 2608: 68 bytes on wire (544 bits), 68 bytes captured (544 bits)
      IEEE 802.15.4 Data, Dst: 0x0000, Src: 0x1ca9
      ZigBee Network Layer Data, Dst: 0x0000, Src: 0x1ca9
      ZigBee Application Support Layer Data, Dst Endpt: 64, Src Endpt: 42
      ZigBee Cluster Library Frame, Command: Read Attributes Response, Seq: 1
         Frame Control Field: Profile-wide (0x18)
         Sequence Number: 1
         Command: Read Attributes Response (0x01)
         Status Record
            Attribute: Measured Value (0x0000)
            Status: Success (0x00)
            Data Type: 16-Bit Signed Integer (0x29)
            Measured Value: 98.4 [kPa]

#. Run the following command to read the relative humidity attribute:

   .. code-block:: console

      uart:~$ zcl attr read 0x1ca9 42 0x0405 0x0104 0x0000

   Compared with the previous command, ``0x0405`` in this command stands for the relative humidity cluster ID.
   The output will be similar to the following one:

   .. code-block:: console

      ID: 0 Type: 21 Value: 3293

   Here, the value ``3293`` means a humidity reading of 32.93%.
   In Wireshark, the corresponding response frame for this output looks like as follows:

   .. code-block::

      ZigBee HA	3014	6241.372769	0x1ca9	0x0000	0x1ca9	0x0000	f4:ce:36:23:d7:4e:77:1d	f4:ce:36:55:fc:08:bf:6b		0xbf9e	ZCL: Read Attributes Response, Seq: 2

      Frame 3014: 68 bytes on wire (544 bits), 68 bytes captured (544 bits)
      IEEE 802.15.4 Data, Dst: 0x0000, Src: 0x1ca9
      ZigBee Network Layer Data, Dst: 0x0000, Src: 0x1ca9
      ZigBee Application Support Layer Data, Dst Endpt: 64, Src Endpt: 42
      ZigBee Cluster Library Frame, Command: Read Attributes Response, Seq: 2
         Frame Control Field: Profile-wide (0x18)
         Sequence Number: 2
         Command: Read Attributes Response (0x01)
         Status Record
            Attribute: Measured Value (0x0000)
            Status: Success (0x00)
            Data Type: 16-Bit Unsigned Integer (0x21)
            Measured Value: 32.93 [%]

Creating bindings for periodic notifications
--------------------------------------------

After the device has connected to the Zigbee network, you can create bindings for periodic notifications.
Complete the following steps using the development kit programmed with the :ref:`Zigbee shell <zigbee_shell_sample>` sample (the network coordinator):

1. Run the following commands to create the bindings for each weather attribute:

   * Temperature cluster binding:

     .. code-block:: console

        uart:~$ zdo bind on f4ce3623d74e771d 42 f4ce3655fc08bf6b 64 0x0402 0x1ca9

     In this command:

     * ``f4ce3623d74e771d`` is the long network address of Thingy:53.
     * ``42`` is the endpoint number for aggregating the clusters.
     * ``f4ce3655fc08bf6b`` is the long network address of the network coordinator device.
     * ``64`` is the endpoint number of the network coordinator device.
     * ``0x0402`` is the temperature cluster ID.
     * ``0x1ca9``  is the short network address of Thingy:53.

   * Pressure cluster binding:

     .. code-block:: console

        uart:~$ zdo bind on f4ce3623d74e771d 42 f4ce3655fc08bf6b 64 0x0403 0x1ca9

     Compared with the previous command, ``0x0403`` in this command stands for the pressure cluster ID.
   * Relative humidity cluster binding:

     .. code-block:: console

        uart:~$ zdo bind on f4ce3623d74e771d 42 f4ce3655fc08bf6b 64 0x0405 0x1ca9

     Compared with the previous command, ``0x0405`` in this command stands for the relative humidity cluster ID.

#. Observe periodic reports in Wireshark for the respective clusters:

   * Temperature cluster periodic report will look like as follows:

      .. code-block::

         ZigBee HA	3882	7705.599377	0x1ca9	0x0000	0x1ca9	0x0000	f4:ce:36:23:d7:4e:77:1d	f4:ce:36:55:fc:08:bf:6b		0xbf9e	ZCL: Report Attributes, Seq: 128

         Frame 3882: 67 bytes on wire (536 bits), 67 bytes captured (536 bits)
         IEEE 802.15.4 Data, Dst: 0x0000, Src: 0x1ca9
         ZigBee Network Layer Data, Dst: 0x0000, Src: 0x1ca9
         ZigBee Application Support Layer Data, Dst Endpt: 64, Src Endpt: 42
         ZigBee Cluster Library Frame, Command: Report Attributes, Seq: 128
            Frame Control Field: Profile-wide (0x08)
            Sequence Number: 128
            Command: Report Attributes (0x0a)
            Attribute Field
               Attribute: Measured Value (0x0000)
               Data Type: 16-Bit Signed Integer (0x29)
               Measured Value: 24.26 [°C]

   * Pressure cluster periodic report will look like as follows:

      .. code-block::

         ZigBee HA	3888	7705.727682	0x1ca9	0x0000	0x1ca9	0x0000	f4:ce:36:23:d7:4e:77:1d	f4:ce:36:55:fc:08:bf:6b		0xbf9e	ZCL: Report Attributes, Seq: 129

         Frame 3888: 67 bytes on wire (536 bits), 67 bytes captured (536 bits)
         IEEE 802.15.4 Data, Dst: 0x0000, Src: 0x1ca9
         ZigBee Network Layer Data, Dst: 0x0000, Src: 0x1ca9
         ZigBee Application Support Layer Data, Dst Endpt: 64, Src Endpt: 42
         ZigBee Cluster Library Frame, Command: Report Attributes, Seq: 129
            Frame Control Field: Profile-wide (0x08)
            Sequence Number: 129
            Command: Report Attributes (0x0a)
            Attribute Field
               Attribute: Measured Value (0x0000)
               Data Type: 16-Bit Signed Integer (0x29)
               Measured Value: 98.4 [kPa]

   * Relative humidity cluster periodic report will look like as follows:

      .. code-block::

         ZigBee HA	3894	7705.856763	0x1ca9	0x0000	0x1ca9	0x0000	f4:ce:36:23:d7:4e:77:1d	f4:ce:36:55:fc:08:bf:6b		0xbf9e	ZCL: Report Attributes, Seq: 130

         Frame 3894: 67 bytes on wire (536 bits), 67 bytes captured (536 bits)
         IEEE 802.15.4 Data, Dst: 0x0000, Src: 0x1ca9
         ZigBee Network Layer Data, Dst: 0x0000, Src: 0x1ca9
         ZigBee Application Support Layer Data, Dst Endpt: 64, Src Endpt: 42
         ZigBee Cluster Library Frame, Command: Report Attributes, Seq: 130
            Frame Control Field: Profile-wide (0x08)
            Sequence Number: 130
            Command: Report Attributes (0x0a)
            Attribute Field
               Attribute: Measured Value (0x0000)
               Data Type: 16-Bit Unsigned Integer (0x21)
               Measured Value: 32.99 [%]

.. WIP: PLACEHOLDER

   .. _zigbee_weather_station_app_dfu:

   Updating the device firmware
   ============================


      To update the device firmware, complete the following steps;

Dependencies
************

This application uses the following |NCS| libraries:

* :ref:`lib_zigbee_application_utilities`
* :ref:`lib_zigbee_error_handler`
* :ref:`lib_ram_pwrdn`
* Zigbee subsystems:

  * :file:`zb_nrf_platform.h`

* :ref:`dk_buttons_and_leds_readme`

This application uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss` |zboss_version| (`API documentation`_)
* :file:`addons/zcl/zb_zcl_temp_measurement_addons.h`

In addition, it uses the following Zephyr libraries:

* :file:`include/device.h`
* :file:`include/drivers/sensor.h`
* :file:`include/drivers/uart.h`
* :file:`include/usb/usb_device.h`
* :file:`include/zephyr.h`
* :ref:`zephyr:logging_api`
