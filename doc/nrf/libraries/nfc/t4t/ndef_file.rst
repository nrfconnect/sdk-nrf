.. _nfc_t4t_ndef_file_readme:

NDEF file
#########

.. contents::
   :local:
   :depth: 2

The NDEF file stores the length and content of the NDEF message.
Use this library to encode standardized data for the NFC Type 4 Tag.
To generate an NDEF message, you can use the :ref:`nfc_ndef_msg` and :ref:`nfc_ndef_record` modules.

The following code sample demonstrates how to encode the NDEF file for NFC Type 4 Tag:

.. literalinclude:: ../../../../../samples/nfc/writable_ndef_msg/src/ndef_file_m.c
    :language: c
    :start-after: include_startingpoint_ndef_file_rst
    :end-before: include_endpoint_ndef_file_rst

API documentation
*****************

| Header file: :file:`include/nfc/t4t/ndef_file.h`
| Source file: :file:`subsys/nfc/t4t/ndef_file.c`

.. _nfc_t4t_ndef_file:

.. doxygengroup:: nfc_t4t_ndef_file
   :project: nrf
   :members:
