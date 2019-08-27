.. _nfc_t4t_cc_file_readme:

Capability Containers file parser
#################################

To detect and access :ref:`nfc_ndef` NFC Data Exchange Format data, the NFC reader uses the Capability Container (CC)
file contained inside the NDEF Tag Application. The CC file is a read-only file with file identifier equal to E103h.
It is used to store management data for the Type 4 Tag platform.

The CC file module provides functions to parse raw CC file data to its descriptor structure.

CC file format
**************

You can use the module to print out the tag content. The output of the parser consists of the following data:

=================================== ======================================
Field name                          Description
=================================== ======================================
CCLEN                               Size of the CC file.
Mapping Version                     Tag 4 Tag version number.
MLe                                 Maximum R-APDU data size.
MLc                                 Maximum C-APDU data size.
Extended NDEF/NDEF File Control TLV Management data for NDEF file with its
                                    payload
TLV Block
=================================== ======================================

Certain types of TLV blocks are supported by Type 4 Tag:

============================== =============== ==================
TLV Block Name                 Tag Field Value Length Field Value
============================== =============== ==================
NDEF File Control TLV          04h             06h
Proprietary File Control TLV   05h             06h
Extended NDEF File Control TLV 06h             08h
============================== =============== ==================

More detailed information about each TLV block inside Type 4 Tag is also printed out.
The output of the parser consists of the following data:

====================== ========================================
Field name             Description
====================== ========================================
File identifier        Used for the Select procedure.
Maximum file size      Maximum capacity of the file (in bytes).
Read access condition  Read access level of the file.
Write access condition Write access level of the file.
====================== ========================================

Optionally, content of the file (described by the TLV block) can also be printed out. However,
in such case you must call an additional function which binds the TLV structure with the described file content.

API documentation
*****************

.. doxygengroup:: nfc_t4t_cc_file
   :project: nrf
   :members:
