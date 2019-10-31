.. _lib_fota_download:

FOTA download
#############

The firmware over-the-air (FOTA) download library provides functions for downloading a firmware file as an upgrade candidate to the DFU target that is used in the :ref:`lib_dfu_target` library.

To start a FOTA download, you must provide the URL for the file that should be downloaded.
The required arguments are ``host`` (for example, ``my_domain.com``) and ``file`` (for example, ``path/to/resource/file.bin``).

.. note::
   To upgrade the upgradable bootloader, you must provide two firmware files: one linked against the S0 partition and one linked against the S1 partition.
   See :ref:`upgradable_bootloader` for more information.

   To do this, you must provide two paths in the ``file`` argument, separated by a space character.
   The first entry  must point to the MCUboot variant linked against the S0 partition.
   The second entry  must point to the MCUboot variant linked against the S1 partition.

   If an upgradeable bootloader is included in the project, the FOTA download library uses the :ref:`lib_secure_services` library to deduce the active MCUboot partition (S0 or S1).
   If the ``file`` argument contains a space character, it is internally updated to contain only the entry that corresponds to the non-active slot.
   This updated path is then used to download the image.

The library uses the :ref:`lib_download_client` to download the image.
After downloading the first fragment, the :ref:`lib_dfu_target` library is used to identify the type of the image that is being downloaded.
Examples of image types are modem upgrades and upgrades handled by MCUboot.

Once the download has been started, all received data fragments are passed to the :ref:`lib_dfu_target` library.
The :ref:`lib_dfu_target` library takes care of where the upgrade candidate is stored, depending on the image type that is being downloaded.

When the download client sends the event indicating that the download has completed, the received firmware is tagged as an upgrade candidate, and the download client is instructed to disconnect from the server.
The library then sends a :cpp:enumerator:`FOTA_DOWNLOAD_EVT_FINISHED<fota_download::FOTA_DOWNLOAD_EVT_FINISHED>` callback event.
When the consumer of the library receives this event, it should issue a reboot command to apply the upgrade.

By default, the FOTA download library uses HTTP for downloading the firmware file.
To use HTTPS instead, apply the changes described in :ref:`the HTTPS section of the download client documentation <download_client_https>` to the library.

The FOTA download library is used in the :ref:`http_application_update_sample` sample.



API documentation
*****************

| Header file: :file:`include/net/fota_download.h`
| Source files: :file:`subsys/net/lib/fota_download/src/`

.. doxygengroup:: fota_download
   :project: nrf
   :members:
