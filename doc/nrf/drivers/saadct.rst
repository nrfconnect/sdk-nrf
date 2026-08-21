.. _saadct:

SAADC + TIMER (SAADCT) driver
#############################

.. contents::
   :local:
   :depth: 2

The SAADCT driver provides timer-triggered SAADC sampling on Nordic nRF devices.
An external TIMER instance triggers the SAADC sample task through GPPI at a
configured rate.
Measurement results are delivered as series of interleaved channel samples stored
in memory slabs.

Configuration
*************

Enable the driver with the :kconfig:option:`CONFIG_SAADCT` Kconfig option.

You can define a SAADCT instance in devicetree like this:

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

The ``timer-instance`` property must point to the TIMER used to trigger SAADC
sampling.

API documentation
*****************

| Header file: :file:`include/drivers/saadct.h`
| Source file: :file:`drivers/saadct/saadct_nrfx.c`

.. doxygengroup:: saadct
