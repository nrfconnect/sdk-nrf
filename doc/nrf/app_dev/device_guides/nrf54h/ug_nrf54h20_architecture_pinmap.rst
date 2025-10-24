.. _ug_nrf54h20_architecture_pinmap:

nRF54H20 Pin mapping
####################

.. contents::
   :local:
   :depth: 2

The pins of the nRF54H20 SoC are spread across domains, like the peripherals.

The nRF54H20 SoC manages power to both peripherals and pins automatically and efficiently, when both the mapped
peripheral and pins are in the same domain.

The nRF54H20 also supports mapping pins across domains. This comes at the cost of additional management by the
firmware, thus additional latency and/or power consumption.

The following segments describe how pins are powered, which domains they are in, and how to map and handle them.

Pin groups
----------

A pin group is a group of pins designated by an index, like P0.x, where P0 is the group containing
[P0.0, P0.1, ...]. The pin group P0 is destinct from the GPIO controller with the same instance, P0. The P0
peripheral is routed to pins in the pin group P0, but is not neccesarily in the same domain as the pin group P0.

The following table shows which domains the pin groups belong to.

.. list-table:: Pin group domains
   :header-rows: 1

   * - Pin group
     - Domain
   * - P0
     - SLOW_MAIN
   * - P1
     - SLOW_MAIN
   * - P2
     - SLOW_MAIN
   * - P6
     - FAST_ACTIVE_1
   * - P7
     - FAST_ACTIVE_1
   * - P9
     - SLOW_MAIN

Pin group power
---------------

Pins require power to drive their output, and the muxes routing a pin to a peripheral require power. This power is
provided by the domain the pin and the muxes are in. If a pin is configured as an output, the output value will be
retained even when the power domain is suspended.

Peripheral domains
------------------

Peripherals with an instance ending in 120, like UARTE120, PWM120, SPIS120, are in the FAST_ACTIVE_1 domain, the rest
are in the SLOW_MAIN and SLOW_ACTIVE domains.

Pin group power management
--------------------------

If a pin and the peripheral it is routed to are in the same domain, the peripheral will force the domain on when in
use, thus powering the pin and the required muxes as well.

If a pin and the peripheral it is routed to are in different domains, the peripheral can only force on its own
domain, not the domain the pin is in.

Peripherals in the FAST_ACTIVE_1 domain should be routed to pins in the FAST_ACTIVE_1 domain, other peripherals should
be routed to pins in the SLOW_MAIN domain.

It is possible to route pins across domains, though this is not recommended as:

* A "fast" peripheral in the FAST_ACTIVE_1 domain will be limited by the slower slew rate of pins in the "slow" domain.
* Firmware must force on the domain the pins are in before the peripheral is used if power management (CONFIG_PM) is enabled.

To force on either the FAST_ACTIVE_1 or SLOW_MAIN power domains on, power management is used:

.. code-block:: c

   #include <zephyr/pm/device_runtime.h>

   static const struct device *fast_active_pd = DEVICE_DT_GET(DT_NODELABEL(gdpwr_fast_active_1));
   static const struct device *slow_main_pd = DEVICE_DT_GET(DT_NODELABEL(gdpwr_slow_main));

   int main(void)
   {
           /*
            * Reference counted API to force FAST_ACTIVE_1 power domain on, this operation is slow
            * and must be done from a thread, thus call it once before use of the peripheral,
            * and release it after.
            */
           pm_device_runtime_get(fast_active_pd);

           /*
            * Use peripheral in SLOW_MAIN domain with pins routed to fast domain,
            * like GPIO controller P6
            */

           /* Release reference */
           pm_device_runtime_put(fast_active_pd);
   }

If power management is not enabled, all power domains will be constantly powered on, so no additional management is
required.
