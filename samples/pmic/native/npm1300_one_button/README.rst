.. _npm1300_one_button:

nPM1300: One button
###################

.. contents::
   :local:
   :depth: 2

The One button sample demonstrates how to support wake-up, shutdown, and user interactions of `nPM1300 <nPM1300 product website_>`_ using a single button on a compatible development kit that is connected to the PMIC.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an `nPM1300 EK <nPM1300 EK product page_>`_ that you need to connect to the development kit as described in `Wiring`_.

Overview
********

The nPM1300's **SHPHLD/RESET** button controls the device state.
The sample controls an LED, load switch and nPM1300 power mode based on button press duration.

The nPM1300's **GPIO3** pin is configured as an interrupt output, and is used to signal button press and USB detection events to the host.

.. _npm1300_one_button_wiring:

Wiring
******

To connect your DK to the nPM1300 EK, complete the following steps:

#. Make the following connections on the nPM1300 EK:

   * Remove all existing connections.
   * On the **P1** pin header, connect **VBATIN** and **VBAT** pins with a jumper.
   * On the **P13** pin header, connect **RSET1** and **VSET1** pins with a jumper.
   * On the **P14** pin header, connect **RSET2** and **VSET2** pins with a jumper.
   * On the **P15** pin header, connect **VOUT1** and **LSIN1** pins with a jumper.
   * On the **P17** pin header, connect **HOST** and **LED2** pins with a jumper.
   * Connect a suitable battery to the **J2** connector.

#. Connect the chosen DK to the nPM1300 EK as in the following table:

   .. list-table:: nPM1300 EK connections.
      :widths: 25 25 25 25 25
      :header-rows: 1

      * - nPM1300 EK pins
        - nRF52 DK pins
        - nRF52840 DK pins
        - nRF5340 DK pins
        - nRF9160 DK pins
      * - SDA
        - P0.26
        - P0.26
        - P1.02
        - P0.30
      * - SCL
        - P0.27
        - P0.27
        - P1.03
        - P0.31
      * - GPIO3
        - P0.22
        - P0.22
        - P1.12
        - P0.10
      * - VDDIO
        - VDD
        - VDD
        - VDD
        - VDD
      * - GND
        - GND
        - GND
        - GND
        - GND

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/native/npm1300_one_button`

.. include:: /includes/build_and_run.txt

Testing
*******

|test_sample|

#. |connect_kit|
#. |connect_terminal|

If the initialization was successful, the terminal displays the following message with status information:

.. code-block:: console

   PMIC device ok

Different length button presses:

.. _table::
   :widths: auto

   ============  ============  =========================================================
   Duration      Log output    Outcome
   ============  ============  =========================================================
   < 1 sec       Short press   Sample flashes **LED** at 5 Hz, LDSW 1 enabled
   1 - 5 sec     Medium press  Sample flashes **LED** at 1 Hz, LDSW 1 disabled
   5 - 10 sec    Long press    nPM1300 enters ship mode provided **J3** is not connected
   > 10 sec      None          nPM1300 long press reset activates
   ============  ============  =========================================================

Ship mode is the lowest power state of the nPM1300.
To exit ship mode, press the **SHPHLD** button or attach a USB cable to **J3**.
In a production design, this will power off any device that is powered from an nPM1300 output.

The long press reset performs a full power cycle of the nPM1300 and resets all setting to powerup defaults.
In a production design, this will reset any device that is powered from an nPM1300 output.

When using a separately powered development kit, you must restart the application to reconfigure the nPM1300 after exiting ship mode or long press reset.

Dependencies
************

The sample uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:shell_api`
