.. _lib_fota_download:

FOTA download
#############

The firmware over-the-air (FOTA) download library provides functions for downloading a firmware file as an upgrade candidate to the DFU target that is used in the :ref:`lib_dfu_target` library.

The library uses the :ref:`lib_download_client` to download the image.
Once the download has been started, all received data fragments are passed to the :ref:`lib_dfu_target` library.
The :ref:`lib_dfu_target` library takes care of where the upgrade candidate is stored, depending on the image type that is being downloaded.

When the download client sends the event indicating that the download has completed, the received firmware is tagged as an upgrade candidate, and the download client is instructed to disconnect from the server.
The library then sends a :cpp:enumerator:`FOTA_DOWNLOAD_EVT_FINISHED<fota_download::FOTA_DOWNLOAD_EVT_FINISHED>` callback event.
When the consumer of the library receives this event, it should issue a reboot command to apply the upgrade.

By default, the FOTA download library uses HTTP for downloading the firmware file.
To use HTTPS instead, apply the changes described in the HTTPS section of the download client documentation to the library.

The FOTA download library is used in the :ref:`http_application_update_sample` sample.


API documentation
*****************

| Header file: :file:`include/net/fota_download.h`
| Source files: :file:`subsys/net/lib/fota_download/src/`

.. doxygengroup:: fota_download
   :project: nrf
   :members:
