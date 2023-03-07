.. _lib_fota_download:

FOTA download
#############

.. contents::
   :local:
   :depth: 2

The firmware over-the-air (FOTA) download library provides functions for downloading a firmware file as an upgrade candidate to the DFU target that is used in the :ref:`lib_dfu_target` library.

Configuration and implementation
********************************

To start a FOTA download, provide the URL for the file that should be downloaded, specifying the required arguments as follows:

   * ``host`` - It indicates the hostname of the target host.
     For example, ``my_domain.com``.
   * ``file`` - It indicates the path to the file.
     For example, ``path/to/resource/file.bin``.

The FOTA library downloads the image using the :ref:`lib_download_client` library.
After downloading the first fragment, it uses the :ref:`lib_dfu_target` library to identify the type of image that is being downloaded.
Examples of image types are *modem upgrades* and upgrades handled by a *second-stage bootloader*.

Once the library starts the download, all received data fragments are passed to the :ref:`lib_dfu_target` library.
The :ref:`lib_dfu_target` library handles the location where the upgrade candidate is stored, depending on the image type that is being downloaded.

When the download client sends the event indicating that the download has been completed, the FOTA library tags the received firmware as an upgrade candidate, and it instructs the download client to disconnect from the server.
The library then sends a :c:enumerator:`FOTA_DOWNLOAD_EVT_FINISHED` callback event.
When the application using the library receives this event, it must issue a reboot command to apply the upgrade.

You can set :kconfig:option:`CONFIG_FOTA_DOWNLOAD_NATIVE_TLS` to configure the socket to be native for TLS instead of offloading TLS operations to the modem.

HTTPS downloads
***************

The FOTA download library is used in the :ref:`http_application_update_sample` sample.

By default, the FOTA download library uses HTTP for downloading the firmware file.
To use HTTPS, apply the changes described in :ref:`the HTTPS section of the download client documentation <download_client_https>` to the library.

Second-stage bootloader upgrades
********************************

To upgrade the second-stage upgradable bootloader, you must provide two firmware files: one linked against the S0 partition and one linked against the S1 partition.
See :ref:`upgradable_bootloader` for more information.

To do this, provide two paths in the ``file`` argument, separated by a space character:

* The first entry must point to the second-stage upgradable bootloader variant linked against the S0 partition.
* The second entry must point to the second-stage upgradable bootloader variant linked against the S1 partition.

When an upgradeable bootloader, like MCUboot, is included in the project, the FOTA download library uses :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` to detect the active second-stage upgradable bootloader partition (S0 or S1).
If the ``file`` argument contains a space character, it is internally updated to contain only the entry that corresponds to the non-active slot.
This updated path is then used to download the image.
A device reset triggers the second-stage upgradable bootloader to copy the image from the update bank to the non-active slot.
An additional reset is then required for the first-stage immutable bootloader to select and use the upgraded second-stage bootloader.

API documentation
*****************

| Header file: :file:`include/net/fota_download.h`
| Source files: :file:`subsys/net/lib/fota_download/src/`

.. doxygengroup:: fota_download
   :project: nrf
   :members:
