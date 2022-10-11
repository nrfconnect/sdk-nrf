.. _matter_weather_station_app:

Thingy:53: Matter weather station
#################################

.. contents::
   :local:
   :depth: 2

This Matter weather station application demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a weather station device using the Nordic Thingy:53.
Such a device lets you remotely gather different kinds of data using the device sensors, such as temperature, air pressure, and relative humidity.
The device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network.
You can use this application as a reference for creating your own application.

.. note::
   The Thingy:53 Matter weather station application featured in the |NCS| v2.1.1 release participated in Matter Specification Validation Event (SVE) and successfully passed all required test cases to be considered as a device compliant with Matter 1.0.
   For this reason, the |NCS| v2.1.1 release version of the application includes additional `Certification files for Matter SVE 1.0`_.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

To commission the weather station device and control it remotely through a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
This requires additional hardware depending on your setup.
The recommended way of getting measurement values is using the mobile Matter controller application that comes with a graphical interface, performs measurements automatically and visualizes the data.

To program a Thingy:53 device where the preprogrammed MCUboot bootloader has been erased, you need the external J-Link programmer.
If you have an nRF5340 DK that has an onboard J-Link programmer, you can also use it for this purpose.

If the Thingy:53 is programmed with Thingy:53-compatible sample or application, you can also update the firmware using MCUboot's serial recovery or DFU over Bluetooth LE.
See :ref:`thingy53_app_guide` for details.

.. note::
    |matter_gn_required_note|

Overview
********

The application uses a single button for controlling the device state.
The weather station device is periodically performing temperature, air pressure, and relative humidity measurements.
The measurement results are stored in the device memory and can be read using the Matter controller.
The controller communicates with the weather station device over the Matter protocol using Zigbee Cluster Library (ZCL).
The library describes data measurements within the proper clusters that correspond to the measurement type.

The application supports over-the-air (OTA) device firmware upgrade (DFU) using one of the two following protocols:

* Matter OTA update protocol that uses the Matter operational network for querying and downloading a new firmware image.
* Simple Management Protocol (SMP) over Bluetooth® LE.
  In this case, the DFU can be done either using a smartphone application or a PC command line tool.
  Note that this protocol is not part of the Matter specification.

In both cases, MCUboot secure bootloader is used to apply the new firmware image.
For information about how to upgrade the device firmware using a PC or a mobile, see the :ref:`matter_weather_station_app_dfu` section.

.. _matter_weather_station_network_mode:

Remote testing in a network
===========================

By default, the Matter accessory device has Thread disabled, and it must be paired with the Matter controller over Bluetooth LE to get configuration from it if you want to use the device within a Thread network.
To do this, the device must be made discoverable over Bluetooth LE.

The Bluetooth LE advertising starts automatically upon the device startup, but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, you can re-enable it manually using **Button (SW3)**.

Additionally, the controller must get the commissioning information from the Matter accessory device and provision the device into the network.
For details, see the `Testing`_ section.

.. _matter_weather_station_cert_files:

Certification files for Matter SVE 1.0
======================================

The |NCS| v2.1.1 version of the application contains a dedicated :file:`certification` directory with information useful for getting to know the Matter certification process:

* :file:`factory_data` directory - This directory contains a generated example of the factory data that was originally used for the weather station Matter certification.
* :file:`PICS` directory - This directory contains Protocol Implementation Conformance Statement (PICS) that was originally used for the weather station Matter certification.
  The PICS is a set of XML files that describe the Matter features supported by a specific device.
* :file:`overlay-factory_data_build.conf` - This overlay file contains examples of configuration options that can be used to generate a new set of Matter device factory data.

.. _matter_weather_station_app_build_types:

Matter weather station build types
==================================

The Matter weather station application does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types and they are located in the :file:`configuration/thingy53_nrf5340_cpuapp` directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /gs_modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by Matter weather station application, depending on the building method.
This application supports the following build types:

* ``debug`` - Debug version of the application. You can use this version to enable additional features for verifying the application behavior, such as logs or command-line shell.
* ``release`` - Release version of the application. You can use this version to enable only the necessary application functionalities to optimize its performance.
* ``factory_data`` - Release version of the application that has factory data storage enabled.
  You can use this version to enable reading factory data necessary for Matter certification from a separate partition in the device non-volatile memory.
  This way, you can read information such as product information, keys, and certificates.
  See `Using certification factory data`_ to learn how to put factory data into device's storage.
  To learn more about factory data read :doc:`matter:nrfconnect_factory_data_configuration` page in the Matter documentation.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

User interface
**************

LED (LD1):
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short flash on (green color, 50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is not advertising over Bluetooth LE.
    * Short flash on (blue color, 50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state, but is advertising over Bluetooth LE.
    * Rapid even flashing (blue color, 100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected through Bluetooth LE.
    * Short flash on (purple color, 50 ms on/950 ms off) - The device is fully provisioned and has Thread enabled.

    .. note::
       Thingy:53 allows to control RGB components of its single LED independently.
       This means that the listed color components can overlap, creating additional color effects.

Button (SW3):
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

Selecting a build type in |VSC|
-------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

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

Using certification factory data
================================

This application contains `Certification files for Matter SVE 1.0`_, including the factory data files.
You can program these files for example to compare your certification test data with the factory data that passed the certification event.

You can use either the pre-prepared factory data that were used for Matter SVE 1.0 or you can generate new factory data.

Programming pre-prepared factory data
-------------------------------------

Before programming pre-prepared factory data, you must build and program the application with the factory data build type selected (see `Matter weather station build types`_).
You can program the pre-prepared :file:`certification/factory_data/factory_data.hex` file with the following command:

.. parsed-literal::
   :class: highlight

   nrfjprog --coprocessor CP_APPLICATION --program factory_data.hex --reset

Generating new factory data
---------------------------

Instead of using the pre-prepared factory data set, you can also generate new one when building for the target board by invoking the following command:

.. parsed-literal::
   :class: highlight

   west build -b thingy53_nrf5340_cpuapp -- -DCONF_FILE=prj_factory_data.conf -DOVERLAY_CONFIG="../../certification/overlay-factory_data_build.conf"

After building the target, the generated :file:`factory_data.hex` file will be merged with the application target HEX file, so you can use the regular command to flash it to the device:

.. parsed-literal::
   :class: highlight

   west flash --erase

Testing
=======

.. note::
   The testing procedure assumes you are using the CHIP Tool for Android Matter controller application.
   You can also obtain the measurement values using the PC-based CHIP Tool for Linux and iOS Matter controller and invoking the read commands manually.
   Compared with the PC Matter controller, the mobile Matter controller only gives access to a subset of clusters supported by the Matter weather station application.
   If you want to access all the supported clusters, including Descriptor, Identify, and Power Source clusters, use the PC Matter controller.
   To see how to send commands from the PC Matter controller, read the :doc:`matter:chip_tool_guide` page in the Matter documentation.

After programming the application, perform the following steps to test the Matter weather station application on the Thingy:53 with the mobile Matter controller application:

1. Turn on the Thingy:53.
   The application starts in an unprovisioned state.
   The advertising over Bluetooth LE and DFU start automatically, and **LED (LD1)** starts blinking blue (short flash on).
#. Commission the device into a Thread network by following the steps in :ref:`ug_matter_configuring_mobile`.
   During the commissioning procedure, **LED (LD1)** of the Matter device starts blinking blue (rapid even flashing).
   This indicates that the device is connected over Bluetooth LE, but does not yet have full Thread network connectivity.

   .. note::
        To start commissioning, the controller must get the commissioning information from the Matter accessory device.
        The data payload, which includes the device discriminator and setup PIN code, is encoded and shared using an NFC tag.
        When using the debug configuration, you can also get this type of information from the USB interface logs.

   Once the commissioning is complete and the device has full Thread connectivity, **LED (LD1)** starts blinking purple (short flash on).
#. Read sensor measurements in CHIP Tool for Android:

   a. In the CHIP Tool for Android application main menu, tap the :guilabel:`SENSOR CLUSTERS` button to open the sensor measurements section.
      This section contains text boxes to enter **Device ID** and **Endpoint ID**, a drop-down menu with available measurements and two buttons, :guilabel:`READ` and :guilabel:`WATCH`.

      .. figure:: /images/chiptool_sensor_cluster.gif
         :alt: Sensor cluster section selection

         Sensor cluster section selection

      On this image, **Device ID** has the value ``5`` and **Endpoint ID** has the value ``1``.
   #. Select one of the available measurement types from the drop-down menu.
   #. Enter one of the following values for **Endpoint ID**, depending on the selected measurement type:

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
   #. To change the displayed measurement, select a different measurement type from the drop-down list and enter the corresponding **Endpoint ID** value.

      .. figure:: /images/chiptool_relative_humidity.gif
         :alt: Relative humidity measurement type selection

         Relative humidity measurement type selection

.. _matter_weather_station_app_dfu:

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
