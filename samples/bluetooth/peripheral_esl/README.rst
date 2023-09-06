.. _peripheral_esl:

Bluetooth: Peripheral ESL
#########################

.. contents::
   :local:
   :depth: 2

The Peripheral ESL sample acts as an ESL(Electronic Shelf Label) tag to work with ESL Access Point between a Bluetooth® LE connection and Periodic Advertising with Response synchronization.
It uses :ref:`esl_service_readme` and implement callbacks for the service.

.. include:: ./esl_experimental.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::
   .. :header: heading
   .. :rows: nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52840dk_nrf52840, nrf21540dk_nrf52840, thingy52_nrf52832

Overview
********

You can use this sample as an ESL tag defined in `Electronic Shelf Label Profile <https://www.bluetooth.com/specifications/specs/electronic-shelf-label-profile-1-0/>`_ and `Electronic Shelf Label Service <https://www.bluetooth.com/specifications/specs/electronic-shelf-label-service-1-0/>`_ to work with the ESL Access Point.
This sample uses **LED** on DK to simulate display on ESL tag. It also contains EPD variant to demonstrate how to work with real (E-Paper display) with shield board.

The sample uses the :ref:`esl_service_readme` library to implement the Electronic Shelf Label Service which allows electronic shelf labels (ESLs) to be controlled and updated using Bluetooth wireless technology.
The sample implements callbacks for the ESL service to handle events from the ESL service.
Once the Tag is associated to an AP, it stores in internal non-volatile memory relevant parameters (e.g. ESL ID, bonding / key material, etc). Such material is kept after power cycle.

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

Development kits
================

LED 1:
   ESL LED 1: Flashing with specified pattern when received ESL LED control OP code.

LED 2:
   ESL LED 2: Flashing with specified pattern when received ESL LED control OP code.

LED 3:
   Simulated Display: Blinks 5 times when receiving ESL display image OP code. (Deactivated when using a real display in the EPD variant)

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
   Debug button to unassociate the tag from Access Point. Remove all stored images and configured ESL addr and key materials including bonding keys

.. _peripheral_esl_build_and_program:

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/esl/peripheral_esl`

.. include:: /includes/build_and_run.txt

.. _peripheral_esl_sample_activating_variants:

.. _peripheral_esl_debug:

Debugging
=========

In this sample, the console is used for ESL tag shell command.
Debug messages are not displayed in this console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.


Configuration
*************

|config|

For each board, at least two standard configurations are available:
The :file:`prj.conf` file represents a ``debug`` build type which includes debug message on RTT segger and shell command for developiong test.
The :file:`prj_release.conf` file represents a ``release`` build type which excludes all debug features to reduce ram usage.

.. _peripheral_esl_sample_activating_epd_variants:

EPD variant
===========

This sample uses :ref:`waveshare_e_paper_raw_panel_shield` and `WAVESHARE 250x122, 2.13inch E-Ink raw display panel` as reference to demonstrate how to display image on ESL.
For now, only :ref:`nrf52dk_nrf52832` and :ref:`nrf52833dk_nrf52833` support this optional feature.
In VS Code with nRF Extension, select prj_release.conf and select “nrf52833dk_nrf52833_power_profiler_epd.conf” as Kconfig fragments.
As alternative, using terminal, to activate the optional feature supported by this sample, use the following build command:

   .. code-block:: console

      west build -p -b nrf52833dk_nrf52833 -- -DOVERLAY_CONFIG=nrf52833dk_nrf52833_power_profiler_release_epd.conf -DDTC_OVERLAY_FILE=conf/nrf52833dk_nrf52833/nrf52833dk_nrf52833_release_epd.overlay

See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

.. note::

   Please refer to the original manufacturer's documentation of the panel for selecting the shield board switch positions. For example, adjust to A0 for `WAVESHARE 250x122, 2.13inch E-Ink raw display panel`.

.. _peripheral_esl_power_profiling_variant:

Power Profiling variant
=======================

Power optimized overlay disables all of unnecessary peripheral to minimize power consumption. You can use power profiler overlay for :ref:`nrf52833dk_nrf52833` to measure current. In VS Code with nRF Extension, select prj_release.conf and select “nrf52833dk_nrf52833_power_profiler_release_led.conf” as Kconfig fragments.
As alternative, using terminal, use the following build command:

   .. code-block:: console

      west build -p -b nrf52833dk_nrf52833 -- -DCONF_FILE=prj_release.conf -DOVERLAY_CONFIG=nrf52833dk_nrf52833_power_profiler_release_led.conf -DDTC_OVERLAY_FILE=conf/nrf52833dk_nrf52833/nrf52833dk_nrf52833_power_profiler_release.overlay

Please follow  :ref:`peripheral_esl_power_measure` to measure currents.

.. _peripheral_esl_power_measure:

Power Consumption Measurement
=============================

Prerequisite: Read :ref:`app_power_opt_general` and get `Power Profiler Kit II (PPK2)`_

1. Build and program the Tag (:ref:`nrf52833dk_nrf52833`) Power Profiling configuration by following :ref:`peripheral_esl_build_and_program` and :ref:`peripheral_esl_power_profiling_variant`

#. Follow `Preparing a DK for current measurement`_ to prepare 52833DK for Power Consumption measurements

   a. Cut SB40.
   b. Connect VOUT of PPK2 to nRF current Measurement on DK, connect GND of PPK2 to External supply negative polarity pin (`-`). By doing so, PPK2 will provide power to only the nRF52 SoC.
   c. Connect the PPK2 (powered ON) in “Source Meter” mode to the :ref:`nrf52833dk_nrf52833` (turned ON) and enable power output 3000mV on the Power Profiler GUI and start measurement. Power the rest of the ESL Tag DevKit via USB (same as programming USB port), and keep the DevKit turned ON.

   .. figure:: /images/esl_power_measure_unsynced.png

#. If previously associated to the AP, wait the :ref:`nrf52833dk_nrf52833` to get synched. **LED 4** on the ESL Tag should be blinking slowly (PAwR interval); otherwise follow the instructions above to associate the Tag to an AP and to test basic functionalities.

#. Measure nRF52833 current while synchronized

   a. On :ref:`nrf52833dk_nrf52833`, set the SW6 switch to “nRF Only”. This will further isolate the nRF52 from some DK components wich may be causes of leakages(i.e. Interface MCU, LED and Buttons). LEDs will stop blinking.
   b. Current consumption should now be less than 5uA.

   .. figure:: /images/esl_power_measure_synced.png


Once completed this procedure (i.e. if you don’t need to use the PPK2 and you want to power the ESL Tag DevKit from the same USB port as used for programming), put Jumper back on SB40.

.. _peripheral_esl_config_options:

Configuration options
=====================

Check and configure the following Kconfig options:

.. _peripheral_bt_esl_security_enabled:

BT_ESL_SECURITY_ENABLED
   This enables BT SMP and bonding which are security requirement of ESL Profile. Disable BLE security of the ESL service for debugging purposes.

.. _esl_simulate_display:

ESL_SIMULATE_DISPLAY
   This enable simulated display feature which uses **LED 2** on DK to simulate E-Paper display. DK will blinks few times times(:kconfig:option:`CONFIG_ESL_SIMULATE_DISPLAY_BLINK_TIMES` + Image index from Display Control OPcode) when receiving valid Display Control OPcode. This could be useful for those developers without real E-Paper display.

.. _esl_simulate_display_blink_times:

ESL_SIMULATE_DISPLAY_BLINK_TIMES
   This configuration option specifies how many basic times to blink when receiving Display Control OPcode with :kconfig:option:`CONFIG_ESL_SIMULATE_DISPLAY` enabled.

.. _bt_esl_pts:

BT_ESL_PTS
   Enable PTS feature. With this feature ESL Tag will uses static BLE address. This feature is used for PTS test only.

.. _peripheral_esl_testing:

Testing
=======

After programming the sample to your development kit, you can test it in combination with a :ref:`nRF5340 DK <ug_nrf5340>` running the ESL Access Point (:ref`central_esl`) by performing the following steps:

1. Turn on the ESL Tag power.
#. Optionally, Push **Button 1** to erase possible previous stored parameters and start from scratch.
#. |connect_terminal|
   The UART console provides ESL Tag shell command for debugging.
#. Optionally, you can display debug messages. See :ref:`peripheral_esl_debug` for details.
   ESL Shell command on UART console could be used to debug.
#. Observe that **LED 4** is blinking with 10hz frequency and the device is advertising with the ESL UUID (``0x1857``). The Tag is ready for being associated by an ESL AP.
#. Use the ESL Access Point( :ref`central_esl`) on the other DK :ref:`nRF5340 DK <ug_nrf5340>` to connect to the ESL tag and configure mandatory characteristic by:
   #. Using the automatic onboarding feature in the ESL Access Point.
   #. Using the shell commands in the ESL Access Point.
   Observe that **LED 4** is on while connected to the Access Point.
#. After ESL Tag is being configured with mandatory characteristic and receives PAST (Periodic Advertising Sync Transfer) from the Access Point, the ESL device disconnects the connection.
   Observe that **LED 4** is off for a short time and then. **LED 4** starts blinking with PAwR (Periodic Advertising with Responses) interval which set by ESL Access Point, signaling that the ESL Tag has been associated and it is now synchronized to the AP.
#. Use the AP :ref:`central_esl_user_interface` to send command to the ESL TAG:
   a. You can use :ref:`central_esl_subcommands` or,
   b. if auto onboarding feature has been already activated on the AP to assoicate the tag, you can try to:

   Flash an LED: Push **Button 4** on the AP; Observes that **LED 1** on the Tag is now flashing with specified pattern.

   Change ESL Display Image: push **Button 3** on the AP; Observe that **LED 3** on the Tag is now blinking 5 times to simulate image changed. If EPD is enabled, observe new image displayed on the EPD.
#. Use AP shell command interface to send a predefined ESL Sync Packet:
   | example: send sync packet type 0x7 (broadcast command to display image **0**) to Group **0**:

   .. code-block:: console

      esl_c pawr update_pawr 7 0

Dependencies
************

This sample uses the following |NCS| libraries:

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

.. _WAVESHARE 250x122, 2.13inch E-Ink raw display:
   https://www.waveshare.com/2.13inch-e-paper.htm
