.. _nfc_t4t_hl_procedure_readme:

NFC T4T High Level Procedure
############################

The NDEF High Level procedure module provides functions to perform the NDEF detection procedure,
which is used to retrieve the NDEF message from the data of a tag.

1. NDEF Tag Application select procedure.
#. Capability Container (CC) select procedure.
#. Capability Container read procedure.
#. NDEF select procedure.
#. NDEF read procedure or NDEF update procedure.

After a successful NDEF detection procedure, you can also write data to the NDEF file.
To do it, you need to perform an NDEF update procedure.

This module uses three other modules:
   * :ref:`nfc_t4t_apdu_readme` is used to generated APDU commands.
   * :ref: `nfc_t4t_cc_file_readme` is used to analyzed APDU responses payload and store it within the structure that represents Type 4 Tag content.
   * :ref: `nfc_t4t_isodep_readme` is used to transfer data over ISO-DEP Protocols

API documentation
*****************

| Header file: :file:`include/nfc/t4t/hl_procedure.h`
| Source file: :file:`subsys/nfc/t4t/hl_procedure.c`

NFC Type 4 Tag HL Procedure API
-------------------------------

.. doxygengroup:: nfc_t4t_hl_procedure
   :project: nrf
   :members:
