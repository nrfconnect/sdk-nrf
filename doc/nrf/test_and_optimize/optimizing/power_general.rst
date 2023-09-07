.. _app_power_opt_general:

General power optimization recommendations
##########################################

.. contents::
   :local:
   :depth: 2

Nordic System-on-Chip (SoC) designs are aimed at ease of use and ultra-low power consumption.
However, working on a final design might require fine-tuning some areas of your application.

The following sections describe general recommendations for reducing power consumption of your application.
These recommendations are valid regardless of the SoC you are using.

Use power profiling tools
*************************

A power-optimized development ecosystem typically consists of the following developer tools:

* |NCS|
* Development kits
* `Power Profiler Kit II (PPK2)`_

Power Profiler Kit II is a flexible, low-cost tool that you can use for real-time power measurement of your design.
You can measure power on a connected development kit or any external board.

Together, these tools provide a unified solution for developers to evaluate, develop and characterize ultra-low power designs with ease.
See :ref:`installation` and :ref:`configuration_and_build` for more information about the |NCS| and its development environment.

Disable serial logging
**********************

Current measurements on devices that have the |NCS| samples or applications programmed with the default configuration, might show elevated current values, when compared to the expected current values from  Nordic ultra-low power SoCs.
It is because most of the samples and applications in the |NCS| are configured to perform logging over serial port (associated with UART(E) peripheral) by default.

As an example, the image below shows the power measurement output on Power Profiler Kit II for an nRF9160 DK with the :ref:`zephyr:blinky-sample` sample compiled for the ``nrf9160dk_nrf9160_ns`` build target without modifications in the sample configuration.

.. figure:: images/app_power_opt_blinky_serial_on.png
   :width: 100 %
   :alt: Current measurement on the Blinky sample with serial logging enabled

   Current measurement for the Blinky sample with serial logging enabled

The average current is close to 470 µA, which drains a 500 mAh lithium polymer battery approximately in six weeks.
To reduce current consumption, disable serial logging.

To disable serial output, you must change the project configuration associated with the sample or application.
|config|

.. note::
    If the application consists of multiple images, like applications built for the nRF53 Series, logging must be disabled on both images.
    See :ref:`ug_nrf5340` and :ref:`ug_multi_image`.

1. Set the project configuration ``CONFIG_SERIAL`` to ``n`` irrespective of whether you are building the sample for the :ref:`SPE-only <app_boards_spe_nspe_cpuapp>` build targets or build targets with :ref:`NSPE <app_boards_spe_nspe_cpuapp_ns>`.
#. For the build target with NSPE (``nrf9160dk_nrf9160_ns``), ensure that serial logging is also disabled in Trusted Firmware-M by setting :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` to ``y``.

The output on Power Profiler Kit II shows the power consumption on an nRF9160 DK with the sample compiled for the ``nrf9160dk_nrf9160_ns`` build target with ``CONFIG_SERIAL=n``.

.. figure:: images/app_power_opt_blink_serial_off.png
   :width: 100 %
   :alt: Current measurement on the Blinky sample with serial logging disabled

   Current measurement on the Blinky sample with serial logging disabled

The average current reduces to 6 µA, which implies 9.5 years of battery life on a 500 mAh lithium polymer battery compared to the 6-week battery life of the previous measurement.

For a similar configuration, see the :ref:`udp` sample, which transmits UDP packets to an LTE network using an nRF9160 DK.
You can use the sample to characterize the current consumption of the nRF9160 SiP.
It is optimized for low power operation on the ``nrf9160dk_nrf9160_ns`` build target without any modifications.

Verify idle current due to other peripherals
********************************************

Peripherals other than the serial ports can also cause elevated currents.

The power management of the Nordic SoCs automatically switches in and out the resources that are needed by the active peripherals.
Peripherals that need a high frequency clock like UART, PWM, PDM or high frequency timers will show similar currents if enabled.

You can check the current consumption in peripherals for the SoC you are using in the "Power and clock management" section of the Product Specification for your SoC on `Nordic Semiconductor Infocenter`_.
For example, for the nRF9160 SiP, see the `Electrical specification of nRF9160`_ page.

.. note::
   Be careful with the use of pull-up resistors when designing the hardware for ultra-low power operation.
   An I/O pin with a 10 kΩ pull-up resistor that is set to ``GND`` will result in a current consumption of 300 µA at 3V.
