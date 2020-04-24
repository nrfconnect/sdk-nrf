.. _nfc_pairing:

Bluetooth: NFC pairing
######################

The NFC pairing sample demonstrates Bluetooth LE out-of-band pairing using an NFC tag.
It can be used to test the touch-to-pair feature between Nordic Semiconductor's devices and an NFC polling device with Bluetooth LE support, for example, a mobile phone.

The sample shows the usage of NFC NDEF :ref:`nfc_ch`.
It provides minimal Bluetooth functionality in peripheral role and on GATT level it implements only the Device Information Service.

The sample supports pairing in one of the following mode:

* LE Secure Connections OOB pairing
* Legacy OOB pairing

Overview
********

When the application starts, it initializes and starts the NFCT peripheral, which is used for pairing.
The application does not start advertising immediately, but only when the NFC tag is read by an NFC polling device, for example a smartphone or a tablet with NFC support.
The message that the tag sends to the NFC device contains data required to initiate pairing.
To start the NFC data transfer, the NFC device must touch the NFC antenna that is connected to the development kit.

After reading the tag, the initiator can connect and pair with the device that is advertising.
The connection state of the device is signaled by LEDs.
When the connection is lost due to a time-out, the library automatically triggers direct advertising.

Requirements
************

* One of the following development boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

* NFC polling device (for example, a smartphone or a tablet with NFC support)

User interface
**************

Button 1:
   Removes all bonded devices and terminates current connections.

LED 1:
   Indicates that a Bluetooth connection is established.

LED 2:
   Indicates that an NFC field is present.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/nfc_pairing`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Touch the NFC antenna with the smartphone or tablet and observe that LED 2 is lit.
#. Confirm pairing with ``Nordic_NFC_pairing`` in a pop-up window on the smartphone or tablet and observe that LED 1 lights up.
#. Move the smartphone or tablet away from the NFC antenna and observe that LED 2 turns off.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_ndef_le_oob`
* :ref:`nfc_ch`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the Type 4 Tag library from nrfxlib:

* :ref:`nrfxlib:nfc_api_type4`

The sample uses the following Zephyr libraries:

* ``include/zephyr.h``
* ``include/device.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
