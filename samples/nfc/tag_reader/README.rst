.. _nfc_tag_reader:

NFC: Tag reader
###############

.. contents::
   :local:
   :depth: 2

The NFC Tag reader sample demonstrates how to use the :ref:`st25r3911b_nfc_readme` driver to interact with an NFC-A Tag.

Overview
********

The sample shows how to use the ST25R3911B NFC reader to read data from a tag that supports the ISO/IEC 14443 standard (NFC-A technology).
This device can be used to read and parse content of an NFC Type 2 Tag or Type 4 Tag.

After successful parsing, the tag content is printed using the logging subsystem.

Before reading data, the sample detects which NFC technology is used by sending the appropriate initialization commands (ALL Request, SENS Request).
It also performs automatic collision resolution.

Supported tag types:

* NFC Type 2 Tag
* NFC Type 4 Tag

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833

The sample has the following additional requirements:

* NFC Reader ST25R3911B Nucleo expansion board (X-NUCLEO-NFC05A1)
* NFC Type 2 Tag or Type 4 Tag

Building and running
********************
.. |sample path| replace:: :file:`samples/nfc/tag_reader`

.. include:: /includes/build_and_run.txt

Testing
=======
After programming the sample to your development kit, you can test it with an NFC-A Type 2 Tag or Tag 4 Type.

1. Connect the Nucleo expansion board to the development kit.
#. |connect_terminal|
#. Reset the kit.
#. Touch the ST25R3911B NFC reader with a Type 2 Tag or Type 4 Tag.
#. Observe the output in the terminal.
   The content of the tag is printed there.
#. After a little delay, the tag can be read again.


Dependencies
************

This sample uses the following |NCS| drivers:

* :ref:`st25r3911b_nfc_readme`

This sample uses the following |NCS| libraries:

* :ref:`nfc_t2t_parser_readme`
* :ref:`nfc_ndef_parser_readme`
* :ref:`nfc_ndef_le_oob_rec_parser_readme`
* :ref:`nfc_ndef_ch_rec_parser_readme`
* :ref:`nfc_t4t_apdu_readme`
* :ref:`nfc_t4t_isodep_readme`
* :ref:`nfc_t4t_hl_procedure_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/sys/printk.h``
