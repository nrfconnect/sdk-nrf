.. _nrf_peripheral_sensor_node_app:

nRF Peripheral Sensor node
##########################

.. contents::
   :local:
   :depth: 2

The nRF Peripheral Sensor node application is a demo of the application that periodically acquires the data from a Serial Peripheral Interface (SPI) and Two-wire Interface (TWI) devices, advertises, and enables the reading of sensor values from Bluetooth® service in a power-efficient way.
The application performs burst measurements periodically and in response to IMU interrupts.
Measured data is stored in the non-volatile memory (NVM) and can be retrieved through Bluetooth.
The application holds a number of measurements depending on the size of memory allocated for storage.
New data automatically replaces oldest records.

Application Overview
********************

To perform its tasks, the nRF Peripheral Sensor node application uses components available in Zephyr and the |NCS|, namely the :ref:`lib_caf` modules and :ref:`zephyr:sensor_api` for sampling sensors, :ref:`nsms_readme` for presenting measured data, and dedicated service for historical data retrieval over Bluetooth LE.
The application also stores measurements in a dedicated non-volatile memory partition.
You can download the stored data over Bluetooth.

Sampling sensors
================

The application handles the sensor sampling using the Zephyr's :ref:`zephyr:sensor_api`.
This approach allows to use any sensor available in Zephyr.

By default, you need to use the Peripheral Sensor node Shield for this application.

Sensors shield
--------------

The Peripheral Sensor node shield is an extension for the nRF54L15 PDK that contains the following sensors:

  * ``ADXL362`` Low power accelerometer
  * ``BMI270`` Accelerometer and gyroscope
  * ``BME688`` Environmental sensor

.. figure:: /images/peripheral_sensor_node_shield.png
   :width: 350px
   :align: center
   :alt: Peripheral Sensor node shield

   Peripheral Sensor node shield

This enables you to test the Peripheral Sensor node application on the nRF54L15 PDK.

Pin assignment
^^^^^^^^^^^^^^

For the exact pin assignment, refer to the following table:

+-------------------------+------------+-----------------+
| nRF54L15 connector pin  | SIGNAL     | Shield function |
+=========================+============+=================+
| P1.13                   | BMI270 CS  | Chip Select     |
+-------------------------+------------+-----------------+
| P2.10                   | ADXL362 CS | Chip Select     |
+-------------------------+------------+-----------------+
| P2.08                   | SPI MOSI   | Serial Data In  |
+-------------------------+------------+-----------------+
| P2.09                   | SPI MISO   | Serial Data Out |
+-------------------------+------------+-----------------+
| P2.06                   | SPI SCK    | Serial Clock    |
+-------------------------+------------+-----------------+
| P1.14                   | GPIO       | SPI Interrupt   |
+-------------------------+------------+-----------------+
| P1.11                   | TWI SCL    | TWI Clock       |
+-------------------------+------------+-----------------+
| P1.12                   | TWI SDA    | TWI Data        |
+-------------------------+------------+-----------------+

Shield is an add-on that you can attach to the nRF54L15 PDK to extend its features and functionalities.

If you want to run the application on the nRF54L15 PDK, you need to attach the sensor shield to it.

.. figure:: /images/peripheral_sensor_node_assy.png
   :width: 350px
   :align: center
   :alt: Peripheral Sensor node assembly

   Peripheral Sensor node assembly

The shield must be oriented in a way that allows its silkscreen to read the same way as PDK.
The shield must not be rotated.

Power management
================

To reduce power consumption, the Peripheral Sensor node application mostly remains in the power-down state.
The application performs burst measurement of the environmental and IMU sensors periodically with a given interval and does additional burst measurement when triggered by the interrupt from the ``ADXL362`` sensor.
You can configure the periodic burst measurement intervals through the :ref:`CONFIG_APP_CONTROL_BURST_INTERVAL_S <CONFIG_APP_CONTROL_BURST_INTERVAL_S>` Kconfig option.
By default, the burst interval is set to 120 seconds.
You can configure the number of measurements for a single burst using the :ref:`CONFIG_APP_CONTROL_BURST_MEAS_CNT <CONFIG_APP_CONTROL_BURST_MEAS_CNT>` Kconfig option, and the time interval between measurements is specified in the :ref:`CONFIG_APP_CONTROL_BURST_MEAS_INTERVAL_MS <CONFIG_APP_CONTROL_BURST_MEAS_INTERVAL_MS>` Kconfig option.

When the application performs burst measurements, it also starts advertising.
This means it can be connected through a Bluetooth central device, for example, the `nRF Connect for Mobile`_ application on your smartphone or tablet.

Firmware architecture
=====================

The nRF Peripheral Sensor node application has a modular structure, where each module has a defined scope of responsibilities.
The application uses the :ref:`app_event_manager` to distribute events between modules in the system.

The following figure shows the application architecture.
The figure visualizes relations between Application Event Manager, modules, drivers, and libraries.

.. figure:: /images/peripheral_sensor_node_arch.svg
   :alt: Peripheral Sensor node application architecture

   Peripheral Sensor node application architecture

Programming nRF54L15 PDK
========================

The nRF54L15 PDK has a J-Link debug IC that can be used to program the firmware.

See the `Building and running`_ section for more information.

Requirements
************

The application supports the following development kit:

.. table-from-sample-yaml::

User interface
**************

You can interact with the application by using the logging system and Bluetooth application described in the :ref:`nrf_peripheral_sensor_node_app_testing` section.

Configuration
*************

The nRF Peripheral Sensor node application is modular and event-driven.
You can enable and configure the modules separately for the selected board and build type.
See the documentation page of the selected module for information about functionalities provided by the module and its configuration.

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_APP_CONTROL_BURST_INTERVAL_S:

CONFIG_APP_CONTROL_BURST_INTERVAL_S
   The application configuration specifies the time in seconds between burst data acquisitions from sensors.
   If sensor data burst is not triggered by ``wake_up_event`` from IMU, burst is performed every interval value.

.. _CONFIG_APP_CONTROL_BURST_MEAS_CNT:

CONFIG_APP_CONTROL_BURST_MEAS_CNT
   The application configuration specifies the number of measurements a single burst acquires before going to sleep.
   The default value is ``10``.

.. _CONFIG_APP_CONTROL_BURST_MEAS_INTERVAL_MS:

CONFIG_APP_CONTROL_BURST_MEAS_INTERVAL_MS
   The application configuration specifies the time interval between measurements for a single burst.
   The default value is 1000 ms.

Configuration files
===================

The nRF Peripheral Sensor node application uses the following files as configuration sources:

* Devicetree Specification (DTS) files - These reflect the hardware configuration.
  See :ref:`zephyr:dt-guide` for more information about the DTS data structure.
* Kconfig files - These reflect the software configuration.
  See :ref:`kconfig_tips_and_tricks` for information about how to configure them.
* :file:`_def` files - These contain configuration arrays for the application modules.
  The :file:`_def` files are used by the nRF Peripheral Sensor node application modules and :ref:`lib_caf` modules.

Configuration files are defined in the :file:`applications/peripheral_sensor_node/configuration/nrf54l15dk_nrf54l15_cpuapp@soc1` directory.

Advertising configuration
-------------------------

If a given build type enables Bluetooth, the :ref:`caf_ble_adv` is used to control the Bluetooth advertising.
This CAF module relies on :ref:`bt_le_adv_prov_readme` to manage advertising data and to scan response data.
The nRF Peripheral Sensor node application configures the data providers in the :file:`src/util/Kconfig` file.
By default, the application enables a set of data providers available in the |NCS|.
If the Noridc UART Service (NUS) is enabled in the configuration, and the Bluetooth local identity in use has no bond, the application also adds a custom provider that appends UUID128 of NUS to the scan response data.

Building and running
********************

.. |sample path| replace:: :file:`applications/peripheral_sensor_node`

The Peripheral Sensor node application is built the same way as any other |NCS| application or sample.

Complete the following steps to program the nRF54L15 PDK using the command line interface:

   1. Connect the nRF54L15 PDK to your computer using the IMCU USB port on the PDK.
   #. Navigate to the |sample path| folder containing the sample.
   #. Build the application for the nRF54L15 PDK by invoking ``west build`` and, using the ``-DSHIELD="peripheral_sensor_node"`` option.

      .. code-block:: console

         west build -b nrf54l15dk_nrf54l15_cpuapp@soc1 -- -DSHIELD="peripheral_sensor_node"

   #. Flash the sample using the standard |NCS| flash command:

      .. code-block:: console

         west flash

      The command flashes all required binaries to the PDK from the build directory, then resets and runs the application.

      .. note::

            When programming the device, you might get an error similar to the following message::

               ERROR: The operation attempted is unavailable due to readback protection in
               ERROR: your device. Please use --recover to unlock the device.

            This error occurs when readback protection is enabled.
            To disable the readback protection, you must *recover* your device.

            Enter the following command to recover the core::

               west flash --recover

            The ``--recover`` command erases the flash memory and then writes a small binary into the recovered flash memory.
            This binary prevents the readback protection from enabling itself again after a pin reset or power cycle.


Application logs
================

The application provides logs through the UART of the PDK.

.. _nrf_peripheral_sensor_node_app_testing:

Testing with the PDK
====================

After programming the application, perform the following steps to test the nRF Peripheral Sensor node application on the Development Kit:

1. Turn on the development kit.
   The application starts Bluetooth advertising.
#. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
#. Connect to the device from the application.
   When connected, go to more options and click :guilabel:`Request MTU`.
   Set the value to ``498``.
   The device is advertising as ``Peripheral Sensor Node``.
   The services of the connected device are shown.
   If the device cannot be found, the PDK might be connected to another device.
   Terminate the connection with the other device, refresh the scanning and connect to the PDK.
   The bonding data can be erased by running the following command:

      .. code-block:: console

         west flash --erase

#. Find the ``Env read`` charateristic (UUID ``de550004-acb6-4c73-8445-2563acbb43c2``).
#. Enable notification for this characteristic.
#. Find ``Env read req`` charateristic (UUID ``de550002-acb6-4c73-8445-2563acbb43c2``).
#. Write to ``Env read req`` a number of requested history depth as little endian uint16_t.
#. Observe the returned data in ``Env read``.
#. See the latest measurement results of IMU and environmental sensors through a separate **Nordic Status Message Service** characteristics.
#. To observe the logs, connect to the development kit with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.

Application internal modules
****************************

The nRF Peripheral Sensor node application uses modules available in :ref:`lib_caf` (CAF), a set of generic modules based on Application Event Manager and available to all applications, and a set of dedicated internal modules.
See `Firmware architecture`_ for more information.

The nRF Peripheral Sensor node application uses the following modules available in CAF:

* :ref:`caf_ble_adv`
* :ref:`caf_ble_state`
* :ref:`caf_settings_loader`

See the module pages for more information about the modules and their configuration.

The nRF Peripheral Sensor node application also uses the following dedicated application modules:

``app_control``
  The module controls the state of the application.
  It periodically schedules burst measurements if the burst is not triggered by the ``ADXL362`` trigger.

``sensors``
  The module uses Zephyr's :ref:`zephyr:sensor_api` to handle the sampling and sensor management.

  The module generates the following types of events in relation with the sensor defined in the module configuration file:

  * :c:struct:`sensor_event` when the sensor is sampled.
  * :c:struct:`sensor_state_event` when the sensor state changes.
  * :c:struct:`wake_up_event` when the ``ADXL362`` trigger activates.

  The module is controlled with the following types of events:

  * :c:struct:`sensor_start_event` starts sampling of the desired sensor.
  * :c:struct:`sensor_stop_event` stops sampling of the desired sensor.
  * :c:struct:`power_down_event` enables trigger of the ``ADXL362``.

``env_history``
  The module captures sensor events.
  In case the event contains environmental sensor data, the module prepares ``struct env_storage_chunk`` and requests the non-volatile storage (NVS) write operation.
  NVS partition configured for this module has 12 sectors, 4 kB each.
  When write is requested and there is not enough memory, NVS erases the oldest sector, and write is performed as usual.
  The recorded chunk contains ID of 32 bits, which is incremented each time.
  Chunks can be accessed through the Bluetooth characteristics as described in the :ref:`nrf_peripheral_sensor_node_app_testing` section.

Dependencies
************

The application uses the following Zephyr drivers and libraries:

* :ref:`zephyr:sensor_api`
* :ref:`zephyr:uart_api`

The application uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`
* :ref:`lib_caf`
* :ref:`nus_service_readme`
* :ref:`nsms_readme`

API documentation
*****************

Following are the API elements used by the application.

Application Sensor Event
========================

| Header file: :file:`applications/peripheral_sensor_node/src/events/app_sensor_event.h`
| Source file: :file:`applications/peripheral_sensor_node/src/events/app_sensor_event.c`

.. doxygengroup:: app_sensor_event
   :project: nrf
   :members:
