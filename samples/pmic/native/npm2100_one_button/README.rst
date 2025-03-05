.. _npm2100_one_button:

nPM2100: One button
###################

.. contents::
   :local:
   :depth: 2

The One button sample demonstrates how to support wake-up, shutdown, and user interactions
through a single button connected to the `nPM2100 PMIC <nPM2100 product website_>`_.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an nPM2100 Evaluation Kit (EK) that you need to connect to the
development kit as described in `Wiring`_.

Overview
********

The **SHPHLD** button of the nPM2100 EK controls the device state.
The sample controls an LED and the power mode based on the button press duration.

The **GPIO1** pin of the nPM2100 EK is configured as an interrupt output and used to signal button press events to the host.

.. _npm2100_one_button_wiring:

Wiring
******

With this configuration, the nPM2100 EK is wired to supply power to the DK.
This ensures that the TWI communication is at compatible voltage levels, and represents a realistic use case for the nPM2100 PMIC.

.. note::

   To prevent leakage currents and program the DK, do not remove the USB connection.

   Unplug the battery from the nPM2100 EK and set the DK power switch to "OFF" while
   applying the wiring.
   If you have issues communicating with the DK or programming it after applying the wiring, try to power cycle the DK and EK.

To connect your DK to the nPM2100 EK, complete the following steps:

#. Prepare the DK for being powered by the nPM2100 EK:

   .. tabs::

      .. group-tab:: nRF52840 and nRF5340 DKs

         * Set switch **SW9** ("nRF power source") to position "VDD".
         * Set switch **SW10** ("VEXT -> VnRF") to position "ON".

      .. group-tab:: nRF54L15 DK

         * Remove jumper from **P6** ("VDDM CURRENT MEASURE").

#. Connect the TWI interface and power supply between the chosen DK and the nPM2100 EK
   as described in the following table:

   .. list-table:: nPM2100 EK connections.
      :widths: auto
      :header-rows: 1

      * - nPM2100 EK pins
        - nRF52840 DK pins
        - nRF5340 DK pins
        - nRF54L15 DK pins
      * - SDA
        - P0.26
        - P1.02
        - P1.11
      * - SCL
        - P0.27
        - P1.03
        - P1.12
      * - GPIO1
        - P1.12
        - P1.12
        - P1.08
      * - VOUT
        - P21 External supply +
        - P21 External supply +
        - P6 VDDM current measure, VDD:nRF pin
      * - GND
        - GND / P21 External supply -
        - GND / P21 External supply -
        - GND

#. Make the following connections on the nPM2100 EK:

   * Remove the USB power supply from the **J4** connector.
   * On the **P6** pin header, connect pins 1 and 2 with a jumper.
   * On the **BOOTMON** pin header, select **OFF** with a jumper.
   * On the **VSET** pin header, select **3.0V** with a jumper.
   * On the **VBAT SEL** switch, select **VBAT** position.
   * Connect a battery board to the **BATTERY INPUT** connector.

.. note::

   When using the :ref:`zephyr:nrf54l15dk_nrf54l15`, the nPM2100 **GPIO1** interrupt
   pin assignment uses the **BUTTON 2** pin of the DK.

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/native/npm2100_one_button`

.. include:: /includes/build_and_run.txt

Testing
*******

|test_sample|

#. |connect_kit|
#. |connect_terminal|

If the initialization was successful, the terminal displays the following message with status information:

.. code-block:: console

   PMIC device ok

The following table describes the output and outcome of button presses:

.. _table::
   :widths: auto

   ============  ============  ===================================
   Duration      Log output    Outcome
   ============  ============  ===================================
   < 1 sec       Short press   Sample flashes **LED** at 5 Hz
   1 - 5 sec     Medium press  Sample flashes **LED** at 1 Hz
   5 - 10 sec    Ship mode...  nPM2100 enters ship mode (VOUT off)
   > 10 sec      None          nPM2100 resets (VOUT briefly off)
   ============  ============  ===================================

Ship mode is the lowest power state of the nPM2100 PMIC.
Entering it powers off the host SoC and you will lose connection to the shell interface.
To exit the ship mode, press the **SHPHLD** button at least for one second.

A long press reset performs a full power cycle of the nPM2100 PMIC and resets all settings to powerup defaults.
This will also power cycle the host SoC.

Dependencies
************

The sample uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:shell_api`
