.. _npm2100_fuel_gauge:

nPM2100: Fuel gauge
###################

.. contents::
   :local:
   :depth: 2

The Fuel gauge sample demonstrates how to calculate the state of charge of a supported primary cell battery using the `nPM2100 <nPM2100 product website_>`_ and the :ref:`nrfxlib:nrf_fuel_gauge`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an nPM2100 evaluation kit that you need to connect to the development kit as described in `Wiring`_.

Overview
********

This sample allows the calculation of state of charge from a battery connected to the nPM2100 PMIC.

Battery models for Alkaline AA (1S and 2S configuration), AAA (1S and 2S configuration), LR44, and Lithium-manganese dioxide coin cell CR2032 batteries are included.
You can change the active battery model using a shell command, as illustrated in `Testing`_.
You can also change the battery model at compile time by selecting the type of battery using the following Kconfig options:

+-------------------------------------------------------+-------------------------------------------+
| :kconfig:option:`CONFIG_BATTERY_MODEL_ALKALINE_AA`    | Alkaline AA battery model                 |
+-------------------------------------------------------+-------------------------------------------+
| :kconfig:option:`CONFIG_BATTERY_MODEL_ALKALINE_AAA`   | Alkaline AAA battery model                |
+-------------------------------------------------------+-------------------------------------------+
| :kconfig:option:`CONFIG_BATTERY_MODEL_ALKALINE_2SAA`  | 2x alkaline AA battery (in series) model  |
+-------------------------------------------------------+-------------------------------------------+
| :kconfig:option:`CONFIG_BATTERY_MODEL_ALKALINE_2SAAA` | 2x alkaline AAA battery (in series) model |
+-------------------------------------------------------+-------------------------------------------+
| :kconfig:option:`CONFIG_BATTERY_MODEL_ALKALINE_LR44`  | Alkaline LR44 coin cell model             |
+-------------------------------------------------------+-------------------------------------------+
| :kconfig:option:`CONFIG_BATTERY_MODEL_LITHIUM_CR2032` | Lithium CR2032 coin cell                  |
+-------------------------------------------------------+-------------------------------------------+

.. _npm2100_fuel_gauge_wiring:

Wiring
******

With this configuration, the nPM2100 EK is wired to supply power to the DK.
This ensures that the TWI communication is at compatible voltage levels, and represents a realistic use case for the nPM2100 PMIC.

.. note::

   To prevent leakage currents and program the DK, do not remove the USB connection.

   Unplug the battery from the nPM2100 EK and set the DK power switch to "OFF" while applying the wiring described below.
   If you have issues communicating with the DK or programming it after applying the wiring, try to power cycle the DK and EK.

To connect your DK to the nPM2100 EK, complete the following steps:

#. Prepare the DK for being powered by the nPM2100 EK:

   .. tabs::

      .. group-tab:: nRF52840 and nRF5340 DKs

         * Set switch **SW9** ("nRF power source") to position "VDD".
         * Set switch **SW10** ("VEXT -> VnRF") to position "ON".

      .. group-tab:: nRF54L15 DK

         * Remove jumper from **P6** ("VDDM CURRENT MEASURE").

#. Connect the TWI interface and power supply between the chosen DK and the nPM2100 EK as described in the following table:

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
      * - VOUT
        - P21 External supply
        - P21 External supply
        - P6 VDDM current measure, VDD:nRF pin
      * - GND
        - GND
        - GND
        - GND

#. Make the following connections on the nPM2100 EK:

   * Remove the USB power supply from the **J4** connector.
   * On the **P6** pin header, connect pins 1 and 2 with a jumper.
   * On the **BOOTMON** pin header, select **OFF** with a jumper.
   * On the **VSET** pin header, select **3.0V** with a jumper.
   * On the **VBAT SEL** switch, select **VBAT** position.
   * Connect a battery board to the **BATTERY INPUT** connector.

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/native/npm2100_fuel_gauge`

.. include:: /includes/build_and_run.txt


Testing
*******

|test_sample|

#. |connect_kit|
#. |connect_terminal|

If the initialization was successful, the terminal displays the following message with status information:

.. code-block:: console

   PMIC device ok
   nRF Fuel Gauge version: 1.0.0
   Fuel gauge initialised for Alkaline AA battery.
   V: 1.188, T: 20.62, SoC: 25.00

.. _table::
   :widths: auto

   ======  ===============  ==================================================
   Symbol  Description      Units
   ======  ===============  ==================================================
   V       Battery voltage  Volts
   T       Temperature      Degrees C
   SoC     State of Charge  Percent
   ======  ===============  ==================================================

Determine the active battery type using the following shell command:

.. code-block:: console

   $ battery_model
   Battery model: Alkaline AA

Use the same shell command to change the active battery type:

.. code-block:: console

   $ battery_model Lithium_CR2032
   Fuel gauge initialised for Lithium CR2032 battery.

Dependencies
************

The sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_fuel_gauge`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:shell_api`
