.. _pmic_npm6001_sample:

nPM6001 power management IC sample
####################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to configure and use the nPM6001 power management IC (PMIC) using the two-wire interface (TWI), GPIOs, and a sample driver.

The sample driver is used to initialize the nPM6001 and set the following parameters:

* Enable pull-down on all DCDC_MODE pins.
* Enable DCDC_MODE1 pin for DCDC1 regulator.
* Set DCDC0 ULP mode voltage to 3.3 V.
* Set DCDC1 PWM mode voltage to 1.2 V.
* DCDC2 voltage is left at its default setting.
* Set DCDC3 voltage to 2.8 V, and enable this regulator.
* Set LDO0 voltage to 2.7 V, and enable this regulator.
* Enable LDO1 regulator.
* Configure GENIO0 as an output pin.
* Enable all warning interrupts (overcurrent and thermal).

One can interact with the sample using buttons or shell commands, as described below.

.. _pmic_npm6001_sample_requirements:

Requirements
************

The sample requires an **nPM6001 Evaluation Kit** (nPM6001 EK), and one of the following nRF development kits:

.. table-from-sample-yaml::

Hardware overview
*****************

To use the sample as intended, the nPM6001 evaluation kit and nRF development kit has to be connected in the following ways:

* nPM6001 EK supplied by a voltage source that can provide voltage in the range of 3 to 5.5 V.
* nRF DK supplied by an external voltage pin header, connected to the DCDC0 regulator on nPM6001 EK.
* nRF DK reset pin connected to the nPM6001 READY signal.
* nRF DK and nPM6001 EK TWI and GPIOs connected for communication and control.

    .. note::
         The nRF52-DK USB should be disconnected after programming the sample. While USB is connected TWI communication with nPM6001 may fail.
         nRF52840-DK and nRF5340-DK can be connected and interacted with using USB at all times, by following the described selection switch settings.

Additionally, the nRF DK power selection switches must be configured for external voltage supply.

This table lists the required pin header connections between the nPM6001 EK and the supported nRF5x DKs.

.. list-table::
   :header-rows: 1
   :align: center

   * - Function
     - nPM6001
     - nRF5340-DK pin
     - nRF52840-DK pin
     - nRF52-DK pin
   * - nPM6001 power supply
     - P1 (VIN)
     - -
     - -
     - -
   * - nRF5x power supply
     - VOUT_DCDC0 + GND
     - P21 (External supply)
     - P21 (External supply)
     - P21 (External supply)
   * - Reset signal
     - Header P15, READY
     - Header P1, RESET
     - Header P1, RESET
     - Header P1, RESET
   * - Interrupt signal
     - Header P15, N_INT
     - Header P3, GPIO P1.00 (Arduino D0)
     - Header P3, GPIO P1.01 (Arduino D0)
     - Header P3, GPIO P0.11 (Arduino D0)
   * - Two-Wire, SDA
     - Header P7, SDA
     - Header P4, GPIO P1.02 (Arduino SDA)
     - Header P4, GPIO P0.26 (Arduino SDA)
     - Header P4, GPIO P0.26 (Arduino SDA)
   * - Two-Wire, SCL
     - Header P7, SCL
     - Header P4, GPIO P1.03 (Arduino SCL)
     - Header P4, GPIO P0.27 (Arduino SCL)
     - Header P4, GPIO P0.27 (Arduino SCL)

This table lists the required nRF52840-DK and nRF5340-DK selection switch positions. **The nRF52-DK does not have these switches**.

.. list-table::
   :header-rows: 1
   :align: center

   * - Switch
     - Position
   * - SW9 (nRF power source)
     - VDD
   * - SW10 (VEXT -> nRF)
     - ON
   * - SW6
     - DEFAULT

.. figure:: /images/pmic_npm6001ek_nrf53840dk.svg
   :alt: Wire connections and switch settings for nPM6001 and nRF52840-DK

   Wire connections and switch settings for nPM6001 and nRF52840-DK

Configuration
*************

|config|

User interface
**************

Button 1:
   Toggle DCDC3 regulator on/off.

Button 2:
   Increase DCDC0 voltage by 100 mV.

Button 3:
   Button 3: Decrease DCDC0 voltage by 100 mV.

Button 4:
   Toggle nPM6001 GENIO0 pin.

All four buttons pressed at the same time:
   Hibernate for 8 seconds.

LEDs:
   In case of an error, all LEDs will blink. Typically this indicates a TWI communication issue.

LED 1:
   On after successful initialization.

LED 2:
   On when DCDC3 is turned on.

LED 4:
   On when GENIO0 pin is set.

    .. note::
         The LEDs will likely appear weak if initialization fails, due to the default nPM6001 DCDC0 supply voltage being relatively low.

Building and running
********************
.. |sample path| replace:: :file:`samples/pmic/npm6001`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it by performing the following steps:

1. Observe that the **LED 1** turns on.
   This LED indicates successful initialization and communication with the nPM6001 EK.
#. Using a multimeter, observe a positive voltage on nPM6001 EK **VOUT_DCDC3** pin header.
#. Press **Button 2** one or more times
#. Using a multimeter, observe that the voltage on nPM6001 EK **VOUT_DCDC0** pin header has increased by 100 mV or more (depending on how many times the button was pressed).

   Alternatively, observe that the nRF5x-DK LEDs brightness change according to DCDC0 voltage.

#. Press **Button 1** on the development kit to disable the nPM6001 DCDC3 regulator.
   **LED 2** should now turn off.
#. Press all four buttons at the same time to trigger nPM6001 hibernation mode.
#. Observe that all LEDs turns off, and DCDC0 voltage becomes 0 V.
#. After approximately 8 seconds, observe **LED 1** turning on again. This indicates an exit from hibernation mode, and a successful initialization.

This sample also contains shell commands to control the nPM6001. The following steps can be taken to interact with the sample using the Shell.

    .. note::
         The Shell is disabled when using nRF52-DK. The nRF52-DK power routing is not suited for having both USB and external supply connected simultaneously. The UART connection may or may not work, depending on the external supply voltage, and one could get unwanted leakage currents.

1. Start a console application, like PuTTY, and connect to the nRF5x DK COM port.
   See :ref:`gs_testing` for more information on how to connect with PuTTY through UART.
#. Type the nPM6001 root help command in the console application, "npm6001 --help", followed by the enter key. **Hint:** the tab character works as an autocomplete function in the shell. This is a helpful tool when navigating the available shell commands.
#. Observe the following console output:

   .. code-block:: console

      uart:~$ npm6001 --help
      npm6001 - nPM6001 shell commands
      Subcommands:
      vreg       :Voltage regulator control
      reg        :Register read/write
      watchdog   :Watchdog enable/disable/kick
      hibernate  :Hibernate
      interrupt  :Interrupt enable/disable

#. Set DCDC1 regulator voltage to 2.5 V (2500 mV) by issuing the following shell command:

   .. code-block:: console

      uart:~$ npm6001 vreg DCDC1 2500

#. Set DCDC3 regulator voltage to 3 V (3000 mV) and the mode to PWM by issuing the following shell command:

   .. code-block:: console

      uart:~$ npm6001 vreg DCDC3 3000 pwm

#. Use a multimeter to observe DCDC1 and DCDC3 pin headers on nPM6001 EK voltage to be 2.5 V and 3.0 V respectively.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/device.h`
* :file:`include/drivers/i2c.h`
* :file:`include/drivers/gpio.h`
* :ref:`zephyr:logging_api`

Porting to a different platform
*******************************
The sample is designed with portability in mind, such that the core nPM6001 functions can be easily used on a platform other than nRF Connect SDK.
The hardware-dependent functions relate to the TWI communication interface.

To port the sample driver to a different platform, the following functions must be implemented:

* :c:func:`drv_npm6001_platform_init`
* :c:func:`drv_npm6001_twi_read`
* :c:func:`drv_npm6001_twi_write`

Additionally, the following functionality can optionally be implemented:

* Using GPIO output to control DCDC_MODE pins.
* Using GPIO input to detect interrupts signals from nPM6001 N_INT pin.

The ported functions are given to the sample driver as function pointers, contained in the :c:struct:`drv_npm6001_platform` struct.

When the platform-specific GPIO functionality detects that the N_INT interrupt signal becomes active, the :c:func:`drv_npm6001_int_read` can be called to read and clear the relevant interrupt status registers.
