.. _http_application_update_sample:

Cellular: HTTP application update
#################################

.. contents::
   :local:
   :depth: 2

The HTTP application update sample demonstrates how to do a basic firmware over-the-air (FOTA) update.
It uses the :ref:`lib_fota_download` library to download two image files from a remote server and program them to flash memory.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires two signed firmware images that have to be available for download from an HTTP server.
The images are generated automatically when building the sample, but you must upload them to a server and configure the location from where they can be downloaded.

Overview
********

The sample connects to an HTTP server to download one of two signed firmware images:

* One image has the ``application version`` set to ``1``.
* The other has the ``application version`` set to ``2``.

The sample updates between the two images, either from *version 1* to *version 2* or from *version 2* to *version 1*.

By default, the images are saved to the `MCUboot`_ second-stage bootloader secondary slot.
To be used by MCUboot, the downloaded images must be signed using imgtool.

Using the LwM2M carrier library
===============================

.. |application_sample_name| replace:: HTTP application update sample

.. include:: /includes/lwm2m_carrier_library.txt

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_DOWNLOAD_FILE_V1:

CONFIG_DOWNLOAD_FILE_V1
  This configuration option configures the file name of the *version 1* of the update image.

.. _CONFIG_DOWNLOAD_FILE_V2:

CONFIG_DOWNLOAD_FILE_V2
  This configuration option configures the file name of the *version 2* of the update image.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/http_update/application_update`

.. include:: /includes/build_and_run_ns.txt

Specifying the image files
==========================

Before building the sample, you must specify where the image files are located.
If you do not want to host the files yourself, you can upload them to a public S3 bucket on Amazon Web Services (AWS).

If you do not want to host the image file, you can also upload it to a public S3 bucket on Amazon Web Services (AWS).

.. include:: /includes/aws_s3_bucket.txt

Hosting your image on an AWS S3 Server
--------------------------------------

To upload the file to a public S3 bucket, perform the following steps:

1. Go to `AWS S3 console`_ and sign in.
#. Go to the bucket you have created.
#. Click :guilabel:`Upload` and select the file :file:`app_update.bin`.
   It is located in the :file:`zephyr` subfolder of your build directory.
#. Click the file you uploaded in the bucket and check the **Object URL** field to find the download URL for the file.

Remember to do the following when specifying the filenames:

* Use the ``<bucket-name>.s3.<region>.amazonaws.com`` part of the URL as the hostname of the server hosting the images, without including ``https://``.
* Specify the file names as the remaining part of the URL.

Follow the steps mentioned in the `Specifying the image files`_ section to upload both images.

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Upload the file :file:`app_update.bin` with the application image version 1 to the server you have chosen.
   To upload the file on nRF Cloud, click :guilabel:`Upload` for the firmware URL that you generated earlier, then select the file :file:`app_update.bin` and upload it.
   Remember to rename the file to match the ``CONFIG_DOWNLOAD_FILE_V1`` configuration option.
#. Configure the application image version to be 2 and rebuild the application.
#. Upload the file :file:`app_update.bin` with the application image version 2 to the server you have chosen.
#. Reset your nRF9160 DK to start the application.
#. Open a terminal emulator and observe that an output similar to the following is printed:

   .. code-block::

      ***** Booting Zephyr OS v1.13.99 *****

#. Observe that **LED 1** is lit.
   This indicates that version 1 of the application is running.
#. Press **Button 1** on the nRF9160 DK or type "download" in the terminal emulator to start the download process, and wait until ``Download complete`` is printed in the terminal.
#. Press the **RESET** button on the kit or type "reset" in the terminal emulator.
   MCUboot will now detect the downloaded image and program it to flash memory.
   This can take up to a minute.
   Nothing is printed in the terminal while the writing is in progress.
#. Observe that **LED 1** and **LED 2** are lit.
   This indicates that version 2 or higher of the application is running.
#. You can now downgrade the application by repeating the button presses or typing "download" in the terminal emulator.
   Observe that after the second update only **LED 1** is lit.
   This indicates that the application image has been downgraded to version 1.
   Any further updates will toggle between the versions.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_fota_download`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`zephyr:flash_api`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:gpio_api`
* :ref:`zephyr:shell_api`

It also uses the `MCUboot`_ bootloader.

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
