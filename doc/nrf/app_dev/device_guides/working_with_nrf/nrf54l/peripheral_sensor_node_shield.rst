.. _peripheral_sensor_node_shield:

Developing with Peripheral Sensor node shield
#############################################

.. contents::
   :local:
   :depth: 2

The Peripheral Sensor node shield is an extension for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>`.
It is a testing shield designed for evaluating I2C and SPI protocol drivers.
It includes the following sensors:

* `ADXL362`_ accelerometer (SPI interface)
* `BMI270`_ accelerometer and gyroscope (SPI interface)
* `BME688`_ temperature, pressure, humidity and gas sensor (I2C interface with on-board pull-up resistors 4.7 kOhm).

The shield operates at a supply voltage of 1.8V.

.. figure:: /images/peripheral_sensor_node_shield.png
   :width: 350px
   :align: center
   :alt: Peripheral Sensor node shield

   Peripheral Sensor node shield

Pin assignment
**************

For the exact pin assignment for the nRF54L15 PDK, refer to the following table:

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

.. figure:: /images/peripheral_sensor_node_assy.png
   :width: 350px
   :align: center
   :alt: Peripheral Sensor node assembly

   Peripheral Sensor node assembly

.. note::
   The shield must be oriented in a way that allows its silkscreen to read the same way as the PDK.
   The shield must not be rotated.

Building and programming with Peripheral Sensor node shield
***********************************************************

To build for the Peripheral Sensor node Shield on the compatible :ref:`nRF54L15 PDK <ug_nrf54l15_gs>`, use the CMake ``SHIELD`` option set to ``pca63565``.
For more information on CMake options, see the :ref:`cmake_options` documentation section.

Run the following command:

.. code-block:: console

   west build -b nrf54l15pdk/nrf54l15/cpuapp -- -DSHIELD=pca63565

If you are using the |nRFVSC|, specify the ``-DSHIELD=pca63565`` argument in the **Extra Cmake arguments** field when `setting up a build configuration <How to work with build configurations_>`_.

Alternatively, you can add the shield in the project's :file:`CMakeLists.txt` file:

.. code-block:: none

   set(SHIELD pca63565)
