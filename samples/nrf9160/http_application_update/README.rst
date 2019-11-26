.. _http_application_update_sample:

nRF9160: HTTP application update
################################

The HTTP application update sample demonstrates how to do a basic FOTA (firmware over-the-air) update.
It uses the :ref:`lib_fota_download` library to download a file from a remote server and write it to flash.


Overview
********

The sample connects to an HTTP server to download a signed firmware image.
The image is generated when building the sample, but you must upload it to a server and configure where it can be downloaded.
See `Specifying the image file`_ for more information.

By default, the image is saved to `MCUboot`_ bootloader secondary slot.
The downloaded image must be signed for use by MCUboot with imgtool.


Requirements
************

* The following development board:

  * |nRF9160DK|

* A signed firmware image that is available for download from an HTTP server.
  This image is automatically generated when building the application.

* .. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/http_application_update`

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the nrf9160_pca10090ns board.
Because of this, it automatically includes the :ref:`secure_partition_manager`.
The sample also uses MCUboot, which is automatically built and merged into the final HEX file when building the sample.


Specifying the image file
=========================

Before building the sample, you must specify where the image file will be located.
If you do not want to host it yourself, you can upload it to a public S3 bucket on Amazon Web Services (AWS).
See `Setting up an AWS S3 bucket`_ for instructions.

To specify the location in |SES|:

1. Select **Project** > **Configure nRF Connect SDK Project**.
#. Navigate to **HTTP application update sample** and specify the **Download host name** (``CONFIG_DOWNLOAD_HOST``) and **The file to download** (``CONFIG_DOWNLOAD_FILE``).
#. Click **Configure** to save the configuration.

.. include:: /includes/aws_s3_bucket.txt

Hosting your image on an AWS S3 Server
--------------------------------------

1. Go to `AWS S3 console`_ and sign in.
#. Go to the bucket you have created.
#. Click **Upload** and select the file ``app_update.bin`` (located in the ``zephyr`` subfolder of your build directory).
#. Click the file you uploaded in the bucket and check the **Object URL** field to find the download URL for the file.

When specifying the image file, use the ``<bucket-name>.s3.<region>.amazonaws.com`` part of the URL for the download hostname.
Make sure to not include the ``https``.
Specify the rest of the URL as file name.


Testing
=======

After programming the sample to the board, test it by performing the following steps:

1. Configure the application version to be 2.
   To do so, either change ``CONFIG_APPLICATION_VERSION`` to 2 in the ``prj.conf`` file, or select **Project** > **Configure nRF Connect SDK Project** > **HTTP application update sample** in |SES| and change the value for **Application version**.
   Then rebuild the application.
#. Upload the file ``update.bin`` to the server you have chosen.
   To upload the file on nRF Cloud, click **Upload** for the firmware URL that you generated earlier.
   Then select the file ``update.bin`` and upload it.
#. Reset your nRF9160 DK to start the application.
#. Open a terminal emulator and observe that an output similar to this is printed::

    SPM: prepare to jump to Non-Secure image
    ***** Booting Zephyr OS v1.13.99 *****

#. Observe that LED1 is lit.
   This indicates that version 1 of the application is running.
#. Press Button 1 on the nRF9160 DK to start the download process and wait until "Download complete" is printed in the terminal.
#. Press the **RESET** button on the board.
   MCUboot will now detect the downloaded image and write it to flash.
   This can take up to a minute and nothing is printed in the terminal while this is processing.
#. Observe that LED1 and LED2 is lit.
   This indicates that version 2 or higher of the application is running.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_fota_download`
  * :ref:`secure_partition_manager`

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`zephyr:flash_interface`
  * :ref:`zephyr:logger`
  * :ref:`zephyr:gpio`

From MCUboot
  * `MCUboot`_
