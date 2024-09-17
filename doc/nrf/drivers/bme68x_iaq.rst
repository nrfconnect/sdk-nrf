.. _bme68x_iaq:

BME68X IAQ driver
#################

.. contents::
   :local:
   :depth: 2

You can use the BME68X IAQ driver to run the Bosch Sensor Environmental Cluster (BSEC) library in order to get Indoor Air Quality (IAQ) readings.

The BSEC library is distributed with a Bosch proprietary license (`BSEC license`_) that prevents it from being part of |NCS|.
To start using it, you have to accept the license and enable the download with the following commands:

.. code-block::

   west config manifest.group-filter +bsec
   west update

See `BME680`_ for more information about BME680.

The :ref:`bme68x` sample demonstrates how to run this driver in an application.

Configuration
*************

To use the driver, configure the following Kconfig option:

* :kconfig:option:`CONFIG_BME680`- Set to ``n`` to disable the BME680 Zephyr driver.
* :kconfig:option:`CONFIG_SETTINGS` - Configure the Kconfig option and a settings backend to save the persistent state of the BSEC library.
* :ref:`CONFIG_BME68X_IAQ <CONFIG_BME68X_IAQ>` - To enable this driver.

API documentation
*****************

| Header file: :file:`drivers/sensor/bme68x/bme68x_iaq.h`
| Source file: :file:`drivers/sensor/bme68x/bme68x_iaq.c`
