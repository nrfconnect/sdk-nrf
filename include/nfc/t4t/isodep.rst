.. _nfc_t4t_isodep:

NFC T4T ISO-DEP
###############

Use the :ref:`nfc_t4t_isodep` module to exchange data over the NFC ISO-DEP
Protocol. To start data transfer NFC Type 4 Tag have to selected first by
sending the RATS command after receipt a response from a tag, data can be
exchanged.

The ISO-DEP Protocol define a three frame type:
* I-block used to convey information to the application layer.
* R-block used to positive or negative acknowledgements.
* S-blck used to exchange protocol control information.


Module automatically decide which type of frame is to be used and provide
full protocol support including error recovery and chaining mechanism.

API documentation
*****************

| Header file: :file:`include/nfc/t4t/isodep.h`
| Source file: :file:`subsys/nfc/t4t/isodep.c`

.. doxygengroup:: nfc_t4t_isodep
   :project: nrf
   :members:
