.. _nfc_t4t_isodep_readme:

NFC T4T ISO-DEP
###############

This library is an NFC ISO-DEP protocol implementation for an NFC Reader/Writer.

You can use the :ref:`st25r3911b_nfc_readme` module to exchange data over the NFC ISO-DEP
protocol.
To start data transfer, a poller must select a tag by sending the RATS
command.
After receiving a response (ATS) from the tag, data can be exchanged.

ISO-DEP protocol defines three frame types:

* I-block used to convey information to the application layer.
* R-block used to carry positive or negative acknowledgements.
* S-block used to exchange protocol control information.


The library automatically decides which type of frame is to be used and provides
full protocol support including error recovery and chaining mechanism.

API documentation
*****************

| Header file: :file:`include/nfc/t4t/isodep.h`
| Source file: :file:`subsys/nfc/t4t/isodep.c`

NFC Type 4 Tag ISO-DEP API
--------------------------

.. doxygengroup:: nfc_t4t_isodep
   :project: nrf
   :members:

ISO-DEP Protocol error
----------------------

.. doxygengroup:: isodep_error
   :project: nrf
   :members:
