.. _nfc_t4t_apdu:

NFC T4T APDU READER/WRITER
##########################

Special communication units, called application protocol data units (APDUs), are used when exchanging data with a Type 4 Tag platform. Command APDUs (C-APDUs) are the commands sent from the NFC reader, while the response APDUs (R-APDUs) are the responses to a specific command received by NFC reader from the tag.

The APDU reader/writer module provides functions to encode a C-APDU according to its descriptor and to decode a raw R-APDU data into the appropriate descriptor.

Can be used with :ref:`st25r3911b_nfc_readme` to data exchange with NFC Type 4 Tag.

APDU types
**********
There are three types of APDU that are relevant for the Type 4 Tag platform:

============    ===================================
APDU type       Descriptions
============    ===================================
Select          Selection of applications or files.
ReadBinary      Reading data from a file.
UpdateBinary    Writing data to a file.
============    ===================================

C-APDU and R-APDU format
**********************************

An C-APDU consists the following fields:

===== ======== ======== =============================================
Field Length   Required Description
===== ======== ======== =============================================
CLA   1 byte   yes      Specifies the security level of the message.
INS   1 byte   yes      Specifies the command type to process.
P1    1 byte   yes      Specifies the first parameter for the chosen
P2    1 byte   yes      command type.
Lc    1 or 3   no       Specifies the second parameter for the chosen
      byte              command type.
Data  Lc bytes no       Required if LC field is present.
                        Contains payload of C-APDU.
Le    1 or 2   no       Specifies the expected response body length
      bytes             of R-APDU.
===== ======== ======== =============================================

An R-APDU consists the following fields:

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
