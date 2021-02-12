.. _writable_ndef_msg:

NFC: Writable NDEF message
##########################

.. contents::
   :local:
   :depth: 2

The Writable NDEF message sample shows how to use the NFC tag to expose an NDEF message, which can be overwritten with any other NDEF message by an NFC device.
It uses the :ref:`lib_nfc_ndef`.

Overview
********

When the sample starts, it initializes the NFC tag and loads the NDEF message from the file in flash memory.
If the NDEF message file does not exist, a default message is generated: a URI message with a URI record containing the URL "nordicsemi.com".
The sample then sets up the NFC library for the Type 4 Tag platform, which uses the NDEF message and senses the external NFC field.

The library works in Read-Write emulation mode.
In this mode, procedures for reading and updating an NDEF message are handled internally by the NFC library.
Any changes to the NDEF message update the NDEF message file, which is stored in flash memory.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833

The sample also requires a smartphone or tablet with NFC Tools application (or equivalent).

User interface
**************

LED 1:
   Indicates if an NFC field is present.
LED 2:
   Indicates that the NDEF message was updated.
LED 4:
   Indicates that the NDEF message was read.

Button 1:
   Press during startup to restore the default NDEF message.

Building and running
********************
.. |sample path| replace:: :file:`samples/nfc/writable_ndef_msg`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Touch the NFC antenna with the smartphone or tablet and observe that LED 1 and LED 4 are lit.
#. Observe that the smartphone/tablet tries to open the URL "http\://www.nordicsemi.com" in a web browser.
#. Use a proper application (for example, NFC Tools for Android) to overwrite the existing NDEF message with your own message.
#. Power-cycle your kit and touch the antenna again.
   Observe that the new message is displayed.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_uri`
* :ref:`nfc_t4t_ndef_file_readme`

In addition, it uses the Type 4 Tag library from nrfxlib:

* :ref:`nrfxlib:nfc_api_type4`

The sample uses the following Zephyr libraries:

* ``include/atomic.h``
* ``include/zephyr.h``
* ``include/device.h``
* ``include/nvs/nvs.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
