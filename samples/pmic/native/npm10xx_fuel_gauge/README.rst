.. _npm10xx_fuel_gauge:

.. ncs-sample::
   :title: nPM10xx: Fuel gauge

   The Fuel gauge sample demonstrates how to estimate the battery state of charge using an nPM1012 PMIC and the :ref:`nrfxlib:nrf_fuel_gauge`.

Requirements
************

The sample includes board overlays for the following development kits:

* nRF54L15 DK (``nrf54l15dk/nrf54l15/cpuapp``)
* nRF54LC10 DK (``nrf54lc10dk/nrf54lc10a/cpuapp``)
* nRF54LM20 DK (``nrf54lm20dk/nrf54lm20a/cpuapp`` or ``nrf54lm20dk/nrf54lm20b/cpuapp``)

You also need an nPM1012 evaluation kit (EK) and a rechargeable battery with an NTC thermistor.
Connect the EK to the DK as described in `Wiring`_.

Overview
********

The sample reads battery voltage, current, and NTC temperature through the nPM10xx ADC driver.
It uses the nPM10xx charger driver to read charging status, charge type, and configured charging limits.
These measurements and charger information are supplied to the nRF Fuel Gauge library to estimate state of charge, time to empty, time to full, and state of health.

The sample uses the 20 mAh battery model described in the :file:`src/battery_model_20mAh.inc` file.
The :file:`npm1012.overlay` file configures charging with the following values:

* Termination voltage: 4.2 V.
* Constant charging current: 10 mA.
* Termination current: 6.25% of the constant charging current.
* Thermistor beta: 3434 K.

The sample processes measurements approximately every 500 ms and logs the results.

State of health is calculated based on battery charge cycles.
This is a slow process.
For testing purposes, you can accelerate the process as follows:

* Use a battery model with a very low capacity that can be charged quickly, such as the 20 mAh model included in this sample.
* Configure the charging current to be as high as possible within the limits of your battery or as high as the PMIC allows when using a power supply or battery emulator instead of an actual battery.
* Begin charging at state of charge less than 70%, and ensure that the charging cycle completes by reaching the configured termination voltage and termination current.
* Ensure that charging completes before the battery state of charge reaches 100%, either by using a battery emulator, or configure the charger termination voltage to be lower than the modeled battery termination voltage.
* Repeat the charging cycles at least two times to see any change in state of health.

The state of health will only decrease when the state of charge is less than 100% when charging completes.

The state of health will also be calculated during partial charge cycles, but this is a slower process compared to full charge cycles.

When the Zephyr Settings subsystem is enabled, the sample also demonstrates how to store the fuel gauge state in non-volatile memory and restore it on boot.
You can store the fuel gauge state using a shell command described in `Testing`_.

.. _npm10xx_fuel_gauge_wiring:

Wiring
******

To connect your DK to the nPM1012 EK, complete the following steps:

#. Make the following connections on the EK:

   a. Remove all existing connections, including jumpers and USB-C cables.
   #. On the **P1** pin header, connect **VBATIN** and **VBAT** pins with a jumper.
   #. On the **P17** pin header, connect CHG LED with a jumper.
   #. On the **P14** pin header, connect **RSET2** and **VSET2** pins with a jumper.
   #. If your battery does not include an NTC thermistor, connect the **NTC** and **10kOhm** pins on the **P3** pin header with a jumper.
   #. Connect a suitable battery to either the **J2** or **J1** connector.
   #. Connect a USB power supply to the **J4** connector.

   With these connections, the USB power supply is powering the EK, the BUCK regulators are enabled, and the I/O reference voltage is supplied by the DK as described in the next step.

   .. note::
      Battery charging will not begin if the NTC signal is disconnected.

#. Connect the chosen DK to the EK as in the following table:

   .. list-table:: nPM1012 EK connections.
      :widths: auto
      :header-rows: 1

      * - EK pins
        - nRF54L Series DK pins
      * - SDA
        - P1.11
      * - SCL
        - P1.12
      * - GPIO2
        - P0.04
      * - VDDIO
        - VDDIO
      * - GND
        - GND

.. note::
   When using the :zephyr:board:`nrf54l15dk`, the PMIC **GPIO2** interrupt pin assignment uses the DK's **BUTTON 3** pin.

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/native/npm10xx_fuel_gauge`

.. include:: /includes/build_and_run.txt

Apply the :file:`npm1012.overlay` file as an extra devicetree overlay.
In the nRF Connect for VS Code extension, select it under **Extra Devicetree overlays**.
On the command line, use the ``-DEXTRA_DTC_OVERLAY_FILE`` CMake argument.
For example, to build for an nRF54L15 DK, run the following command:

.. code-block:: bash

   west build -b nrf54l15dk/nrf54l15/cpuapp samples/pmic/native/npm10xx_fuel_gauge -- -DEXTRA_DTC_OVERLAY_FILE=npm1012.overlay

Testing
*******

|test_sample|

#. |connect_kit|
#. |connect_terminal|

After successful initialization, the sample logs the fuel gauge library version, the initial charger state, and ``PMIC device ok``.
Measurement output has the following format (with illustrative values):

.. code-block:: console

   PMIC device ok
   V: 4.101, I: 0.000, T: 23.06, SoC: 93.09, TTE: nan, TTF: nan, SoH: 100.00, Cycles: 0, Chg: 0.00

.. list-table:: Measurement output
   :widths: auto
   :header-rows: 1

   * - Symbol
     - Description
     - Units
   * - V
     - Battery voltage
     - Volts
   * - I
     - Battery current
     - Amps (positive for charging, negative for discharging)
   * - T
     - Battery temperature
     - Degrees Celsius
   * - SoC
     - State of charge
     - Percent
   * - TTE
     - Estimated time to empty
     - Seconds (may be NaN)
   * - TTF
     - Estimated time to full
     - Seconds (may be NaN)
   * - SoH
     - State of health (available battery capacity)
     - Percent of modeled capacity
   * - Cycles
     - Charge-cycle count
     - Integer
   * - Chg
     - Cumulative charge since the last full cycle
     - Milliamp-hours

The displayed current follows the Zephyr sensor API convention.
The sample reverses its sign before passing it to the nRF Fuel Gauge library.
Time estimates can be NaN when the library does not yet have enough information to provide an estimate.

Connect or remove VBUS power and observe the ``Vbus connected`` or ``Vbus removed`` messages.
As charging progresses, the sample also logs charger-state changes, such as:

* ``Trickle charging``
* ``Constant current charging``
* ``Constant voltage charging``
* ``Charge complete``

To save the state, including learned state of health and cycle information, run the following command:

.. code-block:: console

   uart:~$ fuel_gauge_state_store
   Storing state after next update

The fuel gauge state will be stored in non-volatile memory after the next measurement has been performed, and restored on the next boot.
Use the following shell commands to view the stored state and clear it:

.. code-block:: console

   uart:~$ settings list
   fuel_gauge_state
   uart:~$ settings delete fuel_gauge_state

Dependencies
************

The sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_fuel_gauge`

It also uses the Zephyr nPM10xx MFD, ADC, charger, and LED drivers, and the following libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:shell_api`
* :ref:`zephyr:settings_api`
