.. _matter_weather_station_app:

Thingy:53: Matter weather station
#################################

.. contents::
   :local:
   :depth: 2

This Matter weather station application demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a weather station device.
Such a device lets you remotely gather different kinds of data using the device sensors, such as temperature, air pressure, and relative humidity.
The device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network.
You can use this application as a reference for creating your own application.

.. note::
    The Matter protocol is in an early development stage and must be treated as an experimental feature.

Overview
********

The application uses a single button for controlling the device state.
The weather station device is periodically performing temperature, air pressure, and relative humidity measurements.
The measurement results are stored in the device memory and can be read using the Matter controller.
The controller communicates with the weather station device over the Matter protocol using Zigbee Cluster Library (ZCL).
The library describes data measurements within the proper clusters that correspond to the measurement type.

The application uses MCUboot secure bootloader and SMP protocol for performing over-the-air Device Firmware Upgrade using Bluetooth® LE.
For information about how to upgrade the device firmware using a PC or a mobile, see the :ref:`matter_weather_station_app_dfu` section.

.. _matter_weather_station_network_mode:

Remote testing in a network
===========================

By default, the Matter accessory device has Thread disabled, and it must be paired with the Matter controller over Bluetooth LE to get configuration from it if you want to use the device within a Thread network.
To do this, the device must be made discoverable over Bluetooth LE.

The Bluetooth LE advertising starts automatically upon the device startup, but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, you can re-enable it manually using **Button 1**.

Additionally, the controller must get the commissioning information from the Matter accessory device and provision the device into the network.
For details, see the `Testing`_ section.

.. _matter_weather_station_app_build_types:

Matter weather station build types
==================================

The Matter weather station application does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types and they are located in the `configuration/thingy53_nrf5340_cpuapp` directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /gs_modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by Matter weather station application, depending on the building method.
This application supports the following build types:

* ``debug`` -- Debug version of the application - can be used to enable additional features for verifying the application behavior, such as logs or command-line shell.
* ``release`` -- Release version of the application - can be used to enable only the necessary application functionalities to optimize its performance.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

Requirements
************

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy53_nrf5340_cpuapp

To commission the weather station device and control it remotely through a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>` (which requires additional hardware depending on which setup you choose).
The recommended way of getting measurement values is using the mobile Matter controller application that comes with a neat graphical interface, performs measurements automatically and visualizes the data.

To program a Thingy:53 device where the preprogrammed MCUboot bootloader has been erased, you need the external J-Link programmer.
If you own an nRF5340 DK that has an onboard J-Link programmer, you can also use it for this purpose.

If the Thingy:53 is programmed with Thingy:53-compatible sample or application, then you can also update the firmware using MCUboot's serial recovery or DFU over Bluetooth LE.
See :ref:`thingy53_app_guide` for details.

.. note::
    |matter_gn_required_note|

User interface
**************

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short flash on (green color, 50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is not advertising over Bluetooth LE.
    * Short flash on (blue color, 50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state, but is advertising over Bluetooth LE.
    * Rapid even flashing (blue color, 100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected through Bluetooth LE.
    * Short flash on (purple color, 50 ms on/950 ms off) - The device is fully provisioned and has Thread enabled.

Button 1:
    Used during the commissioning procedure.
    Depending on how long you press the button:

    * If pressed for 6 seconds, it initiates the factory reset of the device.
      Releasing the button within the 6-second window cancels the factory reset procedure.
    * If pressed for less than 3 seconds, it starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.

USB port:
    Used for getting logs from the device or communicating with it through the command-line interface.
    It is enabled only for the debug configuration of an application.
    See the `Selecting a build type`_ section to learn how to select the debug configuration.

NFC port with antenna attached:
    Used for obtaining the commissioning information from the Matter accessory device to start the commissioning procedure.

Configuration
*************

|config|

Building and running
********************

.. |sample path| replace:: :file:`applications/matter_weather_station`

.. include:: /includes/build_and_run.txt

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`matter_weather_station_app_build_types`, depending on your building method.

Selecting a build type in SES
-----------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_ses_start
   :end-before: build_types_selection_ses_end

Selecting a build type from command line
----------------------------------------

.. include:: /gs_modifying.rst
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

      File not found: ./ncs/nrf/applications/matter_weather_station/configuration/thingy53_nrf5340_cpuapp/prj_shell.conf

Testing
=======

.. note::
   The testing procedure assumes you are using the mobile Matter controller application.
   You can also obtain the measurement values using the PC command-line-based Matter controller and invoking the read commands manually.
   Compared with the PC Matter controller, the mobile Matter controller only gives access to a subset of clusters supported by the Matter weather station application.
   If you want to access all the supported clusters, including Descriptor, Identify, and Power Source clusters, use the PC Matter controller.
   To see how to send commands from the PC Matter controller, read the :doc:`matter:python_chip_controller_building` guide in the Matter documentation.

After programming the application, perform the following steps to test the Matter weather station application on the Thingy:53 with the mobile Matter controller application:

1. Turn on the Thingy:53.
   The application starts in an unprovisioned state.
   The advertising over Bluetooth LE and DFU start automatically, and **LED 1** starts blinking blue (short flash on).
#. Commission the device into a Thread network by following the steps in :ref:`ug_matter_configuring_mobile`.
   During the commissioning procedure, **LED 1** of the Matter device starts blinking blue (rapid even flashing).
   This indicates that the device is connected over Bluetooth LE, but does not yet have full Thread network connectivity.

   .. note::
        To start commissioning, the controller must get the commissioning information from the Matter accessory device.
        The data payload, which includes the device discriminator and setup PIN code, is encoded and shared using an NFC tag.
        When using the debug configuration, you can also get this type of information from the USB interface logs.

   Once the commissioning is complete and the device has full Thread connectivity, **LED 1** starts blinking purple (short flash on).
#. Read sensor measurements in Android CHIPTool:

   a. In the Android CHIPTool application main menu, tap the :guilabel:`SENSOR CLUSTERS` button to open the sensor measurements section.
      This section contains text boxes to enter :guilabel:`Device ID` and :guilabel:`Endpoint ID`, a drop-down menu with available measurements and two buttons, :guilabel:`READ` and :guilabel:`WATCH`.

      .. figure:: /images/chiptool_sensor_cluster.gif
         :alt: Sensor cluster section selection

         Sensor cluster section selection

      On this image, :guilabel:`Device ID` has the value ``5`` and :guilabel:`Endpoint ID` has the value ``1``.
   #. Select one of the available measurement types from the drop-down menu.
   #. Enter one of the following values for :guilabel:`Endpoint ID`, depending on the selected measurement type:

      * 1 - Temperature measurement
      * 2 - Relative humidity measurement
      * 3 - Air pressure measurement

   #. Tap the :guilabel:`READ` button to read and display the single measurement value.

      .. figure:: /images/chiptool_temperature_read.gif
         :alt: Single temperature measurement read

         Single temperature measurement read

   #. Tap the :guilabel:`WATCH` button to start watching measurement changes in a continuous way and display values on a chart.

      .. figure:: /images/chiptool_temperature_watch.gif
         :alt: Continuous temperature measurement watch

         Continuous temperature measurement watch

      The vertical axis represents the measurement values and the horizontal axis represents the current time.
   #. Change the displayed measurement by selecting a different measurement type from the drop-down list and entering the corresponding :guilabel:`Endpoint ID` value.

      .. figure:: /images/chiptool_relative_humidity.gif
         :alt: Relative humidity measurement type selection

         Relative humidity measurement type selection

.. _matter_weather_station_app_dfu:

Updating the device firmware
============================

.. note::
    Device Firmware Upgrade feature is under development and currently it is possible to update only the application core image.
    Network core image update is not yet available.

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
