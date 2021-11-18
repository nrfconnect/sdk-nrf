.. _record_launch_app:

NFC: Launch App
###############

.. contents::
   :local:
   :depth: 2

The NFC Launch App sample shows how to use the NFC tag to launch an app on the polling smartphone.
It uses the :ref:`lib_nfc_ndef` library.

Requirements
************

This sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuapp_ns, nrf52840dk_nrf52840,  nrf52dk_nrf52832, nrf52833dk_nrf52833


The sample also requires a smartphone or a tablet.
You need to have the nRF Toolbox app installed for iOS devices.

Overview
********

When the sample starts, it initializes the NFC tag and generates an NDEF message with two records that contain the app Universal Link and Android Application Record.
Then it sets up the NFC library to use the generated message and sense the external NFC field.

The only events handled by the application are the NFC events.

User interface
**************

LED 1:
   Indicates if an NFC field is present.

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/record_launch_app`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, perform the following steps to test it:

1. Touch the NFC antenna with the smartphone or tablet and observe that **LED 1** is lit.
#. Observe that the smartphone or tablet launches the nRF Toolbox application.
#. Move the smartphone or tablet away from the NFC antenna and observe that **LED 1**
   turns off.

   .. note::
	   Devices running iOS require the nRF Toolbox app to be installed before testing the sample.
	   Devices running Android will open Google Play when the application is not installed.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_launch_app`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the Type 2 Tag library from `sdk-nrfxlib`_:

:ref:`nrfxlib:type_2_tag`

The sample uses the following Zephyr libraries:

* ``include/zephyr.h``
* ``include/power/reboot.h``
