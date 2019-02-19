.. _nfc_tag_reader:

NFC: Tag Reader
###############

The NFC Tag Reader sample demonstrates how to use the :ref:`st25r3911b_nfc_readme` driver to interact with an NFC-A Tag.

Overview
********

The sample shows how to use the ST25R3911B NFC Reader to read data from a tag that supports the ISO/IEC 14443 standard (NFC-A technology).

Before reading data, the sample detects which NFC technology is used by sending the appropriate initialization commands (ALL Request, SENS Request).
It also performs automatic collision resolution.

Supported tag types:

* NFC Type 2 Tag

Requirements
************

* One of the following development boards:

  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)

* NFC Reader ST25R3911B Nucleo expansion board (X-NUCLEO-NFC05A1)
* NFC Type 2 Tag

Building and running
********************

This sample can be found under :file:`samples/nfc/tag_reader` in the |NCS| folder structure.

See :ref:`gs_programming` for information about how to build and program the application.

Testing
=======
After programming the sample to your board, you can test it with an NFC-A Type 2 Tag.

1. Connect the Nucleo expansion board to the development kit board.
#. Connect to the board with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Touch the ST25R3911B NFC Reader with a Type 2 Tag.
#. Observe the output in the terminal.
   The content of the tag block is printed there.
#. After a little delay, the tag can be read again.


Dependencies
************

This sample uses the following |NCS| drivers:

* :ref:`st25r3911b_nfc_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/misc/printk.h``
