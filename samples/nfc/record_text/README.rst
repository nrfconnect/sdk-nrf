.. _record_text:

NFC: Text record
################

.. contents::
   :local:
   :depth: 2

The NFC Text record sample shows how to use the NFC tag to expose a text record to NFC polling devices.
It uses the :ref:`lib_nfc_ndef`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a smartphone or tablet.

Overview
********

When the sample starts, it initializes the NFC tag and generates an NDEF message with three text records that contain the text "Hello World!" in three languages.
Then it sets up the NFC library to use the generated message and sense the external NFC field.

The only events handled by the application are the NFC events.

User interface
**************

LED 1:
   Indicates if an NFC field is present.

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/record_text`

.. include:: /includes/build_and_run_ns.txt

.. note::
   |nfc_nfct_driver_note|

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. Touch the NFC antenna with the smartphone or tablet and observe that **LED 1** is lit.
#. Observe that the smartphone or tablet displays the encoded text (in the most suitable language).
#. Move the smartphone or tablet away from the NFC antenna and observe that **LED 1** turns off.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_ndef_msg`
* :ref:`nfc_text`

In addition, it uses the Type 2 Tag library from `sdk-nrfxlib`_:

* :ref:`nrfxlib:nfc_api_type2`

It uses the following Zephyr libraries:

* ``include/zephyr.h``
* ``include/device.h``
* ``include/power/reboot.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
