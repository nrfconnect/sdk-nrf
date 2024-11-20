.. _ug_nrf54h20_architecture_clockman:

nRF54H20 clock management
#########################

.. contents::
   :local:
   :depth: 2

The nRF54H20 SoC consists of multiple asynchronous clock domains and requires clock sources for correct operation.
Each of the clock sources needs proper management for the following reasons:

* To start and stop the clock at the appropriate time.
* To select the proper clock source.
* To calibrate or synchronize the clock to a more accurate source.

Some clock management operations are performed solely by hardware circuits, while others require software intervention.
Most clocks can be locked to more accurate ones to improve their accuracy.


Clock domains
*************

The nRF54H20 SoC consists of the following clock domains:

* Low frequency clock (LFCLK)
* High-frequency oscillator (HFXO)
* FLL16M
* Application Core HSFLL
* Radio Core HSFLL
* Secure Domain HSFLL
* Global HSFLL

Each clock domain is enabled independently of others, and it is automatically enabled when at least one sink is active.
The firmware can choose to keep each clock domains enabled even when all its sinks are inactive.

Clock domain sources are selected by ``clock_control`` drivers based on the clock parameter requirements reported by the software modules using the ``clock_control`` functions.
These requirements can include clock frequency, accuracy, or precision.
Some clock domains are configured by ``clock_control`` drivers using ``LRCCONF`` peripherals, while others are configured with the assistance of the System Controller Firmware.

The application-facing APIs expose only the domain that directly clocks the component used by the application.
The management of the clocks on which this domain depends is handled internally by the associated clock driver.

For example, a firmware module might request better timing accuracy for a fast UART instance.
In this case, the firmware module requests the accuracy from the global HSFLL device driver, which directly clocks the UART.
The dependencies, such as FLL16M and LFXO, are managed internally by the global HSFLL driver, not directly by the firmware module.

LFCLK
=====

The Low-Frequency Clock domain is responsible for generating a 32768 Hz clock signal for ultra-low-power peripherals like ``GRTC`` or ``RTC``.

HFXO
====

The high-frequency crystal oscillator domain is responsible for clocking peripherals requiring high accuracy and MHz range frequency.
The drawback of this clock source is relatively high power consumption so it is enabled only when needed.

Hardware capabilities
---------------------

The HFXO, like any other clock domain, can be connected in hardware to one of the available sources.
Additionally, it provides the clock signal to all of its sinks.

FLL16M
======

The FLL16M clock domain clocks most of the peripherals in the system (*slow*, 16 MHz).
Its accuracy results in the timing accuracy of slow ``UART``, ``SPI``, ``TWI``, ``PWM``, ``SAADC``, among others.

Local HSFLLs
============

Local HSFLLs are clocking CPUs in local domains, fast peripherals around them, and local RAM.

Global HSFLL
============

Global HSFLL clocks *fast* peripherals, FLPR, RAM, and MRAM blocks.

Zephyr clock control API
************************

Zephyr RTOS contains a ``clock_control`` device driver class for managing clocks.
The ``clock_control`` API is designed to support multiple requestors.
Moreover, the ``clock_control`` API supports blocking or asynchronous operations based on notifications when the requested clock settings are applied.

The ``clock_control`` subsystem exposes portable parameters in its functions, which include:

* Accuracy requests in ppm values for all the clock domains.
* Precision requests in enumerated high- and low-precision modes.
* Frequency requests (clock rate) for the Application core HSFLL (all the other clock domains have fixed frequencies).

When multiple modules request conflicting parameters from the same clock, the system prioritizes selecting the mode with minimal power consumption that satisfies all requests.
For example, if a UART driver requests 100 ppm accuracy and a SPI driver requests 200 ppm accuracy, the system will choose a mode with 100 ppm accuracy or better, as it meets both requirements (100 ppm accuracy is better than 200 ppm) while optimizing power usage.
This policy applies to all clock parameters, including frequency and precision, following these criteria:

* The applied precision is at least as good as requested.
* The applied frequency is at least as fast as requested.
* All parameters are optimized for power consumption.

For more details, see the following links:

* :ref:`zephyr:clock_control_api`.
* The following calls in the `Zephyr's nRF clock control API extensions`_ (:file:`include/zephyr/drivers/clock_control/nrf_clock_control.h`):

  * ``nrf_clock_control_request()``: Requests a reservation to use a given clock with specified attributes.
  * ``nrf_clock_control_release()``: Releases a reserved use of a clock.
  * ``nrf_clock_control_cancel_or_release()``: Safely cancels a reservation request.

* The following calls in the `clocks devicetree macro API`_ (:file:`include/zephyr/devicetree/clocks.h`):

  * ``DT_CLOCKS_CTLR_BY_IDX()``: Gets the node identifier for the controller phandle from a *clocks* phandle-array property at an index.
  * ``DT_CLOCKS_CTLR()``: It is equivalent to ``DT_CLOCKS_CTLR_BY_IDX()`` with index (idx) set to 0.
