.. _nfc_t4t_cc_file_readme:

Parser for CC files
###################

.. contents::
   :local:
   :depth: 2

To detect and access :ref:`NDEF <ug_nfc_ndef_format>` data, the NFC reader uses the capability container (CC) file that is contained inside the NDEF tag application.
The CC file is a read-only file that contains management data for the Type 4 Tag platform, for example, information about the implemented specification and other capability parameters of the tag.
The file identifier of the CC file is E103h.

This library provides functions to parse raw CC file data to its descriptor structure.
In this way, you can use it to print out the tag content.

CC file content
***************

The parser outputs the following data:

=================================== ======================================
Field name                          Description
=================================== ======================================
CCLEN                               Size of the CC file.
Mapping Version                     Tag 4 Tag version number.
MLe                                 Maximum R-APDU data size.
MLc                                 Maximum C-APDU data size.
Extended NDEF/NDEF File Control TLV Management data for NDEF file with its
                                    payload.
TLV Block                           (see below)
=================================== ======================================

Certain types of TLV blocks are supported by Type 4 Tag:

============================== =============== ==================
TLV Block name                 Tag field value Length field value
============================== =============== ==================
NDEF File Control TLV          04h             06h
Proprietary File Control TLV   05h             06h
Extended NDEF File Control TLV 06h             08h
============================== =============== ==================

More detailed information about each TLV block inside Type 4 Tag is also printed out:

====================== ========================================
Field name             Description
====================== ========================================
File identifier        Used for the Select procedure.
Maximum file size      Maximum capacity of the file (in bytes).
Read access condition  Read access level of the file.
Write access condition Write access level of the file.
====================== ========================================

Optionally, the content of the file that is described by the TLV block can also be printed out.
However, to do so, you must call an additional function that binds the TLV structure with the described file content.

API documentation
*****************

| Header file: :file:`include/nfc/t4t/cc_file.h`
| Source file: :file:`subsys/nfc/t4t/cc_file.c`

.. doxygengroup:: nfc_t4t_cc_file
   :project: nrf
   :members:
