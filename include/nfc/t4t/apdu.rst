.. _nfc_t4t_apdu_readme:

APDU reader and writer
######################

.. contents::
   :local:
   :depth: 2

Application protocol data units (APDUs) are special communication units that are defined in `ISO/IEC 7816-4`_.
The subset of these commands that is used to interact with a Type 4 Tag is implemented in this library.

Command APDUs (C-APDUs) are the commands sent from the polling device, while response APDUs (R-APDUs) are the responses to a specific command received by the polling device from the tag.

The APDU reader/writer module provides functions to encode a C-APDU according to its descriptor and to decode a raw R-APDU data into the appropriate descriptor.

This library can be used with the :ref:`st25r3911b_nfc_readme` library to exchange NFC Type 4 Tag data.

APDU types
**********
There are three types of APDU that are relevant for a Type 4 Tag platform:

============    ===================================
APDU type       Usage
============    ===================================
Select          Selecting applications or files.
ReadBinary      Reading data from a file.
UpdateBinary    Writing data to a file.
============    ===================================

C-APDU and R-APDU format
************************

A C-APDU consists of the following fields:

===== ======== ======== =============================================
Field Length   Required Description
===== ======== ======== =============================================
CLA   1 byte   yes      Specifies the security level of the message.
INS   1 byte   yes      Specifies the command type to process.
P1    1 byte   yes      Specifies the first parameter for the chosen
                        command type.
P2    1 byte   yes      Specifies the second parameter for the chosen
                        command type.
Lc    1 or 3   no       Specifies the data length.
      byte
Data  Lc bytes no       Required if LC field is present.
                        Contains payload of C-APDU.
Le    1 or 2   no       Specifies the expected response body length
      bytes             of R-APDU.
===== ======== ======== =============================================

An R-APDU consists of the following fields:

============= ======== ======== =================================
Field         Length   Required Description
============= ======== ======== =================================
Response body variable no       Carries the data of the R-APDU.
SW1           1 byte   yes      Specifies the first status word.
SW2           1 byte   yes      Specifies the second status word.
============= ======== ======== =================================

API documentation
*****************

| Header file: :file:`include/nfc/t4t/apdu.h`
| Source file: :file:`subsys/nfc/t4t/apdu.c`

.. doxygengroup:: nfc_t4t_apdu
   :project: nrf
   :members:

Parameters for selecting instruction code in C-APDU
===================================================

.. doxygengroup:: nfc_t4t_apdu_select
   :project: nrf
   :members:

Status codes contained in R-APDU
================================

.. doxygengroup:: nfc_t4t_apdu_status
   :project: nrf
   :members:
