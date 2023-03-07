.. _caf_sensor_data_aggregator:

CAF: Sensor data aggregator module
##################################

.. contents::
   :local:
   :depth: 2

The |sensor_data_aggregator| of the :ref:`lib_caf` (CAF) is a simple module responsible for aggregating sensor data in form of ``sensor_event`` and passing them further in packages.
It can be used in both single-core and multi-core SoCs.

When used with multi-core SoCs, the |sensor_data_aggregator| can reduce power consumption.
One core gathers data from sensors and when there is sufficient data to analyze, the first core wakes up the other core and sends the aggregated data to that core.

Configuration
*************

To enable the |sensor_data_aggregator|,select the :kconfig:option:`CONFIG_CAF_SENSOR_DATA_AGGREGATOR` Kconfig option.

To use the module, you must complete the following steps:

1. Enable the :kconfig:option:`CONFIG_CAF_SENSOR_DATA_AGGREGATOR` option.
#. If you are using multi-core SoC and want to receive aggregated data on another core, on the second core enable the :kconfig:option:`CONFIG_CAF_SENSOR_DATA_AGGREGATOR_EVENTS` option.
#. Enable aggregator in devicetree file that describes the aggregator parameters you can use, for example :file:`app.overlay` file.
   Each aggregator should be placed as a separate node.
   For example, the file content could look like follows:

   .. code-block:: devicetree

      agg0: agg0 {
              compatible = "caf,aggregator";
              sensor_descr = "accel_sim_xyz";
              buf_data_length = <240>;
              sample_size = <3>;
              status = "okay";
      };

      agg1: agg1 {
             compatible = "caf,aggregator";
             sensor_descr = "void_test_sensor";
             buf_data_length = <80>;
             sample_size = <1>;
             status = "okay";
      };

Two aggregators are defined here and each one is responsible to handle data of the different sensors.
The aggregator is defined as a separate node in the devicetree and consists of the following parameters:

* ``compatible`` - This is DTS binding and should be set to ``caf,aggregator``.
* ``sensor_descr`` - This parameter represents the description of the sensor and should be the same as the description in the :ref:`caf_sensor_manager`.
* ``buf_data_length`` - This parameter represents the length of the buffer in bytes.
  Its default value is ``120``.
  You should set the value as a multiple of sensor sample size times the size of :c:struct:`sensor_value` (``i*sample_size*sizeof(struct sensor_value)``).
* ``sample_size`` - This parameter represents the sensor sample size and is expressed in ``sensor_value`` per sample.
  Its default value is ``1``.
* ``buf_count`` - This parameter represents the number of buffers in the aggregator.
  Its default value is ``2``.
* ``status`` - This parameter represents the node status and should be set to ``okay``.

Implementation details
**********************

|sensor_data_aggregator| subscribes to following sensor manager events:

* :c:struct:`sensor_state_event`
* :c:struct:`sensor_event`
* :c:struct:`sensor_data_aggregator_release_buffer_event`.

The |sensor_data_aggregator| gathers data from :c:struct:`sensor_event` and stores the data in an active :c:struct:`aggregator_buffer`.
When buffer is full, the |sensor_data_aggregator| sends the buffer to :c:struct:`sensor_data_aggregator_event` struct.
Then module searches for the next free :c:struct:`aggregator_buffer` and sets it as an active buffer.

After changing the sensor state and receiving :c:struct:`sensor_state_event`, the |sensor_data_aggregator| sends the data that is gathered in the active buffer.

After receiving :c:struct:`sensor_data_aggregator_release_buffer_event`, the |sensor_data_aggregator| sets :c:struct:`aggregator_buffer` to free state.

Several buffers can be reduced to one, in case of a situation where the sampling period is greater than the time needed to send and process :c:struct:`sensor_data_aggregator_event`.
In the situation when sampling is much faster than the time needed to send and process :c:struct:`sensor_data_aggregator_event`, the number of buffers should be increased.
