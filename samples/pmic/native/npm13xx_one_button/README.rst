.. _npm13xx_one_button:

nPM1300 and nPM1304: One button
###############################

.. contents::
   :local:
   :depth: 2

The One button sample demonstrates how to support wake-up, shutdown, and user interactions of `nPM1300 <nPM1300 product website_>`_ or `nPM1304 <nPM1304 product website_>`_ using a single button connected to the PMIC's **SHPHLD/RESET** pin.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an `nPM1300 EK <nPM1300 EK product page_>`_ or an `nPM1304 EK <nPM1304 EK product page_>`_ that you need to connect to the development kit as described in `Wiring`_.

Overview
********

The PMIC's **SHPHLD/RESET** button controls the device state.
The sample controls an LED, load switch and PMIC power mode based on button press duration.

The PMIC's **GPIO3** pin is configured as an interrupt output, and is used to signal button press and USB detection events to the host.

.. _npm13xx_one_button_wiring:

Wiring
******

To connect your DK to the nPM1300 or nPM1304 EK, complete the following steps:

#. Make the following connections on the EK:

   a. Remove all existing connections, including jumpers and USB-C cables.
   #. On the **P1** pin header, connect **VBATIN** and **VBAT** pins with a jumper.
   #. On the **P13** pin header, connect **RSET1** and **VSET1** pins with a jumper.
   #. On the **P14** pin header, connect **RSET2** and **VSET2** pins with a jumper.
   #. On the **P15** pin header, connect **VOUT1** and **LSIN1** pins with a jumper.
   #. On the **P17** pin header, connect **HOST** and **LED2** pins with a jumper.
   #. Connect a suitable battery to either the **J2** or **J1** connector. When using the nPM1304-EK, the **J3** connector can also be used.

   With these connections the battery is powering the EK, the BUCK regulators are enabled, and the I/O reference voltage is supplied by a DK as described in the next step.

#. Connect the chosen DK to the EK as in the following table:

   .. list-table:: nPM1300/nPM1304 EK connections.
      :widths: auto
      :header-rows: 1

      * - PMIC EK pins
        - nRF52 DK pins
        - nRF52840 DK pins
        - nRF5340 DK pins
        - nRF54L15/nRF54LM20 DK pins
        - nRF54H20 DK pins
        - nRF9160 DK pins
      * - SDA
        - P0.26
        - P0.26
        - P1.02
        - P1.11
        - P0.05
        - P0.30
      * - SCL
        - P0.27
        - P0.27
        - P1.03
        - P1.12
        - P0.00
        - P0.31
      * - GPIO3
        - P0.04
        - P0.04
        - P0.04
        - P0.04
        - P0.04
        - P0.04
      * - VDDIO
        - VDD
        - VDD
        - VDD
        - VDDIO
        - VDD_P0
        - VDD
      * - GND
        - GND
        - GND
        - GND
        - GND
        - GND
        - GND

.. note::

   When using the :zephyr:board:`nrf54l15dk`, the PMIC **GPIO3** interrupt pin assignment uses the DK's **BUTTON 3** pin.

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/native/npm13xx_one_button`

.. include:: /includes/build_and_run.txt

To build this sample for either nPM1300 or nPM1304, you need to apply the respective extra DTC overlay.
You can use either the nRF Connect for VS Code extension or the command line.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      To apply an extra overlay, choose the respective file from the **Extra Devicetree overlays** drop-down menu

   .. group-tab:: Command line

      To apply the appropriate configuration, use the ``-DEXTRA_DTC_OVERLAY`` CMake argument.
      For example, to build for an nRF54L15 DK and an nPM1300 EK use the following command:

      .. code-block:: bash

         west build -b nrf54l15dk/nrf54l15/cpuapp samples/pmic/native/npm13xx_one_button -- -DEXTRA_DTC_OVERLAY=npm1300.overlay

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
   5 - 10 sec    Long press    PMIC enters ship mode provided **J3** is not connected
   > 10 sec      None          PMIC long press reset activates
   ============  ============  =========================================================

Ship mode is the lowest power state of the PMIC.
To exit ship mode, press the **SHPHLD** button or attach a USB cable to **J3**.
In a production design, this powers off any device that is powered from a PMIC output.

The long press reset performs a full power cycle of the PMIC and resets all setting to powerup defaults.
In a production design, this power cycles any device that is powered from a PMIC output.

When using a separately powered development kit, you must restart the application to reconfigure the PMIC after exiting the ship mode or long press reset.

Dependencies
************

The sample uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:shell_api`
