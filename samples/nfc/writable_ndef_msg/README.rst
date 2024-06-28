.. _writable_ndef_msg:

NFC: Writable NDEF message
##########################

.. contents::
   :local:
   :depth: 2

The Writable NDEF message sample shows how to use the NFC tag to expose an NDEF message that can be overwritten with any other NDEF message by an NFC device.
It uses the :ref:`lib_nfc_ndef`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a smartphone or tablet with NFC Tools application (or equivalent).

Overview
********

When the sample starts, it initializes the NFC tag and loads the NDEF message from the file in flash memory.
If the NDEF message file does not exist, a default message is generated.
It is a URI message with a URI record containing the URL "nordicsemi.com".
The sample then sets up the NFC library for the Type 4 Tag platform, which uses the NDEF message and senses the external NFC field.

The library works in Read-Write emulation mode.
In this mode, procedures for reading and updating an NDEF message are handled internally by the NFC library.
Any changes to the NDEF message update the NDEF message file stored in flash memory.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Indicates if an NFC field is present.
      LED 2:
         Indicates that the NDEF message was updated.
      LED 4:
         Indicates that the NDEF message was read.

      Button 1:
         Press during startup to restore the default NDEF message.

   .. group-tab:: nRF54 DKs

      LED 0:
         Indicates if an NFC field is present.
      LED 1:
         Indicates that the NDEF message was updated.
      LED 3:
         Indicates that the NDEF message was read.

      Button 0:
         Press during startup to restore the default NDEF message.

Building and running
********************
.. |sample path| replace:: :file:`samples/nfc/writable_ndef_msg`

.. include:: /includes/build_and_run_ns.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

.. note::
   |nfc_nfct_driver_note|

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Touch the NFC antenna with the smartphone or tablet and observe that **LED 1** and **LED 4** are lit.
      #. Observe that the smartphone or tablet tries to open the URL "http\://www.nordicsemi.com" in a web browser.
      #. Use a proper application (for example, NFC Tools for Android) to overwrite the existing NDEF message with your own message.
      #. Restart your development kit and touch the antenna again.
         Observe that the new message is displayed.

   .. group-tab:: nRF54 DKs

      1. Touch the NFC antenna with the smartphone or tablet and observe that **LED 0** and **LED 3** are lit.
      #. Observe that the smartphone or tablet tries to open the URL "http\://www.nordicsemi.com" in a web browser.
      #. Use a proper application (for example, NFC Tools for Android) to overwrite the existing NDEF message with your own message.
      #. Restart your development kit and touch the antenna again.
         Observe that the new message is displayed.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_uri`
* :ref:`nfc_t4t_ndef_file_readme`

In addition, it uses the Type 4 Tag library from `sdk-nrfxlib`_:

* :ref:`nrfxlib:nfc_api_type4`

It uses the following Zephyr libraries:

* ``include/atomic.h``
* ``include/zephyr.h``
* ``include/device.h``
* ``include/nvs/nvs.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
