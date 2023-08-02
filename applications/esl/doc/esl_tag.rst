.. _esl_tag:

ESL Tag
#######

.. contents::
   :local:
   :depth: 2

The ESL Tag application demonstrates how to use the :ref:`esl_service_readme`.
It uses the Electronic Shell Label service to act as an ESL tag to work with ESL Access Point between a Bluetooth® LE connection and Periodic Advertising with Response synchronization.

.. include:: esl_experimental.txt

Requirements
************

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52840dk_nrf52840, nrf21540dk_nrf52840, nrf52840dongle_nrf52840, thingy52_nrf52832

Overview
********

When powered up, the application enters unassociated state and advertises BLE service with ESL UUID (``0x1857``). Waiting Access Point to connect and associate it.
The application acts as an ESL device defined in `Electronic Shelf Label Profile <https://www.bluetooth.com/specifications/specs/electronic-shelf-label-profile-1-0/>`_ and `Electronic Shelf Label Service <https://www.bluetooth.com/specifications/specs/electronic-shelf-label-service-1-0/>`_.
This application uses **LED** on DK to simulate display on ESL tag. It also contains EPD variant to demonstrate how to work with real (E-Paper display) with shield board.

.. _peripheral_esl_debug:

Debugging
=========

In this application, the console is used for ESL tag shell command.
Debug messages are not displayed in this console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

User interface
**************

The user interface of the application depends on the hardware platform you are using.

Development kits
================

LED 1:
   ESL LED 1: Flashing with specified pattern when received ESL LED control OP code.

LED 2:
   ESL LED 2: Flashing with specified pattern when received ESL LED control OP code.

LED 3:
   Simulated Display: Blinks 5 times when receiving ESL display image OP code.

LED 4:
   Status LED: Blinks fast (10hz) when advertising ESL service.
   Blinks slow (PAwR interval) when being synchronized.
   Stable ON when connected with Access Point.

Button 1:
   Debug button to unassociate the tag from Access Point. Remove all stored images and configured ESL addr and key materials.

Thingy:52
=========

RGB LED:
   The RGB LED channels are used independently to display the following information:

   * Red channel (ESL LED) flashing when specified pattern when received ESL LED control OP code..
   * Green channel (Simulated Display) blinks 5 times when receiving ESL display image OP code.
   * Blue channel (Status LED) blinks fast (10hz) when advertising ESL service.
     Blinks slow (PAwR interval) when being synchronized.
     Stable ON when connected with Access Point.

Button:
   Debug button to unassociate the tag from Access Point. Remove all stored images and configured ESL addr and key materials.

.. _peripheral_esl_build_and_program:

Building and running
********************

.. |sample path| replace:: :file:`applications/esl/peripheral_esl`

.. include:: /includes/build_and_run.txt

.. _peripheral_esl_sample_activating_variants:

Configuration
*************

|config|

For each board, at least two standard configurations are available:
The :file:`prj.conf` file represents a ``debug`` build type which includes debug message on RTT segger and shell command for developiong test.
The :file:`prj_release.conf` file represents a ``release`` build type which excludes all debug features to reduce ram usage.


EPD variant
===========

This application uses :ref:`waveshare_e_paper_raw_panel_shield` and `GDEH0213B72 EPD <https://www.waveshare.com/2.13inch-e-paper.htm>`_ as reference to demonstrate how to display image on ESL.
For now, only :ref:`nrf52dk_nrf52832` and :ref:`nrf52833dk_nrf52833` support this optional feature.
To activate the optional feature supported by this application, use the following build command:

   .. code-block:: console

      west build -p -b nrf52833dk_nrf52833 -- -DOVERLAY_CONFIG=nrf52833dk_nrf52833_power_profiler_release_epd.conf -DDTC_OVERLAY_FILE=conf/nrf52833dk_nrf52833/nrf52833dk_nrf52833_release_epd.overlay

See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

.. _peripheral_esl_power_profiling_variant:

Power Profiling variant
=======================

Power optimized overlay disables all of unnecessary peripheral to minimize power consumption. You can use power profiler overlay for :ref:`nrf52833dk_nrf52833` to measure current. To use Power Profiling variant, use the following build command:

   .. code-block:: console

      west build -p -b nrf52833dk_nrf52833 -- -DCONF_FILE=prj_release.conf -DOVERLAY_CONFIG=nrf52833dk_nrf52833_power_profiler_release_led.conf -DDTC_OVERLAY_FILE=conf/nrf52833dk_nrf52833/nrf52833dk_nrf52833_power_profiler_release.overlay

Please see instruction :ref:`peripheral_esl_power_measure`.

.. _peripheral_esl_testing:

Testing with another kit
========================

After programming the application to your development kit, you can test it in combination with a `nRF5340 DevKit <nrf5340dk_nrf5340>`_ running the ESL Access Point (:ref:`esl_ap`) by performing the following steps:

1. Turn the ESL device power on.

#. |connect_terminal|
   The UART console provides ESL Tag shell command for debugging.
#. Optionally, you can display debug messages. See :ref:`peripheral_esl_debug` for details.
   ESL Shell command on UART console could be used to debug.
#. Observe that **LED 4** is blinking with 10hz frequency and the device is advertising with the ESL UUID (``0x1857``).
#. Use the ESL Access Point( :ref:`esl_ap`) on the other DK to connect to the ESL tag and configure mandatory characteristic by:
   #. Using the automatic onboarding feature in the ESL Access Point.
   #. Using the shell commands in the ESL Access Point.
   Observe that **LED 4** is on while connected to the Access Point.
#. After configured mandatory characteristic and PAST (Periodic Advertising Sync Transfer) is sent from the Access Point, the ESL device disconnects the connection.
   Observe that **LED 4** is off. Now **LED 4** is blinking with PAwR (Periodic Advertising with Responses) interval which set by ESL Access Point.
#. Send ESL LED control OP code from ESL Access Point to the ESL Tag.
   Observe that the LED specified in ESL control OP code is now flashing on the tag with specified pattern (either **LED 1** or **LED 2**).
#. Send ESL Display image OP code from ESL Access Point.
   Observe that **LED 3** is blinking 5 times to simulate image changed. If EPD is enabled, observe new image displayed on the EPD.


.. _peripheral_esl_power_measure:

Power Consumption Measurement
=============================

Prerequisite: Read :ref:`app_power_opt_general` and get `Power Profiler Kit II (PPK2)`_

1. Build and progam the Tag (nRF52833DK) Power Profiling configuration by following :ref:`peripheral_esl_build_and_program` and :ref:`peripheral_esl_power_profiling_variant`

#. Follow `Preparing a DK for current measurement`_ to prepare 52833DK for Power Consumption measurements

   a. Cut SB40.
   b. Connect VOUT of PPK2 to nRF current Measurement on DK, connect GND of PPK2 to External supply **-** polarity.
   c. Connect the PPK2 (powered ON) in “Source Meter” mode to the nRF52833DK (turned ON) and enable power output 3000mV on the Power Profiler GUI and start measurement.

   .. figure:: /images/esl_power_measure_unsynced.png

#. Wait the nRF52833DK to get synced. **LED 4** will be blinking with PAwR interval.

   a. You can trigger actions from the AP: example, by pushing **Button 4** in the nRF53(AP), **LED 1** in the Tags in Group 0 (i.e. the nRF52833DK) is turned ON for few seconds. By pushing **Button 3** it will request Tags in Group 0 to display image (blinks 5 times on **LED 3**).

#. Measure nRF52833 current while synchronized

   a. On nRF52833DK, set the SW6 switch to “nRF Only”. This will isolate the nRF52 from rest of the DK components, so to measure only the nRF52833 current. LEDs will stop blinking.
   b. Current consumption should now be less than 5uA.

   .. figure:: /images/esl_power_measure_synced.png

Dependencies
************

This application uses the following |NCS| libraries:

* :ref:`esl_service_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``incude/gpio.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``

References
**********

.. target-notes::

.. _GDEH0213B72 EPD website:
   https://www.waveshare.com/2.13inch-e-paper.htm
