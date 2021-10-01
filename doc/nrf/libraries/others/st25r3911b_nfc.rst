.. _st25r3911b_nfc_readme:

NFC Reader ST25R3911B
#####################

The NFC Device ST25R3911B can be used to read and write NFC-A compatible tags (tags that support the ISO/IEC 14443 standard), creating an NFC polling device.

This driver is intended to be used with the NFC Reader ST25R3911B Nucleo expansion board (X-NUCLEO-NFC05A1).

The driver is used in the :ref:`nfc_tag_reader` sample.

NFC Reader hardware
*******************

The X-NUCLEO-NFC05A1 is an Arduino-compatible shield that can be used to communicate over NFC.
It contains the NFC Reader chip ST25R3911B.

The ST25R3911B can both generate its own field to read and write NFC tags and behave like a tag itself.
It is a slave device, and the external microcontroller initiates all communication.

Communication is performed by 4-wire Serial Pheripheral Interface (SPI).
The ST25R3911B sends an interrupt request (pin IRQ) to the microcontroller.
For the documentation of the NFC chip, see the `ST25R3911B chip documentation`_.

Functionality
*************

The driver provides the following functionality:

* Detecting ISO/IEC 14443 A tags and reading their UID
* Performing automatic collision resolution
* Data exchange with the tag (write/read) with manually set Frame Delay Time

  * With auto-generated CRC
  * With manually generated CRC

* Putting the tag to sleep

The driver currently supports only NFC-A technology.

Architecture
************

The driver consists of four layers:

* Layer responsible for SPI communication
* Layer to manage ST25R3911B interrupts
* Layer with common reader functionality that can be shared by all NFC technologies
* NFC-A layer, which is responsible for NFC-A technology activities

The driver requires calling its specific function periodically.

API documentation
*****************

| Header file: :file:`include/st25r3911b_nfca.h`
| Source files: :file:`lib/st25r3911b/`

.. doxygengroup:: st25r3911b_nfca
   :project: nrf
   :members:
