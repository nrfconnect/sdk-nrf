.. _saadct:

SAADC + TIMER (SAADCT) driver
#############################

.. contents::
   :local:
   :depth: 2

The SAADCT driver provides timer-triggered SAADC sampling on Nordic nRF devices.
An external TIMER instance triggers the SAADC sample task through GPPI at a configured rate.
Measurement results are delivered as series of interleaved channel samples stored in memory slabs.
For an example of how to use the driver, see the :ref:`saadct_sample` sample.

Configuration
*************

Enable the driver with the :kconfig:option:`CONFIG_SAADCT` Kconfig option.

To use the driver, define an SAADCT instance in the devicetree and point its ``timer-instance`` property to the TIMER that triggers SAADC sampling:

.. code-block:: devicetree

        &timer20 {
                prescaler = <1>;
                status = "okay";
        };

        &adc {
                compatible = "nordic,nrf-saadct";
                status = "okay";
                timer-instance = <&timer20>;
        };

API documentation
*****************

| Header file: :file:`include/drivers/saadct.h`
| Source file: :file:`drivers/saadct/saadct_nrfx.c`

.. doxygengroup:: saadct
