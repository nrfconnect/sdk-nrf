.. _http_application_update_sample:

nRF9160: HTTP application update
################################

The HTTP application update sample demonstrates how to do a basic FOTA (firmware over-the-air) update.
It uses the :ref:`lib_download_client` library to download a file from a remote server and write it to flash.


Overview
********

The sample connects to an HTTP server to download a signed firmware image.
You must provide the image file and configure where it can be downloaded.
See `Specifying the image file`_ for more information.

By default, the image is saved to `MCUboot`_ bootloader secondary slot.
The downloaded image must be signed for use by MCUboot with imgtool.


Requirements
************

* The following development board:

  * nRF9160 DK board (PCA10090)

* A signed firmware image that is available for download from an HTTP server. This image is automatically generated when
  building the application.


Building and running
********************

This sample can be found under :file:`samples/nrf9160/http_application_update` in the |NCS| folder structure.

The sample is built as a non-secure firmware image for the nrf9160_pca10090ns board. Is uses MCUboot and SPM
(Secure Partition Manager) and they will be automatically built and merged into the final hex-file when building
the sample.

See :ref:`gs_programming` for information about how to build and program the application.


Specifying the image file
=========================

Before building the sample, you must specify the location of the image file that should be downloaded.

To do so, open the :file:`prj.cnf` file of the sample and add the name of your server and the file to be downloaded::

   CONFIG_HOST="your.server.com"
   CONFIG_RESOURCE="filename.bin"


Programming the sample
======================

1. Connect the nRF9160 DK board to your computer.
#. Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller.
#. Build the HTTP application update sample (this sample) for the nrf9160_pca10090ns board and program it.
#. Verify that the sample was programmed successfully by connecting to the serial port with a terminal emulator (for example, PuTTY) and checking the output.
   See :ref:`putty` for the required settings.


Testing
=======

After programming the sample to the board, test it by performing the following steps:

1. Change CONFIG_APPLICATION_VERSION to 2 in the ``prj.conf`` file and rebuild the application.
#. Upload the file ``update.bin`` to the server you have chosen.
#. Reset your nRF9160 DK to start the application.
#. Open a terminal emulator and observe that an output similar to this is printed::

    SPM: prepare to jump to Non-Secure image
    ***** Booting Zephyr OS v1.13.99 *****

#. Observe that LED1 is lit. This indicates that version 1 of the application is running.
#. Press Button 1 on the nRF9160 DK to start the download process and wait until "Download complete" is printed in the terminal.
#. Press the **RESET** button on the board. MCUboot will now detect the downloaded image and write it to flash. This can take up to a
   minute and nothing is printed in the terminal while this is processing.
#. Observe that LED1 and LED2 is lit. This indicates that version 2 or higher of the application is running.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_download_client`
  * :ref:`secure_partition_manager`

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`zephyr:flash_interface`
  * :ref:`zephyr:logger`
  * :ref:`zephyr:gpio`

From MCUboot
  * `MCUboot`_

