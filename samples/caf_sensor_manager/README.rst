.. _caf_sensor_manager_sample:

CAF: Sensor manager
###################

.. contents::
   :local:
   :depth: 2

The sensor manager sample demonstrates the functionality of the :ref:`caf_sensor_manager`.
It uses the :ref:`sensor_sim`, the sensor manager module, the :ref:`caf_sensor_data_aggregator`, and the workload simulator module to automatically sample and gather data and then receive and process it in packages.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample provides simulated input data using :ref:`sensor_sim`.
The input data simulates acceleration in X, Y, and Z axes, with the acceleration generated as value of a periodic wave signal.
Whenever new data is generated, the :ref:`caf_sensor_manager` generates the appropriate :c:struct:`sensor_event` instances that include the generated data.

The data from the events is then passed to the :ref:`caf_sensor_data_aggregator`.
When the aggregator's buffer is full, the application sends aggregated data to the workload simulator module (``workload_sim``).
The workload simulator is a custom module used by this sample only.
This module handles the data received from the :ref:`caf_sensor_data_aggregator`, simulates workload on data using :c:func:`k_busy_wait` and notifies the aggregator module when the aggregator buffer can be freed.

If you are running this sample on an SoC with multiple cores, the workload simulator module (``workload_sim``) is placed on the second core.
All communication between the cores is done using :ref:`event_manager_proxy` and Zephyr subsystem :file:`include/ipc/ipc_service.h`.

Sensor stub
===========

By default, the sensor manager sample uses :ref:`sensor_sim` for sensor data simulation.
You can select the sensor stub module instead.
The sensor stub module has a simple implementation that relates fully to the application implementing the function to generate the sensor data.
This sample implements a simple generator for the sensor stub that uses no floating point mathematics.

For more details on the sensor stub configuration, see the :ref:`sensor_stub_config` section.

Configuration
*************

|config|

Single core configuration
=========================

For the MCU with multiple cores, the default configuration will use one core to simulate the sensor and the other core to process the sensor.
These multiple core MCU can support single core configuration, where the sensor is simulated and processed on the single, selected core.
The configuration is placed in the :file:`boards/<board>_singlecore.conf file`.

To use this configuration, specify the ``-DOVERLAY_CONFIG=boards/<board>_singlecore.conf`` parameter along with the build command when building the sample:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=boards/nrf5340dk_nrf5340_cpuapp_nrf5340_singlecore.conf

.. _sensor_stub_config:

Sensor stub configuration
=========================

The Sensor stub configuration is provided in the :file:`sensor_stub_overlay.conf` file.
To use this configuration, specify the ``-DOVERLAY_CONFIG=sensor_stub_overlay.conf`` parameter along with the build command when building the sample.

For the multicore configuration, it would change to ``-Dremote_OVERLAY_CONFIG=sensor_stub_overlay.conf`` as shown in the following example:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -- -Dremote_OVERLAY_CONFIG=sensor_stub_overlay.conf

When single-core configuration is used on multicore MCU, add the sensor selection after the single-core configuration parameter as shown in the following example:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG="boards/nrf5340dk_nrf5340_cpuapp_nrf5340_singlecore.conf;sensor_stub_overlay.conf"

Building and running
********************

.. |sample path| replace:: :file:`samples/caf_sensor_manager`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe that output similar to the following is logged on UART:

   .. parsed-literal::
      :class: highlight

      [00:12:*** Booting Zephyr OS build v2.7.99-ncs1-2182-gbb39dfe3121a  ***
      [00:00:00.258,605] <inf> event_manager: e:module_state_event module:main state:READY
      [00:00:00.258,728] <inf> event_manager: e:module_state_event module:sensor_sim_ctrl state:READY
      [00:00:00.258,850] <inf> event_manager: e:sensor_state_event sensor:accel_sim_xyz state:ACTIVE
      [00:00:00.258,941] <inf> event_manager: e:sensor_data_aggregator_event Send sensor buffer desc: accel_sim_xyz
      [00:00:00.259,033] <inf> event_manager: e: sensor_data_aggregator_release_buffer_event
      [00:00:00.259,124] <inf> event_manager: e:module_state_event module:sensor_manager state:READY
      [00:00:00.458,526] <inf> event_manager: e:sensor_data_aggregator_event Send sensor buffer desc: accel_sim_xyz
      [00:00:00.458,709] <inf> event_manager: e: sensor_data_aggregator_release_buffer_event
      [00:00:00.658,538] <inf> event_manager: e:sensor_data_aggregator_event Send sensor buffer desc: accel_sim_xyz
      [00:00:00.658,721] <inf> event_manager: e: sensor_data_aggregator_release_buffer_event



Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`caf_sensor_manager`
* :ref:`caf_sensor_data_aggregator`
* :ref:`wave_gen`
* :ref:`app_event_manager`
* :ref:`event_manager_proxy`

This sample uses the following |NCS| drivers:

* :ref:`sensor_sim`

In addition, it uses the following Zephyr subsystems:

* :file:`include/ipc/ipc_service.h`
* :ref:`zephyr:logging_api`
