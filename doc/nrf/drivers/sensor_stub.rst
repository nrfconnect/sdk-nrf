.. _sensor_stub:

Sensor stub driver
##################

.. contents::
   :local:
   :depth: 2

The sensor stub, a simple simulated sensor driver, is an interface between the sensor API and simple callback functions to fetch and forward the data.
The sensor stub implements sensor simulation with minimal required resources.
It can also be used to provide some real sensor data from the application in the simplest way possible.
The application or the sample supports multiple instances of the sensor stub driver.

Configuration
*************

You can define instances of the sensor stub on devicetree like this:

.. code-block:: devicetree

        sensor_stub: sensor_stub {
                compatible = "nordic,sensor-stub";
                generator = "sensor_stub_gen";
                status = "okay";
        };

The most important setting here is the ``generator`` property.
The driver requires the following functions to be implemented:

- ``int <generator>_init(const struct device *dev)`` - This function initializes the driver.
- ``int <generator>_get(const struct device *dev, enum sensor_channel ch, struct sensor_value *val)`` - This function gets the measured value.
- ``int <generator>_fetch(const struct device *dev, enum sensor_channel ch)`` - This function fetches (prepares) the measured value.

The functions must have global linkage and connect directly to the sensor stub driver.

API documentation
*****************

| Header file: :file:`include/drivers/sensor_stub.h`
| Source file: :file:`drivers/sensor/sensor_stub/sensor_stub.c`

.. doxygengroup:: sensor_stub
   :project: nrf
   :members:
