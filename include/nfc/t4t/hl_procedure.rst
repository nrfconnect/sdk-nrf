.. _nfc_t4t_hl_procedure_readme:

Type 4 Tag procedures
#####################

.. contents::
   :local:
   :depth: 2

This module provides functions to perform the NDEF detection procedure, which is used to retrieve the NDEF message from the data of a tag.

The full NDEF detection procedure consists of the following procedures:

1. NDEF tag application select.
#. Capability container (CC) select.
#. Capability container read.
#. NDEF select.
#. NDEF read or NDEF update.

After a successful NDEF detection procedure, you can also write data to the NDEF file.
To do this, you must perform an NDEF update procedure.

This module uses three other modules:

* :ref:`nfc_t4t_apdu_readme` for generating APDU commands
* :ref:`nfc_t4t_cc_file_readme` for analyzing APDU responses payload and storing it within the structure that represents the Type 4 Tag content
* :ref:`nfc_t4t_isodep_readme` for transferring data over ISO-DEP protocols

API documentation
*****************

| Header file: :file:`include/nfc/t4t/hl_procedure.h`
| Source file: :file:`subsys/nfc/t4t/hl_procedure.c`

.. doxygengroup:: nfc_t4t_hl_procedure
   :project: nrf
   :members:
