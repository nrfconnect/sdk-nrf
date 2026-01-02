.. |matter_name| replace:: Window covering
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |matter_dks_tfm| replace:: ``nrf54l15dk/nrf54l15/cpuapp`` board target
.. |sample path| replace:: :file:`samples/matter/window_covering`
.. |matter_qr_code_payload| replace:: MT:SAGA442C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_window_covering.png
                          :width: 200px
                          :alt: QR code for commissioning the window covering device

.. include:: /includes/matter/shortcuts.txt

.. _matter_window_covering_sample:

Matter: Window covering
#######################

.. contents::
   :local:
   :depth: 2

This sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a window covering device.

.. include:: /includes/matter/introduction/sleep_thread_ssed_sit.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread.txt

Overview
********

The sample uses buttons for gradually changing the position and movement mode of the window cover, and LEDs to show the state of these changes.
The following movement modes are available:

* Lift - In this movement mode, the window cover moves up and down.
* Tilt - In this movement mode, the window cover slats are tilted forward or backward without the cover moving vertically.

See `User interface`_ for information about how to switch the movement modes.

Window covering features
========================

The window covering sample implements the following features:

* :ref:`SSED device type <matter_window_covering_sample_ssed>` - The window cover operates as a Thread Synchronized Sleepy End Device (SSED).

.. _matter_window_covering_sample_ssed:

.. include:: /includes/matter/overview/ssed.txt

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt
The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/tfm.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   Indicates the lift position of the window cover, which is represented by the brightness of the LED.
   The brightness level ranges from ``0`` to ``255``, where the brightness level set to ``0`` (switched off LED) indicates a fully opened window cover (lifted) and the brightness level set to ``255`` indicates a fully closed window cover (lowered).

   Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
   The command's argument can be used to specify the duration of the effect.

Third LED:
   Indicates the tilt position of the window cover, which is represented by the brightness of the LED.
   The brightness level ranges from ``0`` to ``255``, where the brightness level set to ``0`` (switched off LED) indicates a fully opened window cover (tilted to a horizontal position) and the brightness level set to ``255`` indicates a fully closed window cover (tilted to a vertical position).

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   When pressed once and released, it moves the window cover towards the open position by one step.
   Depending on the movement mode of the cover (see `Overview`_), the button decreases the brightness of either  **LED 2** for the lift mode or **LED 3** for the tilt mode.

Third Button:
   When pressed once and released, it moves the cover towards the closed position by one step.
   Depending on the movement mode of the cover (see `Overview`_), the button increases the brightness of either  **LED 2** for the lift mode or **LED 3** for the tilt mode.

Second and Third Buttons:
   When pressed at the same time, they toggle the cover movement mode between lift and tilt.
   After each device reset, the mode is set to lift by default.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

.. note::
   Completely opening and closing the cover requires 20 button presses (steps).
   Each step takes approximately 200 ms to simulate the real window cover movement.

   The cover position and the LED brightness values are stored in non-volatile memory and are restored after every device reset.
   After the firmware update or factory reset both LEDs are switched off by default, which corresponds to the cover being fully open, both lift-wise and tilt-wise.

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt

|matter_ble_advertising_auto|

Testing
*******

.. include:: /includes/matter/testing/intro.txt

Testing with CHIP Tool
======================

Complete the following steps to test the |matter_name| device using CHIP Tool:

.. |node_id| replace:: 1

.. include:: /includes/matter/testing/1_prepare_matter_network_thread.txt
.. include:: /includes/matter/testing/2_prepare_dk.txt
.. include:: /includes/matter/testing/3_commission_thread.txt

.. rst-class:: numbered-step

Observe that |Second LED| and |Third LED| are turned off, which means that the window cover is fully open
---------------------------------------------------------------------------------------------------------

.. rst-class:: numbered-step

Close the cover in the lift movement mode
-----------------------------------------

.. parsed-literal::
   :class: highlight

   chip-tool windowcovering go-to-lift-percentage 100 |node_id| 1

|Second LED| lights up and its brightness increases until it reaches full brightness.

.. rst-class:: numbered-step

Close the cover's slats in the tilt movement mode
-------------------------------------------------

.. parsed-literal::
   :class: highlight

   chip-tool windowcovering go-to-tilt-percentage 100 |node_id| 1

|Third LED| light up and its brightness increases until it reaches full brightness.

Testing using DK buttons
========================

Complete the following steps to test the |matter_name| device using the DK buttons:

.. rst-class:: numbered-step

Observe the initial state
-------------------------

Observe that |Second LED| and |Third LED| are turned off, which means that the window cover is fully open.

.. rst-class:: numbered-step

Press the |Third Button| 20 times
---------------------------------

Press the |Third Button| 20 times to fully close the cover in the lift movement mode.
|Second LED| lights up and its brightness increases with each button press until it reaches full brightness.

.. rst-class:: numbered-step

Press the |Second Button| 20 times
----------------------------------

Press the |Second Button| 20 times to fully lift the cover up by running the following command:
The brightness of |Second LED| decreases with each button press until the LED turns off.

.. rst-class:: numbered-step

Press the |Second Button| and the |Third Button| together
---------------------------------------------------------

Press the |Second Button| and the |Third Button| together to switch into the tilt movement mode

.. rst-class:: numbered-step

Press the |Third Button| 20 times
---------------------------------

Press the |Third Button| 20 times to fully tilt the cover's slats into the closed position.
|Third LED| light up and its brightness increases with each button press until it reaches full brightness.

.. rst-class:: numbered-step

Press the |Second Button| 20 times
----------------------------------

Press the |Second Button| 20 times to fully tilt the cover into the open position.
The brightness of |Third LED| decreases with each button press until the LED turns off.

Testing with commercial ecosystem
=================================

.. include:: /includes/matter/testing/ecosystem.txt

Dependencies
************

.. include:: /includes/matter/dependencies.txt
