.. _record_text:

NFC: Text Record
################

The NFC Text Record sample shows how to use the NFC tag to expose a Text record to NFC polling devices.

Overview
********

When the sample starts, it initializes the NFC tag and generates an NDEF message with three Text records that contain the text "Hello World!" in three languages.
Then it sets up the NFC library to use the generated message and sense the external NFC field.

The only events handled by the application are the NFC events.

Requirements
************

* One of the following development boards:

  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)

* Smartphone or tablet

User interface
**************

LED 1:
   Indicates if an NFC field is present.

Building and running
********************

This sample can be found under :file:`samples/nfc/record_text` in the |NCS| folder structure.

See :ref:`gs_programming` for information about how to build and program the application.

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Touch the NFC antenna with the smartphone or tablet and observe that LED 1 is lit.
#. Observe that the smartphone/tablet displays the encoded text (in the most
   suitable language).
#. Move the smartphone/tablet away from the NFC antenna and observe that LED 1
   turns off.

Dependencies
************

This sample uses the following |NCS| libraries:

* ``include/nfc/ndef/nfc_ndef_msg.h``
* ``include/nfc/ndef/nfc_text_rec.h``

In addition, it uses the Type 2 Tag library from nrfxlib:

* :ref:`nrfxlib:nfc_api_type2`

The sample uses the following Zephyr libraries:

* ``include/zephyr.h``
* ``include/device.h``
* ``include/misc/reboot.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
