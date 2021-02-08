.. _http_application_update_sample:

nRF9160: HTTP application update
################################

.. contents::
   :local:
   :depth: 2

The HTTP application update sample demonstrates how to do a basic firmware over-the-air (FOTA) update.
It uses the :ref:`lib_fota_download` library to download two image files from a remote server and program them to flash memory.

Overview
********

The sample connects to an HTTP server to download one of two signed firmware images:

* One image has the ``application version`` set to ``1``.
* The other has the ``application version`` set to ``2``.

The sample updates between the two images, either from *version 1* to *version 2* or from *version 2* to *version 1*.

By default, the images are saved to the `MCUboot`_ second-stage bootloader secondary slot.
To be used by MCUboot, the downloaded images must be signed using imgtool.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns

The sample also requires two signed firmware images that have to be available for download from an HTTP server.
The images are generated automatically when building the sample, but you must upload them to a server and configure the location from where they can be downloaded.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/http_update/application_update`

.. include:: /includes/build_and_run.txt

.. include:: /includes/spm.txt

Specifying the image files
==========================

Before building the sample, you must specify where the image files are located.
If you do not want to host the files yourself, you can upload them to a public S3 bucket on Amazon Web Services (AWS).

Two options configure the names of the updated image files:

- ``CONFIG_DOWNLOAD_FILE_V1`` - it configures the file name of the *version 1* of the update image.
- ``CONFIG_DOWNLOAD_FILE_V2`` - it configures the file name of the *version 2* of the update image.

To set these Kconfig options, follow these steps:

.. tabs::

   .. group-tab:: From |SES|

      1. Select :guilabel:`Project` > :guilabel:`Configure nRF Connect SDK Project`.
      #. Navigate to :guilabel:`HTTP application update sample` and specify both the download hostname (``CONFIG_DOWNLOAD_HOST``) and the names of the files that have to be downloaded (``CONFIG_DOWNLOAD_FILE_V1`` and ``CONFIG_DOWNLOAD_FILE_V2``).
      #. Click :guilabel:`Configure` to save the configuration.

   .. group-tab:: From the :file:`prj.conf` configuration file

      1. Locate your :file:`prj.conf` project configuration file.
      #. Change the ``CONFIG_DOWNLOAD_HOST`` option to indicate the hostname of the server where the image file is located (for example ``website.net``).
      #. Change the ``CONFIG_DOWNLOAD_FILE_V1`` and ``CONFIG_DOWNLOAD_FILE_V2`` options to indicate the names and paths of the image files (for example ``download/app_update.bin``).

      You can then use the :file:`prj.conf` project configuration file in your builds from the command line or using the *nRF Connect for Visual Studio Code* extension.
      See the `Building and running`_ section for more information.

If you do not want to host the image file, you can also upload it to a public S3 bucket on Amazon Web Services (AWS).

.. include:: /includes/aws_s3_bucket.txt

Hosting your image on an AWS S3 Server
--------------------------------------

To upload the file to a public S3 bucket, do the following:

1. Go to `AWS S3 console`_ and sign in.
#. Go to the bucket you have created.
#. Click :guilabel:`Upload` and select the file :file:`app_update.bin`.
   It is located in the :file:`zephyr` subfolder of your build directory.
#. Click the file you uploaded in the bucket and check the :guilabel:`Object URL` field to find the download URL for the file.

Remember to do the following when specifying the filenames:

* Use the ``<bucket-name>.s3.<region>.amazonaws.com`` part of the URL as the hostname of the server hosting the images, without including ``https://``.
* Specify the file names as the remaining part of the URL.

You can then use the steps mentioned in the `Specifying the image files`_ section above to upload both images.

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Upload the file :file:`app_update.bin` with the application image version 1 to the server you have chosen.
   To upload the file on *nRF Connect for Cloud*, click :guilabel:`Upload` for the firmware URL that you generated earlier, then select the file :file:`app_update.bin` and upload it.
   Remember to rename the file to match the ``CONFIG_DOWNLOAD_FILE_V1`` configuration option.
#. Configure the application image version to be 2 and rebuild the application.
#. Upload the file :file:`app_update.bin` with the application image version 2 to the server you have chosen.
#. Reset your nRF9160 DK to start the application.
#. Open a terminal emulator and observe that an output similar to the following is printed:

   .. code-block::

      SPM: prepare to jump to Non-Secure image
      ***** Booting Zephyr OS v1.13.99 *****

#. Observe that **LED 1** is lit.
   This indicates that version 1 of the application is running.
#. Press **Button 1** on the nRF9160 DK to start the download process, and wait until ``Download complete`` is printed in the terminal.
#. Press the **RESET** button on the kit.
   MCUboot will now detect the downloaded image and program it to flash memory.
   This can take up to a minute.
   Nothing is printed in the terminal while the writing is in progress.
#. Observe that **LED 1** and **LED 2** are lit.
   This indicates that version 2 or higher of the application is running.
#. You can now downgrade the application by repeating the button presses mentioned above.
   Observe that after the second update only **LED 1** is lit.
   This indicates that the application image has been downgraded to version 1.
   Any further updates will toggle between the versions.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_fota_download`
* :ref:`secure_partition_manager`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`zephyr:flash_api`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:gpio_api`

It also uses the `MCUboot`_ bootloader.
