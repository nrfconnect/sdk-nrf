.. _sensor_stub:

Simple simulated sensor driver
##############################

.. contents::
   :local:
   :depth: 2

The sensor stub is just blind interface between sensor API and simple callback functions to fetch and get data.
The sensor stub was designed to implement sensor simulation with minimal required resources.
It can be used also to provide some real sensor data from the application in simplest way possible.
Muliple instances of the sensor stub are supported.

Configuration
*************

You can define instances of the sensor stub on Devicethree like this:

.. code-block:: devicetree

        sensor_stub: sensor_stub {
                compatible = "nordic,sensor-stub";
                label = "SENSOR_STUB";
                generator = "sensor_stub_gen";
                status = "okay";
        };

The most important setting here is the ``generator`` property.
The driver would require the user to implement following functions:
- ``int <generator>_init(const struct device *dev)`` - initialize the driver
- ``int <generator>_get(const struct device *dev, enum sensor_channel ch, struct sensor_value *val)`` - get the measure value
- ``int <generator>_fetch(const struct device *dev, enum sensor_channel ch)`` - fetch (prepare) the measured value

The functions has to have global linkage - they would be connected directly in the sensor stub driver.

API documentation
*****************

| Header file: :file:`include/drivers/sensor_stub.h`
| Source file: :file:`drivers/sensor/sensor_stub/sensor_stub.c`

.. doxygengroup:: sensor_stub
   :project: nrf
   :members:
