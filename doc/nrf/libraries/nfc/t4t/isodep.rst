.. _nfc_t4t_isodep_readme:

ISO-DEP protocol
################

.. contents::
   :local:
   :depth: 2

This library is an NFC ISO-DEP protocol implementation for an NFC polling device.

You can use the :ref:`st25r3911b_nfc_readme` library to transmit and receive frames with a tag in NFC-A technology.
To start data transfer, a polling device must activate the ISO-DEP protocol by sending the RATS command to the tag.
After receiving a response (ATS) from the tag, data can be exchanged.

The ISO-DEP protocol defines three frame types:

* I-block - used to convey information to the application layer
* R-block - used to carry positive or negative acknowledgments
* S-block - used to exchange protocol control information

The library automatically decides which frame type to use and provides full protocol support including error recovery and chaining mechanism.

API documentation
*****************

| Header file: :file:`include/nfc/t4t/isodep.h`
| Source file: :file:`subsys/nfc/t4t/isodep.c`

.. doxygengroup:: nfc_t4t_isodep
   :project: nrf
   :members:

ISO-DEP protocol errors
=======================

.. doxygengroup:: isodep_error
   :project: nrf
   :members:
