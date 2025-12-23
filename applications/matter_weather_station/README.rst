.. |matter_name| replace:: weather station
.. |matter_type| replace:: application
.. |matter_dks_thread| replace:: ``thingy53/nrf5340/cpuapp`` board target
.. |matter_dks_wifi| replace:: ``thingy53/nrf5340/cpuapp`` board target with the ``nrf7002eb`` expansion board attached
.. |sample path| replace:: :file:`applications/matter_weather_station`

.. include:: /includes/matter/shortcuts.txt

.. _matter_weather_station_app:

Thingy:53: Matter weather station
#################################

.. contents::
   :local:
   :depth: 2

This Matter weather station application demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a weather station device using the Nordic Thingy:53.
Such a device lets you remotely gather different kinds of data using the device sensors, such as temperature, air pressure, and relative humidity.

.. include:: /includes/matter/introduction/sleep_thread_wifi.txt

.. note::
   The `Matter weather station application from the v2.1.1`_ |NCS| release participated in Matter Specification Validation Event (SVE) and successfully passed all required test cases to be considered as a device compliant with Matter 1.0.
   You can use the |NCS| v2.1.1 release to see the application configuration and the files that were originally used in Matter 1.0 certification.

Application overview
********************

The application uses a single button for controlling the device state.
The weather station device is periodically performing temperature, air pressure, and relative humidity measurements.
The measurement results are stored in the device memory and can be read using the Matter controller.
The controller communicates with the weather station device over the Matter protocol and exchanges data using the Matter Data Model.
The data model describes data measurements within the proper clusters that correspond to the measurement type.

.. include:: /includes/matter/overview/matter_quick_start.txt

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread_wifi.txt

Programming requirements
========================

To commission the weather station device and control it remotely through a Thread or Wi-Fi network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
This requires additional hardware depending on your setup.
The recommended way of getting measurement values is using the mobile Matter controller application that comes with a graphical interface, performs measurements automatically and visualizes the data.

To program a Thingy:53 device where the preprogrammed MCUboot bootloader has been erased, you need the external J-Link programmer.
If you have an nRF5340 DK that has an onboard J-Link programmer, you can also use it for this purpose.

If the Thingy:53 is programmed with Thingy:53-compatible sample or application, you can also update the firmware using MCUboot's serial recovery or DFU over Bluetooth Low Energy (LE).

.. note::
   If you build Matter Weather Station firmware with factory data support it will not be compatible with other Thingy:53 samples and applications.
   Then, the only way you can program the new firmware image is by flashing the board with J-Link programmer.

See :ref:`thingy53_app_guide` for details.

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

.. include:: /includes/matter/interface/nfc.txt

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic.txt

.. note::

   Currently, only the release configuration is supported when :ref:`Building for the nRF7002 Wi-Fi expansion board <matter_weather_station_app_building_nrf7002eb>`.

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt
.. include:: /includes/matter/building_and_running/select_configuration.txt
.. include:: /includes/matter/building_and_running/commissioning.txt

|matter_ble_advertising_auto|

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

Advanced building options
=========================

.. _matter_weather_station_app_build_configuration_overlays:

Building with factory data support
----------------------------------

.. toggle::

   To build the application with the factory data support, run the following command:

   .. code-block:: console

      west build -b thingy53/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=overlay-factory_data.conf -DPM_STATIC_YML_FILE=pm_static_factory_data.yml -DFILE_SUFFIX=factory_data

   .. note::
      Matter factory data support requires the dedicated partition layout.
      This means that if you build the application using the ``overlay-factory_data`` configuration overlay, it will not be compatible with other :ref:`Thingy:53 applications and samples <thingy53_compatible_applications>`.

   To generate a new factory data set when building for the given board target, invoke the following command:

   .. code-block:: console

      west build -b thingy53/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=overlay-factory_data.conf -DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y -DFILE_SUFFIX=factory_data

   This command builds the application with default certificates.
   After building for the board target, the generated :file:`factory_data.hex` file will be merged with the application target HEX file, so you can use the :ref:`regular command to flash it to the device <programming>`.

   If you want to use Vendor ID, Product ID or other data that is not reserved for tests, you need custom test certificates.
   To build with custom certificates, you need to :ref:`install CHIP Certificate Tool <ug_matter_gs_tools_cert_installation>`.

.. _matter_weather_station_app_building_nrf7002eb:

Building for the nRF7002 Wi-Fi expansion board
----------------------------------------------

.. toggle::

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

Testing
*******

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

Factory reset
=============

To restore the device settings and state to its factory set complete the following action:

* Press the **Button (SW3)** for six seconds to initiate the factory reset of the device.

Dependencies
************

.. include:: /includes/matter/dependencies.txt
