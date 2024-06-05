.. _caf_sensor_manager_sample:

CAF: Sensor manager
###################

.. contents::
   :local:
   :depth: 2

The sensor manager sample demonstrates the functionality of the :ref:`caf_sensor_manager`.
It uses the :ref:`sensor_stub`, the sensor manager module, the :ref:`caf_sensor_data_aggregator`, and the workload simulator module to automatically sample and gather data and then receive and process it in packages.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample provides stub input data using :ref:`sensor_stub`.
The input data simulates acceleration in X, Y, and Z axes.
Whenever new data is generated, the :ref:`caf_sensor_manager` generates the appropriate :c:struct:`sensor_event` instances that include the generated data.

The data from the events is then passed to the :ref:`caf_sensor_data_aggregator`.
When the aggregator's buffer is full, the application sends aggregated data to the workload simulator module (``workload_sim``).
The workload simulator is a custom module used by this sample only.
This module handles the data received from the :ref:`caf_sensor_data_aggregator`, simulates workload on data using :c:func:`k_busy_wait` and notifies the aggregator module when the aggregator buffer can be freed.

If you are running this sample on an SoC with multiple cores, the workload simulator module (``workload_sim``) is placed on the second core.
All communication between the cores is done using :ref:`event_manager_proxy` and Zephyr subsystem :file:`include/ipc/ipc_service.h`.

Configuration
*************

|config|

Single-core configuration
=========================

For the MCU with multiple cores, the default configuration uses one core to simulate the sensor and the other core to process the sensor.
The multicore MCUs can also support a single-core configuration, where the sensor is simulated and processed on a single, selected core.
The configuration is placed in the :file:`boards/<board>_singlecore.conf` file.

To use this configuration, run the following command:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=boards/nrf5340dk_nrf5340_cpuapp_nrf5340_singlecore.conf

Building and running
********************

For the MCU with multiple cores, the default configuration ensures that all the required cores are built.
You can build and flash all the required images by completing the following steps for all the required cores.

.. |sample path| replace:: :file:`samples/caf_sensor_manager`

.. include:: /includes/build_and_run.txt

Complete the following steps to program the sample:

      1. Go to the sample directory.
      #. Open the command line terminal.
      #. Run the following command to build the application code for the host and the remote:

         .. code-block:: console

            west build -b nrf5340dk/nrf5340/cpuapp

      #. Program both the cores:

         .. code-block:: console

            west flash

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe that output similar to the following is logged on UART:

   .. parsed-literal::
      :class: highlight

      *** Booting Zephyr OS build v3.0.99-ncs1-2759-g07737b0b09e7  ***
      [00:00:00.257,232] <inf> main: Event manager initialized
      [00:00:00.258,239] <inf> event_proxy_init: Event proxy remote added
      [00:00:00.259,948] <inf> event_proxy_init: Event proxy sensor_data_aggregator_event registered
      [00:00:00.260,009] <inf> event_proxy_init: Event manager proxy started
      [00:00:00.260,284] <inf> event_proxy_init: All remotes ready
      [00:00:00.260,345] <inf> app_event_manager: e:module_state_event module:main state:READY
      [00:00:00.260,742] <inf> app_event_manager: e:sensor_data_aggregator_event Send sensor buffer desc address: 0x100e28a
      [00:00:00.260,894] <inf> app_event_manager: e: sensor_data_aggregator_release_buffer_event
      [00:00:02.260,620] <inf> app_event_manager: e:sensor_data_aggregator_event Send sensor buffer desc address: 0x100e28a
      [00:00:02.260,864] <inf> app_event_manager: e: sensor_data_aggregator_release_buffer_event



Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`caf_sensor_manager`
* :ref:`caf_sensor_data_aggregator`
* :ref:`app_event_manager`
* :ref:`event_manager_proxy`

This sample uses the following |NCS| drivers:

* :ref:`sensor_stub`

In addition, it uses the following Zephyr subsystems:

* :file:`include/ipc/ipc_service.h`
* :ref:`zephyr:logging_api`
