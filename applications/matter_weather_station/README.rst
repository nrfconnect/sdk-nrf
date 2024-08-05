.. _matter_weather_station_app:

Thingy:53: Matter weather station
#################################

.. contents::
   :local:
   :depth: 2

This Matter weather station application demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a weather station device using the Nordic Thingy:53.
Such a device lets you remotely gather different kinds of data using the device sensors, such as temperature, air pressure, and relative humidity.
The device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread or 802.11ax (Wi-Fi 6) network.
You can use this application as a reference for creating your own application.

.. note::
   The `Matter weather station application from the v2.1.1`_ |NCS| release participated in Matter Specification Validation Event (SVE) and successfully passed all required test cases to be considered as a device compliant with Matter 1.0.
   You can use the |NCS| v2.1.1 release to see the application configuration and the files that were originally used in Matter 1.0 certification.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

To commission the weather station device and control it remotely through a Thread or Wi-Fi network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
This requires additional hardware depending on your setup.
The recommended way of getting measurement values is using the mobile Matter controller application that comes with a graphical interface, performs measurements automatically and visualizes the data.

To program a Thingy:53 device where the preprogrammed MCUboot bootloader has been erased, you need the external J-Link programmer.
If you have an nRF5340 DK that has an onboard J-Link programmer, you can also use it for this purpose.

If the Thingy:53 is programmed with Thingy:53-compatible sample or application, you can also update the firmware using MCUboot's serial recovery or DFU over Bluetooth® Low Energy (LE).

.. note::
   If you build Matter Weather Station firmware with factory data support it will not be compatible with other Thingy:53 samples and applications.
   Then, the only way you can program the new firmware image is by flashing the board with J-Link programmer.

See :ref:`thingy53_app_guide` for details.

.. note::
    |matter_gn_required_note|

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``thingy53/nrf5340/cpuapp``.
* Matter over Wi-Fi is supported for ``thingy53/nrf5340/cpuapp`` with the ``nrf7002`` expansion board attached, for the :ref:`release configuration <matter_weather_station_custom_configs>` only.
  See `Building for the nRF7002 Wi-Fi expansion board`_ for more information.

Overview
********

The application uses a single button for controlling the device state.
The weather station device is periodically performing temperature, air pressure, and relative humidity measurements.
The measurement results are stored in the device memory and can be read using the Matter controller.
The controller communicates with the weather station device over the Matter protocol using Zigbee Cluster Library (ZCL).
The library describes data measurements within the proper clusters that correspond to the measurement type.

The application supports over-the-air (OTA) device firmware upgrade (DFU) using one of the two following protocols:

* Matter OTA update protocol that uses the Matter operational network for querying and downloading a new firmware image.
* Simple Management Protocol (SMP) over Bluetooth LE.
  In this case, the DFU can be done either using a smartphone application or a PC command line tool.
  Note that this protocol is not part of the Matter specification.

In both cases, MCUboot secure bootloader is used to apply the new firmware image.
For information about how to upgrade the device firmware using a PC or a mobile, see the :ref:`matter_weather_station_app_dfu` section.

.. _matter_weather_station_network_mode:

Remote testing in a network
===========================

By default, the Matter accessory device has no IPv6 network configured.
You must pair it with the Matter controller over Bluetooth LE to get the configuration from the controller to use the device within a Thread or Wi-Fi network.

The Bluetooth LE advertising starts automatically upon the device startup, but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, you can re-enable it manually using **Button (SW3)**.

Additionally, the controller must get the `Onboarding information`_ from the Matter accessory device and provision the device into the network.
For details, see the `Testing`_ section.

User interface
**************

LED (LD1):
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short flash on (green color, 50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is not advertising over Bluetooth LE.
    * Short flash on (blue color, 50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state, but is advertising over Bluetooth LE.
    * Rapid even flashing (blue color, 100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected through Bluetooth LE.
    * Short flash on (purple color, 50 ms on/950 ms off) - The device is fully provisioned and has Thread enabled or has Wi-Fi connection established.
    * Rapid even flashing after commissioning (blue color, 100 ms on/100 ms off) - The device lost connection to Wi-Fi network (only in Wi-Fi mode).

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

NFC port with antenna attached:
    Used for obtaining the `Onboarding information`_ from the Matter accessory device to start the commissioning procedure.

Configuration
*************

|config|

.. _matter_weather_station_custom_configs:

Matter weather station custom configurations
============================================

The Matter weather station application uses a :file:`prj.conf` configuration file located in the root directory for the default configuration.
It also provides additional files for different custom configurations.
When you build the application, you can select one of these configurations using the :makevar:`FILE_SUFFIX` variable.

See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information.

The application supports the following configurations:

.. list-table:: Matter weather station configurations
   :widths: auto
   :header-rows: 1

   * - Configuration
     - File name
     - :makevar:`FILE_SUFFIX`
     - Supported board
     - Description
   * - Debug (default)
     - :file:`prj.conf`
     - No suffix
     - All from `Requirements`_
     - Debug version of the application.

       Enables additional features for verifying the application behavior, such as logs.
   * - Release
     - :file:`prj_release.conf`
     - ``release``
     - All from `Requirements`_
     - Release version of the application.

       Enables only the necessary application functionalities to optimize its performance.

       Currently, only the release configuration is supported when `Building for the nRF7002 Wi-Fi expansion board`_.

.. _matter_weather_station_app_build_configuration_overlays:

Matter weather station build configuration overlays
===================================================

The application comes with the following overlays:

* ``overlay-factory_data`` - factory data storage support enabled.
  You can use this optional configuration overlay to enable reading necessary factory data from a separate partition in the device non-volatile memory.
  This way, you can read information such as product information, keys, and certificates, useful for example for Matter certification.
  See `Generating factory data`_ to learn how to put factory data into device's storage.
  To learn more about factory data, read the :doc:`matter:nrfconnect_factory_data_configuration` page in the Matter documentation.

  This overlay requires providing of the :file:`pm_static_factory_data.yml` Partition Manager static configuration file.

  To build the example with the factory data support, run the following command:

  .. code-block:: console

      west build -b thingy53/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=overlay-factory_data.conf -DPM_STATIC_YML_FILE=pm_static_factory_data.yml

.. note::
   Matter factory data support requires the dedicated partition layout.
   This means that if you build the application using the ``overlay-factory_data`` configuration overlay, it will not be compatible with other :ref:`Thingy:53 applications and samples <thingy53_compatible_applications>`.

Building and running
********************

.. |sample path| replace:: :file:`applications/matter_weather_station`

.. include:: /includes/build_and_run.txt

Selecting a configuration
=========================

Before you start testing the application, you can select one of the :ref:`matter_weather_station_custom_configs`.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information how to select a configuration.

Building for the nRF7002 Wi-Fi expansion board
==============================================

To build this application to work with the nRF7002 Wi-Fi expansion board:

1. Connect the nRF7002 EB to the **P9** connector on Thingy:53.
#. Build the application:

   .. tabs::

      .. group-tab:: nRF Connect for VS Code

         To build the application in the nRF Connect for VS Code IDE for Thingy:53 with the nRF7002 EB attached, add the expansion board and the file suffix variables in the build configuration's :guilabel:`Extra CMake arguments` and rebuild the build configuration.
         For example: ``-- -Dmatter_weather_station_SHIELD=nrf7002eb -DFILE_SUFFIX=release -DSB_CONFIG_WIFI_NRF70=y``.

      .. group-tab:: Command line

         To build the sample from the command line for Thingy:53 with the nRF7002 EB attached, use the following command within the sample directory:

         .. code-block:: console

            west build -b thingy53/nrf5340/cpuapp -- -Dmatter_weather_station_SHIELD=nrf7002eb -DFILE_SUFFIX=release -DSB_CONFIG_WIFI_NRF70=y

Generating factory data
=======================

To enable factory data support, you need to select the ``overlay-factory_data`` configuration overlay from the available application :ref:`build configuration overlays <matter_weather_station_app_build_configuration_overlays>`, set the ``SB_CONFIG_MATTER_FACTORY_DATA_GENERATE`` kconfig option to ``y``, and use the ``factory_data`` file suffix.
You can generate new factory data set when building for the given board target by invoking the following command:

.. code-block:: console

   west build -b thingy53/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=overlay-factory_data.conf -DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y -DFILE_SUFFIX=factory_data

This command builds the application with default certificates.
After building for the board target, the generated :file:`factory_data.hex` file will be merged with the application target HEX file, so you can use the :ref:`regular command to flash it to the device <programming>`.

If you want to use Vendor ID, Product ID or other data that is not reserved for tests, you need custom test certificates.
To build with custom certificates, you need to :ref:`install CHIP Certificate Tool <ug_matter_gs_tools_cert_installation>`.

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
#. Commission the device to the network of your choice:

   * For Thread network, follow the steps in :ref:`ug_matter_gs_testing_thread_separate_otbr_linux_macos`.

   * For Wi-Fi network, follow the steps in :ref:`ug_matter_gs_testing_wifi_pc`.

   During the commissioning procedure, **LED (LD1)** of the Matter device starts blinking blue (rapid even flashing).
   This indicates that the device is connected over Bluetooth LE, but does not yet have full Thread network connectivity.

   .. note::
        To start commissioning, the controller must get the `Onboarding information`_ from the Matter accessory device.

   Once the commissioning is complete and the device has full Thread or Wi-Fi connectivity, **LED (LD1)** starts blinking purple (short flash on).
#. Request to read sensor measurements in CHIP Tool for Linux or macOS:

   a. Choose one of the following measurement type and invoke the command using CHIP Tool for Linux or macOS (Fill the **Device ID** argument with the same as was used to commissioning):

      * To read temperature:

         .. code-block:: console

            chip-tool temperaturemeasurement read measured-value <Device ID> 1

      * To read relative humidity:

         .. code-block:: console

            chip-tool relativehumiditymeasurement read measured-value <Device ID> 2

      * To read air pressure:

         .. code-block:: console

            chip-tool pressuremeasurement read measured-value <Device ID> 3

   #. After invoking the chosen command, search the CHIP Tool for Linux or macOS console logs and look for the measurement value:

      * Example of the temperature measurement value log:

         .. code-block:: console

            [1675846190.922905][72877:72879] CHIP:TOO: Endpoint: 1 Cluster: 0x0000_0402 Attribute 0x0000_0000 DataVersion: 1236968801
            [1675846190.922946][72877:72879] CHIP:TOO:   MeasuredValue: 2348

         This means that the current temperature value is equal to 23.48°C.

      * Example of the relative humidity measurement value log:

         .. code-block:: console

            [1675849697.750923][164859:164861] CHIP:TOO: Endpoint: 2 Cluster: 0x0000_0405 Attribute 0x0000_0000 DataVersion: 385127250
            [1675849697.750953][164859:164861] CHIP:TOO:   measured value: 2526

         This means that the current relative humidity value is equal to 25.26%.

      * Example of the air pressure measurement value log:

         .. code-block:: console

            [1675849714.536985][164896:164898] CHIP:TOO: Endpoint: 3 Cluster: 0x0000_0403 Attribute 0x0000_0000 DataVersion: 3096547
            [1675849714.537008][164896:164898] CHIP:TOO:   MeasuredValue: 1015

         This means that the current air pressure value is equal to 1015 hPa.

Onboarding information
----------------------

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this application, the data payload, which includes the device discriminator and setup PIN code, is encoded and shared using an NFC tag.
When using the debug configuration, you can also get this type of information from the USB interface logs.

Alternatively, depending on your configuration and selected overlay, you can also use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data required:

* For the debug and release configurations:

  .. list-table:: Weather station application onboarding information for the debug or release configurations
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: /images/matter_qr_code_weather_station_default.png
            :width: 200px
            :alt: QR code for commissioning the weather station device (debug or release configuration)

       - MT:M1TJ342C00KA0648G00
       - 34970112332

* Additionally, if the factory data build configuration overlay is selected:

  .. list-table:: Weather station application onboarding information for the factory data build configuration overlay
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: /images/matter_qr_code_weather_station_factory_data.png
            :width: 200px
            :alt: QR code for commissioning the weather station device (factory data build configuration overlay)

       - MT:KAYA36PF1509673GE10
       - 14575339844

|matter_cd_info_note_for_samples|

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
