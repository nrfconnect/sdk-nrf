.. _pmic_npm6001_sample:

nPM6001 power management IC sample
##################################

.. contents::
   :local:
   :depth: 2

This sample shows how to configure and use the nPM6001 power management IC (PMIC) using the two-wire interface (TWI), GPIOs, and a sample library.

The sample library is used to initialize the nPM6001 and to set the following parameters:

* Enable pull-down on all BUCK_MODE pins.
* Enable BUCK_MODE1 pin for BUCK1 regulator.
* Set BUCK0 voltage to 3.3 V.
* Set BUCK1 voltage to 1.2 V.
* Set BUCK2 voltage to 1.4 V.
* Set BUCK3 voltage to 0.5 V, and enable this regulator.
* Set LDO0 voltage to 2.7 V, and enable this regulator.
* Enable LDO1 regulator.
* Configure GPIO0, GPIO1, and GPIO2 as output pins.
* Enable overcurrent interrupts.

    .. note::
         On some versions of the nPM6001 EK, the **BUCK** regulator labels are named **DCDC**, and **GPIO** labels are named **GENIO**.

         For example, **BUCK0** in this documentation is equivalent of **DCDC0** in these nPM6001 EK versions, **GENIO0** equivalent of **GPIO0**, and so on.

You can interact with the sample using buttons or shell commands outlined in this article.

.. _pmic_npm6001_sample_requirements:

Requirements
************

The sample requires nPM6001 Evaluation Kit (nPM6001 EK), and one of the following nRF development kits:

.. table-from-sample-yaml::

.. _pmic_npm6001_sample_hardware:

Hardware overview
*****************

To use the sample, connect the nPM6001 evaluation kit and nRF development kit as follows:

* Ensure the nPM6001 EK is supplied by a voltage source that can provide voltage in the range of 3 to 5.5 V.
* Ensure your nRF DK is supplied by the external voltage pin header, connected to the BUCK0 regulator on nPM6001 EK.
* Connect the nRF DK reset pin to the nPM6001 READY signal.
* Connect nRF DK, nPM6001 EK TWI and GPIOs for communication and control.

    .. note::
         The nRF52 DK USB should be disconnected after programming the sample.
         While USB is connected, TWI communication with nPM6001 might fail.
         When using USB, nRF52840 DK and nRF5340 DK can be connected and interacted with at all times.
         You can do it by following the described selection switch settings.
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
     - ---
     - ---
     - ---
   * - nRF5x power supply
     - VOUT_BUCK0 + GND
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
   * - Two-Wire, reference voltage
     - Header P7, VCC
     - Header P1, VDD
     - Header P1, VDD
     - Header P1, VDD

The table lists the required nRF52840 DK and nRF5340 DK selection switch positions.
The nRF52 DK, however, does not have these switches.

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

.. note::
   Turn the nRF5x DK power switch (SW8) off while changing these switch positions.

.. figure:: /images/pmic_npm6001ek_nrf53840dk.svg
   :alt: Wire connections and switch settings for nPM6001 and nRF52840 DK

   Wire connections and switch settings for nPM6001 and nRF52840 DK.

.. _pmic_npm6001_sample_config:

Configuration
*************

|config|

.. _pmic_npm6001_sample_ui:

User interface
**************

Button 1:
   Toggle BUCK3 regulator on/off.

Button 2:
   Increase BUCK3 voltage by 25 mV.

Button 3:
   Decrease BUCK3 voltage by 25 mV.

Button 4:
   Toggle nPM6001 GPIO pins.

All four buttons pressed at the same time:
   Hibernate for 8 seconds.

LEDs:
   In case of an error, all LEDs will blink.
   It typically indicates a TWI communication issue.

LED 1:
   On after successful initialization.

LED 2:
   On when BUCK3 is turned on.

LED 4:
   On when nPM6001 GPIO pins are set.

    .. note::
         The nRF5x DK LEDs will appear quite weak if nPM6001 initialization fails, due to the default voltage of the nPM6001 BUCK0 regulator being 1.8V.

    .. note::
         The nRF5x DK interface MCU LED (LD5) should be green when connected to USB and the nPM6001 is powering the nRF5x DK external supply header. If not, try turning the nRF5x DK power switch (SW8) off and on again.

Building and running
********************
.. |sample path| replace:: :file:`samples/pmic/npm6001`

.. include:: /includes/build_and_run.txt


.. _pmic_npm6001_sample_testing:

Testing
*******

After programming the sample, you can test it by performing the following steps:

1. Ensure the **LED 1** turns on.
   It indicates successful initialization and communication with the nPM6001 EK.

#. Using a multimeter, confirm there is a positive voltage on nPM6001 EK **BUCK3** pin header.
#. Press **Button 2** one or more times.
#. Using a multimeter, confirm that the voltage on nPM6001 EK **BUCK3** pin header has increased by 25 mV or more (depending on how many times you pressed the button).
#. Press **Button 1** on the DK to disable the nPM6001 BUCK3 regulator.
   **LED 2** should now turn off.
#. Press all four buttons at the same time to trigger nPM6001 hibernation mode.
#. Confirm that all LEDs turned off, and BUCK0 voltage became 0 V.

   The nRF5x DK USB device will also disappear from the connected computer during this period.

#. After approximately 8 seconds, check if **LED 1** turns on again. This indicates exiting the hibernation mode, and a successful initialization.

Testing with shell
------------------

This sample also contains shell commands to control the nPM6001.
Complete the following steps to interact with the sample using the shell:

    .. note::
         When using nRF52 DK, the shell is disabled.
         The nRF52 DK power routing is not suited for having both USB and external supply connected simultaneously. The UART connection may or may not work, depending on the external supply voltage, and can cause unwanted leakage currents.

1. Start a console application, for example PuTTY, and connect to the nRF5x DK COM port.
   See :ref:`gs_testing` for more information on how to connect with PuTTY through UART.

   Hint: the **nrfjprog** command line utility can be used to find the COM port associated with a nRF5x DK connected to USB.

   .. code-block:: console

      nrfjprog --com

#. Type the nPM6001 root help command in the console application

   .. code-block:: console

      npm6001 --help

   In the shell you can use **TAB** for autocomplete function, which is helpful when navigating the available shell commands.
   The output is as follows:

   .. code-block:: console

      uart:~$ npm6001 --help
      npm6001 - nPM6001 shell commands
      Subcommands:
      vreg       :Voltage regulator control
      reg        :Register read/write
      watchdog   :Watchdog enable/disable/kick
      hibernate  :Hibernate
      interrupt  :Interrupt enable/disable

#. Set BUCK1 regulator voltage to 1.2 V (1200 mV) by running the command:

   .. code-block:: console

      uart:~$ npm6001 vreg BUCK1 1200

#. Set BUCK3 regulator voltage to 3 V (3000 mV) and the mode to PWM by running the command:

   .. code-block:: console

      uart:~$ npm6001 vreg BUCK3 3000 pwm

#. Use a multimeter to ensure the BUCK1 and BUCK3 pin headers on nPM6001 EK voltage are 1.2 V and 3.0 V respectively.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/device.h`
* :file:`include/drivers/i2c.h`
* :file:`include/drivers/gpio.h`
* :ref:`zephyr:logging_api`
